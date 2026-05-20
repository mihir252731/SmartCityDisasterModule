package inet.common.earthquake;

simple DisasterFailure
{
    parameters:
        // What to fail
        string iotPattern = default("**.iot[*]");
        string apPattern  = default("**.ap[*]");
        int    wlanIndex  = default(0);
        bool   deleteNodes = default(false); // false = turn radio OFF, true = delete modules
		
		int iotCountOnSWave    = default(-1);   // if >=0, fail exactly this many IoTs at S-wave
        int apCountOnSWave     = default(-1);   // if >=0, fail exactly this many APs  at S-wave
        int iotCountOnSurfWave = default(-1);   // if >=0, fail exactly this many IoTs at SURF-wave
        int apCountOnSurfWave  = default(-1);   // if >=0, fail exactly this many APs  at SURF-wave
		
        // === TIMER MODE (no epicenter needed) ===
        bool   useEpicenterTrigger = default(false);
        string failAtTimes = default("");    // e.g. "13.21s, 20s, 45s"
        double lambdaOnTimer = default(0);   // Poisson mean for total failures at each time

        // === SIGNAL MODE (optional) ===
        string epicenterPath = default("^.epicenter");
        string triggerNames  = default("S-wave,SURF-wave"); // packet names when using SeismicWaveEmitter
        double lambdaPWave   = default(0);
        double lambdaSWave   = default(0);
        
        // === NEW: deterministic failure counts per wave (optional) ===
        int countOnSWave     = default(-1);    // if >=0, fail exactly this many on S-wave
        int countOnSurfWave  = default(-1);    // if >=0, fail exactly this many on SURF-wave
        double lambdaSurfWave = default(0);    // optional Poisson mean for SURF-wave if countOnSurfWave < 0
        
                // ---- research signals for failures
    	@signal[failedIots](type=long);
    	@signal[failedAps](type=long);

    	// ---- vectors for failure counts over time
    	@statistic[failedIots](source=failedIots; record=vector,count; interpolationmode=none);
    	@statistic[failedAps](source=failedAps;   record=vector,count; interpolationmode=none);

}