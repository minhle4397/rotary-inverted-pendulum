# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Firmware for a rotary inverted pendulum (embedded C, STM32-style MCU with a 16-bit hardware timer used as quadrature encoder counter). The project is at an early stage: only `drivers/encoder`, `services/crc16` and `control/ctrl_math.h` exist. `firmware/App/{app,bsp,config}`, `docs/`, `sim/`, `tools/` are empty placeholders.

## Layout

- `firmware/App/` — hardware-independent application code (drivers, services, control). Must stay free of HAL/register includes so it can be compiled on the PC.
- `host/` — PC build of the firmware core plus unit tests. `host/CMakeLists.txt` compiles the firmware sources into the static library `rip_core` and links each test against it.
- `build/` — CMake output (generated; do not edit).

## Commands

Run from the repo root:

```bash
cmake -S host -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure        # all tests
ctest --test-dir build/host -R test_crc16 --output-on-failure   # one test
./build/host/test_crc16                                 # run a test binary directly
clang-format -i <files>                                 # style: .clang-format (LLVM-based, 4 spaces, 120 cols)
```

Host builds use `-Wall -Wextra -Wpedantic -Wshadow -Wdouble-promotion -Wfloat-conversion -Wmissing-prototypes -Wstrict-prototypes -Werror` on `rip_core`, and AddressSanitizer + UBSan on everything. Warnings are errors, so new code must be warning-clean.

## Adding code and tests

- New firmware source files must be added to the `rip_core` source list in `host/CMakeLists.txt` (and their directory to `target_include_directories`) or they will not be built or found by tests.
- New tests: create `host/tests/test_<name>.c` and add `<name>` to the `foreach(t ...)` list in `host/CMakeLists.txt`. Tests use the minimal in-repo framework `host/tests/utest.h` (`UT_CHECK`, `UT_NEAR`, `UT_RUN`, `UT_REPORT`), not an external one.
- Headers are C, not C++: use `<stdint.h>`/`<stddef.h>`, never `<cstdint>`. Headers have no `extern "C"` guards.

## Architecture / conventions that span files

- **Encoder** (`drivers/encoder`): extends the 16-bit hardware counter to a 32-bit `count` by accumulating `(int16_t)(uint16_t)(raw - last_raw)`. This relies on the counter moving less than 32768 counts between `encoder_update` calls, so the update must run at a fast, fixed rate. The first `encoder_update` only latches the reference (`primed`). The wiring direction `dir` is applied inside `count`, so `encoder_angle = offset_rad + count * 2π/cpr` (no extra `dir`). `cpr` is counts per revolution in x4 mode. `encoder_rezero` redefines the current position as a given angle without touching `last_raw`.
- **CRC** (`services/crc16`): CRC-16/CCITT-FALSE (poly 0x1021, init `CRC16_INIT` = 0xFFFF, no reflection, no final XOR), bitwise implementation. Check value for `"123456789"` is `0x29B1`. Any host-side tool talking to the firmware must use the same variant (`crc-ccitt-false` in Python's crcmod). `crc16_update` takes the running CRC, so it can be fed in chunks.
- **Control math** (`control/ctrl_math.h`): header-only float helpers (`ctrl_wrap_pi` wraps to [-π, π), `ctrl_sign1f` treats 0 as +1). Single-precision `float` is used throughout; the hardware has an FPU-class target in mind, so avoid accidental `double` (the build uses `-Wdouble-promotion`).
- Angle convention: `encoder_init(..., offset_rad)` takes the angle at power-up (e.g. π for a hanging pendulum). Confirm the project's intended zero/sign convention with the user before relying on it in control code.
