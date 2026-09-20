import os
import matplotlib.pyplot as plt
import numpy as np

def parse_time(time_str):
    """Parse a time string in the format 'XmY.YYs' (e.g., '0m0.001s')."""
    return float(time_str) 

def read_times(filename):
    """Read the times from a file and return the average time."""
    with open(filename, 'r') as f:
        times = []
        for line in f:
            if line.startswith("time = "):
                time_str = line.split()[2]
                times.append(parse_time(time_str))
        return np.mean(times)

def plot_times():
    # Set the path for the data folder
    data_folder = 'data/'

    # Create lists to store the sizes and average times
    sizes_filter1 = []
    sizes_filter2 = []

    # Iterate through the files in the data folder
    for filename in os.listdir(data_folder):
        if filename.startswith('filter1') and filename.endswith('.out'):
            size = int(filename.split('.')[1])  # Extract size from the filename
            avg_time = read_times(os.path.join(data_folder, filename))
            sizes_filter1.append({"size": size*size*12, "time": avg_time})
        elif filename.startswith('filter2') and filename.endswith('.out'):
            size = int(filename.split('.')[1])  # Extract size from the filename
            avg_time = read_times(os.path.join(data_folder, filename))
            sizes_filter2.append({"size": size*size*12, "time": avg_time})

    sizes_filter1 = sorted(sizes_filter1, key=lambda x: x['size'])
    sizes_filter2 = sorted(sizes_filter2, key=lambda x: x['size'])

    # Plot the data
    plt.figure(figsize=(10, 6))
    plt.plot([s["size"] for s in sizes_filter1],[s["time"] for s in sizes_filter1],label='Filter 1', marker='o', linestyle='-', color='blue')
    plt.plot([s["size"] for s in sizes_filter2],[s["time"] for s in sizes_filter2], label='Filter 2', marker='s', linestyle='-', color='red')

    # Labels and title
    plt.xlabel('Size', fontsize=12)
    plt.ylabel('Average Time (seconds)', fontsize=12)
    plt.title('Average Execution Time vs. Size for Filter 1 and Filter 2', fontsize=14)
    plt.legend()
    plt.grid(True)

    # Show the plot
    plt.savefig("plot.png")

if __name__ == "__main__":
    plot_times()
