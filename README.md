# Linux C Sensor Frame Debugger (GCC + GDB)

A small, **simulated** UART-style sensor packet-processing command-line program. No physical UART or sensor hardware is required. Designed for demonstrating C development, GCC compilation, GDB debugging, input validation, and reproducible testing on Linux.

## Packet format

Each line of input contains five hexadecimal bytes:

`AA SENSOR_ID SIGNED_TEMP_C HUMIDITY_PCT XOR_CHECKSUM`

- The start byte is `AA`.
- `SIGNED_TEMP_C` is a signed 8-bit, two's-complement integer (e.g., `FB` is -5 C).
- The XOR checksum covers the first **four** bytes only.
- Accepted temperatures: -40 through +85 C; humidity: 0 through 100%.
- Invalid start byte, checksum, sensor ID, temperature, humidity, and malformed lines are rejected.

## 1. Install compiler and debugger (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential gdb
```

## 2. Compile and test on Linux

```bash
make
./build/sensor_monitor samples/frames.txt
make test
```

The fixed build prints two valid packets (including a **-5 C** reading) and three rejected packets. `make test` uses `diff` to verify exact output.

## 3. Reproduce an intentional bug

```bash
make buggy
./build/sensor_monitor_buggy samples/frames.txt
```

The second frame (`AA 02 FB 32 61`) represents **-5 C**, but the buggy build interprets `FB` (hex) as unsigned **251 C**, then incorrectly rejects the packet.

## 4. Debug with GDB

```bash
gdb --args ./build/sensor_monitor_buggy samples/frames.txt
```

At the `(gdb)` prompt, enter commands **one per line**:

```gdb
break decode_temperature if raw == 251
run
print raw
next
print temperature_c
print (signed char)raw
list
bt
quit
```

Expected diagnosis: `raw` is 251, `temperature_c` is 251 in the buggy branch, but `(signed char)raw` is -5. GDB allows you to inspect program values at the breakpoint and reveal the unsigned/signed conversion error.

## 5. Fix and confirm

The source has a compile-time switch to show the difference:

- With `-DINTRODUCE_BUG`: `int temperature_c = (int)raw;` (**wrong**).
- Without that flag: `int temperature_c = (int)(int8_t)raw;` (**correct** for GCC on standard Linux targets).

`make` produces the fixed build. Run `make test` to confirm valid negative temperatures are decoded correctly.

For extra practice, manually remove the `INTRODUCE_BUG` branch and use the fixed conversion in both builds. Add a sixth input row with an invalid start byte and update `samples/expected.txt`.

## 6. Useful GCC flags

- `-g3`: include rich debug information for GDB.
- `-O0`: avoid optimization so variables and control flow are easier to inspect.
- `-Wall -Wextra -Wpedantic -Werror`: surface and enforce compile-time warnings.
- `-std=c11`: compile as C11.

## What you can accurately claim

After personally executing the GDB workflow on Linux, you can describe **compiled and debugged a C sensor-frame parser with GCC/GDB; inspected a signedness bug with conditional breakpoints; implemented checksum/range validation and automated sample testing**.

Do **not** claim physical UART communication: the input comes from a text file and only *simulates* UART-like packet payloads.
