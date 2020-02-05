#ifndef RELAYCONTROLLER_H
#define RELAYCONTROLLER_H

#include <Wire.h>
#include "SparkFunSX1509.h"

class RelayController
{
public:
    enum Relay
    {
    	Relay_FuelPump = 0, //connectcted to pin 0
    	Relay_N75 = 1, //connected to pin 1
    };

    enum RelayState
    {
    	RelayState_Connected,
    	RelayState_Disconnected
    };

    RelayController();

    bool setupRelays(const byte sx1509Address);
    void setRelayState(Relay relay, RelayState state);

    int changeRelay(String command, RelayState state);

private:
    SX1509 mIO; // Create an SX1509 object to be used throughout
};

#endif
