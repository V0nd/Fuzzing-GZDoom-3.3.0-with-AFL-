# FUzzing Development Diary

A chronological log of fuzzes and  harness iterations during the GZDoom WAD parser
fuzzing project. Documents design decisions, observed problems, and
solutions across multiple iterations.

## Iteration 1: Initial harness (05/05/2026)

### Goal
Build a minimal working harness that wraps `CheckWad()` for AFL++ fuzzing.

### Approach
- Wrap AFL input in `FileReader::OpenMemory()`
- Call `CheckWad()` to parse
- Catch all C++ exceptions to prevent harness-side crashes

### Result
- ✅ Harness builds and runs successfully
- ✅ Smoke test with `freedoom1.wad` returns exit 0
- ❌ After 11 hours fuzzing: 0 saved crashes

### Diagnosis
The `catch(...)` clause suppressed all C++ exceptions, including those
that should propagate to AFL as crashes. I tested it with crafted invalid
WAD (NumLumps=0xFFFFFFFF). It should have lead to crash, I got exit code 0. 
Harness was hiding all bugs from AFL, confirmed.

---

## Iteration 2: Remove all exception handling (07/05/2026)

### Goal
Allow all C++ exceptions and signals to propagate to AFL.

### Approach
Removed `try/catch` entirely. Let any exception or signal
become a crash visible to AFL.

### Result
- ✅ Crafted invalid WAD now exits with code 134 (SIGABRT)
- ✅ Validated harness propagates crashes correctly
- ⚠️ After 2 minute fuzzing: total_crashes = 2500, saved_crashes = 1

### Observation
AFL deduplicates crashes by coverage path, so 2500 → 1 unique. But
running the saved crash manually showed:

id:000000,sig:06,src:000000,time:3643,execs:1844,op:(null),pos:0

```
terminate called after throwing an instance of 'CRecoverableError'
Aborted
```

It's GZDoom's intentional rejection
of malformed WADs via `I_Error()`. The parser working as designed.

### Insight
"Catching nothing" was just as wrong as "catching everything." I need more selective `try/catch` statement.

---

## Iteration 3: Selective exception handling (12/05/2026)

### Goal
Filter parser's intentional error events while preserving real bugs.

### Investigation

GZDoom's exception hierarchy (in `doomerrors.h`):

```
CDoomError (base)
├── CRecoverableError - parser detected invalid input (intentional)
├── CFatalError - severe condition (potentially interesting)
├── CNoRunExit - exit logic running
├── CFraggleScriptError - scripting subsystem
└── CVMAbortException - VM execution issues
```

Searched for `I_FatalError` calls — all in OpenGL/shader code paths
(`gl/renderer/`, `gl/shaders/`), never in WAD parser. So `CFatalError`
is irrelevant for the fuzzing target.

### Approach
Catch only `CRecoverableError`:

```cpp
try 
{
    result = CheckWad("fuzz_input.wad", reader, true);
}
catch (const CRecoverableError&) 
{
    // Parser's intentional rejection - not a bug
}
// All other exceptions propagate to AFL
```

### Result
- ⚠️ After 4.5 hours fuzzing / 4.5M executions:
  - `total_crashes`: 0
  - `saved_crashes`: 0
  - `corpus_count`: 81 (stagnation)
  - `last_new_find`: 3h 52min ago at time of analysis

Parse validation grafecully handles all "obvious broken" inputs at 
file, header, lump and read level.

All detected errors funnel through `I_Error()` → `CRecoverableError`
exception path resulting in graceful rejection rather than crashes.

### Contemplation
Could there be bugs on deeper levels like lumps, JPEG textures, MIDI music, ...
I like the sound of the lump content processing. Attacker controlled 'Position'
and 'LumpSize' values get used in seek and read operations.

## Iteration 4: Lump-level harness (ongoing)

### Hypothesis
The `CheckWad` harness only tries container parsing — header validation
and lump directory reading. The actual lump *content* processing happens
later, in `FWadFileLump::FillCache()`, which uses the `Position` and
`LumpSize` values stored during `CheckWad`. Could there be validation gaps?

### Approach
Extend harness to lump-level processing.

