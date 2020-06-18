# RSA Private Key Extraction in GnuPG

## Build GnuPG

1. Download GnuPG from [here](https://gnupg.org/ftp/gcrypt/gnupg/gnupg-1.4.13.tar.gz).
2. Extract it.
3. `cd path/to/gnupg`
4. Install `lib32-glibc` and `gcc-multilib`
5. `./configure –build=i686-pc-linux-gnu "CFLAGS=-m32 -g" "CXXFLAGS=-m32 -g" "LDFLAGS=-m32 -g"`
6. `make`

The binary will show up in `path/to/gnupg/g10/gpg`.

## Pre-Attack Steps

7. Create victim private key.
8. `objdump -D -M intel path/to/gnupg/g10/gpg | less`.
9. Search for `mpih_sqr_n()`, `mpihelp_divrem()`, and `mul_n()` functions to locate target memory addresses. 
10. In the source code, the target instructions are at line numbers 270, 329, and 121 in files `mpih-mul.c`, `mpih-div.c`, and `mpih-mul.c`, respectively.
11. Change `SQR_ADDR`, `MUL_ADDR`, and `MOD_ADDR` in `dff.c` and `dfr.c`.

## Mount Attack

To mount D+F+R, compile `dfr.c` and use `attack.sh`.
