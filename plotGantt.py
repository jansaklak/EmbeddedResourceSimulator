import matplotlib.pyplot as plt
from collections import OrderedDict

data = []
with open("gantt_data.dat", "r") as file:
    for line in file:
        parts = line.split()
        task_id = parts[0]  # Task ID as string
        task_type = parts[1]
        start_time = int(parts[2])
        end_time = int(parts[3])
        data.append((task_id, task_type, start_time, end_time))

unique_units = OrderedDict()
colors = {}
color_index = 0
for _, unit, _, _ in data:
    if unit not in unique_units:
        unique_units[unit] = color_index
        colors[unit] = plt.cm.tab10(color_index)
        color_index += 1

grouped_data = {}
for task_id, task_type, start, end in data:
    if task_type not in grouped_data:
        grouped_data[task_type] = []
    grouped_data[task_type].append((task_id, task_type, start, end))

legend_patches = [plt.Rectangle((0,0),1,1,fc=colors[unit], edgecolor='black') for unit in unique_units]

rows = [grouped_data[unit] for unit in unique_units]

fig, ax = plt.subplots()

for i, row in enumerate(rows):
    for task_id, task_type, start, end in row:
        ax.broken_barh([(start, end-start)], (i-0.4, 0.8), facecolors=colors[task_type], edgecolors='black')
        ax.text((start+end)/2, i, str(task_id), ha='center', va='center', color='white')

ax.set_xlabel('Czas')
ax.set_ylabel('Zadanie')
ax.set_yticks(range(len(rows)))
ax.set_yticklabels(list(unique_units.keys()))
ax.grid(True)

ax.legend(legend_patches, list(unique_units.keys()))

plt.savefig('data/gantt_chart.png', format='png', dpi=1200)

print("Wykres został zapisany do pliku gantt_chart.png")
plt.show()
plt.close(fig)
