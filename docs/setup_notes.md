#Setup Notes

##Issues with original GZDoom 3.3.0
gzdoom/src/scripting/types.cpp:742:62: error: 'numeric_limits' is not a member of 'std'

GCC 11 stopped transitively including ``. The file uses
`std::numeric_limits::quiet_NaN()` but doesn't include ``
explicitly. Older compilers worked due to transitive includes from STL.

**Fix:** Just added "#include </limits/>" src/scripting/types.cpp

##GZDoom specifics
AFL build
CC=afl-clang-fast CXX=afl-clang-fast++ cmake .. -DCMAKE_BUILD_TYPE=Debug -DNO_OPENAL=ON -DNO_GTK=ON

GZDoom specific variables
=> NO_OPENAL=ON - built without audio support
=> NO_GTK=ON    - build without GUI toolkit
