#!/bin/bash

# Change number of iterations to run (1 iteration = 4 encryptions)
ITERS=(5000 50000 200000)

# Change noise levels using the C-M-I convention. 
# For eg., add H-H-L by adding "1 1 0" to the array
NOISE_LEVELS=("0 0 0" "1 1 1")

# Change number of runs to calculate average
RUNS_PER_ITERS=3

if [ $# -eq 0 ]
  then
    echo "Usage: ./run_exp.sh <dfr-or-dff>"
    exit
fi

baseline="fr"
baseline_threshold=1

if [ "$1" == "dff" ]
then
    baseline="ff"
fi
if [ -n "$2" ]
then
    baseline_threshold=$2
fi

rm -f aes_$1_accuracy.csv aes_${baseline}accuracy.csv

echo "${ITERS[@]}" >> aes_$1_accuracy.csv
echo "${ITERS[@]}" >> aes_${baseline}_accuracy.csv

for noise in "${NOISE_LEVELS[@]}"
do
    echo "noise level: " $noise
    ./dabangg_noise ${noise} &
    acc_vals=()
    base_acc_vals=()
    for iters in "${ITERS[@]}"
    do
        echo "attack loop iterations: " $iters
        rm -f computed_keys.$1 computed_keys.${baseline}
        for (( i=1; i<=${RUNS_PER_ITERS}; i++ ))
        do
            echo "run " $i " out of " ${RUNS_PER_ITERS}
            ./server >/dev/null 2>&1 &
            sleep 0.5
            key=`./$1 ${iters} | grep KEY | awk '{print $2}'`
            echo $key >> computed_keys.$1
            ./server >/dev/null 2>&1 &
            sleep 0.5
            key=`./$1 ${iters} ${baseline_threshold} | grep KEY | awk '{print $2}'`
            echo $key >> computed_keys.${baseline}
        done
        acc=`python calc_accuracy.py $1`
        base_acc=`python calc_accuracy.py ${baseline}`
        acc_vals+=($acc)
        base_acc_vals+=(${base_acc})
    done
    echo "Killing the 'dabangg_noise' process.."
    pkill -9 dabangg_noise
    echo "${noise}" "${acc_vals[@]}" >> aes_$1_accuracy.csv
    echo "${noise}" "${base_acc_vals[@]}" >> aes_${baseline}_accuracy.csv
done