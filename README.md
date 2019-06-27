# IPCaster
> MPEG Transport Stream over IP Sender

IPCaster is an open source application capable of simultaneously sending one or more MPEG-TS (ITU-T H.222) files through an IP network to a remote endpoint. The sending real-time bitrate of the stream will match the bitrate of the file so it can be received, decoded and rendered in real-time at the remote endpoint

The IP encapsulation is based on the SMPTE2022-2 standard.

### About MPEG-TS (ISO/IEC 13818-1 or ITU-T Recommendation H.222.0)

This format is the standard media container for TV (terrestrial, satelite, cable) media broadcasting systems (DVB, ATSC, ISDB). The media container allows to encapsulate and multiplex media streams such as video (mpeg-2, h.264, hevc,...), audio (mpeg-1 layer II, aac, ac3,...), metadata and signaling (teletext, subtitles, ...). All those streams can be grouped in programs and many different programs can be holded in one TS.

Sample TS files are included with the project in the **tsfiles/** directory

## Platforms

The code is written entirely in C++11 and can be build, at least, in the following platforms:

* Linux on x86/x64
* Raspbian on Raspberry PI 3 B+
* Windows on x86/x64

## Usage

```sh
ipcaster -s ts_file target_ip target_port ... [-s ...]
```


**ts_file** Is the file to sent.

**target_ip** Is the IPv4 address of the endpoint.

**target_port** Is the IP port of the endpoint.

 ## Examples

We'll use VLC in these examples to watch at the video output.

```sh
# Installing VLC on Ubuntu
sudo snap install vlc
```

### Sending one file
```sh
# Launch VLC listening on port 50000
vlc udp://@:50000
```

Open another terminal( go to "ipcaster/build" directory) 

```sh
# Send the test.ts file
./ipcaster -s ../tsfiles/test.ts 127.0.0.1 50000
```

## Docker image

Donwloadable image is avaiable at Dockerhub. In case you use Docker follow the next steps to use ipcaster from Docker.

We will use VLC to watch at the video stream.
Open a console and type
```sh
# Launch VLC listening on port 50000
vlc udp://@50000
```

In another console 
```sh
# Download the image to your machine
docker pull adolfomarver/ipcaster

# Find out your docker0 network interface IP address
ip a # In my case 172.17.0.1

# Run the image on a container (the test.ts file is embedded in the image)
docker run adolfomarver/ipcaster ipcaster -s tsfiles/test.ts 172.17.0.1 50000
```





## How to build

The project uses CMake to support cross-platform building. IPCaster makes use of two 3rd party open source libraries: **boost** and **libevent**

Linux (Ubuntu):

```sh

# Install 3rd party dependencies

sudo apt install libboost-all-dev

sudo apt install libevent-dev

# Clone the repository

https://github.com/adolfomarver/ipcaster.git

# CMake build

cd ipcaster
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make

```

## Tests
If your working directory is not "ipcaster/build" you need to "cd" there before execute the tests.

The UDP port 50000 must be free

```sh
# Run tests
./tests
```

## Roadmap

IPCaster is today only a console application with little features, but the plan is to evolve it to become a full server manageable through a web-based user interface or a REST API. In order to complete those goals the next steps will be:

* Add a REST API public and documented.
* Add a front end HTML interface.