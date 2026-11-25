# Signedness defect reproduction

The intentionally buggy executable converts the unsigned byte `0xFB` to integer 251, rather than decoding -5°C. It rejects a valid frame as a temperature-range error. The normal build casts through `int8_t` on the GCC Linux target.

```bash
make buggy
./build/sensor_monitor_buggy samples/frames.txt
gdb --args ./build/sensor_monitor_buggy samples/frames.txt
```

At the GDB prompt:

```gdb
break decode_temperature if raw == 251
run
print /x raw
next
print temperature_c
print (int)(signed char)raw
bt
quit
```

Expected observations: raw 0xfb; buggy decoded value 251; signed interpretation -5. These are expected values, not a recorded GDB session. Confirm the corrected executable with `make test`. Capture actual debugger output if GDB is available.
