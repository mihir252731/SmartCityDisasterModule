#include <inet/common/earthquake/DisasterPropogation.h>
#include <cmath>
#include "inet/common/ModuleAccess.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

namespace inet {

Define_Module(DisasterPropogation);

// --- research signals (definitions + registration) ---
omnetpp::simsignal_t DisasterPropogation::waveSentSignal   = cComponent::registerSignal("waveSent");
omnetpp::simsignal_t DisasterPropogation::waveSentTimeSignal   = cComponent::registerSignal("waveSentTime");
omnetpp::simsignal_t DisasterPropogation::originTimeSignal = cComponent::registerSignal("originTime");

void DisasterPropogation::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        vectorName    = par("vectorName").stdstringValue();
        originTime    = par("originTime");
        epicX         = par("epicX").doubleValue();     // meters
        epicY         = par("epicY").doubleValue();     // meters
        vP            = par("vP").doubleValue();        // m/s
        vS            = par("vS").doubleValue();        // m/s
        vSurf         = par("vSurf").doubleValue();     // m/s
        port          = par("port");
        messageLength = par("messageLength");
        pName         = par("pName").stdstringValue();
        sName         = par("sName").stdstringValue();
        surfName      = par("surfName").stdstringValue();
        verbose       = par("verbose").boolValue();   // <-- ADD
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        socket.setOutputGate(gate("socketOut"));
        socket.setCallback(this);
        // No bind required for pure sending, but binding to ephemeral helps stats
        socket.bind(1025);

        // --- ADD: publish the origin time as a scalar-like signal once
        emit(originTimeSignal, SIMTIME_DBL(originTime));

        scheduleForAllIoTs();
    }
}

void DisasterPropogation::scheduleForAllIoTs()
{
    cModule *net = getSystemModule();
    L3AddressResolver resolver;

    for (int i = 0; ; ++i) {
        cModule *iot = net->getSubmodule(vectorName.c_str(), i);
        if (!iot) break;

        // fetch IoT coordinates (meters) as configured in your SensorHost NED
        double x = iot->par("myX").doubleValue();   // already meters
        double y = iot->par("myY").doubleValue();

        // distance from epicenter
        double dx = x - epicX;
        double dy = y - epicY;
        double dist = std::sqrt(dx*dx + dy*dy);     // meters

        // arrival times
        simtime_t tP    = originTime + dist / vP;
        simtime_t tS    = originTime + dist / vS;
        simtime_t tSurf = originTime + dist / vSurf;

        // destination address (unicast)
        //L3Address dst = resolver.resolve(iot);
        // Option B: explicitly ask for IPv4 by addressOf()
        L3Address dst = resolver.addressOf(iot, L3AddressResolver::ADDR_IPv4);

        if (verbose){
            EV_INFO << "Quake schedule for " << iot->getFullPath()
                    << " dist=" << dist << "m  tP=" << tP << "  tS=" << tS
                    << "  tSurf=" << tSurf << "  dst=" << dst << "\n";
        }
        scheduleSend(dst, Phase::P,    tP);
        scheduleSend(dst, Phase::S,    tS);
        scheduleSend(dst, Phase::SURF, tSurf);
    }
}

void DisasterPropogation::scheduleSend(const L3Address& dst, Phase ph, simtime_t t)
{
    // store event payload and create a self-message pointing to it by index
    events.push_back(Event{dst, ph});
    auto *m = new cMessage("snd");
    m->setKind((int)ph);
    // encode index in contextPointer to find back
    m->setContextPointer(reinterpret_cast<void*>((intptr_t)(events.size()-1)));
    scheduleAt(t, m);
}

Packet* DisasterPropogation::buildPacket(Phase ph) const
{
    const std::string& nm = (ph==P ? pName : (ph==S ? sName : surfName));
    auto *pk = new Packet(nm.c_str());
    const auto bytes = makeShared<BytesChunk>(std::vector<uint8_t>(messageLength, 0));
    pk->insertAtBack(bytes);
    pk->addTag<CreationTimeTag>()->setCreationTime(simTime());
    return pk;
}

void DisasterPropogation::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        size_t idx = (size_t)(intptr_t)msg->getContextPointer();
        ASSERT(idx < events.size());
        const Event& ev = events[idx];

        auto *pk = buildPacket(ev.phase);

        // --- ADD: record the actual emission time (vector)
        emit(waveSentSignal, pk);
        emit(waveSentTimeSignal, SIMTIME_DBL(simTime()));  // numeric for plotting

        socket.sendTo(pk, ev.addr, port);
        delete msg;
    }
    else {
        socket.processMessage(msg);
    }
}

} // namespace inet
