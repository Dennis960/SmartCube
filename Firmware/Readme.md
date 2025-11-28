# Firmware

This project provides a CMake-based setup for building firmware for the PY32F0 microcontrollers. It also supports flashing via ST-Link.
It is taken from [https://github.com/decaday/py32f0-cmake](https://github.com/decaday/py32f0-cmake)

---

## Prerequisites

- **ARM GCC Toolchain** (`arm-none-eabi-gcc`) with `sudo apt install gcc-arm-none-eabi`
- **CMake** (≥3.13)
- **Make**
- **ST-Link utilities** (`st-flash` or `st-util`) with `sudo apt install stlink-tools`
- **Git** (to clone the repository)

---

## 1. Build Firmware Using CMake + Make

1. Generate Makefiles with CMake:

```bash
cmake -B build -S .
```

2. Compile the firmware:

```bash
make -C build
```

- Output firmware: `build/py32-cmake.elf` and `build/py32-cmake.bin`

---

## 2. Flash Firmware to the Board

Using **ST-Link**:

```bash
st-flash write build/py32-cmake.bin 0x8000000
```

- Replace `0x8000000` with your MCU’s flash base address if different.

Optional: use **st-util** for debugging with GDB:

```bash
st-util
```

---

## 3. VS Code Integration

This repository includes **VS Code tasks** and a **launch configuration** for convenience:

- **Tasks**: Automate building and flashing.
- **Launch configuration**: Debug firmware using GDB and ST-Link.

### Usage

1. Open the project in VS Code.
2. Use the **Command Palette → Run Task** to build or flash firmware.
3. Start `st-util` in a terminal for debugging:

```bash
st-util
```
