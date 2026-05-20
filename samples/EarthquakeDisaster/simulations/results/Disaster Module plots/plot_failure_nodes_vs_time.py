import os
import glob
import pandas as pd
import matplotlib.pyplot as plt

RESULT_DIR = r"D:\omnetpp-5.7.1\samples\EarthquakeDisaster\simulations\results"
OUTPUT = os.path.join(RESULT_DIR, "failure_nodes_vs_time.png")

plt.rcParams.update({
    "figure.figsize": (8,5),
    "figure.dpi": 300,
    "font.size": 12
})


def parse_vec_file(filepath):
    """
    Reads OMNeT++ .vec file and extracts vectors
    """
    vectors = {}
    id_map = {}

    with open(filepath, "r", errors="ignore") as f:
        for line in f:

            if line.startswith("vector"):
                parts = line.split()

                vec_id = int(parts[1])
                module = parts[2]
                name = parts[3]

                key = f"{module}.{name}"

                id_map[vec_id] = key
                vectors[key] = {"t": [], "v": []}

            elif line and line[0].isdigit():

                parts = line.split()

                if len(parts) < 3:
                    continue

                try:
                    vec_id = int(parts[0])
                    t = float(parts[1])
                    v = float(parts[2])
                except:
                    continue

                if vec_id in id_map:
                    key = id_map[vec_id]
                    vectors[key]["t"].append(t)
                    vectors[key]["v"].append(v)

    dfs = {}
    for k in vectors:
        dfs[k] = pd.DataFrame(vectors[k])

    return dfs


# ----------------------------------------------------
# LOAD ALL VECTOR FILES
# ----------------------------------------------------

vec_files = glob.glob(os.path.join(RESULT_DIR, "*.vec"))

print("Found", len(vec_files), "vec files")

all_iot_fail = []
all_ap_fail = []

for vf in vec_files:

    print("Reading:", os.path.basename(vf))

    data = parse_vec_file(vf)

    for key, df in data.items():

        if "failedIots" in key:
            all_iot_fail.append(df)

        if "failedAps" in key:
            all_ap_fail.append(df)


# ----------------------------------------------------
# PLOT GRAPH
# ----------------------------------------------------

plt.figure()

if all_iot_fail:
    for df in all_iot_fail:
        plt.plot(df["t"], df["v"], color="red", alpha=0.6, label="IoT Failures")

if all_ap_fail:
    for df in all_ap_fail:
        plt.plot(df["t"], df["v"], color="blue", alpha=0.6, label="AP Failures")

plt.xlabel("Simulation Time (s)")
plt.ylabel("Number of Failed Nodes")
plt.title("Infrastructure Failures During Earthquake")

plt.grid(True)

# remove duplicate legend labels
handles, labels = plt.gca().get_legend_handles_labels()
unique = dict(zip(labels, handles))
plt.legend(unique.values(), unique.keys())

plt.tight_layout()
plt.savefig(OUTPUT)

print("Graph saved to:", OUTPUT)

plt.show()