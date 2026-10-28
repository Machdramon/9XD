# Super Trucks executable / missing UV analysis - 2026-10-28

## Scope

Read-only analysis requested by owner. No production app source or executable changed.
Original PS2 ELF: SLUS_214.50, 4,685,872 bytes; SHA256 3fc97ab1719e326143983d2833a564662b2087c22d20c4aa9ece5eada3c384fe.
ELF32 little-endian MIPS (machine8); symbol/string symbol tables empty. Loaded section file
offset0x80 maps to virtual0x100000. Addresses below are ELF virtual addresses, not file offsets.
Capstone MIPS disassembly used from pre-existing private dependencies; R5900-only instructions
remain undecoded .byte entries. No complete decompilation, emulator capture or microcode emulation claimed.

## Executable evidence

The serialized cluster descriptor table preserves names and VIF-format templates:

| Descriptor VA | Name string VA | Cluster | VIF template word |
|---|---|---|---|
| 0x535e20 | 0x535da0 | PS2xyzw.csl | 0x6c002001 |
| 0x535e30 | 0x535db0 | PS2uv.csl | 0x64002001 |
| 0x535e40 | 0x535dc0 | PS2uv2.csl | 0x6c002001 |
| 0x535e50 | 0x535dd0 | PS2rgba.csl | 0x6e006001 |
| 0x535e60 | 0x535de0 | PS2normal.csl | 0x6e002001 |

PS2uv uses V2-32 (command0x64); PS2uv2 uses V4-32 (command0x6c). This is direct
cluster metadata evidence of an additional four-component UV stream, not merely float-pattern guessing.
A code reference at0x1ce308 loads the string "PS2 material pipes" at0x535e90 in a pipeline
registration/error path. It does not by itself establish texture-stage binding or game shading.

## Packet comparison

Private instrumented copy of current rw_core.h (namespace RwAudit, not installed) compared
BARCELL and STADIO native batches. In BARCELL dry SLI, 331 position-bearing batches lacked
recognized UVs; each included V4-32 UNPACK at masked VU address1 (command0x6c, immediate0x8001).
Positions use V3-32 at address0. Existing production nativeMesh accepts V2-32 UVs only and ignores
that V4-32/address1 stream. The executable UV2 cluster and the packets independently agree.

## Private UV2 prototype

Prototype adds V4-32/address1 interpretation at stride16, selecting the first two Float32
components as primary UVs, leaving all existing position/index decoding unchanged. Four values
may represent multiple texture coordinates, but exact secondary-stage semantics remain unproven.
No UV flip, scaling guess, texture replacement, or material-name heuristic was introduced.

| File | Prior textured triangle coverage | Prototype UV + image-ready triangles | UV2 packets | UV/position count mismatches |
|---|---:|---:|---:|---:|
| ALBACETE.MIX | 84.34% | 52,136/52,136 | 450 | 0 |
| ALPI.MIX | 79.84% | 21,489/21,489 | 306 | 0 |
| BARCELL.MIX | 72.12% | 20,138/20,138 | 331 | 0 |
| BERLINO.MIX | 84.47% | 36,177/36,177 | 348 | 0 |
| CARSEL.MIX | 100.00% | 892/892 | 0 | 0 |
| CORSICA.MIX | 77.39% | 23,853/23,853 | 319 | 0 |
| COSTA.MIX | 84.22% | 28,372/28,372 | 297 | 0 |
| ENVSPHER.MIX | 100.00% | 809/809 | 0 | 0 |
| HIGH.MIX | 79.27% | 27,884/27,884 | 308 | 0 |
| STADIO.MIX | 95.40% | 17,561/17,561 | 56 | 0 |
| TOSCANA.MIX | 76.64% | 17,819/17,819 | 265 | 0 |
| VALENCIA.MIX | 78.67% | 17,281/17,281 | 241 | 0 |

All twelve archives (ten circuits and two ancillary resources) retain the original material
triangle counts, have zero UV2 nonfinite first-pair values and zero UV/position count mismatches.
Every default material surface now has UV data plus an already validated named image binding.
This yields 100% potential primary-texture coverage in the private prototype, not a production
installed/rendered result and not proof of original shader fidelity. Existing images were validated
in the preceding production corpus run (2,237 raster decodes); this prototype tests geometry/UV
readiness and binding metadata rather than re-rendering all textures.

## Integration recommendation and validation remaining

1. Add a bounded, explicitly identified V4-32 UV path to nativeMesh. Preserve V3/V4 positions at
   address0; never mistake position W/ADC for UV. Require full UV-per-vertex coverage, finite data,
   no ambiguous duplicate UV streams. Avoid silently treating any arbitrary V4 UNPACK as UV.
2. Add synthetic V2/V4-32 channel and address-isolation tests, truncated/malformed packets and
   actual BARCELL/STADIO/ALBACETE coverage tests, plus D1GP regression.
3. Verify primary pair/channel and orientation visually on road markings/signs, then repeat
   actual production mesh/image decoding and export tests. Full shader, second UV set, blend/
   environment passes and Selene vehicle placements are separate work.

Next owner: Codex, implement/test UV2 path when continuing integration. No production code, build,
commit, push or release in this analysis. Source game files unchanged. Private evidence in workspace
supertrucks-research: elf_probe.py, executable-strings.private.txt, exe_xrefs.py,
executable-xrefs.private.json, rw_audit.h, audit.cpp/cmd, vif-uv-audit.private.json,
uv2.cpp/cmd, uv2-prototype.private.json. Public report contains derived findings only.
