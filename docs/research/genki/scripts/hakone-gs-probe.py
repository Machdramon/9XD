from pathlib import Path
import ctypes,struct,json,hashlib,shutil
root=Path('txrd2-hakone-capture');root.mkdir(exist_ok=True)
snaps=Path(r'C:\Users\Jarky\Documents\PCSX2\snaps');source=max(snaps.glob('Tokyo Xtreme Racer Drift 2*.gs.zst'),key=lambda p:p.stat().st_mtime);compressed=source.read_bytes();assert len(compressed)<64*1024*1024
z=ctypes.CDLL(r'C:\Users\Jarky\Downloads\pcsx2-v2.8.2-windows-x64-Qt\zstd.dll');z.ZSTD_decompress.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.c_size_t];z.ZSTD_decompress.restype=ctypes.c_size_t;z.ZSTD_isError.argtypes=[ctypes.c_size_t];z.ZSTD_isError.restype=ctypes.c_uint
out=ctypes.create_string_buffer(64*1024*1024);n=z.ZSTD_decompress(out,len(out),compressed,len(compressed));assert not z.ZSTD_isError(n),n;b=out.raw[:n];(root/'hakone.gs').write_bytes(b);shutil.copy2(source,root/'hakone.gs.zst');shutil.copy2(source.with_suffix('').with_suffix('.png'),root/'hakone-reference.png')
magic,hs=struct.unpack_from('<II',b);assert magic==0xffffffff and hs>=36 and 8+hs<=len(b)
fields=['stateVersion','stateSize','serialOffset','serialSize','crc','screenshotWidth','screenshotHeight','screenshotOffset','screenshotSize'];info=dict(zip(fields,struct.unpack_from('<9I',b,8)));info['serial']=b[8+info['serialOffset']:8+info['serialOffset']+info['serialSize']].decode('ascii');info['source']=str(source);info['decompressedBytes']=len(b);info['sha256']=hashlib.sha256(compressed).hexdigest();state=b[8+hs:8+hs+info['stateSize']];assert struct.unpack_from('<I',state)[0]==info['stateVersion'];(root/'gs-state.bin').write_bytes(state)
(root/'capture-info.json').write_text(json.dumps(info,indent=2));print(json.dumps(info));print('stateHead',state[:32].hex(),'stateTail',state[-84:].hex())
