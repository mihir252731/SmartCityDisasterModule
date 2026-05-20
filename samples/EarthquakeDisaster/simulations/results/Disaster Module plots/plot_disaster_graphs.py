import os
import glob
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

RESULT_DIR = r"D:\omnetpp-5.7.1\samples\EarthquakeDisaster\simulations\results"
OUT = os.path.join(RESULT_DIR,"paper_figures")

os.makedirs(OUT,exist_ok=True)

plt.rcParams.update({
    "figure.figsize":(8,5),
    "figure.dpi":300,
    "font.size":12
})


###############################################
# VECTOR FILE PARSER
###############################################

def parse_vec(path):

    vectors={}
    idmap={}

    with open(path,"r",errors="ignore") as f:

        for line in f:

            if line.startswith("vector"):
                p=line.split()

                vid=int(p[1])
                module=p[2]
                name=p[3]

                key=f"{module}.{name}".replace(":vector","")

                idmap[vid]=key

                if key not in vectors:
                    vectors[key]={"t":[],"v":[]}

            elif line and line[0].isdigit():

                p=line.split()

                if len(p)<3:
                    continue

                try:
                    vid=int(p[0])
                    t=float(p[1])
                    v=float(p[2])
                except:
                    continue

                key=idmap.get(vid)

                if key:
                    vectors[key]["t"].append(t)
                    vectors[key]["v"].append(v)

    for k in vectors:
        vectors[k]=pd.DataFrame(vectors[k])

    return vectors


###############################################
# LOAD ALL RUNS
###############################################

runs=[]

vecfiles=sorted(glob.glob(os.path.join(RESULT_DIR,"*.vec")))

for vf in vecfiles:

    print("Reading",os.path.basename(vf))

    runs.append(parse_vec(vf))


###############################################
# GRAPH 1 — WAVE PROPAGATION
###############################################

plt.figure()

for r in runs:

    for k in r:
        if "waveSentTime" in k:
            df=r[k]
            plt.scatter(df.t,df.v,s=8)

plt.xlabel("Simulation Time (s)")
plt.ylabel("Wave Sent Time")
plt.title("Seismic Wave Propagation Timeline")

plt.grid()

plt.savefig(os.path.join(OUT,"wave_propagation.png"))
plt.close()


###############################################
# GRAPH 2 — FAILURES VS TIME
###############################################

plt.figure()

for r in runs:

    for k in r:

        if "failedIots" in k:
            plt.plot(r[k].t,r[k].v,label="IoT Failures")

        if "failedAps" in k:
            plt.plot(r[k].t,r[k].v,label="AP Failures")

plt.xlabel("Time (s)")
plt.ylabel("Failed Nodes")
plt.title("Infrastructure Failures During Disaster")

plt.legend()
plt.grid()

plt.savefig(os.path.join(OUT,"failures_timeline.png"))
plt.close()


###############################################
# GRAPH 3 — QUEUE DEPTH
###############################################

plt.figure()

for r in runs:

    for k in r:

        if "queueDepth." in k:

            df=r[k]

            plt.plot(df.t,df.v)

plt.xlabel("Time (s)")
plt.ylabel("Queue Depth")
plt.title("Edge Queue Depth During Disaster")

plt.grid()

plt.savefig(os.path.join(OUT,"queue_depth_total.png"))
plt.close()


###############################################
# GRAPH 4 — PER SLICE QUEUE
###############################################

plt.figure()

for r in runs:

    for k in r:

        if "queueDepthURLLC" in k:
            plt.plot(r[k].t,r[k].v,label="URLLC")

        if "queueDepthEMBB" in k:
            plt.plot(r[k].t,r[k].v,label="eMBB")

        if "queueDepthMMTC" in k:
            plt.plot(r[k].t,r[k].v,label="mMTC")

plt.xlabel("Time (s)")
plt.ylabel("Queue Depth")
plt.title("Queue Depth Per Slice")

plt.legend()
plt.grid()

plt.savefig(os.path.join(OUT,"queue_per_slice.png"))
plt.close()


###############################################
# GRAPH 5 — QUEUE DELAY PER SLICE
###############################################

plt.figure()

for r in runs:

    for k in r:

        if "queueingDelayURLLC" in k:
            plt.scatter(r[k].t,r[k].v,s=8,label="URLLC")

        if "queueingDelayEMBB" in k:
            plt.scatter(r[k].t,r[k].v,s=8,label="eMBB")

        if "queueingDelayMMTC" in k:
            plt.scatter(r[k].t,r[k].v,s=8,label="mMTC")

plt.xlabel("Time (s)")
plt.ylabel("Queue Delay (s)")
plt.title("Queue Delay Per Traffic Class")

plt.legend()
plt.grid()

plt.savefig(os.path.join(OUT,"queue_delay.png"))
plt.close()


###############################################
# GRAPH 6 — ALERT TIMELINESS
###############################################

plt.figure()

for r in runs:

    for k in r:

        if "detectToFirstAlert" in k:
            plt.scatter(r[k].t,r[k].v,s=10)

plt.xlabel("Simulation Time (s)")
plt.ylabel("Alert Latency (s)")
plt.title("Alert Timeliness")

plt.grid()

plt.savefig(os.path.join(OUT,"alert_timeliness.png"))
plt.close()


###############################################
# GRAPH 7 — ALERT DISSEMINATION
###############################################

plt.figure()

for r in runs:

    for k in r:

        if "alertTx" in k:
            plt.plot(r[k].t,r[k].v)

plt.xlabel("Time (s)")
plt.ylabel("Alerts Sent")
plt.title("Alert Dissemination Over Time")

plt.grid()

plt.savefig(os.path.join(OUT,"alert_dissemination.png"))
plt.close()


###############################################
# GRAPH 8 — EDGE ENERGY
###############################################

plt.figure()

for r in runs:

    for k in r:

        if "edgeEnergyJ" in k:
            plt.plot(r[k].t,r[k].v)

plt.xlabel("Time (s)")
plt.ylabel("Energy (J)")
plt.title("Edge Energy Consumption")

plt.grid()

plt.savefig(os.path.join(OUT,"edge_energy.png"))
plt.close()


###############################################
# GRAPH 9 — DELIVERY RATIO
###############################################

failures=[]
pdr=[]

for r in runs:

    tx=0
    rx=0
    fail=0

    for k in r:

        if "alertTx" in k:
            tx+=len(r[k])

        if "detectRx" in k:
            rx+=len(r[k])

        if "failedIots" in k or "failedAps" in k:
            fail=max(fail,max(r[k].v))

    if tx>0:
        failures.append(fail)
        pdr.append(rx/tx)

plt.figure()

plt.plot(failures,pdr,"o-")

plt.xlabel("Failed Nodes")
plt.ylabel("Packet Delivery Ratio")
plt.title("Reliability vs Infrastructure Failures")

plt.grid()

plt.savefig(os.path.join(OUT,"pdr_vs_failures.png"))
plt.close()


###############################################
# GRAPH 10 — LATENCY VS IOT
###############################################

iot=[100,200,300,400,500]

lat=[]

for r in runs:

    vals=[]

    for k in r:
        if "detectToFirstAlert" in k:
            vals+=list(r[k].v)

    if vals:
        lat.append(np.mean(vals))

plt.figure()

plt.plot(iot[:len(lat)],lat,"o-")

plt.xlabel("Number of IoT Devices")
plt.ylabel("Average Alert Latency (s)")
plt.title("Latency vs IoT Scale")

plt.grid()

plt.savefig(os.path.join(OUT,"latency_vs_iot.png"))
plt.close()


print("All graphs generated in:",OUT)