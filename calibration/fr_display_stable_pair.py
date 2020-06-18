import csv
import numpy as np

DETAILED_STATS = False
SHOW_PLOT = False

# Initializations
x = []
y1 = []
y2 = []
low_t = []
high_t = []
overall_miss_list = []
ITERATIONS = 0

# Read latency values
with open('fr_latency_freq.csv','r') as csvfile:
    plots = csv.reader(csvfile, delimiter=',')
    for row in plots:
        x.append(int(row[0]))
        y1.append(int(row[1]))
        y2.append(int(row[2]))
        ITERATIONS = ITERATIONS + 1

ITERATIONS = ITERATIONS/4
lower_lim = [ITERATIONS/5, ITERATIONS+ITERATIONS/5, 2*ITERATIONS + ITERATIONS/5, 3*ITERATIONS + ITERATIONS/5]
upper_lim = [ITERATIONS, 2*ITERATIONS, 3*ITERATIONS, 4*ITERATIONS]

confidence = 400
step_width = 100000

# Calculates threshold pairs based on percentile of hits/ misses covered by the pair and 
# calculates confidence based on the coverage and overlap of hits/ misses
for i in range(0,4):

    overall_miss_list.extend(y2[lower_lim[i]:upper_lim[i]])
    x0 = np.asarray(x[lower_lim[i]:upper_lim[i]])
    median_hit = np.median(y1[lower_lim[i]:upper_lim[i]])
    p001_hit = np.percentile(y1[lower_lim[i]:upper_lim[i]],99)
    p001_miss = np.percentile(y2[lower_lim[i]:upper_lim[i]],1)
    p005_hit = np.percentile(y1[lower_lim[i]:upper_lim[i]],95)
    p005_miss = np.percentile(y2[lower_lim[i]:upper_lim[i]],5)
    p010_hit = np.percentile(y1[lower_lim[i]:upper_lim[i]],90)
    p010_miss = np.percentile(y2[lower_lim[i]:upper_lim[i]],10)
    p090_hit = np.percentile(y1[lower_lim[i]:upper_lim[i]],10)
    p090_miss = np.percentile(y2[lower_lim[i]:upper_lim[i]],90)
    p095_miss = np.percentile(y2[lower_lim[i]:upper_lim[i]],95)

    if DETAILED_STATS:
        print "Hit median: ", median_hit
        print "Hit 99th percentile: ", p001_hit
        print "Miss 1st percentile: ", p001_miss
        print "Hit 95th percentile: ", p005_hit
        print "Miss 5th percentile: ", p005_miss 
        print "Hit 90th percentile: ", p010_hit
        print "Miss 10th percentile: ", p010_miss
        print "Hit 10th percentile: ", p090_hit
        print "Miss 90th percentile: ", p090_miss
        print "Miss 95th percentile: ", p095_miss

    if (p001_miss - p001_hit) > (0.1*p001_hit) and (p001_miss - p001_hit) < (0.15*p001_miss):
        p100 = (p001_hit + p001_miss)/2
        step_width = ((1+(p001_miss - p001_hit))*step_width)/p001_hit
    elif (p001_miss - p001_hit) > (0.1*p001_hit) and (median_hit - p001_hit) < 0.1*median_hit:
        p100 = p001_hit
    elif (p005_miss - p005_hit) > (0.1*p005_hit) and (p005_hit - p005_miss) < (0.15*p005_hit):
        confidence = confidence - 20
        p100 =  (p005_hit + p005_miss)/2
        step_width = ((1+(p005_miss - p005_hit))*step_width)/p005_hit
    elif (p005_miss - p005_hit) > (0.1*p005_hit) and (median_hit - p005_hit) < 0.1*median_hit:
        confidence = confidence - 30
        p100 = p005_hit
    elif (p010_miss - p010_hit) > (0.1*p010_hit) and (p010_hit - p010_miss) < (0.15*p010_hit):
        confidence = confidence - 40
        p100 = (p010_hit + p010_miss)/2
        step_width = ((1+(p010_miss - p010_hit))*step_width)/p010_hit
    elif (p010_miss - p010_hit) > (0.1*p010_hit) and (median_hit - p010_hit) < 0.1*median_hit:
        confidence = confidence - 50
        p100 = p010_hit 
    else:
        confidence = confidence - 60
        p100 = p010_hit

    if (p100 - p090_hit) > 0.15*median_hit:
        confidence = confidence - 20
        step_width = 0.85*step_width
        
    low_t.append(int(p090_hit))
    high_t.append(int(p100))

    if DETAILED_STATS:
        print "Preliminary pairs ", i+1, ": <", p090_hit, ",", p100, ">"

overall_p001_miss = np.percentile(overall_miss_list,1)

if DETAILED_STATS:
    print "Overall miss 1st percentile: ", overall_p001_miss

low_t = np.sort(low_t)
high_t = np.sort(high_t)
i = 0
t_count = 0

for i in range(4):
    if (overall_p001_miss - high_t[3-i]) > 0.1*high_t[3-i]:
        t_count = t_count + 1
        print "Suggested threshold pair ", t_count, ": <", low_t[0], ",", high_t[3-i], ">"
        i = 3-i
        break
    confidence = confidence - 10

for k in range(4):
    if k>i:
        t_count = t_count + 1
        print "Suggested threshold pair ", t_count, ": <", low_t[k], ",", high_t[k], ">"
        
print "Threshold array size: ", t_count
print "Step width: ", step_width
print "Confidence in calibration: ", int(confidence/4), "%"
if confidence/4 < 70:
    print "NOTE: Confidence is low, consider running calibration again"

if SHOW_PLOT:
    import matplotlib.pyplot as plt

    for i in range(4*ITERATIONS):
        if i>=ITERATIONS and i<2*ITERATIONS:
            x[i] = x[i] + ITERATIONS
        elif i >= 2*ITERATIONS and i<3*ITERATIONS:
            x[i] = x[i] + 2*ITERATIONS
        elif i >= 3*ITERATIONS:
            x[i] = x[i] + 3*ITERATIONS

    x = np.asarray(x)
    plt.plot(x,y1,color='r',label="reload hit")
    plt.plot(x,y2,color="yellow",label="reload miss")
    axes = plt.gca()
    plt.yticks(np.arange(0,1000,step=50),fontsize=24)
    plt.xticks(fontsize=24)
    plt.xlabel("iterations",fontsize=24)
    plt.ylabel("latency (cycles)",fontsize=24)
    plt.legend(loc="upper right",bbox_to_anchor=(0.5,1.13),frameon=False,ncol=3,prop={'size':32})
    plt.grid(True,color="black")
    plt.show()
