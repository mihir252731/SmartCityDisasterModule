#pragma once

#include <vector>
#include <string>
#include <unordered_set>
#include "omnetpp.h"
#include "omnetpp/cpatternmatcher.h"
#include "omnetpp/cstringtokenizer.h"
#include "inet/physicallayer/contract/packetlevel/IRadio.h"

namespace inet {

class DisasterFailure : public omnetpp::cSimpleModule, public omnetpp::cListener
{
  protected:
    // parameters
    std::string iotPattern, apPattern;
    std::unordered_set<int> failedIotIds;
    std::unordered_set<int> failedApIds;
    int  wlanIndex = 0;
    bool deleteNodes = false;

    // timer mode
    bool        useEpicenterTrigger = false;
    std::string failAtTimesStr;
    double      lambdaOnTimer = 0.0;

    // signal mode
    std::string epicenterPath;
    std::vector<std::string> triggerNames;
    double lambdaP = 0.0;
    double lambdaS = 0.0;

    int countOnSWave = -1;
    int countOnSurfWave = -1;
    double lambdaSurfWave = 0;  // like lambdaPWave/lambdaSWave

    int totalFailedIots = 0;
    int totalFailedAps  = 0;

    int iotCountOnSWave    = -1;
    int apCountOnSWave     = -1;
    int iotCountOnSurfWave = -1;
    int apCountOnSurfWave  = -1;

    // state
    omnetpp::cModule   *epicenter = nullptr;
    omnetpp::simsignal_t waveSentSig = SIMSIGNAL_NULL;
    bool subscribed = false;

    // Add to class QuakeFailure (protected:)
    bool handledSWave   = false;
    bool handledSurfWave = false;

    std::vector<omnetpp::SimTime> failTimes;   // absolute times
    omnetpp::cMessage *timer = nullptr;
    size_t nextTimeIndex = 0;

    // result signals (vectors)
    omnetpp::simsignal_t failedIotsSig = SIMSIGNAL_NULL;
    omnetpp::simsignal_t failedApsSig  = SIMSIGNAL_NULL;

  protected:
    // cSimpleModule
    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void finish() override;
    virtual ~DisasterFailure() override;

    // cListener (we add safe no-op overloads so mismatched signal types never crash)
    virtual void receiveSignal(omnetpp::cComponent *src, omnetpp::simsignal_t id,
                               omnetpp::cObject *obj, omnetpp::cObject *details) override;
    virtual void receiveSignal(omnetpp::cComponent *src, omnetpp::simsignal_t id,
                               long l, omnetpp::cObject *details) override;
    virtual void receiveSignal(omnetpp::cComponent *src, omnetpp::simsignal_t id,
                               unsigned long l, omnetpp::cObject *details) override;
    virtual void receiveSignal(omnetpp::cComponent *src, omnetpp::simsignal_t id,
                               double d, omnetpp::cObject *details) override;
    virtual void receiveSignal(omnetpp::cComponent *src, omnetpp::simsignal_t id,
                               const omnetpp::SimTime& t, omnetpp::cObject *details) override;
    virtual void receiveSignal(omnetpp::cComponent *src, omnetpp::simsignal_t id,
                               const char *s, omnetpp::cObject *details) override;
    virtual void receiveSignal(omnetpp::cComponent *src, omnetpp::simsignal_t id,
                               bool b, omnetpp::cObject *details) override;

    // helpers
    void parseFailTimes(const std::string& s);
    void failRandomTargets(int totalToFail);
    void failSpecificTargets(int failIots, int failAps);
    void scheduleNextTimer();
    void onTimerFire();
    void onWavePacket(const char *pktName);
    void applyFailures(double lambda);
    std::vector<omnetpp::cModule*> collectModulesByPattern(const std::string& patt) const;
    void turnRadioOff(omnetpp::cModule* node, int wlanIdx);
};

} // namespace inet
