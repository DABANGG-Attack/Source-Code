#!/bin/bash

rm to_encrypt.txt.gpg

../gnupg-1.4.13/g10/./gpg -r "TestKey" -e ./to_encrypt.txt