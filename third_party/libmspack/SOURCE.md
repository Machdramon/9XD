# LZX decoder dependency

Upstream: https://github.com/kyz/libmspack/tree/master/libmspack/mspack
Retrieved: 2026-10-23. Upstream files are unmodified; see COPYING.LIB (LGPL-2.1).
Only lzxd.c is compiled, as the separately replaceable nines_lzx shared library.
nines_lzx.c is the 9XD-owned bounded memory adapter, also supplied in source form.
Build with the project CMake configuration; no Xbox SDK or proprietary runtime is required.
Release packages include this complete source subset and license alongside the DLL.

SHA-256 of downloaded upstream files:

- COPYING.LIB: `a190dc9c8043755d90f8b0a75fa66b9e42d4af4c980bf5ddc633f0124db3cee7`
- lzx.h: `22b1ffa9ddc15aa5fac95d4f4f0c60486f590e55636a17ab94f3ad1b770f907f`
- lzxd.c: `bc4a14a5938d99cb1e2521a0fcc16fa44e8ea262ab0bc072e3f9003e19c816da`
- macros.h: `29a9628cd7a8881bd36defb811209c2a48852e0bf6899d579d6cb80d65337b0d`
- mspack.h: `df9e9e2b18cc145e6cf652a43ffbc88bde21bfd6fd3024318fbc2140a4efd059`
- readbits.h: `7c849edb9b939b4eb1e9e2b895b7fee3eb19e23d0a1dab87c6c504cf007010ab`
- readhuff.h: `446c7ca1cfb11a80cd1b4e86006abea3f6052059636361ddb1e227f29ccee560`
- system.h: `98997127296c7baccd61b5ade9362cab509c9664b00259f9f62e6008b7369e42`
