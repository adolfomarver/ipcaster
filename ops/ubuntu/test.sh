cd ipcaster/build
# test with ctest
ctest --output-on-failure
# launch ipcaster as service in background
./ipcaster service &
ipcaster_pid=$!
cd ../src/tests
# run python tests
pytest-3
pyret=$?
kill -KILL $ipcaster_pid
if [ $pyret != 0 ] 
then
    exit $pyret
fi
