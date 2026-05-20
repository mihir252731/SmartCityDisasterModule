package inet.common.earthquake;

import inet.node.inet.StandardHost;
import inet.node.wireless.AccessPoint;
import inet.node.inet.WirelessHost;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.ethernet.EtherSwitch;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;

import inet.common.earthquake.SensorHost;
import inet.common.earthquake.DisasterFailure;

network SmartCity
{
    parameters:
        int numIoT = default(100);
        int numAp = default(2);

        // Random placement bounds (used in .ini to assign myX/myY)
        double areaXMin @unit(m) = default(0m);
        double areaXMax @unit(m) = default(1500m);
        double areaYMin @unit(m) = default(0m);
        double areaYMax @unit(m) = default(900m);

        double eqOrigin @unit(s) = default(12.7s);
        double epicX @unit(m) = default(-80000m);
        double epicY @unit(m) = default(-30000m);
        double pVel @unit(mps) = default(6500mps);
        double rVel @unit(mps) = default(3000mps);

        @display("bgb=1500,900");

    submodules:
        medium: Ieee80211ScalarRadioMedium {
            @display("p=60,60");
        }

        configurator: Ipv4NetworkConfigurator {
            @display("p=1400,60");
        }

        ap[numAp]: AccessPoint {
            @display("i=device/accesspoint;p=420,380;r=750,#FFFF80,green,2");
        }

        core: EtherSwitch {
            @display("i=device/switch;p=700,380");
        }

        fog: StandardHost {
            parameters:
                numEthInterfaces = 2;
                @display("i=device/server;p=1096,380");
        }

        epicenter: WirelessHost {
            parameters:
                @display("i=misc/signal_departure;p=200,120;is=vl");
                numWlanInterfaces = 1;
                mobility.typename = "StationaryMobility";
                mobility.initialX = 200m;
                mobility.initialY = 120m;
                mobility.initialZ = 0m;
        }

        iot[numIoT]: SensorHost {
            parameters:
                numWlanInterfaces = 1;
                @display("i=device/wifilaptop;p=199.28,387.28");
        }

        quake: DisasterFailure {
            parameters:
                epicenterPath = "^.epicenter";
                @display("p=372,36");
        }

        edge0: StandardHost {
            parameters:
                numEthInterfaces = 1;
                @display("i=device/pc;p=784,199");
        }
        edge1: StandardHost {
            parameters:
                numEthInterfaces = 1;
                @display("i=device/pc;p=784,573");
        }

    connections allowunconnected:
        for a=0..numAp-1 {
            ap[a].ethg++ <--> {  delay = 0.004ms; datarate = 10Gbps; } <--> core.ethg++;
        }
        edge0.ethg++ <--> {  delay = 0.004ms; datarate = 10Gbps; } <--> core.ethg++;
        edge1.ethg++ <--> {  delay = 0.004ms; datarate = 10Gbps; } <--> core.ethg++;
        fog.ethg++ <--> {  delay = 0.003ms; datarate = 10Gbps; } <--> core.ethg++;
}
