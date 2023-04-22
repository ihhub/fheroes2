FROM debian:unstable-slim
#FROM debian:testing-slim

ENV DEBIAN_PRIORITY=critical
ENV DEBIAN_FRONTEND=noninteractive

ENV PROJECT_NAME="fheroes2"
ENV VERSION="HEAD"

ARG SRC_DIR="/src/${PROJECT_NAME}"
ENV SRC_DIR="${SRC_DIR}"
RUN mkdir -p "${SRC_DIR}"

ENV OUT_DIR="/out"
ENV OUT_DIR="${OUT_DIR}"
RUN mkdir -p "${OUT_DIR}"

ENTRYPOINT "/srv/entrypoint.sh"
WORKDIR "${SRC_DIR}"

ARG DEP_PKGS="gettext zlib1g-dev libpng-dev libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev"
ENV DEP_PKGS="${DEP_PKGS}"

RUN apt-get update && apt-get install -y --no-install-recommends \
  ca-certificates \
  git \
  build-essential \
  cmake make \
  g++ \
  ${DEP_PKGS}

RUN git init
RUN git remote add origin "https://github.com/ihhub/fheroes2.git"

COPY entrypoint.sh /srv
