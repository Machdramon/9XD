"""Bounded, research-only checks of PCSX2 v9 GS dumps. Not a GS renderer.
Original project analysis code. Register-layout references are recorded in reports.
"""
from pathlib import Path
import argparse,collections,hashlib,json,struct,ctypes
import numpy as np

MEM=4194304

def sha(b):return hashlib.sha256(b).hexdigest()
def gs(x,y,width,bits):
 pw=128 if bits<=8 else 64;ph=128 if bits==4 else 64 if bits<=16 else 32;bw=32 if bits==4 else 16 if bits<=16 else 8;bh=16 if bits<=8 else 8;bx=(x%pw)//bw;by=(y%ph)//bh
 block=((by&1)|((bx&1)<<1)|((by&2)<<1)|((bx&2)<<2)|((by&4)<<2))if bits in(4,16)else((bx&1)|((by&1)<<1)|((bx&2)<<1)|((by&2)<<2)|((bx&4)<<2));xx=x%bw;yy=y%bh
 col=(xx&1)|((yy&1)<<1)|((xx&6)<<1)|((yy&6)<<3) if bits==32 else ((xx&8)>>3)|((xx&1)<<1)|((yy&1)<<2)|((xx&6)<<2)|((yy&6)<<4) if bits==16 else ((yy&2)>>1)|((xx&8)>>2)|((xx&1)<<2)|((yy&1)<<3)|((xx&2)<<3)|((((xx>>2)^(yy>>1)^(yy>>2))&1)<<5)|((yy&12)<<4) if bits==8 else ((yy&2)>>1)|((xx&8)>>2)|((xx&16)>>2)|((xx&1)<<3)|((yy&1)<<4)|((xx&2)<<4)|((((xx>>2)^(yy>>1)^(yy>>2))&1)<<6)|((yy&12)<<5)
 return ((y//ph)*((width+pw-1)//pw)+x//pw)*65536+block*2048+col*bits

def tex(value):return {'raw':hex(value),'base':(value&16383)*256,'pitch':((value>>14)&63)*64,'psm':(value>>20)&63,'width':1<<((value>>26)&15),'height':1<<((value>>30)&15),'cbp':((value>>37)&16383)*256,'cpsm':(value>>51)&15,'csm':(value>>55)&1,'csa':(value>>56)&31,'cld':(value>>61)&7}

def source_pictures(path,headerless=None):
 b=path.read_bytes();rows=[];banks=[]
 if headerless:
  rows=json.loads(headerless.read_text())['entries']
  for r in rows:
   r=dict(r);r['pictureOffset']=r.pop('offset');r['imageOffset']=r['pictureOffset']+r['headerSize'];r['paletteOffset']=r['imageOffset']+r['imageBytes'];r['clut']=3
   banks.append(r)
 else:
  at=0
  while True:
   at=b.find(b'TIM2',at)
   if at<0:break
   bank=at;at+=4
   if bank%16 or bank+16>len(b):continue
   align=b[bank+5];n=struct.unpack_from('<H',b,bank+6)[0]
   if align>1 or not 1<=n<=4096:continue
   p=bank+(128 if align else 16)
   for i in range(n):
    if p+48>len(b):break
    total,pal,size,hs,nc,pf,lv,cl,ty,w,h=struct.unpack_from('<IIIHHBBBBHH',b,p)
    if not(48<=hs<=total and p+total<=len(b) and hs+pal+size<=total and 0<w<=2048 and 0<h<=2048):break
    banks.append({'bankOffset':bank,'surface':i,'pictureOffset':p,'imageOffset':p+hs,'imageBytes':size,'paletteOffset':p+hs+size,'paletteBytes':pal,'width':w,'height':h,'type':ty,'format':pf,'clut':cl,'TEX0':struct.unpack_from('<Q',b,p+24)[0],'kind':'pixels'});p+=total
 for r in banks:
  if r.get('kind')=='reference-only' or r['format'] not in(0,16,48) or r['type'] not in(4,5):continue
  r=dict(r);bits=4 if r['type']==4 else 8;size=(r['width']*r['height']*bits+7)//8
  if size>r['imageBytes']:continue
  raw=b[r['imageOffset']:r['imageOffset']+size]
  if r['format']:
   transfer=16 if r['format']==16 and bits==4 else 32 if r['format']==48 and bits==8 else 0
   if not transfer or r['imageBytes']!=size or r.get('headerSize',128)!=128:continue
   tw=r['width']//2;th=r['height']*bits*2//transfer;step=transfer//8
   if not tw or tw*th*step!=size:continue
   y,x=np.mgrid[:th,:tw];addr=gs(x,y,tw,transfer)//8;idx=addr.ravel()[:,None]+np.arange(step);memory=np.zeros(1048576,dtype=np.uint8);written=np.zeros(1048576,dtype=bool)
   if idx.max()>=len(memory):continue
   memory[idx]=np.frombuffer(raw,dtype=np.uint8).reshape(-1,step);written[idx]=True;y,x=np.mgrid[:r['height'],:r['width']];a=gs(x,y,128,bits)
   if a.max()//8>=len(memory) or not np.all(written[a//8]):continue
   values=((memory[a//8]>>(a%8))&((1<<bits)-1)).astype(np.uint8).ravel()
   if bits==4:values=values[::2]|(values[1::2]<<4)
   raw=values.tobytes();r['normalization']='Research transfer-form indexed unswizzle with complete written-byte coverage; palette separately checked'
  r['file']=str(path);r['bits']=bits;r['pixelSha256']=sha(raw);r['paletteRaw']=b[r['paletteOffset']:r['paletteOffset']+r['paletteBytes']];rows.append(r) if not headerless else None
 if headerless:
  rows=[]
  for r in banks:
   if r.get('kind')!='pixels':continue
   r=dict(r);r['file']=str(path);r['bits']=4 if r['type']==4 else 8;r['pixelSha256']=sha(b[r['imageOffset']:r['imageOffset']+r['imageBytes']]);r['paletteRaw']=b[r['paletteOffset']:r['paletteOffset']+r['paletteBytes']];rows.append(r)
 return rows

class Replay:
 def __init__(self,state,sources):
  assert len(state)==4194813 and struct.unpack_from('<I',state)[0]==9
  self.memory=np.frombuffer(state[-84-MEM:-84],dtype=np.uint8).copy();self.valid=np.full(MEM,255,dtype=np.uint8);self.sources=sources;self.reg={};self.seq=0;self.generation=0;self.frame=0;self.pending=None;self.uploads=[];self.matches=[];self.draws=collections.Counter();self.unsupported=collections.Counter();self.checked=set();self.invalidated=set();self.texture_events=[];self.texhistory=[];self.transfer_modes=collections.Counter();self.localcopies=[]
  for reg,off in [(0,4),(0x1a,12),(0x1c,20),(0x50,84),(0x53,92),(0x51,100),(0x52,108)]:self.reg[reg]=struct.unpack_from('<Q',state,off)[0]
  names=[0x18,6,0x14,8,0x34,0x36,0x40,0x42,0x47,0x4a,0x4c,0x4e]
  for ctx in range(2):
   for i,reg in enumerate(names):self.reg[reg+ctx]=struct.unpack_from('<Q',state,124+ctx*96+i*8)[0]
  self.sample('initial',False)
 def indices(self,t,bits):
  y,x=np.mgrid[:t['height'],:t['width']];a=(t['base']*8+gs(x,y,t['pitch'],bits))%(MEM*8);return a
 def sample(self,stage,draw=True):
  contexts=[((self.reg.get(0,0)>>9)&1)] if draw else [0,1]
  for ctx in contexts:
   value=self.reg.get(6+ctx,0);t=tex(value)
   if t['psm'] not in(19,20) or not t['pitch']:continue
   key=(value,self.generation,stage=='initial')
   if key in self.checked:continue
   self.checked.add(key);bits=4 if t['psm']==20 else 8;candidates=[r for r in self.sources if r['width']==t['width'] and r['height']==t['height'] and r['bits']==bits]
   if not candidates:continue
   a=self.indices(t,bits);idx=a//8;mask=((1<<bits)-1)<<(a%8)
   if not np.all((self.valid[idx]&mask)==mask):continue
   vals=((self.memory[idx]>>(a%8))&((1<<bits)-1)).astype(np.uint8).ravel()
   if bits==4:
    if len(vals)%2:vals=np.pad(vals,(0,1))
    vals=vals[::2]|(vals[1::2]<<4)
   digest=sha(vals.tobytes())
   for r in candidates:
    if r['pixelSha256']!=digest:continue
    match={'sequence':self.seq,'frame':self.frame,'stage':stage,'source':r['file'],'pictureOffset':r['pictureOffset'],'width':t['width'],'height':t['height'],'runtime':t,'pixelSha256':digest,'vramGeneration':self.generation,'storedPaletteEqual':None,'sourcePaletteSha256':sha(r['paletteRaw']),'paletteScope':'CBP storage at sample; not emulated draw-time CLUT cache'}
    n=16 if bits==4 else 256;pal=r['paletteRaw'];stride=3 if (r.get('clut',0)&7)==2 and len(pal)>=n*3 and len(pal)<n*4 else 4
    if t['cpsm'] in(0,1) and t['csm']==0 and len(pal)>=n*stride:
     ci=np.arange(n,dtype=np.int64)+(t['csa']&15)*16 if bits==4 else np.arange(n,dtype=np.int64);physical=(ci&~24)|((ci&8)<<1)|((ci&16)>>1) if bits==4 else ci
     adr=(t['cbp']+gs(physical%16,physical//16,64,32)//8) % MEM;idx=(adr[:,None]+np.arange(stride))%MEM
     if np.all(self.valid[idx]==255):match['storedPaletteEqual']=self.memory[idx].tobytes()==pal[:n*stride];match['paletteByteStride']=stride
    self.matches.append(match)
 def invalidate_draw(self):
  ctx=(self.reg.get(0,0)>>9)&1;frame=self.reg.get(0x4c+ctx,0);sc=self.reg.get(0x40+ctx,0);z=self.reg.get(0x4e+ctx,0);key=(frame,sc,z,self.generation)
  if key in self.invalidated:return
  self.invalidated.add(key);height=((sc>>48)&2047)+1;pitch=((frame>>16)&63)*64
  if not pitch:return
  for base,enabled in [((frame&511)*8192,(frame>>32)!=0xffffffff),((z&511)*8192,not((z>>32)&1))]:
   if not enabled:continue
   # Conservative page-aligned 32-bit upper bound; no rasterization is attempted.
   size=min(MEM,((height+31)//32)*((pitch+63)//64)*8192)
   if base+size<=MEM:self.valid[base:base+size]=0
   else:self.valid[base:]=0;self.valid[:(base+size)%MEM]=0
 def register(self,reg,value,kick=True):
  self.seq+=1;self.reg[reg]=value
  if reg in(6,7):self.texture_events.append({'sequence':self.seq,'frame':self.frame,'context':reg-6,**tex(value)})
  if reg==0x53:
   self.finish_transfer();mode=value&3;self.transfer_modes[mode]+=1
   if mode==0:
    bb=self.reg.get(0x50,0);pos=self.reg.get(0x51,0);rect=self.reg.get(0x52,0);self.pending={'sequence':self.seq,'frame':self.frame,'base':((bb>>32)&16383)*256,'pitch':((bb>>48)&63)*64,'psm':(bb>>56)&63,'x':(pos>>32)&2047,'y':(pos>>48)&2047,'direction':(pos>>59)&3,'width':rect&4095,'height':(rect>>32)&4095,'data':bytearray()}
   elif mode==2:
    bb=self.reg.get(0x50,0);pos=self.reg.get(0x51,0);rect=self.reg.get(0x52,0);record={'sequence':self.seq,'frame':self.frame,'BITBLTBUF':hex(bb),'TRXPOS':hex(pos),'TRXREG':hex(rect),'scope':'Destination invalidation only; copy pixels not emulated'};self.localcopies.append(record);psm=(bb>>56)&63;bits={0:32,1:32,2:16,19:8,20:4}.get(psm);width=rect&4095;height=(rect>>32)&4095;pitch=((bb>>48)&63)*64;base=((bb>>32)&16383)*256
    if bits and width*height<=4194304 and width and height:
     y,x=np.mgrid[:height,:width];address=(base*8+gs(x+((pos>>32)&2047),y+((pos>>48)&2047),pitch or 64,bits))%(MEM*8);idx=address.ravel()//8
     for k in range(max(1,bits//8)):self.valid[(idx+k)%MEM]=0
    else:self.valid[:]=0
    self.generation+=1;self.unsupported['local-to-local transfer (destination invalidated, not copied)']+=1
  if reg in(4,5) and kick:
   self.sample('draw-binding');self.draws[hex(self.reg.get(6+((self.reg.get(0,0)>>9)&1),0))]+=1;self.invalidate_draw()
 def image(self,data):
  if self.pending is None:self.unsupported['IMAGE without a captured host transfer start']+=1;return
  p=self.pending;p['data']+=data;bits={0:32,1:24,2:16,19:8,20:4}.get(p['psm'])
  if bits and len(p['data'])*8>=p['width']*p['height']*bits:self.finish_transfer()
 def finish_transfer(self):
  if self.pending is None:return
  p=self.pending;self.pending=None;data=bytes(p.pop('data'));bits={0:32,1:24,2:16,19:8,20:4}.get(p['psm']);count=p['width']*p['height'];p['receivedBytes']=len(data);p['payloadSha256']=sha(data);p['replayed']=False
  if bits is None or not count or count>4194304 or p['direction'] or (p['pitch']==0 and (p['width']>64 or p['height']>32)):
   self.unsupported['unsupported host transfer layout']+=1;self.valid[:]=0;self.generation+=1;self.uploads.append(p);return
  needed=(count*bits+7)//8
  if len(data)<needed:self.unsupported['incomplete host transfer']+=1;self.valid[:]=0;self.generation+=1;self.uploads.append(p);return
  self.generation+=1;y,x=np.mgrid[:p['height'],:p['width']];a=(p['base']*8+gs(x+p['x'],y+p['y'],p['pitch'] or 64,32 if bits==24 else bits))%(MEM*8);flat=a.ravel();raw=np.frombuffer(data[:needed],dtype=np.uint8)
  if bits==4:
   values=np.empty(len(raw)*2,dtype=np.uint8);values[::2]=raw&15;values[1::2]=raw>>4;values=values[:count];idx=flat//8;shift=flat%8;mask=(15<<shift).astype(np.uint8);np.bitwise_and.at(self.memory,idx,~mask);np.bitwise_or.at(self.memory,idx,(values<<shift).astype(np.uint8));np.bitwise_or.at(self.valid,idx,mask)
  else:
   stride=bits//8;idx=((flat//8)[:,None]+np.arange(stride))%MEM;self.memory[idx]=raw.reshape(-1,stride);self.valid[idx]=255
  p['replayed']=True;p['usedBytes']=needed;self.uploads.append(p)

def run(folder,sources):
 info=json.loads((folder/'capture-info.json').read_text());compressed=next(folder.glob('*.gs.zst'));packed=compressed.read_bytes();assert sha(packed)==info['sha256'];dump=next(folder.glob('*.gs'));b=dump.read_bytes();assert len(b)==info['decompressedBytes']
 z=ctypes.CDLL(r'C:\Users\Jarky\Downloads\pcsx2-v2.8.2-windows-x64-Qt\zstd.dll');z.ZSTD_decompress.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.c_size_t];z.ZSTD_decompress.restype=ctypes.c_size_t;z.ZSTD_isError.argtypes=[ctypes.c_size_t];z.ZSTD_isError.restype=ctypes.c_uint
 assert len(packed)<64*1024*1024 and len(b)<64*1024*1024;decoded=ctypes.create_string_buffer(64*1024*1024);n=z.ZSTD_decompress(decoded,len(decoded),packed,len(packed));assert not z.ZSTD_isError(n) and n==len(b) and decoded.raw[:n]==b
 magic,hs=struct.unpack_from('<II',b);assert magic==0xffffffff and 36<=hs<=len(b)-8;fields=struct.unpack_from('<9I',b,8);version,state_size,so,ss,crc,sw,sh,po,ps=fields;assert version==9 and 0<=so<=hs-ss and 0<=po<=hs-ps;assert b[8+so:8+so+ss].decode('ascii')==info['serial'];assert state_size==info['stateSize'] and sw==info['screenshotWidth'] and sh==info['screenshotHeight'];state=b[8+hs:8+hs+state_size];assert state==(folder/'gs-state.bin').read_bytes();assert len(state)==4194813
 for i in range(4):assert struct.unpack_from('<I',state,len(state)-84+i*20+16)[0]<=15
 replay=Replay(state,sources);p=8+hs+len(state)+8192;stream=bytearray();segments=[];records=collections.Counter()
 while p<len(b):
  kind=b[p];p+=1;records[kind]+=1
  if kind==0:
   path=b[p];size=struct.unpack_from('<I',b,p+1)[0];p+=5;assert p+size<=len(b)
   if path!=3:raise ValueError('Multi-path ordering unsupported')
   segments.append((len(stream),replay.frame));stream+=b[p:p+size];p+=size
  elif kind==1:replay.frame+=1;p+=1
  elif kind==2:p+=4
  elif kind==3:p+=8192
  else:raise ValueError('Unknown dump record')
 assert p==len(b);replay.frame=0;q=0;tags=0;segment=0;image_bytes=0
 while q<len(stream):
  while segment+1<len(segments) and segments[segment+1][0]<=q:segment+=1
  replay.frame=segments[segment][1];assert q+16<=len(stream);tag,registers=struct.unpack_from('<QQ',stream,q);q+=16;loops=tag&32767;flg=(tag>>58)&3;nreg=(tag>>60)&15 or 16;size=loops*16 if flg>=2 else loops*nreg*(16 if flg==0 else 8);padded=(size+15)&~15;assert q+padded<=len(stream);tags+=1
  if flg==0 and (tag>>46)&1:replay.register(0,(tag>>47)&2047)
  if flg>=2:image_bytes+=size;replay.image(stream[q:q+size])
  else:
   for i in range(loops*nreg):
    reg=(registers>>((i%nreg)*4))&15;off=q+i*(16 if flg==0 else 8);value=struct.unpack_from('<Q',stream,off)[0];kick=True
    if flg==0 and reg==14:reg=stream[off+8]&127
    elif flg==0 and reg in(4,5):kick=not((stream[off+13]>>7)&1)
    elif flg==0 and reg not in(0,6,7,8,9,10,12,13):continue
    replay.register(reg,value,kick)
  q+=padded
 replay.finish_transfer();report={'capture':folder.name,'serial':info['serial'],'integrity':'passed','compressedSha256':info['sha256'],'records':dict(records),'gifTags':tags,'imageTransferBytes':image_bytes,'hostTransfers':replay.uploads,'localCopies':replay.localcopies,'transferModes':dict(replay.transfer_modes),'textureEvents':replay.texture_events,'drawBindingKicks':sum(replay.draws.values()),'uniqueDrawTex0':len(replay.draws),'matches':replay.matches,'unsupported':dict(replay.unsupported),'rawDumpSha256':sha(b),'scope':'Chronological supported host-image upload replay and conservative draw-target invalidation. Texture matches are exact bytes. CBP palette storage equality is NOT CLUT-cache equality or proof of unique mesh identity. No rasterization, local-to-local copy or full TEX1/CLAMP/PRMODE/CLUT-cache emulation.'}
 report['sourceImages']=len(sources);report['distinctMatchedSourcePictures']=len({(m['source'],m['pictureOffset']) for m in replay.matches});report['distinctMatchedPixelPayloadsAtDraw']=len({m['pixelSha256'] for m in replay.matches if m['stage']=='draw-binding'});report['distinctMatchedAtDraw']=len({(m['source'],m['pictureOffset']) for m in replay.matches if m['stage']=='draw-binding'});return report

if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('folder',type=Path);ap.add_argument('--source',type=Path,action='append',default=[]);ap.add_argument('--headerless',type=Path);ap.add_argument('--out',type=Path,required=True);args=ap.parse_args();sources=[]
 for path in args.source:sources+=source_pictures(path,args.headerless)
 report=run(args.folder,sources);args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps({k:report[k] for k in ['capture','integrity','gifTags','drawBindingKicks','sourceImages','distinctMatchedSourcePictures','distinctMatchedAtDraw','unsupported']}),flush=True)
