# RpiSound (Draft)

**RpiSound** is a lightweight audio playback library intended for embedded Linux systems, with a focus on Raspberry Pi and USB sound cards. The library is designed to support asynchronous PCM playback with low-latency audio output.

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

## TODO:
- Sound manager implementation with randomization for realistic experience.
- Sound loader interface to mmap the sound files.

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

### Useful commands
```bash
# play raw PCM data with ffplay
ffplay -autoexit -skip_initial_bytes 47 -f s16le -ch_layout stereo -ar 44100 ./sound/demo/tom_low/tom_low_0.pcm

# get file format details with ffprobe
ffprobe -v error -show_format -show_streams -print_format json ./sound/demo/tom_low/tom_low_0.pcm
```
