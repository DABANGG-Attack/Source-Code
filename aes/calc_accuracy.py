# Christopher P. Matthews
# christophermatthews1985@gmail.com
# Sacramento, CA, USA
import sys
import numpy as np

def levenshtein(s, t):
        ''' From Wikipedia article; Iterative with two matrix rows. '''
        if s == t: return 0
        elif len(s) == 0: return len(t)
        elif len(t) == 0: return len(s)
        v0 = [None] * (len(t) + 1)
        v1 = [None] * (len(t) + 1)
        for i in range(len(v0)):
            v0[i] = i
        for i in range(len(s)):
            v1[0] = i + 1
            for j in range(len(t)):
                cost = 0 if s[i] == t[j] else 1
                v1[j + 1] = min(v1[j] + 1, v0[j + 1] + 1, v0[j] + cost)
            for j in range(len(v0)):
                v0[j] = v1[j]
                
        return v1[len(t)]

# Secret key
t='1122334455667788ff00eeddccbbaa99'
# t='000100010010001000110011010001000101010101100110011 \
#     10111100010001111111100000000111011101101110111001100101110111010101010011001'

source_keys = []
with open('computed_keys.'+sys.argv[1],'r') as file:
    source_keys = [line.rstrip() for line in file]

acc = []
for key in source_keys:
    acc.append(100*(1-(levenshtein(key, t)/32)))

print("{:.2f}".format(np.average(acc)))