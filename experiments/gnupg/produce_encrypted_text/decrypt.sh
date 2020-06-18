#!/bin/bash

rm decrypted.txt

gpg --output decrypted.txt --decrypt encrypted.txt.gpg
