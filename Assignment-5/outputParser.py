import matplotlib
matplotlib.use("agg")
import matplotlib.pyplot as plt 

output_files = ['output_area.txt', 'output_node.txt', 'output_flow.txt']
x_axes = [[250, 500, 750, 1000, 1250], [20, 40, 60, 80, 100], [10, 20, 30, 40, 50]]
x_labels = ['area', 'node', 'flow']
y_labels = ['Thoughput (bits/sec)', 'Average Delay (sec)', 'Delivery Ratio', 'Drop Ratio']

for i in range(len(output_files)):

	throughput = []
	delay = []
	delivery = []
	drop = []

	try:
		f = open(output_files[i], "r")
		for line in f:
			if line.startswith('Throughput'):
				throughput.append(float(line.replace('Throughput:', '').replace('bits/sec', '').strip(' ')))
			elif line.startswith('Average Delay'):
				delay.append(float(line.replace('Average Delay:', '').replace('seconds', '').strip(' ')))
			elif line.startswith('Delivery ratio'):
				delivery.append(float(line.replace('Delivery ratio:', '').strip(' ')))
			elif line.startswith('Drop ratio'):
				drop.append(float(line.replace('Drop ratio:', '').strip(' ')))
		
		f.close()

		plt.grid()
		plt.plot(x_axes[i], throughput, marker='o')
		plt.xlabel(x_labels[i])
		plt.ylabel(y_labels[0])

		figName = 'throughput vs ' + x_labels[i] + '.png'
		plt.savefig(figName, bbox_inches='tight')
		plt.close()
		plt.show()

		plt.grid()
		plt.plot(x_axes[i], delay, marker='o')
		plt.xlabel(x_labels[i])
		plt.ylabel(y_labels[1])

		figName = 'delay vs ' + x_labels[i] + '.png'
		plt.savefig(figName, bbox_inches='tight')
		plt.close()
		# plt.show()

		plt.grid()
		plt.plot(x_axes[i], delivery, marker='o')
		plt.xlabel(x_labels[i])
		plt.ylabel(y_labels[2])

		figName = 'delivery vs ' + x_labels[i] + '.png'
		plt.savefig(figName, bbox_inches='tight')
		plt.close()
		# plt.show()

		plt.grid()
		plt.plot(x_axes[i], drop, marker='o')
		plt.xlabel(x_labels[i])
		plt.ylabel(y_labels[3])

		figName = 'drop vs ' + x_labels[i] + '.png'
		plt.savefig(figName, bbox_inches='tight')
		plt.close()
		# plt.show()
	except Exception as e:
		print(e)