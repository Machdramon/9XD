# Repeatable local-AI testing

`decoder_probe` is a headless development adapter around the production image/mesh readers and setup validators. `tools/ai_benchmark.py` supplies the bounded test loop. Neither executes input files or applies proposals to the current UI session.

## Run

Build `decoder_probe` in the project's configured compiler/Qt environment:

```powershell
cmake --build --preset windows-local --target decoder_probe
python tools/test_ai_benchmark.py
python tools/ai_benchmark.py "C:/Users/Jarky/.codex/.chatgpt-projects/own tests" --probe build/windows-local/decoder_probe.exe --output test-results/own-tests --guided
```

Qt runtime libraries must be available in the executable directory or on PATH, as with other developer builds. The probe is enabled by default when BUILD_TESTING is enabled; use `-DWORKBENCH_BUILD_DECODER_PROBE=ON` to enable it explicitly in another configuration.

Use `--no-ai` for the deterministic baseline. Omit `--guided` to withhold independent findings from the first model round. `--names file1 file2` selects cases. `--rounds 1` reduces inference work. The default local model is `qwen3:4b`; `--model` and `--endpoint` are configurable. Remote models/endpoints, proxies and HTTP redirects are not used.

The current runner inspects up to 32 regular files at the folder's top level. It ignores symlinks. The output folder must be outside the source folder. Individual inputs are limited to 256 MiB. The image decoder retains its existing 32 MiB encoded-input limit and raw-image limits; meshes retain their existing 100,000-vertex / 600,000-index limits. Do not silently clamp counts or raise limits to turn a failed test into a pass.

## Loop

1. Snapshot/hash each file, identify bounded signatures and metadata, and check recipe dependencies.
2. Run deterministic container candidates through the production decoder adapter.
3. Ask the local model for JSON: a short hypothesis, up to two bounded reads, and up to two decoder proposals.
4. Validate proposals in separate probe processes. Use only the source file chosen by the runner. There is no model-selected path, shell command or script execution.
5. Return verified facts, requested bytes and decoder receipts for one refinement round.
6. Check the original file hash again, save results and prepare app-loadable recipes/chat prompts for accepted candidates.

Limits: two model rounds per file, two reads of at most 256 bytes per round, 60-second model request timeout, 20-second decoder-process timeout, and a circuit breaker after three consecutive model failures. Signature/metadata scans and emitted candidates are capped; this is not exhaustive archive extraction.

`structured_test_facts.py` independently checks small pieces of TIM2/RIFF/GLB/CrCh metadata. It does not decode texture pixels or vertex buffers itself. Actual image/mesh interpretation stays in the C++ code shared with 9XD. GLB candidates test ordinary local Float32 positions and triangle indices; transformations, packed attributes, skins, compression extensions and full scenes are outside this adapter.

## Outputs and review

- `REPORT.md` separates independent facts from **unverified AI text**.
- `report.json` and per-case JSON retain evidence, proposals, read results, errors, token counts, timings and source hashes.
- `OPEN-IN-WORKBENCH.md` links accepted `.bwir` recipes and generated mesh chat prompts. Open the matching original binary before applying them.
- Accepted-candidate provenance records the source hash and remains marked **pending human visual review**. A readable image or finite mesh is not proof of correct semantics.

Model replies are cached using evidence (including source hash), model digest and `VERSION`. Bump VERSION when changing prompt policy/schema/model options. Decoder results are recomputed on each run; identical proposals can reuse a receipt within one unchanged case. Cache and reports contain local file samples, so keep them with the user's project data rather than publishing them by default.

For a future in-app workflow, expose the test loop as a cancellable background job with phase progress, candidate previews and explicit promotion of reviewed results into regression fixtures. Keep the model a proposal generator; use application-owned decoder results for success/failure and generate factual status text from those receipts.
