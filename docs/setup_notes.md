# Setup Notes

## Issues with original GZDoom 3.3.0

### Missing `<limits>` include
```
gzdoom/src/scripting/types.cpp:742:62: error: 'numeric_limits' is not a member of 'std'
```

GCC 11 stopped transitively including `<limits>`. The file uses
`std::numeric_limits<double>::quiet_NaN()` but doesn't include `<limits>`
explicitly. Older compilers worked due to transitive includes from STL.

**Fix:** Just added `#include </limits/>` src/scripting/types.cpp

## GZDoom specifics
### AFL build
```bash
CC=afl-clang-fast CXX=afl-clang-fast++ cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DNO_OPENAL=ON \
    -DNO_GTK=ON
```

GZDoom specific variables
- `NO_OPENAL=ON` - built without audio support
- `NO_GTK=ON`    - built without GUI toolkit

Standard cmake variable:
- `CMAKE_BUILD_TYPE=Debug` — debug symbols for crash analysis