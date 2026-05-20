import numpy as np
import matplotlib.pyplot as plt

# ------------------------------------
# Earthquake parameters (same as your simulation)
# ------------------------------------

origin_time = 12.8215        # earthquake start
vP = 6500              # P-wave velocity (m/s)
vS = 3600              # S-wave velocity (m/s)
vSurf = 3200           # Surface-wave velocity (m/s)

# simulation time
t = np.linspace(0,40,500)

# distance travelled by waves
p_wave = vP*(t-origin_time)
s_wave = vS*(t-origin_time)
surf_wave = vSurf*(t-origin_time)

# negative distances not valid
p_wave[p_wave<0] = np.nan
s_wave[s_wave<0] = np.nan
surf_wave[surf_wave<0] = np.nan

# ------------------------------------
# Plot
# ------------------------------------

plt.figure(figsize=(10,6))

plt.plot(t,p_wave,label="P-Wave (Fast Detection)")
plt.plot(t,s_wave,label="S-Wave (Structural Damage)")
plt.plot(t,surf_wave,label="Surface Wave (Major Damage)")

plt.axvline(origin_time,linestyle="--",label="Earthquake Origin")

plt.title("Earthquake Wave Propagation Model")
plt.xlabel("Time (seconds)")
plt.ylabel("Distance from Epicenter (meters)")

plt.legend()
plt.grid(True)

plt.savefig("wave_propagation_model.png",dpi=300)

plt.show()