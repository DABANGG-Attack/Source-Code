# Keylogging

You can perform [single character](./single_char_lookup) or [multiple character](./multi_char_lookup) keylogging. The [target program](./target_program) processes the auto-fed character sequence.

The character sequence is present in `true_char_seq`.

To check accuracy, F1 score, etc. for single character lookup, use `calculate_accuracy.c` (compile using `gcc -o calc_acc calculate_accuracy.c`). Use `multi_char_check.py` for multiple-character accuracy check.