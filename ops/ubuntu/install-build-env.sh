apt update

# install gcc
apt install -y gcc g++

# install cmake
apt install -y cmake

# install boost
apt install -y libboost-program-options-dev libboost-system-dev

# install libevent
apt install -y libevent-dev

# install cpprest
apt install -y libcpprest-dev

#install python && pytest
apt install -y python3 python3-pip python3-pytest
pip3 install requests
