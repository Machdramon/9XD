"""Generate a private procedural nonplanar camera-translation fixture (NumPy/Pillow).
Output: ignored build/reconstruction-tools/sfm-fixture; no external media required.
"""
import numpy as np
from PIL import Image,ImageDraw
from pathlib import Path
import json
root=Path(__file__).resolve().parents[1]/'build/reconstruction-tools/sfm-fixture';(root/'images').mkdir(parents=True,exist_ok=True)
rng=np.random.default_rng(42)
textures=[]
for j in range(5):
 a=rng.integers(70,220,(768,768,3),dtype=np.uint8);im=Image.fromarray(a);d=ImageDraw.Draw(im)
 for k in range(500):
  x,y=rng.integers(0,740,2);sz=int(rng.integers(6,30));color=tuple(int(v) for v in rng.integers(0,256,3));d.rectangle((x,y,x+sz,y+sz),outline=color,width=3);d.text((x,y),str(k),fill=(0,0,0))
 textures.append(np.asarray(im))
# normal-axis, plane position, two other-axis bounds; texture layouts are fixed in world space.
planes=[(2,1.5,[-3,3],[-2,2]),(2,-.4,[-.9,.2],[-.7,.5]),(2,.3,[.6,1.5],[-.9,.9]),(1,1.1,[-3,3],[-1,3]),(0,-1.4,[-1,1],[-.6,1.5])]
w,h=640,480;u,v=np.meshgrid(np.arange(w),np.arange(h));pixels=np.stack(((u-w/2)/560,(v-h/2)/560,np.ones_like(u)),axis=-1)
for frame,x in enumerate(np.linspace(-.9,.9,12)):
 eye=np.array([x,-.08*np.sin(frame/3),-3.5]);forward=np.array([0,0,.3])-eye;forward/=np.linalg.norm(forward);right=np.cross(forward,[0,-1,0]);right/=np.linalg.norm(right);down=np.cross(forward,right);rays=pixels@np.stack((right,down,forward));depth=np.full((h,w),1e9);rgb=np.zeros((h,w,3),dtype=np.uint8)
 for k,(axis,pos,lo,hi) in enumerate(planes):
  axes=[a for a in range(3) if a!=axis]
  with np.errstate(divide='ignore',invalid='ignore'):t=(pos-eye[axis])/rays[:,:,axis]
  xyz=eye+rays*t[:,:,None];a,b=xyz[:,:,axes[0]],xyz[:,:,axes[1]];valid=(t>0)&(t<depth)&(a>=lo[0])&(a<=lo[1])&(b>=hi[0])&(b<=hi[1]);ta=np.clip((a-lo[0])/(lo[1]-lo[0])*767,0,767).astype(int);tb=np.clip((b-hi[0])/(hi[1]-hi[0])*767,0,767).astype(int);rgb[valid]=textures[k][tb[valid],ta[valid]];depth[valid]=t[valid]
 Image.fromarray(rgb).save(root/'images'/f'frame-{frame+1:06}.png')
(root/'job.json').write_text(json.dumps({'schema':1,'state':'frames_ready','sourceType':'synthetic','source':'procedural nonplanar scene','fps':1,'maxFrames':12,'maxSize':640,'frameCount':12}));print(root)
