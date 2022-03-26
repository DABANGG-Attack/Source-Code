import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import matplotlib.lines as mlines
import csv
import sys

t_z = []
encryptions = []
encrypt_labels = []
dff_at_noise = []
ff_at_noise = []
markers = ['o', '^', 'v', 'X','s']
linestyles = ['solid', 'dashed', 'dotted', 'dashdot', 'solid']
acc_labels = ['0%','20%','40%','60%','80%','100%']
noise_labels=[]

def make_noise_label(cmi_code):
    noise_str = ''
    for i in range(3):
        if cmi_code[i] == 1:
            noise_str += 'H'
        else:
            noise_str += 'L'
        if i < 2:
            noise_str += '-'
    return noise_str

with open('aes_'+sys.argv[1]+'_accuracy.csv','r') as csvfile:
    file = list(csv.reader(csvfile, delimiter=' '))
    for num_iters in file[0]:
        encryptions.append(int(num_iters))
        encrypt_labels.append(str(int(num_iters)//1000)+'K')
    for noise_id in range(1, len(file)):
        dff_at_noise.append([])
        cmi_code = []
        for i in range(3):
            cmi_code.append(int(file[noise_id][i]))
        noise_labels.append(make_noise_label(cmi_code))
        for i in range(3, len(file[noise_id])):
            dff_at_noise[noise_id-1].append(int(float(file[noise_id][i])))

with open('aes_'+sys.argv[1][1:]+'_accuracy.csv','r') as csvfile:
    file = list(csv.reader(csvfile, delimiter=' '))
    for noise_id in range(1, len(file)):
        ff_at_noise.append([])
        for i in range(3, len(file[noise_id])):
            ff_at_noise[noise_id-1].append(int(float(file[noise_id][i])))

fig, ax = plt.subplots()
legends = []
if sys.argv[1] == 'dfr':
    attack_type = 'RELOAD'
else:
    attack_type = 'FLUSH'
dff_patch = mpatches.Patch(color='y', label='DABANGG+FLUSH+'+attack_type)
ff_patch = mpatches.Patch(color='red', label='FLUSH+'+attack_type)

for i in range(len(dff_at_noise)):
    plt.plot(encryptions, dff_at_noise[i], color='y', linestyle=linestyles[i], 
                marker=markers[i], markersize=10, linewidth=3,label=noise_labels[i])
    plt.plot(encryptions, ff_at_noise[i], color='red', linestyle=linestyles[i], 
                marker=markers[i], markersize=10, linewidth=3)
    legends.append(mlines.Line2D([], [], color='k', linestyle=linestyles[i], 
                marker=markers[i], markersize=10, linewidth=3,label=noise_labels[i]))

ax.set_xscale('log')
ax.set_xlabel('noise level',fontsize=24)
ax.set_xticks(encryptions)
ax.set_xticklabels(encrypt_labels,fontsize=24,rotation=45)
ax.set_xlabel('attack iterations (1 iteration = 4 encryptions)',fontsize=24)
ax.set_ylabel('accuracy',fontsize=24,rotation=90)
ax.set_yticklabels(acc_labels,fontsize=24)
ax.set_ylim([0,100])
ax.set_xlim([encryptions[0]-1, encryptions[-1]+1])
plt.legend(bbox_to_anchor=(0,1.1),handles=[dff_patch, ff_patch]+legends,
			frameon=False,loc="upper left",ncol=3,prop={'size':28})
plt.grid(True)
plt.show()