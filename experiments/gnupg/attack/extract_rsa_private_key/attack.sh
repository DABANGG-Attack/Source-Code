#!/bin/bash

rm result.txt

# $1 is binary of either D+F+F or D+F+R code
# $2 is passphrase for the generated RSA private key, if applicable
# $3 is the root directory of GnuPG source code

./$1 & echo $2 | $3/g10/./gpg --batch --yes --passphrase-fd 0 -d encrypted.txt.gpg > /dev/null