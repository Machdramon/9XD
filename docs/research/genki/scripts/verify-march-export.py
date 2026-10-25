from pathlib import Path
import json,struct,hashlib
p=Path('genki-integration/exports/txrd2_car_000002a0.sdb.glb');b=p.read_bytes();n=struct.unpack_from('<I',b,12)[0];g=json.loads(b[20:20+n]);off=20+n;raw=b[off+8:];images=[]
for image in g.get('images',[]):
 v=g['bufferViews'][image['bufferView']];data=raw[v.get('byteOffset',0):v.get('byteOffset',0)+v['byteLength']];assert data[:8]==b'\x89PNG\r\n\x1a\n';images.append({'width':struct.unpack_from('>I',data,16)[0],'height':struct.unpack_from('>I',data,20)[0],'sha256':hashlib.sha256(data).hexdigest()})
faces=sum(g['accessors'][p['indices']]['count']//3 for m in g['meshes'] for p in m['primitives']);textured=sum('baseColorTexture' in m.get('pbrMetallicRoughness',{}) for m in g['materials']);assert len(g['meshes'])==54 and faces==8328 and textured==19 and len({i['sha256'] for i in images})==3
r={'file':str(p),'meshes':54,'triangles':faces,'texturedMaterials':textured,'embeddedImageEntries':len(images),'uniqueImagePayloads':3,'uniqueImageDimensions':sorted({(i['width'],i['height']) for i in images}),'fbxExists':p.with_suffix('.fbx').is_file(),'scope':'GLB geometry/material/image structure checked; FBX export success is checked by Qt corpus regression.'};Path('genki-integration/export-validation.json').write_text(json.dumps(r,indent=2));print(json.dumps(r))
