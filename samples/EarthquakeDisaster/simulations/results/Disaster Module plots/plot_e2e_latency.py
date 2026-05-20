import os
import glob
import matplotlib.pyplot as plt
import numpy as np

results_dir = r"D:\omnetpp-5.7.1\samples\EarthquakeDisaster\simulations\results"
vec_files = glob.glob(os.path.join(results_dir, "*.vec"))

vector_map = {}

# --------------------------
# Map vector IDs
# --------------------------
for file in vec_files:
    with open(file, "r", errors="ignore") as f:
        for line in f:
            if line.startswith("vector"):
                parts = line.split()
                if len(parts) >= 4:
                    vector_map[parts[1]] = parts[2] + "." + parts[3]

times = []
delays = []

# --------------------------
# Extract udpE2EDelay samples
# --------------------------
for file in vec_files:
    with open(file, "r", errors="ignore") as f:
        for line in f:
            parts = line.split()
            if len(parts) == 3 and parts[0].isdigit():
                vec_id = parts[0]
                t = float(parts[1])
                v = float(parts[2])
                signal = vector_map.get(vec_id, "")

                if "udpE2EDelay" in signal:
                    times.append(t)
                    delays.append(v)

print("udpE2EDelay samples:", len(delays))

if not delays:
    raise RuntimeError("No udpE2EDelay samples found in .vec files")

delays = np.array(delays)
times = np.array(times)

# --------------------------
# Plot 1: histogram
# --------------------------
plt.figure(figsize=(8, 6))
plt.hist(delays, bins=30, edgecolor="black")
plt.axvline(0.10, linestyle="--", label="0.10 s target")
plt.xlabel("True End-to-End Latency (seconds)")
plt.ylabel("Number of Packets")
plt.title("End-to-End Latency Distribution (udpE2EDelay)")
plt.grid(True)
plt.legend()

out1 = os.path.join(results_dir, "true_e2e_latency_histogram.png")
plt.savefig(out1, dpi=300, bbox_inches="tight")
print("Saved:", out1)
plt.show()

# --------------------------
# Plot 2: trend over time
# --------------------------
order = np.argsort(times)
times = times[order]
delays = delays[order]

# ---- window size in seconds ----
window = 0.5

bins = np.arange(0, times.max() + window, window)

mean_latency = []
median_latency = []
p95_latency = []
bin_centers = []

for i in range(len(bins)-1):

    mask = (times >= bins[i]) & (times < bins[i+1])
    values = delays[mask]

    if len(values) > 0:
        mean_latency.append(np.mean(values))
        median_latency.append(np.median(values))
        p95_latency.append(np.percentile(values,95))
        bin_centers.append((bins[i]+bins[i+1])/2)

mean_latency = np.array(mean_latency)
median_latency = np.array(median_latency)
p95_latency = np.array(p95_latency)
bin_centers = np.array(bin_centers)

plt.figure(figsize=(10,6))

plt.plot(bin_centers, mean_latency, label="Mean Latency")
plt.plot(bin_centers, median_latency, label="Median Latency")
plt.plot(bin_centers, p95_latency, label="95th Percentile")

plt.xlabel("Simulation Time (s)")
plt.ylabel("End-to-End Latency (seconds)")
plt.title("Aggregated End-to-End Alert Latency")

plt.grid(True)
plt.legend()

plt.tight_layout()

plt.savefig("aggregated_latency_trend.png", dpi=300)

plt.show()

sorted_latency = np.sort(delays)
cdf = np.arange(len(sorted_latency)) / float(len(sorted_latency))

plt.figure(figsize=(9,6))

plt.plot(sorted_latency, cdf, linewidth=2)

plt.xlabel("End-to-End Latency (seconds)", fontsize=12)
plt.ylabel("Cumulative Probability", fontsize=12)

plt.title("CDF of End-to-End Alert Latency", fontsize=14)

plt.grid(True)

plt.tight_layout()
plt.savefig("e2e_latency_cdf.png", dpi=300)
plt.show()

plt.figure(figsize=(6,6))

plt.boxplot(delays, vert=True)

plt.ylabel("End-to-End Latency (seconds)")
plt.title("Alert Latency Distribution")

plt.grid(True)

plt.savefig("e2e_latency_boxplot.png", dpi=300)
plt.show()