FROM ubuntu:18.04 as intermediate
COPY src /ipcaster/src
COPY ops /ipcaster/ops
COPY tsfiles /ipcaster/tsfiles
COPY CMakeLists.txt /ipcaster
WORKDIR /ipcaster
RUN /ipcaster/ops/ubuntu/install-build-env.sh
WORKDIR /
RUN /ipcaster/ops/ubuntu/build.sh 
WORKDIR /
RUN /ipcaster/ops/ubuntu/test.sh
RUN cp /ipcaster/build/ipcaster /usr/local/bin

FROM ubuntu:18.04
COPY --from=intermediate /ipcaster/build/ipcaster /usr/local/bin
COPY ops/ubuntu /ipcaster/ops/ubuntu
COPY tsfiles /ipcaster/tsfiles
RUN /ipcaster/ops/ubuntu/install-prod-env.sh
ENTRYPOINT ["ipcaster"]

