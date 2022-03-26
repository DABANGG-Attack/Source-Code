# DABANGG: A Case for Noise Resilient Flush-Based Cache Attacks

DABANGG are a set of refinements over existing flush-based cache attacks which make them resilient to noise by taking frequency and core placement of victim and attack threads into account. We term DABANGG-enabled Flush+Reload and Flush+Flush attacks D+F+R and D+F+F, respectively.

This artifact aims to showcase impact of frequency changes and relative core placement on latency variation of key instructions like clflush and movl (henceforth, reload). In particular, we focus on 3 goals:

1. Capture and showcase the variable latency of execution of clflush and reload instructions. 
2. Post-process the frequency distribution to obtain DABANGG attacker-specific parameters.
3. Showcase the noise resilience of DABANGG-enabled attacks by extracting the private key from OpenSSL-based AES cryptosystem in a cross-core attack.

To achieve these goals, please follow the steps:

1. Perform [calibration](./calibration) to get threshold and other parameters for both the DABANGG-enabled and baseline F+R and F+F attacks.
2. Mount a cross-core [aes key extraction attack](./aes) and use the provided plotting scripts to visualize the gains in accuracy at various number of encryption calls and at different noise levels. 

**Next Steps:** Perform [calibration](./calibration).