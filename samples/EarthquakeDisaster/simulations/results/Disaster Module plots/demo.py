import os
import glob
import numpy as np
import matplotlib.pyplot as plt

results_dir = r"D:\omnetpp-5.7.1\samples\EarthquakeDisaster\simulations\results"
vec_files = glob.glob(os.path.join(results_dir, "*.vec"))

vector_map = {}

# -------------------------
# Read vector IDs
# -------------------------

for file in vec_files:
    with open(file, "r", errors="ignore") as f:
        for line in f:
            if line.startswith("vector"):
                parts = line.split()
                vector_map[parts[1]] = parts[2] + "." + parts[3]

queueURLLC = []
queueEMBB = []
queueMMTC = []
drops = []

# -------------------------
# Extract values
# -------------------------

for file in vec_files:
    with open(file, "r", errors="ignore") as f:
        for line in f:
            parts = line.split()

            if len(parts) == 3 and parts[0].isdigit():

                vec_id = parts[0]
                value = float(parts[2])
                signal = vector_map.get(vec_id, "")

                if "queueDepthURLLC" in signal:
                    queueURLLC.append(value)

                elif "queueDepthEMBB" in signal:
                    queueEMBB.append(value)

                elif "queueDepthMMTC" in signal:
                    queueMMTC.append(value)

                elif "packetDrop" in signal or "alertDropped" in signal:
                    drops.append(value)

print("URLLC queue samples:", len(queueURLLC))
print("EMBB queue samples:", len(queueEMBB))
print("MMTC queue samples:", len(queueMMTC))
print("Drop samples:", len(drops))

# -------------------------
# Function to build curve
# -------------------------

def build_curve(queue):

    n = min(len(queue), len(drops))

    if n == 0:
        return None, None

    queue = np.array(queue[:n])
    drop = np.array(drops[:n])

    bins = np.linspace(queue.min(), queue.max(), 20)
    digitized = np.digitize(queue, bins)

    x = []
    y = []

    for i in range(1, len(bins)):

        mask = digitized == i

        if np.sum(mask) > 0:
            x.append((bins[i] + bins[i-1]) / 2)
            y.append(np.mean(drop[mask]))

    return x, y


# -------------------------
# Generate curves
# -------------------------

x1, y1 = build_curve(queueURLLC)
x2, y2 = build_curve(queueEMBB)
x3, y3 = build_curve(queueMMTC)


# -------------------------
# Plot function
# -------------------------

def plot_graph(x, y, label, filename):

    if x is None:
        print(label, "no data")
        return

    plt.figure(figsize=(10,6))

    plt.plot(x, y, marker='o', linewidth=2)

    plt.xlabel("Queue Depth (Packets)")
    plt.ylabel("Packet Drops")
    plt.title(f"Packet Drops vs Queue Depth ({label})")

    plt.grid(True)

    output = os.path.join(results_dir, filename)
    plt.savefig(output, dpi=300, bbox_inches="tight")

    print("Saved:", output)

    plt.show()


# -------------------------
# Plot all 3 graphs
# -------------------------

plot_graph(x1, y1, "URLLC", "packet_drop_URLLC.png")
plot_graph(x2, y2, "eMBB", "packet_drop_eMBB.png")
plot_graph(x3, y3, "mMTC", "packet_drop_mMTC.png")