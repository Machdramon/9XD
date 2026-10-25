# Asset memory budget

Settings > Performance provides a synchronized slider and numeric value from 1 to 64 GiB. The default and Restore Defaults value is 8 GiB. Apply/OK persists `performance/assetMemoryGiB`; Cancel leaves the applied value unchanged. Startup loads it through the existing workspace preferences. Memory is allocated on demand, not reserved in advance.

The shared policy is used by preview batch estimates, raw mesh validation/decoding, reusable mesh profiles, GLB candidates and NFSU2 scene aggregation. Vertex/index and source-range validation remains mandatory. Estimated geometry cost is 96 bytes per vertex plus 32 bytes per index to allow for source mapping, assembly, staging and rendering copies; raw decoder input buffers also count against the operation's estimate. Actual memory depends on the workflow and hardware.

Discovery and profile-test input windows scale to one quarter of the configured budget, leaving room for parsing and derived data. NFSU2 stream inspection uses this same input budget instead of its former 128 MiB cap. Scene part sizes and instance metadata limits scale with the configured budget. The full-scene result appears when its estimate fits; sector alternatives remain available.

This is an estimated budget per asset operation/batch, not a hard limit on the process working set, all resident scenes together, or GPU memory. Already loaded assets remain resident. Raising the value does not reserve RAM, guarantee allocation success or bypass invalid data, unknown formats, hardware restrictions, bounded parser complexity, image limits or the separate compressed-resource expansion protections. Generic pattern search and platform-specific reverse-engineering evidence readers retain their own bounded search rules.

Tests cover defaults, slider/value synchronization, Apply, persistence, Cancel, Restore Defaults, budget-sensitive geometry admission and production decoding of 100,001 vertices. Optional NFSU2 tests validate the small fixture and discovery of the large stream's 48,224 resolved sector instances with the full-scene alternative available at the default budget.
