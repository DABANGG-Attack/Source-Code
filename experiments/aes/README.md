# AES Private Key Extraction

Code adapted from [nepoche/Flush-Reload](https://github.com/nepoche/Flush-Reload).

## OpenSSL Installation

1. Download openssl-1.1.0f from [here](https://www.openssl.org/source/old/)
2. `tar -xvf openssl-1.1.0f.tar.gz`
3. `cd openssl-1.1.0f`
4. `./config -d shared no-asm no-hw`
5. `sudo make; sudo make install_sw`

## T-table Addresses

A flush-based attack monitors cache lines. To monitor the correct cache lines, we must find the offset of addresses of the T-tables
with respect to the **libcrypto.so** shared object. To find this, perform the following commands:

1. `cd /usr/lib`
2. `readelf -a libcrypto.so > ~/aeslib.txt`
3. `vi ~aeslib.txt`
4. `/Te0` to get offset of Te0
5. Update `probe` array in `dfr.cpp` or `dff.cpp`
6. [Calibrate](../../calibration)

## Compile

1. `export LD_LIBRARY_PATH=/usr/local/lib`
2. `chmod +x make.sh`
3. `./make.sh`

## Run

To run same-process attack, set `ENABLE_DEBUG` to `1` in `dfr.cpp` or `dff.cpp`. For cross-VM attack, `ENABLE_DEBUG` is `0` and appropriate setup must be done. The VMs must share the same physical processor (same chip) and the IPs must be internally accessible. To run cross-VM, run `./server` in victim VM and `./dfr` (or `./dff`) in attacker VM.