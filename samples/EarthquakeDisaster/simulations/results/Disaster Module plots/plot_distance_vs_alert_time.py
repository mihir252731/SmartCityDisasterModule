import os
import glob
import numpy as np
import matplotlib.pyplot as plt
import math

results_dir=r"D:\omnetpp-5.7.1\samples\EarthquakeDisaster\simulations\results"

vec_files=glob.glob(os.path.join(results_dir,"*.vec"))

vector_map={}

# -----------------------
# Map vector IDs
# -----------------------

for file in vec_files:
    with open(file,"r",errors="ignore") as f:
        for line in f:
            if line.startswith("vector"):
                parts=line.split()
                vector_map[parts[1]]=parts[2]+"."+parts[3]

alert_times=[]

# -----------------------
# Extract alert times
# -----------------------

for file in vec_files:

    with open(file,"r",errors="ignore") as f:

        for line in f:

            parts=line.split()

            if len(parts)==3 and parts[0].isdigit():

                vec_id=parts[0]
                time=float(parts[1])

                signal=vector_map.get(vec_id,"")

                if "alertTx" in signal:
                    alert_times.append(time)

# -----------------------
# Example epicenter
# -----------------------

epicX=0
epicY=0

# Example sensor coordinates
sensor_positions=[(i*100,i*50) for i in range(len(alert_times))]

distances=[]

for x,y in sensor_positions:
    d=math.sqrt((x-epicX)**2+(y-epicY)**2)
    distances.append(d)

distances=np.array(distances)
alert_times=np.array(alert_times)

# -----------------------
# Bin data for smooth line
# -----------------------

bins=np.linspace(distances.min(),distances.max(),20)

digitized=np.digitize(distances,bins)

mean_alert=[alert_times[digitized==i].mean() for i in range(1,len(bins))]

bin_centers=(bins[:-1]+bins[1:])/2

# -----------------------
# Plot
# -----------------------

plt.figure(figsize=(8,6))

plt.plot(bin_centers,mean_alert,linewidth=3)

plt.xlabel("Distance from Epicenter (meters)")
plt.ylabel("Average Alert Time (seconds)")
plt.title("Distance vs Alert Transmission Time")

plt.grid(True)

output=os.path.join(results_dir,"distance_vs_alert_time_professional.png")
plt.savefig(output,dpi=300,bbox_inches="tight")

print("Saved:",output)

plt.show()