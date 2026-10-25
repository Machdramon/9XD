from pathlib import Path
import struct,sys
sys.path.insert(0,r'C:/Users/Jarky/.cache/codex-runtimes/codex-primary-runtime/dependencies/python');from PIL import Image,ImageDraw
b=Path(r'C:\Users\Jarky\Documents\study cases\txr zero\0x000000ba.dat').read_bytes();at=b.find(b'TIM2');p=at+128;pictures=[]
for i in range(struct.unpack_from('<H',b,at+6)[0]):
 total,pals,img,hs,nc,pf,levels,clut,kind,w,h=struct.unpack_from('<IIIHHBBBBHH',b,p);pictures.append((b[p+hs:p+hs+img],b[p+hs+img:p+hs+img+pals],w,h,4 if kind==4 else 8,nc));p+=total
def gs(x,y,width,bits):
 pw=128 if bits<=8 else 64;ph=128 if bits==4 else 64 if bits<=16 else 32;bw=32 if bits==4 else 16 if bits<=16 else 8;bh=16 if bits<=8 else 8;bx=(x%pw)//bw;by=(y%ph)//bh
 block=((by&1)|((bx&1)<<1)|((by&2)<<1)|((bx&2)<<2)|((by&4)<<2))if bits in(4,16)else((bx&1)|((by&1)<<1)|((bx&2)<<1)|((by&2)<<2)|((bx&4)<<2));xx=x%bw;yy=y%bh
 col=(xx&1)|((yy&1)<<1)|((xx&6)<<1)|((yy&6)<<3) if bits==32 else ((xx&8)>>3)|((xx&1)<<1)|((yy&1)<<2)|((xx&6)<<2)|((yy&6)<<4) if bits==16 else ((yy&2)>>1)|((xx&8)>>2)|((xx&1)<<2)|((yy&1)<<3)|((xx&2)<<3)|((((xx>>2)^(yy>>1)^(yy>>2))&1)<<5)|((yy&12)<<4) if bits==8 else ((yy&2)>>1)|((xx&8)>>2)|((xx&16)>>2)|((xx&1)<<3)|((yy&1)<<4)|((xx&2)<<4)|((((xx>>2)^(yy>>1)^(yy>>2))&1)<<6)|((yy&12)<<5)
 return ((y//ph)*((width+pw-1)//pw)+x//pw)*65536+block*2048+col*bits
def morton(x,y,w,h):
 value=0;k=0;mask=1
 while mask<w or mask<h:
  if mask<w:value|=bool(x&mask)<<k;k+=1
  if mask<h:value|=bool(y&mask)<<k;k+=1
  mask*=2
 return value
sheet=Image.new('RGB',(1000,4*230),(32,32,32));draw=ImageDraw.Draw(sheet)
for row,index in enumerate([0,20,33,38]):
 raw,pal,w,h,bits,nc=pictures[index]
 for col,mode in enumerate(['linear','morton','gs32','gs16']):
  memory=bytearray(1048576)
  if mode in('gs32','gs16'):
   transfer=32 if mode=='gs32' else 16;tw=w//2;th=(h*bits)//(transfer//2)
   for y in range(th):
    for x in range(tw):
     a=gs(x,y,tw,transfer)//8;s=(y*tw+x)*transfer//8;memory[a:a+transfer//8]=raw[s:s+transfer//8]
  im=Image.new('RGB',(w,h));pix=im.load()
  for y in range(h):
   for x in range(w):
    a=(y*w+x)*bits if mode=='linear'else morton(x,y,w,h)*bits if mode=='morton'else gs(x,y,128,bits)
    data=raw if mode in('linear','morton')else memory
    if a//8>=len(data):continue
    v=(data[a//8]>>(a%8))&((1<<bits)-1)
    if bits==8:v=(v&~24)|((v&8)<<1)|((v&16)>>1)
    pix[x,y]=tuple(pal[v*4:v*4+3])if v*4+3<=len(pal)else(255,0,255)
  im=im.resize((200,190),Image.Resampling.NEAREST);sheet.paste(im,(col*250,row*230+25));draw.text((col*250+4,row*230+4),str(index)+' '+mode,fill='white')
sheet.save('txr-analysis/txr0-storage-probes.png')
