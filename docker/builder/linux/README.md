A self-contained, sandboxed docker image that will build and install the
[fheroes2](https://github.com/ihhub/fheroes2) game engine, without
the need to setup a developer environment, install unwanted packages in
your machine or clutter it with build artifacts.

**TL;DR**: build and install latest version under `/usr/local`:
```
docker run -it --rm -v /usr/local:/out bstlang/fheroes2-linux-builder
```

The source code for this image can be found at
[ihhub/fheroes2](https://github.com/ihhub/fheroes2/tree/master/docker/builder/linux).

A pre-built image can be found in [docker
hub](https://hub.docker.com/r/bstlang/fheroes2-linux-builder). Note that when using
the pre-built image there's **no need** to clone this repository. The
repository is only needed when building the image itself.

Binaries will be installed into guest directory `/out`. Mount that directory
with `-v host_dir:/out` to install it into some host directory.

Choose the version to build by setting environment variable `VERSION` to the
appropriate [revision tag](https://github.com/ihhub/fheroes2/releases)
(defaults to `HEAD` for bleeding edge).

Example: install fheroes2 `1.0.3` under `~/opt` with (change `OUT_DIR` accordingly):
```
OUT_DIR="$HOME/opt"
VERSION=1.0.3
mkdir -p "${OUT_DIR}"
docker run -it --rm -v "${OUT_DIR}:/out" -e "VERSION=${VERSION}" bstlang/fheroes2-linux-builder
unset OUT_DIR
```
