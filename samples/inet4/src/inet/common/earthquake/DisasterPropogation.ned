package inet.common.earthquake;

import inet.applications.contract.IApp;

//**
 //* Emits three seismic phases (P, S, Surface) as UDP packets, scheduled so that
 //* each IoT receives them at the physically correct arrival time based on
 //* epicenter location and wave speeds.
 //*
 //* Works with INET 4 / OMNeT++ 5.7. Uses unicast UDP to each iot[*].
 //*/
simple DisasterPropogation like IApp
{
    parameters:
        @class(DisasterPropogation);
        string  vectorName = default("iot");     // submodule vector holding IoTs
        double  originTime @unit(s); // quake origin (simulation time)
        double  epicX @unit(m) = default(0m);
        double  epicY @unit(m) = default(0m);

        double  vP    @unit(mps) = default(6500 mps);  // P-wave speed
        double  vS    @unit(mps) = default(3600 mps);  // S-wave speed
        double  vSurf @unit(mps) = default(3200 mps);  // Surface (Rayleigh) speed

        int     port = default(7001);               // UDP destination port at IoTs
        int     messageLength @unit(B) = default(64B);       // payload size
        string  pName = default("P-wave");
        string  sName = default("S-wave");
        string  surfName = default("SURF-wave");
        bool verbose = default(false);

        @display("i=block/waves;is=l");
        
        // ---- research signals/statistics for emitter timelines ----
        @signal[waveSent](type=inet::Packet);        // emit at actual send time (s)
        @signal[waveSentTime](type=double);       // numeric time for plotting
        @signal[originTime](type=double);      // emit once so analyses can align

        @statistic[waveSentTime](title="Seismic wave sent time"; unit=s;
                              source=waveSentTime; record=vector,count; interpolationmode=none);
        @statistic[originTime](title="Seismic origin time"; unit=s;
                               source=originTime; record=last);

        
    gates:
        input  socketIn;
        output socketOut;
}
