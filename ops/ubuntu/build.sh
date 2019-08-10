cd ipcaster
if [ -d "build" ] 
then
    rm -r build
fi
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
