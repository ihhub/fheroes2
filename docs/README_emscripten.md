# Emscripten (Wasm) Port of [fheroes2](README.md) Project

## Current Status of this Port

Please note that this port is currently still **experimental**. You may experience various problems, crashes, or it not even working at all in particular browsers.

## Building

### Prerequisites

* emsdk 4.0 or later
* GNU gettext

### Building using Docker

To save time, you can use a ready-made Docker image:

```sh
docker run --rm -v "$(pwd):/src" emscripten/emsdk:latest sh -c "apt-get -y update; apt-get -y install gettext; emmake make -f Makefile.emscripten"
```

### Building without Docker

If you do not want to use Docker, then you will need to install all the prerequisites manually using your platform's package manager, and then run the following command:

```sh
emmake make -f Makefile.emscripten
```

### Building with Additional Parameters

If you want to specify some additional ad hoc build parameters, you can use the appropriate environment variables for this. For example, the following command:

```sh
FHEROES2_WITH_THREADS=ON LDFLAGS="-sMODULARIZE -sEXPORTED_RUNTIME_METHODS=run -sEXPORT_NAME=fheroes2" emmake make -f Makefile.emscripten
```

will build a WebAssembly binary with multithreading support, as well as with additional parameters for creating a module.

## Running the Wasm Port on a Web Server

### Configuring the Web Server

Emscripten uses web workers along with `SharedArrayBuffer` to support multithreading. `SharedArrayBuffer` usage is usually protected by Cross Origin Opener Policy (COOP)
and Cross Origin Embedder Policy (COEP) HTTP headers, and the web server should send these headers as part of its response to a request for resources related to the Wasm
build:

```text
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

Please see the following link for details:

<https://web.dev/coop-coep>

### Copying Necessary Files

After the build is completed, copy the following files to the appropriate directory on your website:

```text
fheroes2.data
fheroes2.js
fheroes2.wasm
fheroes2.wasm.map (if fheroes2 has been built in debug mode)
```

If you want to use the stock launcher (pretty basic at the moment), then you will also need to copy all the files from `files/emscripten` to the same directory.
