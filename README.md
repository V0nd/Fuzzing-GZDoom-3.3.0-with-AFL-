# GZDoom Fuzzing

Fuzzing the GZDoom game engine version 3.3.0 using AFL++.

## Goal

- Reverse engineer WAD file parsing in GZDoom
- Develop a libFuzzer-style harness
- Fuzz with AFL++ for
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
- [ ] AFL instrumented build
- [ ] Fuzzing harness
- [ ] Seed corpus collection
- [ ] Fuzz run
- [ ] Crash triage
- [ ] Exploit development

## ChangesFixes made from the original GZDoom 3.3.0 to run on modern Linux
/work/gzdoom/src/scripting/types.cpp:742:62: error: 'numeric_limits' is not a member of 'std'
=> adding #include \<limits\>
