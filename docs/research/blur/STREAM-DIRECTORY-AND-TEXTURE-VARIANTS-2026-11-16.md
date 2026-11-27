# Blur stream directory and texture variants — 2026-11-16

Research-only follow-up to RESOURCE-LINKS-2026-11-16.md. No production code/build/install,
application launch, commit or push. No game entrypoint or GPU executed.

## Recovered stream directory

The showroom's level.levelrender has a framed31-record directory at768744..790840.
Records are contiguous, beginning768752, with framed version1 bodies. Reader chain in the
original executable:0x6ebec0 ->0x6ebcd0 ->0x6eb610 ->0x6ea000 ->0x6e9230
->0x6e8ae0 (counted88-byte memory records) ->0x6e8630 ->0x6e7300.

Each record serializes:

1. Framed32-byte bounds-like data via0x6d9cd0 (keep raw interpretation).
2. At memory+0x28, framed counted pairs, each framed pair ofu32, via0x6e4b80/0x6e3e60/0x6d7350.
3. At+0x20,+0x30,+0x38, three framed counted u32 arrays, via0x6e4c10/0x6e3ee0.
4. At+0x40, stream offsetu64; +0x48, reserved spanu64; +0x50, version>0 rawu64 metric.

Do not label the last metric as actual file bytes: it is0 for the first four populated blocks.
Its exact residency/accounting meaning remains pending. JSON labels it rawMetric50.

Independent bounded parsing verifies all31 reserved ranges are2048-aligned, contiguous,
nonoverlapping and cover exactly24,725,504 stream bytes. Every start has the paired ARCH header.
The enclosing serialized count is31, and the ARCH count at block+20 equals that directory
record's pair count for ALL31 records. This is stronger than the previous signature-only scan.
Evidence: SHOWROOM-STREAM-DIRECTORY-2026-11-16.json.

## Resource subrecords are ordered by pairs, not just unique group IDs

Original streaming path0x6d31c0..0x6d3453 iterates directory+0x28 pairs in file order.
At0x6d3244..0x6d325b the first member selects Scene+0x98 with132-byte stride;
the second member enters the read context at0x6d3294..0x6d32a6, before reader0x6d3eb0.
0x6d3eb0 eventually calls body0x6ea2a0 (xref0x6d3f21).

For example block4 has55 unique groups but234 ordered pairs. Reading only55 records leaves
valid variant/update records unread. The same group occurs in several stream blocks;124 of142
groups listed by directory+0x20 occur more than once. Do not select the last encountered copy.
Three allocated group IDs0,5,9 are absent from+0x20; that alone does not establish missing assets.
All217 distinct non-global texture-reference pairs from the four RenderLists have candidate
directory membership, but membership alone does not resolve a concrete image or variant.

## Texture variants and inheritance

Texture-only resource subrecords have an outerRA0, framed mode2, and a u32 variant equal to
the ordered directory pair's second member. Variant0 includes leading resource arrays; this
probe admits only the texture-only case where all three arrays are empty. Other variants skip
that section. A counted texture list follows.

Texture entry reader0x6e3220/body around0x6e17xx..0x6e1bxx uses its context variant:

- Base variant0 includes the counted name and persistent metadata, followed by image data.
- Nonzero variants skip that name/metadata (branch0x6e1824 ->0x6e18ae), read an update state,
  and preserve existing data when it is0 (0x6e18e5 ->0x6e1b84).
- State1 admits a bounded DXT descriptor/payload in this probe; other state/compression paths
  remain rejected. Names for updates are inherited by(group,texture index), not guessed by filename.

This explains why inspecting bytes at FOURCC-28 as a universal hash was unsafe: in incremental
records that position can be mode/control data. Preserve(group,variant,index) identity and inheritance.
No uniform UV flip or normal/albedo-by-position substitution was introduced.

## Validated results

-20 entire texture-only blocks parsed through their complete ordered pair list; only zero padding
 remains to each block boundary. Their directory indexes:4,11..29.
-5 further blocks have bounded accepted prefixes (7,8,9,10,30); their tails remain explicitly
 unsupported. Other mixed geometry/texture blocks were not admitted by this strict texture-only path.
-582 nonempty image records decoded independently through DDS/Pillow;663 inherited/no-update
 entries retained. Counts include accepted prefixes; they are not complete-game coverage.
-89 named(group,texture index) identities recovered.96 of329 texture-named parameter values
 in the four showroom RenderLists now resolve to those names.233 remain unresolved by this probe,
 including global references and mixed-block resources. This is slot coverage, NOT mesh coverage.

Concrete links include:

| Parameter/layout | Group/index | Recovered name |
|---|---|---|
| g_tAlbedo / default_LN_NB_NM_NV.fx |81/0|arrowturn02_diff|
| g_tAlbedo / default_LN_NB_NM_NV.fx |84/0|arrow_diff|
| g_tAlbedo / Puddle_LN_NV.fx |87/0|puddle_alpha_diff|
| g_tMaterial / Puddle_LN_NV.fx |87/1|puddle_alpha_ipr0|
| g_tBlendNormal / Puddle_LN_NV.fx |87/2|puddle_alpha_norm|

Base names14/0 b05_stonewall_diff and14/1 b05_stonewall_ipr0 are also explicit paired images.
An ipr0/material/normal image must not be used as albedo just because it is in the same group.

See SHOWROOM-TEXTURE-VARIANT-VALIDATION-2026-11-16.json for exact hashes, offsets, variants,
dimensions and slot mappings. A private contact sheet of recovered named variants was generated
and inspected: workspace blur-research/named-texture-variants-preview.png. No extracted pixels
or source game binaries were added to the repository.

## Tests, limits and next owner

Executed private parse_stream_directory.py, group_directory_links.py, parse_texture_groups.py,
validate_texture_links.py and preview_named_variants.py. Directory enclosure/count, archive counts,
range coverage/alignment, pair variant values, source bounds, DXT lengths/padding and image decode
were checked. Unsupported records stop their block; accepted prefixes remain visibly partial.
The probe accepts base/full mip layouts already validated by earlier work and adds deterministic
aligned-blob validation where a signature-only successor test had rejected a record.

Next Codex: lift mixed-resource buffers from0x6ea2a0 and its child readers, finish update/compression
tails, build variant selection from original metadata, and link one packet's vertex/index ranges,
material and decoded images into a verified textured draw. Complete maps/native app integration
remain pending. Pixel decoding and reference consistency do not verify UVs or original shading.
