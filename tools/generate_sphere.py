"""Deterministic BWS1 fixture: 128 vertices, a closed sphere, restart strips."""
import math, struct, json
from pathlib import Path
root=Path(__file__).resolve().parents[1]/'examples'
root.mkdir(exist_ok=True)
p=[(0.,1.,0.)]
for ring in range(1,8):
    theta=math.pi*ring/8
    for j in range(18):
        phi=2*math.pi*j/18
        p.append((math.sin(theta)*math.cos(phi),math.cos(theta),math.sin(theta)*math.sin(phi)))
p.append((0.,-1.,0.))
strips=[]
for j in range(18): strips.append([0,1+j,1+(j+1)%18])
for r in range(6):
    strips.append([v for j in range(19) for v in (1+r*18+j%18,1+(r+1)*18+j%18)])
for j in range(18): strips.append([127,109+j,109+(j+1)%18])
def tris(strip):
    return [(strip[i-2],strip[i-1],strip[i]) if i%2==0 else (strip[i-1],strip[i-2],strip[i]) for i in range(2,len(strip))]
def orientation(t):
    a,b,c=[p[i] for i in t];u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)]
    n=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
    return sum(n[i]*(a[i]+b[i]+c[i]) for i in range(3))
# A repeated first vertex flips strip winding; degenerate triangles are ignored.
for s in strips:
    if orientation(tris(s)[0])<0:s.insert(0,s[0])
triangles=[t for s in strips for t in tris(s) if len(set(t))==3]
assert len(p)==128 and len(triangles)==252 and all(orientation(t)>0 for t in triangles)
edges={}
for t in triangles:
    for a,b in zip(t,(t[1],t[2],t[0])):edges[tuple(sorted((a,b)))]=edges.get(tuple(sorted((a,b))),0)+1
assert all(v==2 for v in edges.values())
indices=[v for s in strips for v in s+[0xffff]]
blob=bytearray(128);blocks={}
def add(name,data,stride):
    blob.extend(bytes((-len(blob))%16));blocks[name]={'offset':len(blob),'bytes':len(data),'stride':stride};blob.extend(data)
add('vertices',b''.join(struct.pack('<3i',*(round(x*10000) for x in v)) for v in p),12)
add('indices',struct.pack('<'+'H'*len(indices),*indices),2)
uv=[(.5,0.)]+[(j/18,ring/8) for ring in range(1,8) for j in range(18)]+[(.5,1.)]
add('uv',b''.join(struct.pack('<2e',*v) for v in uv),4)
add('normals_float32',b''.join(struct.pack('<3f',*v) for v in p),12)
add('normals_snorm16',b''.join(struct.pack('<3h',*(round(x*32767) for x in v)) for v in p),6)
fields=[1,128,len(blob),128,len(indices),252,blocks['vertices']['offset'],12,blocks['indices']['offset'],2,blocks['uv']['offset'],4,blocks['normals_float32']['offset'],12,blocks['normals_snorm16']['offset'],6,0xffff]
struct.pack_into('<4s17If',blob,0,b'BWS1',*fields,0.0001)
(root/'sphere128.bin').write_bytes(blob)
(root/'sphere128.layout.json').write_text(json.dumps({'magic':'BWS1','endianness':'little','vertex_count':128,'index_count':len(indices),'triangle_count':252,'position_scale':0.0001,'blocks':blocks},indent=2)+'\n')
print(json.dumps({'size':len(blob),'indices':len(indices),'blocks':blocks}))
