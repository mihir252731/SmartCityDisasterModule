#ifndef __INET_DISASTERPROPOGATION_H
#define __INET_DISASTERPROPOGATION_H

#pragma once
#include <omnetpp.h>
#include <vector>
#include "inet/common/INETDefs.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/packet/Packet.h"

namespace inet {

class DisasterPropogation : public cSimpleModule, public UdpSocket::ICallback
{
    // --- research signals (declarations only; definitions in .cc) ---
    static omnetpp::simsignal_t waveSentSignal;
    static omnetpp::simsignal_t waveSentTimeSignal;
    static omnetpp::simsignal_t originTimeSignal;

  protected:
    // Params
    std::string vectorName;
    simtime_t   originTime {0};
    double      epicX {0.0};   // meters
    double      epicY {0.0};   // meters
    double      vP {6500.0};   // m/s
    double      vS {3600.0};   // m/s
    double      vSurf {3200.0}; // m/s
    int         port {7001};
    int         messageLength {64};

    std::string pName, sName, surfName;

    bool        verbose {false};

    // UDP
    UdpSocket socket;

    // One self-message per scheduled send
    enum Phase : uint8_t { P=0, S=1, SURF=2 };
    struct Event {
        L3Address addr;
        Phase     phase;
    };
    // For each scheduled cMessage we keep an Event payload in a vector
    std::vector<Event> events;

  protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *msg) override;

    // UdpSocket::ICallback
    virtual void socketDataArrived(UdpSocket*, Packet* pk) override { delete pk; }
    virtual void socketErrorArrived(UdpSocket*, Indication* ind) override { delete ind; }
    virtual void socketClosed(UdpSocket*) override {}

    // Helpers
    void scheduleForAllIoTs();
    void scheduleSend(const L3Address& dst, Phase ph, simtime_t t);

    Packet* buildPacket(Phase ph) const;
};

} // namespace inet

#endif
