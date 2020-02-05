#include "RelayController.h"

RelayController::RelayController()
{

}

bool RelayController::setupRelays(const byte sx1509Address)
{
	// Call io.begin(<address>) to initialize the SX1509. If it
	// successfully communicates, it'll return 1.
	if (!mIO.begin(sx1509Address))
	{
		Serial.println("Relay Controller Failed");
		return false;
	}

	// Call io.pinMode(<pin>, <mode>) to set an SX1509 pin as
	// an output:
	mIO.pinMode(Relay_FuelPump, OUTPUT);
	mIO.pinMode(Relay_N75, OUTPUT);

	setRelayState(Relay_FuelPump, RelayState_Connected);
	setRelayState(Relay_N75, RelayState_Connected);

	return true;
}

void RelayController::setRelayState(Relay relay, RelayState state)
{
	if(state == RelayState_Connected)
	{
		mIO.digitalWrite(relay, HIGH);
	}
	else
	{
		mIO.digitalWrite(relay, LOW);
	}
}

int RelayController::changeRelay(String command, RelayState state)
{
	if(command == "FuelPump")
	{
		setRelayState(Relay_FuelPump, state);
		return 1;
	}
	else if(command == "N75")
	{
		setRelayState(Relay_N75, state);
		return 1;
	}
	return 0;
}
