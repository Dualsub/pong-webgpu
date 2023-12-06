# 3D Pong with WebGPU

Using [WebGPU](https://www.w3.org/TR/webgpu/) to create a 3D Pong game. Created for learning purposes.

Intended to be used as a client for another project of mine, [go-mp](https://www.github.com/Dualsub/go-mp), a dedicated server for Pong, written in Go.

## Building

Make sure to clone the repository with the `--recursive` flag, as it uses submodules.

### *Prerequisites*

You need the following tools to build this project:
- A C++ compiler (tested with [Clang](https://clang.llvm.org/))
- [Cmake](https://cmake.org/download/)
- [Emscripten](https://emscripten.org/docs/getting_started/downloads.html)

### Building for the web

Run the following script:

```bash
cd scripts
./build-web.sh
```

Run by serving the `build` directory with a web server and opening the `app.html` file.

### Building with Dawn (for native)

Run the following script:

```bash
cd scripts
./build-dawn.sh
```

Run the executable in the `build` directory.
