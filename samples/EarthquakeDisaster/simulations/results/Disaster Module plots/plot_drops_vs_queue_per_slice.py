import os
import glob
import matplotlib.pyplot as plt
from collections import defaultdict

results_dir = r"D:\omnetpp-5.7.1\samples\EarthquakeDisaster\simulations\results"
vec_files = glob.glob(os.path.join(results_dir, "*.vec"))

vector_map = {}

# ----------------------------
# Read vector IDs
# ----------------------------

for file in vec_files:
    with open(file, "r", errors="ignore") as f:
        for line in f:
            if line.startswith("vector"):
                parts = line.split()
                vec_id = parts[1]
                name = parts[2] + "." + parts[3]
                vector_map[vec_id] = name

# ----------------------------
# Extract data
# ----------------------------

urllc = []
embb = []
mmtc = []

for file in vec_files:
    with open(file, "r", errors="ignore") as f:
        for line in f:
            parts = line.split()

            if len(parts) == 3 and parts[0].isdigit():

                vec_id = parts[0]
                time = float(parts[1])
                value = float(parts[2])

                signal = vector_map.get(vec_id, "")

                if "queueDepthURLLC" in signal:
                    urllc.append((time, value))

                elif "queueDepthEMBB" in signal:
                    embb.append((time, value))

                elif "queueDepthMMTC" in signal:
                    mmtc.append((time, value))


# ----------------------------
# Downsample data (0.1s bins)
# ----------------------------

def downsample(data, step=0.1):

    bins = defaultdict(list)

    for t, v in data:
        key = round(t / step) * step
        bins[key].append(v)

    times = []
    values = []

    for k in sorted(bins):
        times.append(k)
        values.append(sum(bins[k]) / len(bins[k]))

    return times, values


urllc_t, urllc_v = downsample(urllc)
embb_t, embb_v = downsample(embb)
mmtc_t, mmtc_v = downsample(mmtc)

print("URLLC points:", len(urllc_t))
print("EMBB points:", len(embb_t))
print("MMTC points:", len(mmtc_t))

# ----------------------------
# Plot
# ----------------------------

plt.figure(figsize=(10,6))

plt.plot(urllc_t, urllc_v, label="URLLC", linewidth=2)
plt.plot(embb_t, embb_v, label="eMBB", linewidth=2)
plt.plot(mmtc_t, mmtc_v, label="mMTC", linewidth=2)

plt.xlabel("Simulation Time (s)")
plt.ylabel("Queue Depth (Packets)")
plt.title("Queue Depth per Network Slice")

plt.grid(True)
plt.legend()

plt.xlim(left=0)
plt.ylim(bottom=0)

output = os.path.join(results_dir, "queue_depth_per_slice_clean.png")

plt.savefig(output, dpi=300, bbox_inches="tight")

print("Saved graph:", output)

plt.show()