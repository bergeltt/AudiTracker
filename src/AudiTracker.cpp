/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "application.h"
#line 1 "c:/Users/tony/Documents/Particle/AudiTracker/src/AudiTracker.ino"
/*
 * Project AudiTracker
 * Description:
 * Author:
 * Date:
 */

// hold off connecting to cell/network/cloud. See below.
boolean isPublishing();
int setGpsRate(String command);
int publishLocationNow(String command);
int publishBatteryStatus(String command);
void updatePosition();
void updateAcceleration();
boolean publishLocationIfDirty();
int connectRelay(String command);
int disconnectRelay(String command);
void doUpdates();
void setup();
void loop();
#line 9 "c:/Users/tony/Documents/Particle/AudiTracker/src/AudiTracker.ino"
SYSTEM_MODE(MANUAL)

#include "RelayController.h"
#include "AssetTracker2.h"
#include "KWP.h"
#include "FISLib.h"

FuelGauge battery;
RelayController relays = RelayController();
AssetTracker2 tracker = AssetTracker2();

boolean shouldPublishNow = false;
int publishRateSec = 60*60;
unsigned long lastPublishMillis = 0;

enum UpdateBuffers
{
	UpdateBuffers_Last = 0,
	UpdateBuffers_Current = 1,
	UpdateBuffers_NUM = 2
};

String posStr[UpdateBuffers_NUM];
String lastGoodPosStr[UpdateBuffers_NUM];
String accelStr[UpdateBuffers_NUM];
int maxUnPubAccelMag = -1;

boolean isPublishing()
{
	return shouldPublishNow || (publishRateSec > 0);
}

// Allows changing the measurement rate
int setGpsRate(String command)
{
	Serial.println("Set GPS rate requested");

	const uint16_t rate = atoi(command);
	const int numNavSol = atoi(command.substring(command.indexOf(' ')));
	Serial.print("rate: ");
	Serial.print(rate);
	Serial.print(" numNavSol: ");
	Serial.println(numNavSol);
	tracker.gpsRate(rate, numNavSol);
	return 1;
}

int publishLocationNow(String command)
{
	Serial.println("Publish now requested");

	shouldPublishNow = true;
	lastPublishMillis = 0;
	accelStr[UpdateBuffers_Last] = "Uninitialized";
	posStr[UpdateBuffers_Last] = "Uninitialized";
	lastGoodPosStr[UpdateBuffers_Last] = "Uninitialized";

	return 1;
}

// Lets you remotely check the battery status by calling the function "batt"
// Triggers a publish with the info (so subscribe or watch the dashboard)
// and also returns a '1' if there's >10% battery left and a '0' if below
int publishBatteryStatus(String command)
{
	Serial.println("Publish battery status requested");

	// Publish the battery voltage and percentage of battery remaining
	// if you want to be really efficient, just report one of these
	// the String::format("%f.2") part gives us a string to publish,
	// but with only 2 decimal points to save space
	const String batStr = "v:" + String::format("%.2f", battery.getVCell()) +
		", %" + String::format("%.2f", battery.getSoC());
	Particle.publish("Battery", batStr, 60, PRIVATE);
	Serial.println(batStr);

	return 1;
}

void updatePosition()
{
	if (tracker.gpsFix())
	{
		// "lat, lon, acc, alt, spd, batV, bat%"
		posStr[UpdateBuffers_Current] = String::format("Lt %f, Ln %f, Acc +/-%fft, Alt %fft, Spd %f",
			tracker.readLatDeg(),
			tracker.readLonDeg(),
			(((float)tracker.getGpsAccuracy())/304.8), // integer mm to float feet
			tracker.getAltitude()/304.8,             // mm to feet
			tracker.getSpeed());

		lastGoodPosStr[UpdateBuffers_Current] = posStr[UpdateBuffers_Current];
	}
	else
	{
		posStr[UpdateBuffers_Current] = "Unknown";
	}
}

void updateAcceleration()
{
	/* int x, y, z;
	tracker.readXYZ(x, y, z);

	int curAccelMagnitude = sqrt((x*x)+(y*y)+(z*z));
	if (maxUnPubAccelMag < curAccelMagnitude)
	{
		maxUnPubAccelMag = curAccelMagnitude;
		accelStr[UpdateBuffers_Current] = String::format("%d,%d,%d", x, y, z);
	}*/
}

boolean publishLocationIfDirty()
{
	String eventData;

	if (shouldPublishNow
		|| posStr[UpdateBuffers_Last] != posStr[UpdateBuffers_Current])
	{
		eventData += String::format("Pos: %s, ", posStr[UpdateBuffers_Current].c_str());
		posStr[UpdateBuffers_Last] = posStr[UpdateBuffers_Current];
	}

	if (shouldPublishNow
		|| lastGoodPosStr[UpdateBuffers_Last] != lastGoodPosStr[UpdateBuffers_Current])
	{
		eventData += String::format("LastGoodPos: %s, ", lastGoodPosStr[UpdateBuffers_Current].c_str());
		lastGoodPosStr[UpdateBuffers_Last] = lastGoodPosStr[UpdateBuffers_Current];
	}

	if (shouldPublishNow
		|| accelStr[UpdateBuffers_Last] != accelStr[UpdateBuffers_Current])
	{
		eventData += String::format("Accel: %s, ", accelStr[UpdateBuffers_Current].c_str());
		accelStr[UpdateBuffers_Last] = accelStr[UpdateBuffers_Current];
		maxUnPubAccelMag = -1;
	}

	boolean published = false;
	if (isPublishing() && eventData.length() > 0)
	{
		published |= Particle.publish("Loc", eventData, 60, PRIVATE);
		Serial.println(eventData);
	}

	return published;
}

int connectRelay(String command)
{
	return relays.changeRelay(command, RelayController::RelayState_Connected);
}

int disconnectRelay(String command)
{
	return relays.changeRelay(command, RelayController::RelayState_Disconnected);
}

void doUpdates() 
{
	tracker.updateGPS();
	updatePosition();
	updateAcceleration();
}
Timer updateTimer(50, doUpdates);

void setup()
{
	//Open the Serial Port so for the debug terminal to listen
	Serial.begin(9600);
	//provide time for the developer to open the debug terminal
	//delay(10000);
    Serial.println("Setup Start");

	for (int x = 0; x < UpdateBuffers_NUM; ++x)
	{
		accelStr[x] = "Uninitialized";
		posStr[x] = "Uninitialized";
		lastGoodPosStr[x] = "Uninitialized";
	}

	//function names are max of 12 letters
	Particle.function("pubBattery", publishBatteryStatus);
	//Particle.function("setGpsRate", setGpsRate);
	Particle.function("pubLocNow", publishLocationNow);
	Particle.variable("pubRateSec", &publishRateSec, INT);

    Serial.println("Setup Particle GPS Register End");

	// SX1509 I2C address (set by ADDR1 and ADDR0 (00 by default):
	const byte RELAY_CONTROLLER_SX1509_ADDRESS = 0x3E;  // SX1509 I2C address
	const bool relaysSetup = relays.setupRelays(RELAY_CONTROLLER_SX1509_ADDRESS);
	Particle.function("relayOn", connectRelay);
	Particle.function("relayOff", disconnectRelay);

    Serial.println("Setup Particle Relay Register End");

	Serial.println("Setup Turning Accel On");
	//tracker.turnOnAccel();
	Serial.println("Setup Turning GPS On");
	tracker.gpsOn();
	Serial.println("Setup Turning External GPS Antenna On");
	tracker.antennaExternal();

    Serial.println("Setup Tracker Init End");

	Particle.connect();
	updateTimer.start();

    publishLocationNow("");

    Serial.println("Setup End");
}

void loop()
{
	if (shouldPublishNow || (isPublishing() && (millis() - lastPublishMillis > static_cast<size_t>(publishRateSec) * 1000)))
	{
		bool published = false;

		published |= publishLocationIfDirty();

		if (published)
		{
			lastPublishMillis = millis();
			shouldPublishNow = false;
		}
	}

	Particle.process();
}
