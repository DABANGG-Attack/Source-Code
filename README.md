# DABANGG Attack

DABANGG are a set of refinements over existing flush-based attacks which make them resilient to noise by taking frequency and core placement of victim and attack threads into account. We term DABANGG-enabled Flush+Reload and Flush+Flush attacks D+F+R and D+F+F, respectively.

This repository contains the following experiments:

1. Single and multi-character keylogging
2. AES (T-Table based) private key extraction in OpenSSL
3. RSA (Square-Multiply-Reduce exponentiation) private key in GnuPG
4. Covert channel attack
5. Spectre attack

To calibrate for your system, go to [calibration](./calibration). To view core frequencies in your system, go to [freq_scripts](./freq_scripts). To perform the experiments, go to [experiments](./experiments).

