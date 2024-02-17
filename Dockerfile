FROM ubuntu:22.04

ENV DEBIAN_FRONTEND noninteractive

# Install dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends libsdl2-mixer-dev g++ cmake ninja-build make gettext file dpkg-dev

COPY . /app

WORKDIR /app

RUN mkdir build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -G Ninja .. && \
    ninja && \
    cpack

