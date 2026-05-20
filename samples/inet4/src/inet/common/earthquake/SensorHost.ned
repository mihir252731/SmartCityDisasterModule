package inet.common.earthquake;

import inet.node.inet.WirelessHost;

//
// Wireless IoT sensor host with per-node coordinates.
// Adds per-node "bringupTime" to stagger WiFi association/startup.
//
module SensorHost extends WirelessHost
{
    parameters:

        
        udp.joinLocalMulticastGroups = default(true);
        
        // Position (meters); also used by StationaryMobility
        double myX @unit(m) = default(-1m);
        double myY @unit(m) = default(-1m);

        mobility.typename = default("StationaryMobility");
        mobility.initFromDisplayString = default(false);
        mobility.initialX = default(myX);
        mobility.initialY = default(myY);
        mobility.initialZ = default(0m);

        // -------------------- Bringup batching knobs --------------------
        // We delay the STA "agent" start to avoid association storm.
        bool   bringupEnabled = default(true);
        double bringupTime @unit(s) = default(0s);

        // Optional: extra jitter on top of bringupTime (can be set from .ini)
        double bringupJitter @unit(s) = default(0s);

        // computed effective bringup time
        double bringupEffective @unit(s) = default(bringupTime + bringupJitter);

        // If true, only delay association; do not change radio mode
        bool delayAssociationOnly = default(true);

        // --- Quake parameters ---
        double eqOrigin @unit(s) = default(0s);
        double epicX   @unit(m)  = default(0m);
        double epicY   @unit(m)  = default(0m);
        double pVel    @unit(mps)= default(6500mps);
        double sVel    @unit(mps)= default(3500mps);
        double rVel    @unit(mps)= default(3000mps);
        double preLead @unit(s)  = default(0s);

        // --- Helpers to keep unit algebra stable ---
        double dx_u = default((myX - epicX) / 1m);
        double dy_u = default((myY - epicY) / 1m);
        double dist @unit(m) = default(sqrt(dx_u*dx_u + dy_u*dy_u) * 1m);

        double vP_u = default(pVel / 1mps);
        double vS_u = default(sVel / 1mps);
        double vR_u = default(rVel / 1mps);

        double tP @unit(s) = default(eqOrigin + ((dist / 1m) / vP_u) * 1s);
        double tS @unit(s) = default(eqOrigin + ((dist / 1m) / vS_u) * 1s);
        double tD @unit(s) = default(eqOrigin + ((dist / 1m) / vR_u) * 1s);

        double detectTime @unit(s) = default(max(0s, tP - preLead));
        double damageTime @unit(s) = default(tD);
        
                // -------------------- Traffic enable probabilities --------------------
        // These make % gating scale automatically with numIoT
        double embbEnableProb = default(0.01);   // only 1% generate eMBB
        double mmtcEnableProb = default(0.10);   // 10% generate mMTC (90% disabled)

        bool enableEMBB = default(uniform(0,1) < embbEnableProb);
        bool enableMMTC = default(uniform(0,1) < mmtcEnableProb);

        @display("i=device/wifilaptop");

        // -------------------- IMPORTANT PART --------------------
        // Delay WiFi STA agent start => avoids handshake storm.
        //
        // NOTE: agent exists for Ieee80211Interface (your .ini sets typename = "Ieee80211Interface")
        // This does NOT break anything else: it only shifts when association begins.
        //
        wlan[0].agent.startTime = default( bringupEnabled ? bringupEffective : 0s );

        // If your INET version has mgmt.startTime, you can also uncomment:
        // wlan[0].mgmt.startTime = default( bringupEnabled ? bringupEffective : 0s );
}