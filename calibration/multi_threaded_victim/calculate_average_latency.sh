#!/bin/bash

# Enter configuration binary to calculate an average value over 100 runs

sum=0;
for i in {1..100}
do
    var=$(./$1);
    echo $var;
    sum=$(echo "$sum + $var" | bc);
done
echo "Average is: "
echo "$sum/100" | bc;
