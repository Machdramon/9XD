# Genki reverse-engineering references

Updated: 2026-10-25. This is the canonical shared research location for ChatGPT, Claude and the project owner. No application source changed during migration.

- [Cross-analysis checklist](GENKI-CROSS-ANALYSIS-TODO.md)
- [Capture inventory](GENKI-CAPTURE-INVENTORY.json)
- [March source provenance — exact match](txrd2-march-capture/SOURCE-PROVENANCE.md)
- [Coordination plan](../../COORDINATION-PLAN.md)

Nine capture folders have been copied here, preserving the original ChatGPT workspace copies. MIGRATION-MANIFEST.json records SHA-256 and size checks for every copied capture file. Capture scripts are research helpers under scripts/; their relative inputs assume this directory is the working directory, and original corpus paths remain local to the workstation. Inspect each script before adapting paths. Historical absolute paths in evidence are preserved as provenance, not portable locations.

Compressed GS originals and reference PNGs are preserved. Decompressed .gs files, gs-state.bin and the reextracted game payload remain local copies but are ignored by Git because they are reproducible. The original game archives are not included. Some older research helpers assume the prior workspace layout; migration does not certify that every helper is portable.

Nikko supporting reports and the contact sheet are included under c1-track-research/. The game resource staging ZIP and original assets are still in the previous local research workspace; their absolute source paths and hashes are in the reports. Source files, coordinates and variant identities must not be inferred from shared textures alone.

March provenance was independently verified against BUILD.TOC/BUILD.DAT through a bounded single-entry reextraction. Native headerless-bank parsing, draw tracing and per-mesh selection remain open. QuickBMS was used only for research verification and is not an application dependency.

## Public-source policy — 2026-10-25

Research reports, scripts and inventories are shareable project documentation. Game-derived compressed dumps and images remain local and Git-ignored, along with raw states and reextracted payloads. The portable application package excludes this research directory. No game assets are published by the 0.3.6 release.
