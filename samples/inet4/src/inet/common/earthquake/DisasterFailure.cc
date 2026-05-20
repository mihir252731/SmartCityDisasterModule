#include <inet/common/earthquake/DisasterFailure.h>
#include <algorithm>
#include <random>
#include <sstream>
#include <climits>

#include "inet/common/packet/Packet.h"
#include "inet/common/ModuleAccess.h"
#include "omnetpp/simtime.h"

using namespace omnetpp;

namespace inet {

Define_Module(DisasterFailure);

// ---------- init / finish ----------

void DisasterFailure::initialize()
{
    // params
    iotPattern  = par("iotPattern").stdstringValue();
    apPattern   = par("apPattern").stdstringValue();
    wlanIndex   = par("wlanIndex").intValue();
    deleteNodes = par("deleteNodes").boolValue();

    useEpicenterTrigger = par("useEpicenterTrigger").boolValue();
    failAtTimesStr = par("failAtTimes").stdstringValue();
    lambdaOnTimer   = par("lambdaOnTimer").doubleValue();

    epicenterPath = par("epicenterPath").stdstringValue();
    lambdaP = par("lambdaPWave").doubleValue();
    lambdaS = par("lambdaSWave").doubleValue();

    iotCountOnSWave    = par("iotCountOnSWave");
    apCountOnSWave     = par("apCountOnSWave");
    iotCountOnSurfWave = par("iotCountOnSurfWave");
    apCountOnSurfWave  = par("apCountOnSurfWave");

    countOnSWave     = par("countOnSWave");
    countOnSurfWave  = par("countOnSurfWave");
    lambdaSurfWave   = par("lambdaSurfWave");  // default 0


    // parse trigger names (for signal mode, if ever used)
    {
        std::string s = par("triggerNames").stdstringValue();
        std::stringstream ss(s);
        std::string tok;
        while (std::getline(ss, tok, ',')) {
            auto b = tok.find_first_not_of(" \t");
            auto e = tok.find_last_not_of(" \t");
            if (b != std::string::npos) triggerNames.push_back(tok.substr(b, e - b + 1));
        }
    }

    // result vectors
    failedIotsSig = cComponent::registerSignal("failedIots");
    failedApsSig  = cComponent::registerSignal("failedAps");

    // TIMER MODE
    parseFailTimes(failAtTimesStr);
    if (!failTimes.empty()) {
        timer = new cMessage("quakeTimer");
        scheduleNextTimer();
        EV_INFO << "[DisasterFailure] Timer mode ON with " << failTimes.size()
                << " failure times; lambdaOnTimer=" << lambdaOnTimer << "\n";
    }

    // SIGNAL MODE (optional; we keep it safe)
    if (useEpicenterTrigger) {
        epicenter = getModuleByPath(epicenterPath.c_str());
        if (!epicenter)
            throw cRuntimeError("DisasterFailure: epicenterPath '%s' not found", epicenterPath.c_str());
        waveSentSig = cComponent::registerSignal("waveSent"); // type may vary in emitter
        epicenter->subscribe(waveSentSig, this);
        subscribed = true;
        EV_INFO << "[DisasterFailure] Epicenter trigger mode ON: listening to 'waveSent' at "
                << epicenter->getFullPath() << "\n";
    }
}

void DisasterFailure::finish()
{
    if (timer) { cancelAndDelete(timer); timer = nullptr; }
    if (subscribed && epicenter) {
        // best-effort unsubscribe (avoid Qtenv warning on destruct)
        try { epicenter->unsubscribe(waveSentSig, this); } catch (...) {}
        subscribed = false;
    }
}

DisasterFailure::~DisasterFailure()
{
    // (finish() already cleaned things up)
}

// ---------- message / signal handling ----------

void DisasterFailure::handleMessage(cMessage *msg)
{
    if (msg == timer) {
        onTimerFire();
        scheduleNextTimer();
    }
}

void DisasterFailure::receiveSignal(cComponent *, simsignal_t id, cObject *obj, cObject *)
{
    if (!useEpicenterTrigger || id != waveSentSig) return;
    // We expect a Packet* and use its name; if not a Packet, we just ignore safely.
    auto *pkt = dynamic_cast<Packet*>(obj);
    if (!pkt) { EV_WARN << "[DisasterFailure] waveSent received with non-packet payload (ignored)\n"; return; }
    onWavePacket(pkt->getName());
}

// Safe no-op overloads to avoid "Unsupported signal data type ..." crashes
void DisasterFailure::receiveSignal(cComponent *, simsignal_t, long, cObject *) {}
void DisasterFailure::receiveSignal(cComponent *, simsignal_t, unsigned long, cObject *) {}
void DisasterFailure::receiveSignal(cComponent *, simsignal_t, double, cObject *) {}
void DisasterFailure::receiveSignal(cComponent *, simsignal_t, const SimTime&, cObject *) {}
void DisasterFailure::receiveSignal(cComponent *, simsignal_t, const char*, cObject *) {}
void DisasterFailure::receiveSignal(cComponent *, simsignal_t, bool, cObject *) {}

// ---------- helpers ----------

void DisasterFailure::parseFailTimes(const std::string& s)
{
    failTimes.clear();
    cStringTokenizer tok(s.c_str(), ",");
    while (tok.hasMoreTokens()) {
        const char *token = tok.nextToken();
        if (!*token) continue;
        try {
            simtime_t when = SimTime::parse(token);   // <-- FIX
            if (when >= SIMTIME_ZERO)
                failTimes.push_back(when);
        }
        catch (std::exception& e) {
            EV_WARN << "[DisasterFailure] Ignoring bad time token '" << token
                    << "': " << e.what() << "\n";
        }
    }
    std::sort(failTimes.begin(), failTimes.end());
    failTimes.erase(std::unique(failTimes.begin(), failTimes.end()), failTimes.end());
    nextTimeIndex = 0;
}


void DisasterFailure::scheduleNextTimer()
{
    if (!timer) return;
    while (nextTimeIndex < failTimes.size() && failTimes[nextTimeIndex] < simTime())
        ++nextTimeIndex;
    if (nextTimeIndex < failTimes.size())
        scheduleAt(failTimes[nextTimeIndex], timer);
}

void DisasterFailure::onTimerFire()
{
    if (nextTimeIndex >= failTimes.size()) return;
    SimTime t = failTimes[nextTimeIndex++];
    EV_INFO << "[DisasterFailure] TIMER trigger at " << t << ", lambda=" << lambdaOnTimer << "\n";
    if (lambdaOnTimer > 0) applyFailures(lambdaOnTimer);
}

void DisasterFailure::onWavePacket(const char *pktName)
{
    int toFail = 0;

    if (!strcmp(pktName, "S-wave") || !strcmp(pktName, "SWave")) {
        if (handledSWave) {
            EV_INFO << "[DisasterFailure] S-wave already handled; ignoring duplicate.\n";
            // still emit the current cumulative values to keep the vector flat
            emit(failedIotsSig, totalFailedIots);
            emit(failedApsSig,  totalFailedAps);
            return;
        }
        handledSWave = true;
        toFail = (countOnSWave >= 0) ? countOnSWave : (lambdaS > 0 ? poisson(lambdaS) : 0);

        int wantIots = -1, wantAps = -1;
                if (iotCountOnSWave >= 0 || apCountOnSWave >= 0) {
                    // per-type counts override
                    wantIots = std::max(0, iotCountOnSWave);
                    wantAps  = std::max(0, apCountOnSWave);
                }
                else if (countOnSWave >= 0) {
                    // total count, allocate IoTs first
                    wantIots = countOnSWave;
                    wantAps  = 0;
                }
                else {
                    // Poisson fallback (total), IoTs first
                    int total = (lambdaS > 0) ? poisson(lambdaS) : 0;
                    wantIots = total; wantAps = 0;
                }
                failSpecificTargets(wantIots, wantAps);
                return;

    }
    else if (!strcmp(pktName, "SURF-wave") || !strcmp(pktName, "SURFWave") || !strcmp(pktName, "Surf-wave")) {
        if (handledSurfWave) {
            EV_INFO << "[DisasterFailure] SURF-wave already handled; ignoring duplicate.\n";
            emit(failedIotsSig, totalFailedIots);
            emit(failedApsSig,  totalFailedAps);
            return;
        }
        handledSurfWave = true;
        toFail = (countOnSurfWave >= 0) ? countOnSurfWave : (lambdaSurfWave > 0 ? poisson(lambdaSurfWave) : 0);

        int wantIots = -1, wantAps = -1;
                if (iotCountOnSurfWave >= 0 || apCountOnSurfWave >= 0) {
                    wantIots = std::max(0, iotCountOnSurfWave);
                    wantAps  = std::max(0, apCountOnSurfWave);
                }
                else if (countOnSurfWave >= 0) {
                    wantIots = countOnSurfWave;
                    wantAps  = 0;
                }
                else {
                    int total = (lambdaSurfWave > 0) ? poisson(lambdaSurfWave) : 0;
                    wantIots = total; wantAps = 0;
                }
                failSpecificTargets(wantIots, wantAps);
                return;
    }
    else if (!strcmp(pktName, "P-wave") || !strcmp(pktName, "PWave")) {
        // optional: usually we don't fail on P in your study
        toFail = (lambdaP > 0) ? poisson(lambdaP) : 0;
    }
    else {
        EV_INFO << "[DisasterFailure] Unknown wave '" << pktName << "'; ignoring.\n";
        return;
    }

    if (toFail <= 0) {
        emit(failedIotsSig, totalFailedIots);
        emit(failedApsSig,  totalFailedAps);
        return;
    }

    EV_INFO << "[DisasterFailure] '" << pktName << "' -> failing total " << toFail << " nodes\n";
    failRandomTargets(toFail);  // must choose only from *not yet failed*
}

//void QuakeFailure::onWavePacket(const char *pktName)
//{
    //int toFail = 0;
    //bool recognized = false;

    //if (!strcmp(pktName, "S-wave") || !strcmp(pktName, "SWave")) {
        //recognized = true;
        //if (countOnSWave >= 0)         toFail = countOnSWave;     // deterministic
        //else if (lambdaS > 0)         toFail = poisson(lambdaS);
    //}
    //else if (!strcmp(pktName, "SURF-wave") || !strcmp(pktName, "SURFWave") || !strcmp(pktName, "Surf-wave")) {
        //recognized = true;
        //if (countOnSurfWave >= 0)      toFail = countOnSurfWave;  // deterministic
        //else if (lambdaSurfWave > 0)  toFail = poisson(lambdaSurfWave);
    //}
    //else if (!strcmp(pktName, "P-wave") || !strcmp(pktName, "PWave")) {
        //recognized = true;
        //if (lambdaP > 0)              toFail = poisson(lambdaP);  // legacy
    //}

    //if (!recognized) {
        //EV_INFO << "[QuakeFailure] SIGNAL trigger '" << pktName << "' ignored (not in trigger set)\n";
        //return;
    //}

    //if (toFail <= 0) {
        //EV_INFO << "[QuakeFailure] SIGNAL trigger '" << pktName << "' drew 0 failures (or deterministic=0)\n";
        // emit current cumulative counts (optional)
        //emit(failedIotsSig, totalFailedIots);
        //emit(failedApsSig,  totalFailedAps);
        //return;
    //}

    //EV_INFO << "[QuakeFailure] SIGNAL trigger '" << pktName << "' -> failing total " << toFail << " nodes\n";
    //failRandomTargets(toFail);
//}

std::vector<cModule*> DisasterFailure::collectModulesByPattern(const std::string& patt) const
{
    std::vector<cModule*> out;
    cModule *root = getSimulation()->getSystemModule();
    cPatternMatcher pm;
    pm.setPattern(patt.c_str(), /*wildcard=*/true, /*casesensitive=*/true, /*dottedpath=*/true);

    for (cModule::SubmoduleIterator it(root); !it.end(); ++it) {
        cModule *m = *it;
        if (pm.matches(m->getFullPath().c_str()))
            out.push_back(m);
    }
    return out;
}

void DisasterFailure::failSpecificTargets(int wantIots, int wantAps)
{
    if (wantIots < 0) wantIots = 0;
    if (wantAps  < 0) wantAps  = 0;
    if (wantIots == 0 && wantAps == 0) {
        emit(failedIotsSig, totalFailedIots);
        emit(failedApsSig,  totalFailedAps);
        return;
    }

    // Collect eligible (not yet failed)
    auto allIots = collectModulesByPattern(iotPattern);
    auto allAps  = collectModulesByPattern(apPattern);

    std::vector<cModule*> iots; iots.reserve(allIots.size());
    std::vector<cModule*> aps;  aps.reserve(allAps.size());

    for (auto *m : allIots) if (m && !failedIotIds.count(m->getId())) iots.push_back(m);
    for (auto *m : allAps)  if (m && !failedApIds.count(m->getId()))  aps.push_back(m);

    // Shuffle
    std::mt19937 rng(static_cast<uint32_t>(intrand(INT_MAX)));
    std::shuffle(iots.begin(), iots.end(), rng);
    std::shuffle(aps.begin(),  aps.end(),  rng);

    // Cap to availability
    int takeIots = std::min(wantIots, (int)iots.size());
    int takeAps  = std::min(wantAps,  (int)aps.size());

    EV_INFO << "[DisasterFailure] failing " << takeIots << " IoTs and " << takeAps
            << " APs (" << (deleteNodes ? "DELETE" : "RADIO_OFF") << ")\n";

    // Apply & mark
    for (int i=0; i<takeIots; ++i) {
        cModule *m = iots[i];
        if (deleteNodes) m->deleteModule();
        else             turnRadioOff(m, wlanIndex);
        failedIotIds.insert(m->getId());
    }
    for (int i=0; i<takeAps; ++i) {
        cModule *m = aps[i];
        if (deleteNodes) m->deleteModule();
        else             turnRadioOff(m, wlanIndex);
        failedApIds.insert(m->getId());
    }

    totalFailedIots += takeIots;
    totalFailedAps  += takeAps;

    emit(failedIotsSig, totalFailedIots);
    emit(failedApsSig,  totalFailedAps);
}


void DisasterFailure::failRandomTargets(int totalToFail)
{
    if (totalToFail <= 0) {
        emit(failedIotsSig, totalFailedIots);
        emit(failedApsSig,  totalFailedAps);
        return;
    }

    auto allIots = collectModulesByPattern(iotPattern);
    auto allAps  = collectModulesByPattern(apPattern);

    std::vector<cModule*> iots;  iots.reserve(allIots.size());
    std::vector<cModule*> aps;   aps.reserve(allAps.size());

    for (auto *m : allIots) {
         if (m && !failedIotIds.count(m->getId()))
             iots.push_back(m);
    }
    for (auto *m : allAps) {
         if (m && !failedApIds.count(m->getId()))
             aps.push_back(m);
    }

    // rng
    std::mt19937 rng(static_cast<uint32_t>(intrand(INT_MAX)));
    std::shuffle(iots.begin(), iots.end(), rng);
    std::shuffle(aps.begin(),  aps.end(),  rng);

    int failIots = std::min<int>(totalToFail, (int)iots.size());
    int remain   = totalToFail - failIots;
    int failAps  = std::min<int>(remain, (int)aps.size());

    EV_INFO << "[DisasterFailure] deterministic failing NEW " << failIots << " IoTs and " << failAps
            << " APs (" << (deleteNodes ? "DELETE" : "RADIO_OFF") << ")\n";

    for (int i = 0; i < failIots; ++i) {
         cModule *m = iots[i];
         if (deleteNodes) m->deleteModule();
         else             turnRadioOff(m, wlanIndex);
         failedIotIds.insert(m->getId());
    }
    for (int i = 0; i < failAps; ++i) {
         cModule *m = aps[i];
         if (deleteNodes) m->deleteModule();
         else             turnRadioOff(m, wlanIndex);
         failedApIds.insert(m->getId());
    }

    totalFailedIots += failIots;
    totalFailedAps  += failAps;

    // Emit **cumulative** counts so vectors show the running totals (better for survival plots)
    emit(failedIotsSig, totalFailedIots);
    emit(failedApsSig,  totalFailedAps);
}


void DisasterFailure::applyFailures(double lambda)
{
    // gather candidates
    auto iots = collectModulesByPattern(iotPattern);
    auto aps  = collectModulesByPattern(apPattern);

    // random counts
    std::mt19937 rng(static_cast<uint32_t>(intrand(INT_MAX)));
    std::poisson_distribution<int> pois(lambda);
    int total = pois(rng);
    if (total <= 0) {
        EV_INFO << "[DisasterFailure] drawn total failures = 0\n";
        emit(failedIotsSig, 0);
        emit(failedApsSig,  0);
        return;
    }

    int failIots = std::min<int>(total / 2, (int)iots.size());
    int failAps  = std::min<int>(total - failIots, (int)aps.size());

    std::shuffle(iots.begin(), iots.end(), rng);
    std::shuffle(aps.begin(),  aps.end(),  rng);

    EV_INFO << "[DisasterFailure] failing " << failIots << " IoTs and " << failAps
            << " APs (out of " << iots.size() << " IoTs, " << aps.size() << " APs), "
            << (deleteNodes ? "DELETE" : "RADIO_OFF") << "\n";

    for (int i = 0; i < failIots; ++i) {
        if (deleteNodes) iots[i]->deleteModule();
        else             turnRadioOff(iots[i], wlanIndex);
    }
    for (int i = 0; i < failAps; ++i) {
        if (deleteNodes) aps[i]->deleteModule();
        else             turnRadioOff(aps[i], wlanIndex);
    }

    emit(failedIotsSig, failIots);
    emit(failedApsSig,  failAps);
}

void DisasterFailure::turnRadioOff(cModule* node, int wlanIdx)
{
    if (!node) return;
    cModule *wlan = node->getSubmodule("wlan", wlanIdx);
    if (!wlan) { EV_WARN << node->getFullPath() << ": no wlan[" << wlanIdx << "]\n"; return; }
    cModule *radioMod = wlan->getSubmodule("radio");
    if (!radioMod) { EV_WARN << node->getFullPath() << ": wlan has no radio submodule\n"; return; }

    auto *radio = dynamic_cast<inet::physicallayer::IRadio *>(radioMod);
    if (!radio) { EV_WARN << node->getFullPath() << ": radio doesn't implement IRadio\n"; return; }

    radio->setRadioMode(inet::physicallayer::IRadio::RADIO_MODE_OFF);
    EV_INFO << "[DisasterFailure] turned OFF radio of " << node->getFullPath() << "\n";
}

} // namespace inet
