# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Firmware for a rotary inverted pendulum (embedded C, STM32-style MCU with a 16-bit hardware timer used as quadrature encoder counter). The project is at an early stage: only `drivers/encoder`, `drivers/button`, `drivers/motor`, `drivers/current_sense`, `services/safety`, `services/crc16`, `services/ringbuf`, `services/strfmt`, `control/ctrl_math.h` and `control/ctrl_filter` exist, plus `config/board_config.h` (board constants) and `bsp/bsp.h` (hardware interface, header only) and `host/tests/bsp_fake.[ch]` (fake BSP for tests), see below. `firmware/App/app`, `docs/`, `sim/`, `tools/` are empty placeholders.

## Layout

- `firmware/App/` — hardware-independent application code. Must stay free of HAL/register includes so it can be compiled on the PC.
- `hardware/` — KiCad schematics and PCB, designed on the user's Windows machine. This Ubuntu checkout uses sparse-checkout (`/*` and `!/hardware/`), so the folder is intentionally absent here; do not recreate it, and do not treat its absence as deleted. `git sparse-checkout disable` brings it back.
- `.github/workflows/ci.yml` — GitHub Actions: on push to `main` and on pull requests, runs the same `cmake` configure/build and `ctest` as the host commands below.

## Git workflow

- Work on a branch (`feat/...`, `fix/...`, `ci/...`, `docs/...`), push it and open a PR into `main`; do not commit directly to `main`. Merge with squash once CI is green.
- Commit messages use Conventional Commits (`feat(scope): ...`, `fix`, `docs`, `test`, `refactor`, `chore`, `ci`).
- Keep history linear: use `git pull --rebase`. Only rebase commits that are not pushed yet.
- Hardware files are normally pushed from Windows. If a push is rejected (`fetch first`), run `git pull --rebase origin main` first.
- Ask the user before `git push` or any other outward-facing action.

## Commands

Run from the repo root. The shell's default cwd is `firmware/App`, not the root: use `cd /home/minhle/dev/rotary_inverted_pendulum && ...` or absolute paths.

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
- New tests: create `host/tests/test_<name>.c` and add `test_<name>` (with the `test_` prefix) to the `foreach(t ...)` list in `host/CMakeLists.txt`; the new `.c` under test must also be in `rip_core`. Tests use the minimal in-repo framework `host/tests/utest.h` (`UT_CHECK`, `UT_NEAR`, `UT_RUN`, `UT_REPORT`), not an external one.
- Headers are C, not C++: use `<stdint.h>`/`<stddef.h>`/`<stdbool.h>`, never `<cstdint>`. Headers have no `extern "C"` guards. Every header needs a fully closed include guard (`#endif`).
- The user edits files in the IDE concurrently: re-read a file before `Edit`, and check whether they already wrote a function before adding it (avoids duplicate definitions).

## Architecture / conventions that span files

- **Encoder** (`drivers/encoder`): extends the 16-bit hardware counter to a 32-bit `count` by accumulating `(int16_t)(uint16_t)(raw - last_raw)`. This relies on the counter moving less than 32768 counts between `encoder_update` calls, so the update must run at a fast, fixed rate. The first `encoder_update` only latches the reference (`primed`). The wiring direction `dir` is applied inside `count`, so `encoder_angle = offset_rad + count * 2π/cpr` (no extra `dir`). `cpr` is counts per revolution in x4 mode. `encoder_rezero` redefines the current position as a given angle without touching `last_raw`.
- **CRC** (`services/crc16`): CRC-16/CCITT-FALSE (poly 0x1021, init `CRC16_INIT` = 0xFFFF, no reflection, no final XOR), bitwise implementation. Check value for `"123456789"` is `0x29B1`. Any host-side tool talking to the firmware must use the same variant (`crc-ccitt-false` in Python's crcmod). `crc16_update` takes the running CRC, so it can be fed in chunks.
- **Ring buffer** (`services/ringbuf`): byte SPSC lock-free. Producer writes only `head`, consumer writes only `tail`; both are free-running 16-bit counters (not masked), `used = head - tail`. Size must be a power of 2, ≤ 32768. `ringbuf_write` is all-or-nothing and updates `head` only after the data is copied. `ringbuf_read` reads up to `max` bytes.
- **Button** (`drivers/button`): integrator debounce (0..`threshold`). Call `button_update` at a fixed rate; emits `PRESSED` when the integrator reaches `threshold`, `RELEASED` when it returns to 0. `threshold == 0` is clamped to 1.
- **String formatting** (`services/strfmt`): printf/strtof-free `sbuf_t` builder (`sb_char/str/i32/f32/hex/pad`) that truncates silently and always keeps the NUL terminator; `str_to_f32` rejects whitespace, trailing junk and leaves `*out` untouched on failure. Known issues (unfixed): `sb_pad(sb, n)` loops forever when `n >= cap`; `sb_f32` overflows `uint32_t` when `|v| * 10^decimals > 4.29e9`; negative values rounding to zero print a space instead of `-`.
- **Velocity filter** (`control/ctrl_filter`): `ctrl_diff_t` = backward difference + 1st-order low-pass, `w[k] = a*w[k-1] + (1-a)*(x[k]-x[k-1])/Ts`, `a = exp(-2π·fc·Ts)`. Call at a fixed rate equal to `ts`; feed unwrapped angles (never `ctrl_wrap_pi` output); one instance per axis. The first update returns 0 (`primed`); `ctrl_diff_retune` keeps state. No guard for `ts <= 0` or `fc <= 0`. `fc` is the estimator's bandwidth, not the control loop's: choose it ~3-5x above the loop bandwidth.
- **Control math** (`control/ctrl_math.h`): header-only float helpers (`ctrl_wrap_pi` wraps to [-π, π), `ctrl_sign1f` treats 0 as +1). Single-precision `float` is used throughout; the hardware has an FPU-class target in mind, so avoid accidental `double` (the build uses `-Wdouble-promotion`).
- **Board config** (`config/board_config.h`): wiring-dependent constants for the STM32F1 board (encoder `dir`, motor `dir`, BTN8982TA current sense, LCD I2C address, safety limits). Control tick is 1 kHz (1000 µs); `*_TICKS` constants are in ticks, button debounce in 10 ms samples. Angle limits are in degrees (`APP_START_WINDOW_DEG`), velocity limits in rad/s: convert from radians before comparing. Items marked `[VERIFY]` (encoder/motor direction, `BOARD_IS_KILIS`, `BOARD_IS_SWAP`, LCD address) are unverified on real hardware. `rip_params_autogen.h` (e.g. `RIP_S_THETA_TRIP`) is referenced but does not exist yet. Nothing uses `APP_START_WINDOW_DEG` yet.
- **Motor** (`drivers/motor`): sign-magnitude PWM for 2 x BTN8982TA. `motor_set_voltage` takes the MODEL sign; `dir` only swaps the hardware half-bridge, `motor_voltage` keeps the model sign. V >= 0 drives half A, V < 0 half B. Duty is clamped to 1. Commands are ignored (0 V) while disabled; `motor_init` leaves the bridge disabled. The ADC trigger goes to the middle of the on-time, clamped to `BOARD_ADC_TRIG_MIN_CNT`. No guard for `vbus <= 0`.
- **Current sense** (`drivers/current_sense`): converts the IS ADC counts to amps (`amps_per_lsb` = Vref/full_scale/R_IS*kILIS, 4.76 mA/LSB). Uses the ACTIVE half-bridge channel, signed by the commanded voltage. `valid` is false when the motor is off, duty < `min_duty`, or a fault is seen; `driver_fault` is set when EITHER channel reaches `fault_raw`. The caller applies `BOARD_IS_SWAP` to `raw_a`/`raw_b`; the module does not. No guard for zero `r_is`/`full_scale`.
- **Safety monitor** (`services/safety`): latched faults (E-stop, arm limit, over-current, driver, over-speed, overrun) in a bitmask; `safety_clear` resets. All motor-related checks (E-stop, theta, speed, driver, current) run only while `motor_active`; only OVERRUN is checked when idle. `theta_trip` is in radians while the header comment says 45 deg. Over-current needs `i_trip_ticks` consecutive ticks (counter decays on good ticks). Limits are compile-time (`SAFETY_*` in `board_config.h`), never set over UART.
- **BSP** (`bsp/bsp.h`): the only boundary to hardware; everything above it (drivers, services, control, app) is portable C and never includes a HAL header. Planned implementations: `bsp_stm32f1.c` (target, on CubeMX HAL handles), `host/sil/bsp_sim.c` (software-in-the-loop against a nonlinear plant model), `host/tests/bsp_fake.c` + `bsp_fake.h` (unit tests: records outputs in `g_fake`, injects UART rx with `fake_rx_inject`, `fake_reset` between tests; built as the `bsp_fake` static library in `host/CMakeLists.txt`; link it only into tests that need it, e.g. `test_motor_current`). `bsp_stm32f1.c` and `bsp_sim.c` do not exist yet. Groups: lifecycle (`bsp_init` leaves the bridge DISABLED; `bsp_tick_start(cb)` runs the 1 kHz TIM3 interrupt), sensors (`bsp_enc_arm_raw` = TIM4, `bsp_enc_pend_raw` = TIM2, raw 16-bit counters fed to `encoder_update`; `bsp_current_raw` returns the mean of IS1/IS2 ADC samples since the previous call, so call it once per tick only), power stage (`bsp_pwm_set` compare values with `bsp_pwm_period()` = 3600 = 100% at 20 kHz, i.e. a 72 MHz timer; `bsp_bridge_enable` drives INH via the E-stop chain on PB0; `bsp_bridge_inhibit_sense` reads PB1 and is true only if the bridge is enabled AND E-stop is closed), HMI (`bsp_btn_start_raw`/`bsp_btn_stop_raw` return true = pressed, active low on the PCB, raw and undebounced, feed them to `button_update`; `bsp_led_set(on)`; `bsp_i2c_write(addr7, buf, len)` for the LCD; non-blocking DMA UART: the tx buffer must stay valid until `bsp_uart_tx_busy()` is false), time (`bsp_cycles`/`bsp_cycles_per_us` measure tick execution against `SAFETY_EXEC_MAX_US`; `bsp_millis` is `uint32_t`, compare with subtraction), system (`bsp_watchdog_kick`, `bsp_irq_lock`/`unlock` with a key so it nests).- Angle convention: `theta` is the arm angle (encoder on TIM4), `alpha` is the pendulum angle (encoder on TIM2). `encoder_init(..., offset_rad)` takes the angle at power-up (e.g. π for a hanging pendulum). The zero/sign convention (is 0 upright or hanging, which direction is positive) is still unconfirmed with the user; ask before relying on it in control code.

## Current status (2026-11-23)

- Done and merged into `main`: CI (PR #1), `config/board_config.h` and `bsp/bsp.h` (#2), `bsp_fake` (#3), CLAUDE.md (#4), `services/safety` (#5), `drivers/motor` (#6), `drivers/current_sense` (#7). Tests: `test_safety`, `test_motor_current` (motor and current_sense, uses `bsp_fake`).
- Pending: `bsp_stm32f1.c` and `host/sil/bsp_sim.c`; `app/` (state machine tying safety, motor, current_sense, encoder together); enable branch protection on `main`; verify the `[VERIFY]` items in `board_config.h`; the control tick (1 kHz) has not been validated by simulation.
- Open question: `CLAUDE.md` says "FPU-class target" but the STM32F1 has no FPU, so `float` math is software-emulated; measure tick time against the 800 µs budget.
- Open question: should `ARM_LIMIT` also be checked while the bridge is disabled? Today it is not.
