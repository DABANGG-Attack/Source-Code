import csv
from re import L
import numpy as np
from scipy import stats

SHOW_PLOT = False

# Initializations
x = []
hit_hist = []
miss_hist = []
low_t = []
high_t = []
confidence = 100
overall_miss_list = []
ITERATIONS = 0

# Read latency values
with open('ff_latency_freq.csv','r') as csvfile:
    plots = csv.reader(csvfile, delimiter=',')
    for row in plots:
        x.append(int(row[0]))
        hit_hist.append(int(row[1]))
        miss_hist.append(int(row[2]))
        ITERATIONS = ITERATIONS + 1

hit_modes_aux = [hit for hit in hit_hist]
miss_modes_aux = [miss for miss in miss_hist]

hit_modes = []
miss_modes = []

for i in range(50):
    if len(hit_modes_aux) < ITERATIONS/4000 \
        or len(miss_modes_aux) < ITERATIONS/4000 \
        or len(stats.mode(hit_modes_aux)[1]) < 1 \
        or len(stats.mode(miss_modes_aux)[1]) < 1 \
        or stats.mode(hit_modes_aux)[1][0] < ITERATIONS/400 \
        or stats.mode(miss_modes_aux)[1][0] < ITERATIONS/400:
        break
    hit_modes.append(stats.mode(hit_modes_aux)[0][0])
    miss_modes.append(stats.mode(miss_modes_aux)[0][0])
    hit_modes_aux[:] = [hit for hit in hit_modes_aux if hit != hit_modes[i]]
    miss_modes_aux[:] = [miss for miss in miss_modes_aux if miss != miss_modes[i]]

hit_modes.sort()
miss_modes.sort()

if hit_modes[-1] < miss_modes[0]:
    print("[Abnormal machine behavior]",
            "Lowest miss latency is higher than highest hit latency")
    if (hit_modes[0] <= 0):
        low_t.append(hit_modes[0])
    else:
        low_t.append(hit_modes[0])
    high_t.append((hit_modes[-1]+miss_modes[0])//2)
    confidence = 50
elif miss_modes[-1] < hit_modes[0]:
    low_t.append((hit_modes[0]+miss_modes[-1])//2)
    high_t.append(hit_modes[-1])
else:
    miss_id = 0
    hit_id = 0
    confidence -= 5
    while hit_id < len(hit_modes) and miss_id < len(miss_modes):
        if hit_modes[hit_id] < miss_modes[miss_id]:
            hit_end = hit_id
            for i in range(hit_id, len(hit_modes)):
                if hit_modes[i] > miss_modes[miss_id]:
                    hit_end = i - 1
                    break
                else:
                    hit_end = i
            low_t.append(hit_modes[hit_id])
            high_t.append((hit_modes[hit_end]+miss_modes[miss_id])//2)
            hit_id = hit_end + 1

        elif miss_id == len(miss_modes) - 1:
            low_t.append((hit_modes[hit_id]+miss_modes[miss_id])//2) 
            high_t.append(hit_modes[-1])
            miss_end = miss_id
            for i in range(miss_id, len(miss_modes)):
                miss_end = i
                if hit_modes[hit_id] < miss_modes[i]:
                    break
            miss_id += 1
        else:   
            miss_end = miss_id
            for i in range(miss_id, len(miss_modes)):
                miss_end = i
                if hit_modes[hit_id] < miss_modes[i]:
                    break
            miss_id = miss_end
            confidence -= 5

arr_size = len(low_t)-1
i = 0
while i < arr_size:
    if high_t[i] + 5 >= low_t[i+1]:
        high_t[i] = high_t[i+1]
        del low_t[i+1]
        del high_t[i+1]
        i -= 1
        arr_size -= 1
        confidence += 5
    i += 1

low_t.reverse()
high_t.reverse()
   
print ("T_ARRAY_SIZE: ", len(low_t))
print("TL_array: ", low_t)
print("TH_array: ", high_t)
 
hit_modes_aux = [hit for hit in hit_hist[3*ITERATIONS//4:ITERATIONS]]

hit_modes = []
for i in range(5):
    if len(hit_modes_aux) < ITERATIONS/4000 \
        or len(stats.mode(hit_modes_aux)[1]) < 1 \
        or stats.mode(hit_modes_aux)[1][0] < ITERATIONS/400:
        break
    hit_modes.append(stats.mode(hit_modes_aux)[0][0])
    hit_modes_aux[:] = [hit for hit in hit_modes_aux if hit != hit_modes[i]]

hit_modes.sort()

step_width = 0
stabilized_freq_threshold = 100
curr_freq_threshold = 0

for i in range(3*ITERATIONS//4,ITERATIONS):
    step_width += 1
    if hit_hist[i] <= hit_modes[0]:
        curr_freq_threshold += 1
    if curr_freq_threshold > stabilized_freq_threshold:
        break

if len(low_t) > 1:
    step_width = (step_width*25)//(len(low_t)-1)
else:
    step_width = (step_width*25)

print ("Step width: ", step_width)
print ("Confidence in calibration: ", confidence, "%")
if confidence < 75:
    print ("[NOTE] Confidence is low, consider running calibration again")

if SHOW_PLOT:
    import matplotlib.pyplot as plt
    x = np.arange(0,ITERATIONS)
    fig, ax = plt.subplots()
    plt.plot(x,hit_hist,color='r',label="clflush hit")
    plt.plot(x,miss_hist,color="yellow",label="clflush miss")
    axes = plt.gca()
    plt.yticks(np.arange(0,1000,step=50),fontsize=24)
    plt.xticks(fontsize=24)
    plt.xlabel("iterations",fontsize=24)
    ax.set_xlim([0,ITERATIONS+1])
    plt.ylabel("latency (cycles)",fontsize=24)
    plt.legend(loc="upper right",bbox_to_anchor=(0.75,1.12),frameon=False,ncol=3,prop={'size':32})
    plt.grid(True,color="black")
    plt.show()
