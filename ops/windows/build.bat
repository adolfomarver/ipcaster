REM Build with CMake
md build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
REM msbuild ipcaster.sln