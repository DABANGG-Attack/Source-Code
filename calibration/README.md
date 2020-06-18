# Calibration

The calibration step determines the attacker-specific parameters. Specifically, we need to obtain `T_array` and `step_width`.

Attacker-specific parameters be obtained for D+F+R attack as follows:

1. `make`
2. `./fr_stepped_distribution`
3. `python fr_display_stable_pair.py`

One can tweak the number of attack loop iterations (default: 200,000) by changing the `ITERATIONS` parameter in `fr_stepped_distribution.c`. 

The detailed statistics for hit and miss latency at different configurations are viewable by setting `DETAILED_STATS` knob to `True` in `fr_display_stable_pair.py`. 

The frequency distribution curve can be be obtained by setting the `SHOW_PLOT` knob to `True` in `fr_display_stable_pair.py` (requires `matplotlib`).

The average `clflush` latency for a multi-threaded victim application at different core placements can be obtained. View [multi_threaded_victim](./multi_threaded_victim).

