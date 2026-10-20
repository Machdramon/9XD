import json
from pathlib import Path
import struct
import unittest
from ai_benchmark import inspect,LocalModel
from structured_test_facts import extract

class EvidenceTests(unittest.TestCase):
    def glb(self,mutate=None):
        g={'asset':{'version':'2.0'},'buffers':[{'byteLength':42}],
           'bufferViews':[{'buffer':0,'byteOffset':0,'byteLength':36},{'buffer':0,'byteOffset':36,'byteLength':6}],
           'accessors':[{'bufferView':0,'componentType':5126,'count':3,'type':'VEC3'}, {'bufferView':1,'componentType':5123,'count':3,'type':'SCALAR'}],
           'meshes':[{'primitives':[{'attributes':{'POSITION':0},'indices':1}]}]}
        if mutate:mutate(g)
        raw=json.dumps(g).encode();raw+=b' '*((-len(raw))%4);binary=bytes(44)
        body=struct.pack('<I4s',len(raw),b'JSON')+raw+struct.pack('<I4s',len(binary),b'BIN\0')+binary
        return b'X'*17+struct.pack('<4sII',b'glTF',2,12+len(body))+body
    def test_embedded_glb_offsets(self):
        data=self.glb();facts,candidates=extract(data)
        self.assertEqual(len(candidates),1);s=candidates[0]['settings'];self.assertEqual(s['vertexCount'],3);self.assertEqual(s['indexCount'],3)
        self.assertEqual(int(s['indexOffset'])-int(s['vertexOffset']),36);self.assertEqual(facts[0]['offset'],'17')
    def test_rejects_accessor_outside_view(self):
        facts,proposals=extract(self.glb(lambda g:g['accessors'][0].update(count=100)))
        self.assertTrue(facts);self.assertEqual(proposals,[])
    def test_external_buffer_not_read(self):
        self.assertEqual(extract(self.glb(lambda g:g['buffers'][0].update(uri='../outside.bin'))),([],[]))
    def test_malformed_json_types_do_not_crash(self):
        for mutate in (lambda g:g.update(asset='bad'),lambda g:g.update(buffers=[None]),lambda g:g.update(meshes=[None]),lambda g:g['meshes'][0]['primitives'][0]['attributes'].update(POSITION=[])):
            extract(self.glb(mutate))
    def test_material_counts_and_missing_dependency(self):
        facts,_=inspect(Path('test.mtl'),b'newmtl A\nKd 1 0 0\nnewmtl B\n',set())
        self.assertEqual(facts['independentFacts'][0]['materials'],2)
        facts,_=inspect(Path('recipe.tmr'),b'{"file":"absent.bin"}',set())
        self.assertFalse(facts['independentFacts'][1]['presentInTestFolder'])
    def test_signature_without_valid_header_is_not_a_candidate(self):
        facts,proposals=inspect(Path('bad.dat'),b'TIM2'+bytes(100),set())
        self.assertTrue(facts['signatures']);self.assertFalse(proposals)
    def test_remote_endpoints_rejected_before_connection(self):
        for endpoint in ('https://localhost','http://example.com','http://localhost@remote.test','http://localhost/redirect'):
            with self.assertRaises(ValueError):LocalModel(endpoint,'qwen3:4b',Path('.'))

if __name__=='__main__':unittest.main()
