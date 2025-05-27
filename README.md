# RpiSound (Draft)

**RpiSound** is a lightweight audio playback library intended for embedded Linux systems, with a focus on Raspberry Pi and USB sound cards. The library is designed to support asynchronous WAV playback with low-latency audio output.

> 🚧 **This project is under active development and not yet production-ready.**

---

## Features

- 🎵 WAV file parsing  
- 🔁 PCM16 format conversion  
- ▶️ Blocking WAV playback  
- ⚙️ TinyALSA backend (only dependency is TinyALSA)  
- 🐳 Docker-based build environment  
- 🛠️ Cross-compilation support (e.g., aarch64/Raspberry Pi)  
- 🧪 GTest-based unit testing  

---

## Roadmap

- [ ] Asynchronous audio playback
- [ ] Audio mixing support
- [ ] Sound effects processing
- [ ] End-to-end latency < 50ms

---

## Build Instructions

### Prerequisites

- CMake 3.10+
- C++17-compatible compiler
- [TinyALSA](https://github.com/tinyalsa/tinyalsa)

### Building (Native)

```bash
git submodule update --init --recursive
mkdir -p build && cd build
cmake ..
make -j
```

### Building (Docker)
```bash
docker build -t rpisound-dev .

docker run -it --rm \
    -v "$PWD":/rpisound \
    -w /rpisound \
    rpisound-dev

mkdir -p build && cd build
cmake ..
make -j
```

### Cross-Compilation for aarch64

```bash
mkdir -p build_aarch64 && cd build_aarch64
cmake .. -DBUILD_FOR_AARCH64=ON
make -j
```
