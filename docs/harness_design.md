# Code structure needed for building harness

## WAD file structure

```
+0:              4 bytes     "IWAD" / "PWAD" (magic)
+4:              4 bytes     NumLumps (uint32)
+8:              4 bytes     InfoTableOfs (uint32)   - offset to directory
+12:             <data lumps>
+InfoTableOfs:   <lump directory>
```

## Key classes

- `class FWadFile : public FResourceFile`
  - constructor: `FWadFile::FWadFile(const char *filename, FileReader &file) : FResourceFile(filename, file)`
  - open file represented by `Reader` member
  - parsing logic is in `bool FWadFile::Open(bool quiet)`

- `class FWadFileLump : public FResourceLump`
  - represents individual lumps within the WAD

## Harness entry point

`CheckWad()` is the top-level function that validates magic bytes and triggers WAD parsing:

```cpp
FResourceFile *CheckWad(const char *filename, FileReader &file, bool quiet)
{
    char head[4];

    // files shorter than 12 bytes cannot be WAD
    if (file.GetLength() >= 12)
    {
        file.Seek(0, FileReader::SeekSet);
        file.Read(&head, 4);
        file.Seek(0, FileReader::SeekSet);

        // magic detection
        if (!memcmp(head, "IWAD", 4) || !memcmp(head, "PWAD", 4))
        {
            // if magic checks, create FWadFile and call Open()
            FResourceFile *rf = new FWadFile(filename, file);

            if (rf->Open(quiet)) return rf;
            file = std::move(rf->Reader); // to avoid destruction of reader
            delete rf;
        }
    }
    return NULL;
}
```


## Calling CheckWad
The harness needs to call `CheckWad`, which is defined in `resourcefiles/file_wad.cpp`. `CheckWad` is not in any public header files, so the harness does not know it exists. To solve this the harness uses a forward declaration (basically a promise to a compiler that this specific signature will be defined later on):

```cpp
class FResourceFile;
FResourceFile *CheckWad(const char *filename, FileReader &file, bool quiet);
```

This can be done because:
- `FResourceFile` is only used as a **pointer return type**
- The C++ compiler does not need to know structure of `FResourceFile`, it is a pointer
- The actual definition will be resolved later by the linker, which find `CheckWad` in the compiled `filed_wad.cpp.o`, GZDoom uses the same thing in forward declaration of `CheckWad` in `resourcefiles/resourcefile.cpp:282`

This turned out to be a bad call. The `FResourceFile` had to be defined fully with `#include "resourcefiles/resourcefile.h"` because the compiler doesn't know about virtual destructor this way. More on this [here](harness_implementation.md).

## Attack surfaces

- **Integer overflow on `NumLumps` / `InfoTableOfs`**
  - multiplication `NumLumps * sizeof(wadlump_t)` can overflow uint32
  - bypasses size sanity check

- **Untrusted offset to `Reader.Seek` + `Reader.Read`**
  - potential out-of-bounds read
  - memory disclosure (info leak)
  - or heap corruption if Read writes to undersized buffer

- **Parser storing attacker-controlled data**
  - `Position`, `LumpSize`, `Name` from input are stored in `Lumps[i].*`
  - used later in `FWadFileLump::FillCache()` for `Seek` + `Read`
  - delayed exploit?

- **Allocation with untrusted size**
  - potential DoS via huge allocation
  - ```cpp
    wadlump_t *fileinfo = new wadlump_t[NumLumps];
    ```
  - not that interesting DoS is not really what I am after

## Memory Buffer FileReader

For the harness, we need a `FileReader` that reads from an in-memory buffer. 
It resides in `FileReader` class:

```cpp
bool OpenMemory(const void *mem, Size length);  // read directly from the buffer
```

This will be used to wrap AFL's `(data, size)` input into a `FileReader`
that `CheckWad` can consume.

FileReader cannot be copied!!
Copy constructor and copy assignment are deleted:
```cpp
FileReader(const FileReader &r) = delete;
FileReader &operator=(const FileReader &r) = delete;
```
When passing `FileReader` to `CheckWad()`, reference needs to be used. The function's paramater `FileReader &file` already does this:)
