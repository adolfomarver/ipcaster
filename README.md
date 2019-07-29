<sup> Linux CI </sub>\
[![CircleCI](https://circleci.com/gh/adolfomarver/ipcaster.svg?style=svg)](https://circleci.com/gh/adolfomarver/ipcaster)

<sup>Windows CI</sup>\
[![Build Status](https://dev.azure.com/adolfomarver/Pegasus/_apis/build/status/adolfomarver.ipcaster?branchName=master)](https://dev.azure.com/adolfomarver/Pegasus/_build/latest?definitionId=2&branchName=master)

# IPCaster
> Video Server Microservice

IPCaster is an application capable of simultaneously sending several MPEG-TS files through an IP network to remote endpoints. The IP encapsulation is based on the SMPTE2022-2 standard, the sending real-time bitrate of the stream will match the bitrate of the file so it can be received, decoded and rendered in real-time at the endpoints.

IPCaster can be run as an standalone service controlled through its REST API, or can be also executed as a video sender command line application.

### About MPEG-TS (ISO/IEC 13818-1 or ITU-T Recommendation H.222.0)

This format is the standard media container for TV (terrestrial, satellite, cable) media broadcasting systems (DVB, ATSC, ISDB). The media container allows to encapsulate and multiplex media streams such as video (mpeg-2, h.264, hevc,...), audio (mpeg-1 layer II, aac, ac3,...), metadata and signaling (teletext, subtitles, ...). All those streams can be grouped in programs and many different programs can be transported in one TS.

Sample TS files are included with the project in the **tsfiles/** directory

## Platforms

The code is written in C++11 and can be built, at least, in the following platforms:

* Linux on x86/x64
* Raspbian on Raspberry PI 3 B+
* Windows on x86/x64

CMake is used to support cross-platform building.

## Build and test

DevOps scripts for all the supported platforms can be found at **ipcaster/ops/[platform]**.
In the next section these scripts are use to build and test the application inside a Docker container

## Build and Test in a Docker container

```sh

# Clone the repository
git clone https://github.com/adolfomarver/ipcaster.git

# cd into ipcaster directory where Dockerfile resides
cd ipcaster

# Build the docker image. 
# The Dockerfile installs the build dependencies,
# build ipcaster and run the tests in an intermediate stage. 
# Then, in the final stage, generates a minimum dependency image 
# with the required artifacts from the intermediate stage.
docker build -t ipcaster .

```
## Usage as a service example

We'll use the docker image generated in the previous step, We'll also need: cURL to send REST requests to the service and VLC to watch at the video output.

```sh
# Install cURL
sudo apt install curl

# Install VLC
sudo snap install vlc

# Launch VLC listening in the port 50000
vlc udp://@:50000
```

Open another console 
```sh
# Launch VLC listening in the port 50001
vlc udp://@:50001
```

Now we have two receivers waiting for video streams on different ports.

Open another console
```sh
# Find out your docker0 network interface IP address
ip a # In my case 172.17.0.1

# Run ipcaster as a service exposing the port 8080
docker run -p 8080:8080 ipcaster ipcaster service

# Send a POST request to create a stream for ipcaster.ts to be sent to VLC on port 50000
curl -d '{"source": "ipcaster/tsfiles/ipcaster.ts", "endpoint": {"ip": "172.17.0.1", "port": 50000}}' -H "Content-Type: application/json" -X POST http://localhost:8080/streams

# Send a POST request to create a stream for timer.ts to be sent to VLC on port 50001
curl -d '{"source": "ipcaster/tsfiles/timer.ts", "endpoint": {"ip": "172.17.0.1", "port": 50001}}' -H "Content-Type: application/json" -X POST http://localhost:8080/streams
```

Now we have two streams running!

![IPCasting 2 streams](images/ipcasterrun.png "IPCasting 2 streams")

```sh
# Let's check the running streams list
curl -i -H "Accept: application/json" -H "Content-Type: application/json" -X GET http://localhost:8080/streams
```

Stop a stream

```sh
# Delete the stream with Id 0
curl -X DELETE http://localhost:8080/streams/0
```

## Usage as a command line app example
```sh
# Sends ipcaster.ts -> 172.17.0.1:50000 and timer.ts -> 172.17.0.1:50001
docker run -p 8080:8080 ipcaster ipcaster play ipcaster/tsfiles/ipcaster.ts 172.17.0.1 50000 ipcaster/tsfiles/timer.ts 172.17.0.1 50001
```

## How to create broadcast compatible MPEG TS files

The easiest and cheapest way to create TS files is by using the open source application FFmpeg. Here is an example for a typical distribution bitrate of 4Mbps using two pass encoding.

Just substitute "myvideo.mp4" for the name of the file you want to encode.

```sh
# Pass 1
ffmpeg -y -i myvideo.mp4 -c:v libx264 -preset veryslow -profile:v high -level 4.0 -vf format=yuv420p -bsf:v h264_mp4toannexb -b:v 3.5M -maxrate 3.5M -bufsize 3.5M -pass 1 -f mpegts /dev/null

# Pass 2
ffmpeg -i myvideo.mp4 -c:v libx264 -preset veryslow -profile:v high -level 4.0 -vf format=yuv420p -bsf:v h264_mp4toannexb -b:v 3.5M -maxrate 3.5M -bufsize 3.5M -pass 2 -c:a aac -b:a 128k -muxrate 4000000 myvideo.ts
```

## Roadmap

Next steps:

* Add a front end HTML interface.
