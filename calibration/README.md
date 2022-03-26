# Calibration

The calibration step determines the attacker-specific parameters. Specifically, we need to obtain `T_ARRAY_SIZE`, `tl_array`, `th_array`, and `step_width`.

## DABANGG Attacker-Specific Parameters

Attacker-specific parameters be obtained for D+F+R attack as follows:

1. `make`
2. `./fr_stepped_distribution`
3. `python fr_process_data.py`

Similarly, replace `fr_stepped_distribution` with `ff_stepped_distribution` and `fr_process_data.py` with `ff_process_data.py` to perform calibration for D+F+F attack.

One can tweak the number of attack loop iterations (default: 10K) by changing the `ITERATIONS` parameter in `fr_stepped_distribution.c`. 

### Frequency Distribution Plot

The frequency distribution curve can be be obtained by setting the `SHOW_PLOT` knob to `True` in `fr_process_data.py` (requires `matplotlib`).

**Note:** We recommend viewing the plot using a maximized window for best experience.

The plotted distribution is similar to Figure 1 of the paper. Overall, the program captures all relevant thresholds by running attacker and victim in same program, same logical core, same physical core, and different physical cores for `ITERATIONS` iterations each for an overall `4*ITERATIONS` iterations. The distribution plot showcases this and is useful in troubleshooting.

The post-processing script uses multi-modal sampling to get the most representative hit and miss thresholds. Then, we perform interval-matching to get the correct interleaving of lower and upper hit thresholds.

## Baseline Attack Calibration

### FLUSH+RELOAD

1. `cd standard_histogram/fr` 
2. `make`
3. `./calibration_fr`
4. The calibration may output the following:
    ```
    Flush+Reload possible!
    The lower the threshold, the lower the number of false positives.
    Suggested cache hit/miss threshold: 117
    ```
    In this case, use the suggested threshold (117 in this example).
5. If the calibration does not output a suggested threshold, use the provided histogram (in the program output) to determine a threshold that accurately distinguishes between a reload hit and a miss.
6. If that's not feasible, you may opt not to provide a threshold for the baseline attack. The attacker program will automatically use the appropriate threshold using DABANGG's thresholds.

### FLUSH+FLUSH

1. `cd standard_histogram/ff` 
2. `make`
3. `./calibration`
4. Use the provided histogram (in the program output) to determine a threshold that accurately distinguishes between a clflush hit and a miss.
5. If that's not feasible, you may opt not to provide a threshold for the baseline attack. The attacker program will automatically use the appropriate threshold using DABANGG's thresholds.

## Troubleshooting

### Low calibration confidence or low atttack accuracy

The post-processing that determines thresholds is done using common statistical techniques which may not be able to accurately distinguish between hits and misses at all frequency and core placement levels. 

Please follow the steps:

1. Re-run the calibration.
2. Increase `ITERATIONS` (to, say, 20K) and recalibrate.
3. Calibrate in presence of noise. To do this, follow the steps:
    * `../aes/dabangg_noise 1 1 1 &`
    * Perform calibration.
    * `pkill dabangg_noise`
4. Manually inspect the plot. If the post-processing script emits abnormal threshold values or a lot of threshold pairs, please follow the steps:
    * Generate the plot by setting `SHOW_PLOT` knob in the `*_process_data.py` script. 
    * Zoom in to see the individual steps and use the x and y values at the bottom right of the screen to determine a threshold that would accurately distinguish a hit from a miss. 
    * The most useful thresholds are present in the first and fourth quarters of the x-axis of the plot as they represent same-program and different physical core scenarios.
    * It may not be possible to accurately distinguish thresholds at all frequency levels. We are most interested in the stablized thresholds in the same-program and different physical core scenarios. 

Please feel free to reach out to us if some issue is unresolved.