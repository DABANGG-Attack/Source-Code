# Single Character Keylogging: Spy

This directory contains the DABANGG+FLUSH+RELOAD (`dfr.c`) and DABANGG+FLUSH+FLUSH (`dff.c`) attack programs.

## Pre-attack steps

 Before running the attack, make sure to obtain the requisite attack-specific parameters using [calibration](../../calibration).

Pre-attack steps for D+F+F attack, for example, are as follows:

1. Specify the `step_width`, `T_ARRAY_SIZE` (number of pairs of thresholds) and each lower and upper threshold in `tl_array` and `th_array`, respectively. 
2. You can change `regular_gap` to change the waiting period of the attack loop.
3. While victim-specific parameters usually don't need to be changed, you may change them through the `acc_interval`, `burst_seq`, `burst_wait`, and `burst_gap` variables in the program. 

We also need to profile the victim to get the cache line address which will be monitored. Refer to [victim](../victim) directory for more details. The target address on my machine is `0x2189`.

## Mount the attack

To mount D+F+R attack, for example: 

1. `./dfr ../target_program/./abcd 0x2189 >> spy_exec_time` in one terminal
2. `./abcd >> abcd_exec_time` in another terminal
