# GZDoom Fuzzing

Fuzzing the GZDoom game engine version 3.3.0 using AFL++.

## Goal

- Reverse engineer WAD file parsing in GZDoom
- Develop a libFuzzer-style harness
- Fuzz with AFL++
- Triage crashes
- Develop exploit for one bug

## Setup

```bash
# Build Docker image with AFL++ and dependencies
docker build -t afl-zdoom .

# Run container
docker run -it --rm -v $(pwd):/work afl-zdoom

# Inside container: clone and build GZDoom
cd /work
git clone https://github.com/ZDoom/gzdoom.git
cd gzdoom
git checkout g3.3.0
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Status

- [x] Docker environment with AFL++
- [x] Vanilla GZDoom build
- [x] AFL instrumented build
- [x] Fuzzing harness draft
- [x] Harness implementation + harness build verified
- [x] Seed corpus collection
- [ ] Fuzz run
- [ ] Crash triage
- [ ] Exploit development

## Documentation
- [Setup Notes](docs/setup_notes.md) – build issues encountered, fixes and notes on interesting tidbits

- [Harness Design](docs/harness_design.md) – original code analysis, attack surfaces and entry points for harness

- [Harness Implementation](docs/harness_implementation.md) – CMakeLists.txt modifications, linker errors resolved
