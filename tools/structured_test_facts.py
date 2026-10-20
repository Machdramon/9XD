"""Small, bounded evidence extractors for the test runner, not a full asset importer.

glTF layout reference: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
Only embedded GLB buffer 0, ordinary non-sparse Float32 VEC3 positions and
UInt16/UInt32 triangle indices are proposed. Skinning/transforms/materials are not imported.
"""
import json
import struct

def extract(data):
    facts=[];proposals=[]
    if data.startswith(b'CrCh') and len(data)>=16:
        version,count,table=struct.unpack_from('<III',data,4)
        if version==0x746 and 0<count<=1024 and table>=16 and table+count*16<=len(data):
            chunks=[]
            for i in range(count):
                kind,revision,identifier,size,offset=struct.unpack_from('<HHIII',data,table+i*16)
                if offset<table+count*16 or offset+size>len(data):break
                chunks.append({'type':hex(kind),'revision':revision,'id':identifier,'offset':str(offset),'size':size})
            if len(chunks)==count:facts.append({'check':'consistent CrCh 0x746 chunk table','count':count,'chunks':chunks[:16],'note':'Bounds verified; chunk meanings not decoded.'})
    at=0
    for _ in range(4):
        at=data.find(b'glTF',at)
        if at<0:break
        start=at;at+=4
        if start+20>len(data):continue
        version,total=struct.unpack_from('<II',data,start+4)
        if version!=2 or total<28 or start+total>len(data):continue
        n,tag=struct.unpack_from('<I4s',data,start+12)
        if tag!=b'JSON' or n>1024*1024 or start+20+n>start+total:continue
        raw=data[start+20:start+20+n]
        try:g=json.loads(raw.rstrip(b'\x00 \t\r\n'))
        except (ValueError,UnicodeError):continue
        if not isinstance(g,dict) or not isinstance(g.get('asset'),dict) or g['asset'].get('version')!='2.0':continue
        p=start+20+n
        if p+8>start+total:continue
        length,tag=struct.unpack_from('<I4s',data,p)
        if tag!=b'BIN\0' or p+8+length>start+total:continue
        buffers=g.get('buffers',[])
        if not isinstance(buffers,list) or not buffers or not isinstance(buffers[0],dict) or 'uri' in buffers[0] or type(buffers[0].get('byteLength'))!=int or not 0<=buffers[0]['byteLength']<=length:continue
        base=p+8;views=g.get('bufferViews',[]);accessors=g.get('accessors',[]);meshes=g.get('meshes',[])
        if not all(isinstance(v,list) for v in (views,accessors,meshes)):continue
        fact={'check':'bounded embedded GLB 2.0 metadata','offset':str(start),'length':total,'meshes':len(meshes),'accessors':len(accessors),
              'jsonNullPadding':raw.endswith(b'\0'),'note':'Raw local positions only; node transforms, skins and materials are not applied. Null JSON padding is tolerated for evidence extraction and recorded.', 'primitives':[]}
        def accessor(index,typ,components):
            if type(index)!=int or not 0<=index<len(accessors):return None
            a=accessors[index]
            if not isinstance(a,dict) or 'sparse' in a or a.get('componentType') not in typ or a.get('type')!=components:return None
            vi=a.get('bufferView')
            if type(vi)!=int or not 0<=vi<len(views):return None
            v=views[vi]
            if not isinstance(v,dict) or v.get('buffer',0)!=0 or v.get('extensions'):return None
            count=a.get('count');item=(4 if a['componentType'] in (5125,5126) else 2)*(3 if components=='VEC3' else 1)
            stride=v.get('byteStride',item);local=a.get('byteOffset',0);offset=v.get('byteOffset',0);size=v.get('byteLength',0)
            if any(type(x)!=int for x in (count,stride,local,offset,size)) or count<1 or stride<item or local<0 or offset<0 or size<0 or local+(count-1)*stride+item>size or offset+size>buffers[0]['byteLength']:return None
            return {'offset':str(base+offset+local),'count':count,'stride':stride,'componentType':a['componentType']}
        seen=set()
        for mi,m in enumerate(meshes[:16]):
            if not isinstance(m,dict) or not isinstance(m.get('primitives'),list):continue
            for pi,primitive in enumerate(m.get('primitives',[])[:32]):
                if not isinstance(primitive,dict) or not isinstance(primitive.get('attributes'),dict):continue
                position=primitive.get('attributes',{}).get('POSITION');index=primitive.get('indices');key=(position,index)
                if type(position)!=int or type(index)!=int:continue
                if key in seen or len(fact['primitives'])>=4:continue
                seen.add(key)
                if primitive.get('mode',4)!=4 or primitive.get('extensions'):continue
                pos=accessor(position,{5126},'VEC3');idx=accessor(index,{5123,5125},'SCALAR')
                if not pos or not idx:continue
                settings={'vertexOffset':pos['offset'],'vertexCount':pos['count'],'vertexStride':pos['stride'],'positionType':'Float32','positionScale':1,
                          'indexOffset':idx['offset'],'indexCount':idx['count'],'indexStride':idx['stride'],'indexType':'UInt16' if idx['componentType']==5123 else 'UInt32',
                          'topology':'Triangle list','uv':False,'normals':False}
                fact['primitives'].append({'mesh':mi,'primitive':pi,'settings':settings,'exceedsCurrentVertexLimit':pos['count']>100000})
                proposals.append({'kind':'mesh','settings':settings,'basis':'Bounded GLB position/index accessors; UVs/normals omitted rather than reinterpreted.'})
        facts.append(fact)
    return facts,proposals
