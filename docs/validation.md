# Validation plan

Run `make clean && make test` on Linux with GCC, Make, diff and Python 3. No third-party Python packages or physical hardware are required.

The original five-frame fixture is checked by exact output comparison. The extended black-box suite contains 19 named checks:

| Coverage | Cases |
|---|---:|
| Valid positive, negative, zero and boundary readings | 5 |
| Out-of-range temperature and humidity | 3 |
| Invalid start, checksum and sensor ID | 3 |
| Malformed, missing, extra and oversized bytes | 4 |
| Comments and empty lines | 1 |
| Missing argument and input file | 2 |
| Intentional signedness bug reproduction | 1 |

Status: proposed extended tests; record actual results after execution. A passing bug-reproduction test means the intentional defect was observed in the deliberately buggy build, not that this build is suitable for normal use. No physical UART testing or comprehensive fuzz testing is claimed.
