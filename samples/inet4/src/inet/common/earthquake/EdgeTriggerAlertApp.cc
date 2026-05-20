#include "inet/common/earthquake/EdgeTriggerAlertApp.h"

#include "inet/common/TimeTag_m.h"                // CreationTimeTag
#include "inet/common/packet/chunk/BytesChunk.h"  // BytesChunk
#include "inet/networklayer/common/L3AddressResolver.h"

namespace inet {

Define_Module(EdgeTriggerAlertApp);

// ---- signal registration ----
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigOfferedURLLC  = omnetpp::cComponent::registerSignal("offeredURLLC");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigOfferedEMBB   = omnetpp::cComponent::registerSignal("offeredEMBB");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigOfferedMMTC   = omnetpp::cComponent::registerSignal("offeredMMTC");

EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigAdmittedURLLC = omnetpp::cComponent::registerSignal("admittedURLLC");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigAdmittedEMBB  = omnetpp::cComponent::registerSignal("admittedEMBB");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigAdmittedMMTC  = omnetpp::cComponent::registerSignal("admittedMMTC");

EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigServedURLLC   = omnetpp::cComponent::registerSignal("servedURLLC");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigServedEMBB    = omnetpp::cComponent::registerSignal("servedEMBB");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigServedMMTC    = omnetpp::cComponent::registerSignal("servedMMTC");

EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigDroppedURLLC  = omnetpp::cComponent::registerSignal("droppedURLLC");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigDroppedEMBB   = omnetpp::cComponent::registerSignal("droppedEMBB");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigDroppedMMTC   = omnetpp::cComponent::registerSignal("droppedMMTC");

EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigQueueDepth      = omnetpp::cComponent::registerSignal("queueDepth");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigQueueDepthURLLC = omnetpp::cComponent::registerSignal("queueDepthURLLC");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigQueueDepthEMBB  = omnetpp::cComponent::registerSignal("queueDepthEMBB");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigQueueDepthMMTC  = omnetpp::cComponent::registerSignal("queueDepthMMTC");

EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigQueueingDelayURLLC = omnetpp::cComponent::registerSignal("queueingDelayURLLC");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigQueueingDelayEMBB  = omnetpp::cComponent::registerSignal("queueingDelayEMBB");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigQueueingDelayMMTC  = omnetpp::cComponent::registerSignal("queueingDelayMMTC");

// legacy
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigDetectRx           = omnetpp::cComponent::registerSignal("detectRx");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigAlertQueued        = omnetpp::cComponent::registerSignal("alertQueued");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigAlertDropped       = omnetpp::cComponent::registerSignal("alertDropped");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigAlertTx            = omnetpp::cComponent::registerSignal("alertTx");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigAlertSeq           = omnetpp::cComponent::registerSignal("alertSeq");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigDetectLatency      = omnetpp::cComponent::registerSignal("detectLatency");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigEdgeProcTime       = omnetpp::cComponent::registerSignal("edgeProcTime");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigDetectToFirstAlert = omnetpp::cComponent::registerSignal("detectToFirstAlert");
EdgeTriggerAlertApp::SignalId EdgeTriggerAlertApp::sigEdgeEnergyJ        = omnetpp::cComponent::registerSignal("edgeEnergyJ");

EdgeTriggerAlertApp::~EdgeTriggerAlertApp()
{
    cancelAndDelete(serviceTimer);
    cancelAndDelete(paceTimer);
    serviceTimer = nullptr;
    paceTimer = nullptr;
}

void EdgeTriggerAlertApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        vectorName = par("vectorName").stdstringValue();
        listenPort = par("listenPort");
        alertPort  = par("alertPort");
        packetName = par("packetName").stdstringValue();
        messageLengthB = par("messageLength");

        sendInterval = par("sendInterval");

        procDelay = par("procDelay");
        computeUnits = par("computeUnits").doubleValue();
        workPerEvent = par("workPerEvent").doubleValue();

        groupAddrStr = par("groupAddr").stdstringValue();
        ttl = par("ttl");

        tosEmergency = par("tosEmergency");
        tosBestEffort = par("tosBestEffort");
        classifyDetectByName = par("classifyDetectByName");

        queueCapacity = par("queueCapacity");
        dropPolicy = par("dropPolicy").stdstringValue();
        scheduler  = par("scheduler").stdstringValue();
        weightEmergency = par("weightEmergency");
        weightBestEffort = par("weightBestEffort");

        slicingScenario = par("slicingScenario").stdstringValue();
        shareEMBB  = par("shareEMBB").doubleValue();
        shareURLLC = par("shareURLLC").doubleValue();
        shareMMTC  = par("shareMMTC").doubleValue();

        preemptService   = par("preemptService");
        preemptBuffer    = par("preemptBuffer");
        preemptAdmission = par("preemptAdmission");

        queueCapURLLC = par("queueCapURLLC");
        queueCapEMBB  = par("queueCapEMBB");
        queueCapMMTC  = par("queueCapMMTC");

        workURLLC = par("workURLLC").doubleValue();
        workEMBB  = par("workEMBB").doubleValue();
        workMMTC  = par("workMMTC").doubleValue();

        respNameURLLC = par("respNameURLLC").stdstringValue();
        respNameEMBB  = par("respNameEMBB").stdstringValue();
        respNameMMTC  = par("respNameMMTC").stdstringValue();

        idlePowerW = par("idlePowerW").doubleValue();
        joulesPerOp = par("joulesPerOp").doubleValue();

        rounds = par("rounds");

        serviceTimer = new omnetpp::cMessage("serviceTimer");
        paceTimer    = new omnetpp::cMessage("paceTimer");

        busy = false;
        haveCurrent = false;
        globalSeq = 0;

        haveFirstDetect = false;
        firstDetectTime = SIMTIME_ZERO;
        firstAlertReported = false;

        rrIndex = 0;
        deficitURLLC = deficitEMBB = deficitMMTC = 0;

        // resolve multicast once
        L3AddressResolver resolver;
        groupAddr = resolver.resolve(groupAddrStr.c_str());

        updateQueueSignals();
    }

    if (stage == INITSTAGE_APPLICATION_LAYER) {
        socket.setOutputGate(gate("socketOut"));
        socket.setCallback(this);
        socket.bind(listenPort);
        socket.setBroadcast(true);
        socket.setTimeToLive(ttl);
    }
}

void EdgeTriggerAlertApp::handleStartOperation(LifecycleOperation *operation)
{
    // app starts "up"; no extra scheduling needed now
    (void)operation;
    startServiceIfIdle();
}

void EdgeTriggerAlertApp::handleStopOperation(LifecycleOperation *operation)
{
    (void)operation;
    cancelEvent(serviceTimer);
    cancelEvent(paceTimer);
    busy = false;
    haveCurrent = false;
}

void EdgeTriggerAlertApp::handleCrashOperation(LifecycleOperation *operation)
{
    (void)operation;
    cancelEvent(serviceTimer);
    cancelEvent(paceTimer);
    busy = false;
    haveCurrent = false;
}

void EdgeTriggerAlertApp::handleMessageWhenUp(omnetpp::cMessage *msg)
{
    if (msg == serviceTimer) {
        serviceComplete();
    }
    else if (msg == paceTimer) {
        // pacing gap elapsed; start next service
        startServiceIfIdle();
    }
    else {
        socket.processMessage(msg);
    }
}

void EdgeTriggerAlertApp::finish()
{
    ApplicationBase::finish();
}

// -------------------- queue helpers --------------------

int EdgeTriggerAlertApp::totalQueueSize() const
{
    return (int)(qURLLC.size() + qEMBB.size() + qMMTC.size());
}

int EdgeTriggerAlertApp::classQueueSize(TrafficClass tc) const
{
    if (tc == CLASS_URLLC) return (int)qURLLC.size();
    if (tc == CLASS_EMBB)  return (int)qEMBB.size();
    return (int)qMMTC.size();
}

int EdgeTriggerAlertApp::classQueueCap(TrafficClass tc) const
{
    if (tc == CLASS_URLLC) return queueCapURLLC;
    if (tc == CLASS_EMBB)  return queueCapEMBB;
    return queueCapMMTC;
}

void EdgeTriggerAlertApp::updateQueueSignals() const
{
    emitLong(sigQueueDepth,      (long)totalQueueSize());
    emitLong(sigQueueDepthURLLC, (long)qURLLC.size());
    emitLong(sigQueueDepthEMBB,  (long)qEMBB.size());
    emitLong(sigQueueDepthMMTC,  (long)qMMTC.size());
}

// -------------------- counters --------------------

void EdgeTriggerAlertApp::noteOffered(TrafficClass tc)
{
    if (tc == CLASS_URLLC) emitLong(sigOfferedURLLC, 1);
    else if (tc == CLASS_EMBB) emitLong(sigOfferedEMBB, 1);
    else emitLong(sigOfferedMMTC, 1);
}

void EdgeTriggerAlertApp::noteAdmitted(TrafficClass tc)
{
    if (tc == CLASS_URLLC) emitLong(sigAdmittedURLLC, 1);
    else if (tc == CLASS_EMBB) emitLong(sigAdmittedEMBB, 1);
    else emitLong(sigAdmittedMMTC, 1);
}

void EdgeTriggerAlertApp::noteServed(TrafficClass tc)
{
    if (tc == CLASS_URLLC) emitLong(sigServedURLLC, 1);
    else if (tc == CLASS_EMBB) emitLong(sigServedEMBB, 1);
    else emitLong(sigServedMMTC, 1);
}

void EdgeTriggerAlertApp::noteDropped(TrafficClass tc)
{
    if (tc == CLASS_URLLC) emitLong(sigDroppedURLLC, 1);
    else if (tc == CLASS_EMBB) emitLong(sigDroppedEMBB, 1);
    else emitLong(sigDroppedMMTC, 1);
}

// -------------------- classification --------------------

EdgeTriggerAlertApp::TrafficClass EdgeTriggerAlertApp::classifyPacket(Packet *pk) const
{
    std::string n = pk->getName() ? pk->getName() : "";
    n = toUpper(n);

    if (n.find("URLLC") != std::string::npos) return CLASS_URLLC;
    if (n.find("EMBB")  != std::string::npos) return CLASS_EMBB;
    if (n.find("MMTC")  != std::string::npos) return CLASS_MMTC;

    if (n.find("DETECT") != std::string::npos) return CLASS_URLLC;

    if (classifyDetectByName)
        return CLASS_URLLC;

    return CLASS_EMBB;
}

double EdgeTriggerAlertApp::workForClass(TrafficClass tc) const
{
    if (tc == CLASS_URLLC && workURLLC > 0) return workURLLC;
    if (tc == CLASS_EMBB  && workEMBB  > 0) return workEMBB;
    if (tc == CLASS_MMTC  && workMMTC  > 0) return workMMTC;
    return workPerEvent;
}

// -------------------- admission + drop --------------------

bool EdgeTriggerAlertApp::pushOutForURLLCIfEnabled()
{
    if (!preemptBuffer) return false;

    bool pushed = false;
    while (totalQueueSize() >= queueCapacity) {
        if (!qEMBB.empty()) {
            qEMBB.pop_front();
            noteDropped(CLASS_EMBB);
            emitLong(sigAlertDropped, 1);
            pushed = true;
            continue;
        }
        if (!qMMTC.empty()) {
            qMMTC.pop_front();
            noteDropped(CLASS_MMTC);
            emitLong(sigAlertDropped, 1);
            pushed = true;
            continue;
        }
        break;
    }

    if (pushed) updateQueueSignals();
    return pushed;
}

bool EdgeTriggerAlertApp::admitOrDrop(const Job& jobIn)
{
    if (preemptAdmission && (jobIn.tc == CLASS_EMBB || jobIn.tc == CLASS_MMTC) && !qURLLC.empty()) {
        noteDropped(jobIn.tc);
        emitLong(sigAlertDropped, 1);
        updateQueueSignals();
        return false;
    }

    int cap = classQueueCap(jobIn.tc);
    if (cap > 0 && classQueueSize(jobIn.tc) >= cap) {
        noteDropped(jobIn.tc);
        emitLong(sigAlertDropped, 1);
        updateQueueSignals();
        return false;
    }

    if (totalQueueSize() >= queueCapacity && jobIn.tc == CLASS_URLLC)
        pushOutForURLLCIfEnabled();

    if (totalQueueSize() >= queueCapacity) {
        noteDropped(jobIn.tc);
        emitLong(sigAlertDropped, 1);
        updateQueueSignals();
        return false;
    }

    noteAdmitted(jobIn.tc);
    emitLong(sigAlertQueued, 1);

    if (jobIn.tc == CLASS_URLLC) qURLLC.push_back(jobIn);
    else if (jobIn.tc == CLASS_EMBB) qEMBB.push_back(jobIn);
    else qMMTC.push_back(jobIn);

    updateQueueSignals();
    return true;
}

// -------------------- UDP receive --------------------

void EdgeTriggerAlertApp::socketDataArrived(UdpSocket *, Packet *pk)
{
    take(pk);

    emitLong(sigDetectRx, 1);

    simtime_t creation = simTime();
    if (auto ct = pk->findTag<CreationTimeTag>()) {
        creation = ct->getCreationTime();
        emitDouble(sigDetectLatency, (simTime() - creation).dbl());
    }

    if (!haveFirstDetect) {
        haveFirstDetect = true;
        firstDetectTime = simTime();
    }

    TrafficClass tc = classifyPacket(pk);
    noteOffered(tc);

    globalSeq++;

    Job job;
    job.tc = tc;
    job.arrivalTime = simTime();
    job.creationTime = creation;
    job.enqueueTime = simTime();
    job.workOps = workForClass(tc);
    job.seq = globalSeq;

    admitOrDrop(job);

    delete pk;

    startServiceIfIdle();
}

// -------------------- scheduling/service --------------------

bool EdgeTriggerAlertApp::hasJobs() const
{
    return !qURLLC.empty() || !qEMBB.empty() || !qMMTC.empty();
}

EdgeTriggerAlertApp::TrafficClass EdgeTriggerAlertApp::pickNextClass()
{
    if (slicingScenario == "preempt" && preemptService) {
        if (!qURLLC.empty()) return CLASS_URLLC;
    }

    if (slicingScenario == "none" && scheduler == "strict") {
        if (!qURLLC.empty()) return CLASS_URLLC;
        if (!qEMBB.empty())  return CLASS_EMBB;
        return CLASS_MMTC;
    }

    auto addQuantum = [&]() {
        int qU = std::max(1, (int)std::round(shareURLLC));
        int qE = std::max(1, (int)std::round(shareEMBB));
        int qM = std::max(1, (int)std::round(shareMMTC));
        deficitURLLC += qU;
        deficitEMBB  += qE;
        deficitMMTC  += qM;
    };

    addQuantum();

    for (int tries = 0; tries < 9; tries++) {
        int idx = rrIndex % 3;
        rrIndex++;

        if (idx == 0) {
            if (!qURLLC.empty() && deficitURLLC > 0) { deficitURLLC--; return CLASS_URLLC; }
            if (slicingScenario == "dynamicBorrow" && !qURLLC.empty()) return CLASS_URLLC;
        }
        else if (idx == 1) {
            if (!qEMBB.empty() && deficitEMBB > 0) { deficitEMBB--; return CLASS_EMBB; }
            if (slicingScenario == "dynamicBorrow" && !qEMBB.empty()) return CLASS_EMBB;
        }
        else {
            if (!qMMTC.empty() && deficitMMTC > 0) { deficitMMTC--; return CLASS_MMTC; }
            if (slicingScenario == "dynamicBorrow" && !qMMTC.empty()) return CLASS_MMTC;
        }

        // staticNoBorrow wastes empty turns intentionally
    }

    if (!qURLLC.empty()) return CLASS_URLLC;
    if (!qEMBB.empty())  return CLASS_EMBB;
    return CLASS_MMTC;
}

EdgeTriggerAlertApp::Job EdgeTriggerAlertApp::popNextJob()
{
    TrafficClass tc = pickNextClass();

    Job job;
    if (tc == CLASS_URLLC) { job = qURLLC.front(); qURLLC.pop_front(); }
    else if (tc == CLASS_EMBB) { job = qEMBB.front(); qEMBB.pop_front(); }
    else { job = qMMTC.front(); qMMTC.pop_front(); }

    updateQueueSignals();
    return job;
}

simtime_t EdgeTriggerAlertApp::computeServiceTime(double workOps) const
{
    if (computeUnits <= 0) return SIMTIME_ZERO;
    double secs = workOps / computeUnits;
    return secs > 0 ? SimTime(secs) : SIMTIME_ZERO;
}

void EdgeTriggerAlertApp::startServiceIfIdle()
{
    if (busy) return;

    // if we are in pacing gap, don’t start a job yet
    if (paceTimer && paceTimer->isScheduled())
        return;

    if (!hasJobs())
        return;

    busy = true;
    haveCurrent = true;
    currentJob = popNextJob();

    double qDelay = (simTime() - currentJob.enqueueTime).dbl();
    if (currentJob.tc == CLASS_URLLC) emitDouble(sigQueueingDelayURLLC, qDelay);
    else if (currentJob.tc == CLASS_EMBB) emitDouble(sigQueueingDelayEMBB, qDelay);
    else emitDouble(sigQueueingDelayMMTC, qDelay);

    simtime_t svc = computeServiceTime(currentJob.workOps);
    emitDouble(sigEdgeProcTime, svc.dbl());

    double energyJ = idlePowerW * svc.dbl() + joulesPerOp * currentJob.workOps;
    emitDouble(sigEdgeEnergyJ, energyJ);

    // IMPORTANT: ensure timer is not already scheduled
    if (serviceTimer->isScheduled())
        cancelEvent(serviceTimer);

    scheduleAt(simTime() + procDelay + svc, serviceTimer);
}

void EdgeTriggerAlertApp::serviceComplete()
{
    if (!haveCurrent) {
        busy = false;
        return;
    }

    sendResponseForJob(currentJob);
    noteServed(currentJob.tc);

    if (haveFirstDetect && !firstAlertReported) {
        emitDouble(sigDetectToFirstAlert, (simTime() - firstDetectTime).dbl());
        firstAlertReported = true;
    }

    haveCurrent = false;
    busy = false;

    // If more work exists, enforce pacing with a separate timer
    if (hasJobs()) {
        if (sendInterval > SIMTIME_ZERO) {
            if (paceTimer->isScheduled())
                cancelEvent(paceTimer);
            scheduleAt(simTime() + sendInterval, paceTimer);
        }
        else {
            startServiceIfIdle();
        }
    }
}

void EdgeTriggerAlertApp::sendResponseForJob(const Job& job)
{
    std::string base;
    int tos = tosBestEffort;

    if (job.tc == CLASS_URLLC) { base = respNameURLLC; tos = tosEmergency; }
    else if (job.tc == CLASS_EMBB) { base = respNameEMBB; tos = tosBestEffort; }
    else { base = respNameMMTC; tos = tosBestEffort; }

    std::string name = base + "-" + std::to_string(job.seq);

    auto *pkt = new Packet(name.c_str());
    auto payload = makeShared<BytesChunk>(std::vector<uint8_t>((size_t)messageLengthB, 0));
    pkt->insertAtBack(payload);

    pkt->addTag<CreationTimeTag>()->setCreationTime(simTime());

    trySetTos(socket, tos);

    emitLong(sigAlertTx, 1);
    emitLong(sigAlertSeq, (long)job.seq);

    socket.sendTo(pkt, groupAddr, alertPort);
}

} // namespace inet
