#pragma once

#include <deque>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>

#include <omnetpp.h>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/InitStages.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

namespace inet {

class EdgeTriggerAlertApp : public ApplicationBase, public UdpSocket::ICallback
{
  protected:
    enum TrafficClass {
        CLASS_URLLC = 0,
        CLASS_EMBB  = 1,
        CLASS_MMTC  = 2
    };

    struct Job {
        TrafficClass tc;
        simtime_t arrivalTime;     // arrival at edge
        simtime_t creationTime;    // CreationTimeTag if present (else simTime())
        simtime_t enqueueTime;     // enqueue time (queueing delay basis)
        double workOps;            // ops required
        int seq;                   // sequence number
    };

    using SignalId = decltype(omnetpp::cComponent::registerSignal("x"));

    // ---- signals (must match NED @signal names) ----
    static SignalId sigOfferedURLLC, sigOfferedEMBB, sigOfferedMMTC;
    static SignalId sigAdmittedURLLC, sigAdmittedEMBB, sigAdmittedMMTC;
    static SignalId sigServedURLLC, sigServedEMBB, sigServedMMTC;
    static SignalId sigDroppedURLLC, sigDroppedEMBB, sigDroppedMMTC;

    static SignalId sigQueueDepth;
    static SignalId sigQueueDepthURLLC, sigQueueDepthEMBB, sigQueueDepthMMTC;

    static SignalId sigQueueingDelayURLLC, sigQueueingDelayEMBB, sigQueueingDelayMMTC;

    // legacy signals you already used
    static SignalId sigDetectRx;
    static SignalId sigAlertQueued;
    static SignalId sigAlertDropped;
    static SignalId sigAlertTx;
    static SignalId sigAlertSeq;
    static SignalId sigDetectLatency;
    static SignalId sigEdgeProcTime;
    static SignalId sigDetectToFirstAlert;
    static SignalId sigEdgeEnergyJ;

    // ---- params ----
    std::string vectorName;
    int listenPort = 5000;
    int alertPort = 6000;
    std::string packetName = "ALERT";
    int messageLengthB = 200;

    simtime_t sendInterval = SIMTIME_ZERO;

    simtime_t procDelay = SIMTIME_ZERO;
    double computeUnits = 1e8;
    double workPerEvent = 5e6;

    std::string groupAddrStr = "224.0.0.1";
    L3Address groupAddr;
    int ttl = 1;

    // legacy 2-class knobs (kept)
    int tosEmergency = 184;
    int tosBestEffort = 0;
    bool classifyDetectByName = true;

    int queueCapacity = 100000;
    std::string dropPolicy = "dropTail";
    std::string scheduler = "strict";
    int weightEmergency = 10;
    int weightBestEffort = 1;

    // NEW 3-class slicing knobs
    std::string slicingScenario = "dynamicBorrow"; // none/staticNoBorrow/dynamicBorrow/preempt
    double shareEMBB = 75;
    double shareURLLC = 15;
    double shareMMTC = 10;

    bool preemptService = true;
    bool preemptBuffer = false;
    bool preemptAdmission = false;

    int queueCapURLLC = -1;
    int queueCapEMBB  = -1;
    int queueCapMMTC  = -1;

    double workURLLC = -1;
    double workEMBB  = -1;
    double workMMTC  = -1;

    std::string respNameURLLC = "ALERT_URLLC";
    std::string respNameEMBB  = "ALERT_EMBB";
    std::string respNameMMTC  = "ALERT_MMTC";

    // energy
    double idlePowerW = 5.0;
    double joulesPerOp = 1e-9;

    int rounds = 1;

    // ---- runtime state ----
    UdpSocket socket;

    omnetpp::cMessage *serviceTimer = nullptr; // service completion
    omnetpp::cMessage *paceTimer    = nullptr; // pacing gap between jobs

    std::deque<Job> qURLLC, qEMBB, qMMTC;

    bool busy = false;
    bool haveCurrent = false;
    Job currentJob;

    int globalSeq = 0;

    bool haveFirstDetect = false;
    simtime_t firstDetectTime = SIMTIME_ZERO;
    bool firstAlertReported = false;

    int rrIndex = 0;
    int deficitURLLC = 0, deficitEMBB = 0, deficitMMTC = 0;

  protected:
    // emit wrappers
    inline void emitLong(SignalId sig, long v) const {
        const_cast<EdgeTriggerAlertApp*>(this)->omnetpp::cComponent::emit(sig, v);
    }
    inline void emitDouble(SignalId sig, double v) const {
        const_cast<EdgeTriggerAlertApp*>(this)->omnetpp::cComponent::emit(sig, v);
    }

    static std::string toUpper(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c){ return (char)std::toupper(c); });
        return s;
    }

    // INET lifecycle (these MUST exist to avoid abstract-class error)
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
    virtual void finish() override;

    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

    // UDP callbacks
    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *, Indication *indication) override { delete indication; }
    virtual void socketClosed(UdpSocket *) override {}

    // logic helpers
    TrafficClass classifyPacket(Packet *pk) const;
    double workForClass(TrafficClass tc) const;

    int totalQueueSize() const;
    int classQueueSize(TrafficClass tc) const;
    int classQueueCap(TrafficClass tc) const;

    void updateQueueSignals() const;

    void noteOffered(TrafficClass tc);
    void noteAdmitted(TrafficClass tc);
    void noteServed(TrafficClass tc);
    void noteDropped(TrafficClass tc);

    bool pushOutForURLLCIfEnabled();
    bool admitOrDrop(const Job& jobIn);

    bool hasJobs() const;
    TrafficClass pickNextClass();
    Job popNextJob();

    simtime_t computeServiceTime(double workOps) const;

    void startServiceIfIdle();
    void serviceComplete();
    void sendResponseForJob(const Job& job);

    // TOS helper: compile-safe if setTos exists, otherwise no-op
    template <typename T>
    static auto trySetTosImpl(T& sock, int tos, int) -> decltype(sock.setTos(tos), void()) { sock.setTos(tos); }
    template <typename T>
    static void trySetTosImpl(T&, int, ...) {}
    static void trySetTos(UdpSocket& sock, int tos) { trySetTosImpl(sock, tos, 0); }

  public:
    virtual ~EdgeTriggerAlertApp();
};

} // namespace inet
