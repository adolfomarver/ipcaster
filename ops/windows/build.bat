REM Build with CMake
cd ipcaster
md build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
REM msbuild ipcaster.sln