# Harness Implementation

## Architecture

The harness consists of two components:

1. **`LLVMFuzzerTestOneInput(data, size)`** — libFuzzer-style API
   - Wraps AFL input bytes into `FileReader` via `OpenMemory()`
   - Calls `CheckWad()` to parse the data
   - Cleans up the returned `FResourceFile` object
   - Catches C++ exceptions to suppress bugs

2. **`main(argc, argv)`** — standalone
   - Reads file from `argv[1]` into memory
   - Calls `LLVMFuzzerTestOneInput` with the buffer
   - Used for smoke testing without AFL

The full source: [`scripts/harness.cpp`](/scripts/harness.cpp)


## CMakeLists modification

Harness added to `src/CMakeLists.txt` as a parallel target to `zdoom`:

```cmake
set( SYSTEM_SOURCES_HARNESS ${SYSTEM_SOURCES} )
list( REMOVE_ITEM SYSTEM_SOURCES_HARNESS posix/sdl/i_main.cpp )

add_executable( harness
    posix/sdl/harness.cpp
    # ... same sources as zdoom, minus i_main.cpp ...
)

target_link_libraries( harness ${ZDOOM_LIBS} gdtoa dumb lzma )
```
`i_main.cpp` is excluded because the harness has its own `main` definition.
This creates two binaries: the normal `gzdoom` and `harness` for fuzzing.

## Forward declarations vs. full includes (FResourceFile)

The harness initially used a forward declaration of `FResourceFile` (see [harness-design.md](harness_design.md)). This worked for calling `CheckWad`, but broke down when deleting the result.

### Deleting incomplete type

The forward declaration of `FResourceFile` was fine for **calling** `CheckWad`, but causes warning when **deleting** the returned object:
```
warning: deleting pointer to incomplete type 'FResourceFile' may cause
undefined behavior [-Wdelete-incomplete]
delete result;
```

Now the choice made during harness design came to haunt me back. The compiler does not know the full class size for proper deallocation and whether the `FResourceFile` has a virtual destructor.

For a fuzzing harness this is quite imporant. AFL++ runs the harness thousands of times per second. The `delete result` calls only `FResourceFile::~FResourceFile`, skipping the `FWadFile` specific cleanup (Lumps array deallocation, Filename deallocation). This leaks memory on every harness iteration. This would eventually cause an OOM crash that AFL would attribute to the target resulting in a false positive.

### Fix: full include

Replace the forward declaration of `FResourceFile` with the full header:

```cpp
#include "resourcefiles/resourcefile.h"

//forward declaration of CheckWad is still needed (not in any header)
FResourceFile *CheckWad(const char *filename, FileReader &file, bool quiet);
```

The forward declaration of `CheckWad` itself is kept. It's still not exposed in any header file, but now the compiler has full knowledge of `FResourceFile` for proper destructor dispatch.

## Linker errors after excluding `i_main.cpp`

Removing `i_main.cpp` from the harness target broke the linking. These symbols defined in `i_main.cpp` are referenced by other GZDoom source files:

- `Args` (global `FArgs*` pointer for CLI arguments)
- `addterm(func, name)` (registers cleanup callbacks)
- `popterm()` (unregisters last cleanup callback)

**Solution:** Stub substitutes directlu in `harness.cpp`. For fuzzing, cleanup callbacks are not neccessary => process is short lived, AFL restarts it. The `Args` pointer can be `nullptr` because the WAD parser does not read CLI args.

## Testing
Smoke test with valid WAD:
```bash
./harness freedoom1.wad
echo $?    # Exit code 0 = parser succeeded gracefully
```

Harness now can load arbitrary bytes from disk, wrap them in `FileReader` correctly, pass them to `CheckWad` and clean up parsed objects without leaking.