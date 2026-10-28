import os
import os,sys,math,collections,json,re
ROOT=os.environ.get('STUDY_CASES',r'C:\Users\Jarky\Documents\study cases')
SIGS={b'0IVX':'0IVX',b'LDMX':'LDMX',b'MRON':'MRON',b'TIM2':'TIM2',b'ARCP':'ARCP',b'DDS ':'DDS',b'\x89PNG':'PNG',b'GBIX':'GBIX',b'PVRT':'PVRT',
 b'NDP3':'NUD/NDP3',b'NTP3':'NUT/NTP3',b'NTWU':'NUT/NTWU',b'NTXR':'NTXR',b'GXT\x00':'GXT',b'AFS\x00':'AFS',b'PK\x03\x04':'ZIP',b'\x1f\x8b\x08':'GZIP',
 b'FArC':'FArC',b'FArc':'FArc',b'ENDI':'ENDI',b'SCNE':'SCNE',b'MESH':'MESH',b'WMSH':'WMSH',b'RIFF':'RIFF',b'BIKi':'BINK',b'UnityFS':'UnityFS',
 b'XPR0':'XPR0',b'XPR2':'XPR2',b'TXP3':'TXP3',b'MIG.':'MIG/GIM',b'GIM':'GIM',b'TMX':'TMX',b'VAGp':'VAG',b'SShd':'SShd',b'\x00\x00\x01\x00':'?',
 b'BRES':'BRES',b'MDL0':'MDL0',b'TEX0':'TEX0',b'GT2K':'GT2K',b'GTPS':'GTPS',b'MDLS':'MDLS',b'TXS3':'TXS3',b'Tex1':'Tex1',b'XMDL':'XMDL',b'0IVX00':'0IVX',
 b'.TMR':'TMR',b'TMR':'TMR',b'\x78\x9c':'zlib9c',b'\x78\xda':'zlibda',b'Model':'Model',b'NUD':'NUD',b'ACTOR':'ACTOR',b'ACPRIM':'ACPRIM',b'IMGF':'IMGF',
 b'GXTX':'GXTX',b'FBX':'FBX', b'CRID':'CRID', b'@UTF':'CRI@UTF', b'CPK ':'CPK', b'BND3':'BND3', b'BHF3':'BHF3', b'SCE\x00':'SCE', b'RTSF':'RTSF'}
def ent(b):
    if not b: return 0
    c=collections.Counter(b);n=len(b);return -sum(v/n*math.log2(v/n) for v in c.values())
rows=[]
for dp,dn,fn in os.walk(ROOT):
    for f in sorted(fn):
        p=os.path.join(dp,f);rel=os.path.relpath(p,ROOT)
        d=open(p,'rb').read()
        head=d[:32]
        hits=collections.Counter()
        for s,name in SIGS.items():
            if len(s)<4: continue
            n=d.count(s)
            if n: hits[name]+=n
        e0=ent(d[:65536]);em=ent(d[len(d)//2:len(d)//2+65536])
        asc=''.join(chr(c) if 32<=c<127 else '.' for c in head)
        rows.append((rel,len(d),head.hex(' '),asc,round(e0,2),round(em,2),dict(hits.most_common(8))))
for r in rows:
    print(f"{r[0]}  [{r[1]}]  H={r[4]}/{r[5]}\n    {r[2][:71]}\n    {r[3]}  {r[6]}")
