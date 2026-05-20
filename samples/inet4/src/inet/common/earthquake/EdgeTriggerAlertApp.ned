package inet.common.earthquake;

import inet.applications.contract.IApp;

//import inet.applications.base.ApplicationBase;

//
// EdgeTriggerAlertApp
// - Listens for UDP packets on listenPort (DETECT + other traffic classes)
// - Enqueues "jobs" into per-class queues (URLLC, eMBB, mMTC)
// - Schedules service based on slicing scenario (static/dynamic/preempt)
// - Sends response as IPv4 multicast (224.0.0.1 by default)
// - Records offered/admitted/served/drop, queue depth per class, queuing delay, etc.
//
simple EdgeTriggerAlertApp like IApp
{
    parameters:
        string  vectorName    = default("iot");     // kept (compat)
        int     listenPort    = default(5000);
        int     alertPort     = default(6000);
        string  packetName    = default("ALERT");
        int     messageLength @unit(B) = default(200B);

        double  sendInterval  @unit(s) = default(50us);

        // edge compute
        double procDelay   @unit(s) = default(2us);
        double computeUnits          = default(1e8);   // ops/s
        double workPerEvent          = default(5e6);   // ops/job (default)

        // multicast destination
        string groupAddr = default("224.0.0.1");
        int ttl = default(1);

        // --------------------------------------------------------------------
        // BACKWARD COMPAT SLICING PARAMS (your old 2-class knobs)
        int tosEmergency  = default(184);
        int tosBestEffort = default(0);
        bool classifyDetectByName = default(true);

        int queueCapacity = default(100000);
        string dropPolicy = default("dropTail");   // dropTail; preempt can enable pushOut
        string scheduler  = default("strict");     // legacy: strict/wfq
        int weightEmergency  = default(10);
        int weightBestEffort = default(1);

        // --------------------------------------------------------------------
        // NEW: 3-class network slicing knobs (Q4)
        //
        // slicingScenario:
        //   "none"               -> simple single-queue behavior
        //   "staticNoBorrow"     -> strict allocation, no borrowing (Scenario 1a)
        //   "dynamicBorrow"      -> borrowing enabled (Scenario 1b)
        //   "preempt"            -> pre-emption enabled (Scenario 2)
        //
        string slicingScenario = default("dynamicBorrow");

        // class shares (must sum to 100 ideally; enforced in code)
        double shareEMBB  = default(75);   // %
        double shareURLLC = default(15);   // %
        double shareMMTC  = default(10);   // %

        // Pre-emption definitions (Scenario 2)
        bool preemptService   = default(true);   // URLLC can jump ahead in scheduling
        bool preemptBuffer    = default(false);  // push-out buffer space for URLLC
        bool preemptAdmission = default(false);  // reject lower class when URLLC arrives

        // per-class queue caps (optional; <=0 means "unlimited, only total cap applies")
        int queueCapURLLC = default(-1);
        int queueCapEMBB  = default(-1);
        int queueCapMMTC  = default(-1);

        // per-class workload (optional; <=0 uses workPerEvent)
        double workURLLC = default(-1);
        double workEMBB  = default(-1);
        double workMMTC  = default(-1);

        // per-class response naming (helps offline parsing)
        string respNameURLLC = default("ALERT_URLLC");
        string respNameEMBB  = default("ALERT_EMBB");
        string respNameMMTC  = default("ALERT_MMTC");

        // Energy model (kept)
        double idlePowerW @unit(W) = default(5W);
        double joulesPerOp @unit(J)= default(1e-9J);

        int rounds = default(1); // kept

        @display("i=block/app");

        // -------------------- SIGNALS --------------------
        // Offered/admitted/served/drop per class (Q3/Q4)
        @signal[offeredURLLC](type=long);
        @signal[offeredEMBB](type=long);
        @signal[offeredMMTC](type=long);

        @signal[admittedURLLC](type=long);
        @signal[admittedEMBB](type=long);
        @signal[admittedMMTC](type=long);

        @signal[servedURLLC](type=long);
        @signal[servedEMBB](type=long);
        @signal[servedMMTC](type=long);

        @signal[droppedURLLC](type=long);
        @signal[droppedEMBB](type=long);
        @signal[droppedMMTC](type=long);

        // Queue depths
        @signal[queueDepth](type=long);
        @signal[queueDepthURLLC](type=long);
        @signal[queueDepthEMBB](type=long);
        @signal[queueDepthMMTC](type=long);

        // Delay metrics
        @signal[queueingDelayURLLC](type=double);
        @signal[queueingDelayEMBB](type=double);
        @signal[queueingDelayMMTC](type=double);

        // Existing research signals you already used
        @signal[detectRx](type=long);
        @signal[alertQueued](type=long);
        @signal[alertDropped](type=long);
        @signal[alertTx](type=long);
        @signal[alertSeq](type=long);
        @signal[detectLatency](type=double);
        @signal[edgeProcTime](type=double);
        @signal[detectToFirstAlert](type=double);
        @signal[edgeEnergyJ](type=double);

        // -------------------- STATISTICS --------------------
        @statistic[offeredURLLC](source=offeredURLLC; record=count,vector; interpolationmode=none);
        @statistic[offeredEMBB](source=offeredEMBB; record=count,vector; interpolationmode=none);
        @statistic[offeredMMTC](source=offeredMMTC; record=count,vector; interpolationmode=none);

        @statistic[admittedURLLC](source=admittedURLLC; record=count,vector; interpolationmode=none);
        @statistic[admittedEMBB](source=admittedEMBB; record=count,vector; interpolationmode=none);
        @statistic[admittedMMTC](source=admittedMMTC; record=count,vector; interpolationmode=none);

        @statistic[servedURLLC](source=servedURLLC; record=count,vector; interpolationmode=none);
        @statistic[servedEMBB](source=servedEMBB; record=count,vector; interpolationmode=none);
        @statistic[servedMMTC](source=servedMMTC; record=count,vector; interpolationmode=none);

        @statistic[droppedURLLC](source=droppedURLLC; record=count,vector; interpolationmode=none);
        @statistic[droppedEMBB](source=droppedEMBB; record=count,vector; interpolationmode=none);
        @statistic[droppedMMTC](source=droppedMMTC; record=count,vector; interpolationmode=none);

        @statistic[queueDepth](source=queueDepth; record=vector,mean,min,max; interpolationmode=none);
        @statistic[queueDepthURLLC](source=queueDepthURLLC; record=vector,mean,min,max; interpolationmode=none);
        @statistic[queueDepthEMBB](source=queueDepthEMBB; record=vector,mean,min,max; interpolationmode=none);
        @statistic[queueDepthMMTC](source=queueDepthMMTC; record=vector,mean,min,max; interpolationmode=none);

        @statistic[queueingDelayURLLC](source=queueingDelayURLLC; unit=s; record=vector,mean,min,max,histogram; bins=30; interpolationmode=none);
        @statistic[queueingDelayEMBB](source=queueingDelayEMBB; unit=s; record=vector,mean,min,max,histogram; bins=30; interpolationmode=none);
        @statistic[queueingDelayMMTC](source=queueingDelayMMTC; unit=s; record=vector,mean,min,max,histogram; bins=30; interpolationmode=none);

        // existing stats (kept)
        @statistic[detectRx](source=detectRx; record=count,vector; interpolationmode=none);
        @statistic[alertQueued](source=alertQueued; record=count,vector; interpolationmode=none);
        @statistic[alertDropped](source=alertDropped; record=count,vector; interpolationmode=none);
        @statistic[alertTx](source=alertTx; record=count,vector; interpolationmode=none);
        @statistic[alertSeq](source=alertSeq; record=vector; interpolationmode=none);
        @statistic[detectLatency](source=detectLatency; unit=s; record=vector,mean,min,max,histogram; bins=20; interpolationmode=none);
        @statistic[edgeProcTime](source=edgeProcTime; unit=s; record=vector,mean; interpolationmode=none);
        @statistic[detectToFirstAlert](source=detectToFirstAlert; unit=s; record=vector,mean; interpolationmode=none);
        @statistic[edgeEnergyJ](source=edgeEnergyJ; unit=J; record=vector,mean,sum; interpolationmode=none);

    gates:
        input  socketIn;
        output socketOut;
}
