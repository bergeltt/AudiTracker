/*
 * Project AudiTracker
 * Description:
 * Author:
 * Date:
 */

// hold off connecting to cell/network/cloud. See below.
SYSTEM_MODE(SEMI_AUTOMATIC)

#include "AssetTracker.h"
#include "KWP.h"
#include "FISLib.h"

AssetTracker tracker = AssetTracker();

boolean shouldPublishNow = false;
int publishRateSec = 15;
unsigned long lastPublishMillis = 0;

FuelGauge battery;

String posStr[2];
String lastGoodPosStr[2];
String accelStr[2];
int maxUnPubAccelMag = -1;

boolean isPublishing()
{
	return shouldPublishNow || (publishRateSec > 0);
}

 // Allows changing the measurement rate
 int setGpsRate(String command)
 {
	Serial.println("Set GPS rate requested");

	uint16_t rate = atoi(command);
	int nav = atoi(command.substring(command.indexOf(' ')));
	Serial.print("rate: ");
	Serial.print(rate);
	Serial.print(" nav: ");
	Serial.println(nav);
	//tracker.gpsRate(rate, nav);
	return 1;
 }

 int publishNow(String command)
 {
	Serial.println("Publish now requested");

	shouldPublishNow = true;
	lastPublishMillis = 0;
	accelStr[0] = "Uninitialized";
	posStr[0] = "Uninitialized";
	lastGoodPosStr[0] = "Uninitialized";

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
	Particle.publish("Battery",
		"v:" + String::format("%.2f", battery.getVCell()) +
		", %" + String::format("%.2f", battery.getSoC()),
		60, PRIVATE
		);

	return 1;
 }

 void updatePosition()
 {
	if (tracker.hasGPSFix())
	{
		// "lat, lon, acc, alt, spd, batV, bat%"
		posStr[1] = String::format("Lt %f, Ln %f, Acc +/-%fft, Alt %fft, Spd %f",
			tracker.readLatDeg(),
			tracker.readLonDeg(),
			(((float)tracker.getGpsAccuracy())/304.8), // integer mm to float feet
			tracker.getAltitude()/304.8,             // mm to feet
			tracker.getSpeed());

		lastGoodPosStr[1] = posStr[1];
	}
	else
	{
		posStr[1] = "Unknown";
	}
 }

 boolean publishPositionIfDirty()
 {
	boolean published = false;

	if (posStr[0] != posStr[1])
	{
		if (isPublishing())
		{
			published |= Particle.publish("Pos", posStr[1], 60, PRIVATE);
		}
		else
		{
			published = true;
		}

		Serial.println(posStr[1]);
		posStr[0] = posStr[1];
	}

	if (lastGoodPosStr[0] != lastGoodPosStr[1])
	{
		if (isPublishing())
		{
			published |= Particle.publish("LastGoodPos", lastGoodPosStr[1], 60, PRIVATE);
		}
		else
		{
			published = true;
		}

		Serial.println(lastGoodPosStr[1]);
		lastGoodPosStr[0] = lastGoodPosStr[1];
	}

	return published;
 }

 void updateAcceleration()
 {
	/* int x, y, z;
	tracker.readXYZ(x, y, z);

	int curAccelMagnitude = sqrt((x*x)+(y*y)+(z*z));
	if (maxUnPubAccelMag < curAccelMagnitude)
	{
		maxUnPubAccelMag = curAccelMagnitude;
		accelStr[1] = String::format("%d,%d,%d", x, y, z);
	}*/
 }

 boolean publishAccelerationIfDirty()
 {
	boolean published = false;

	if (accelStr[0] != accelStr[1])
	{
		published = true;
		if (isPublishing())
		{
			published = Particle.publish("Accel", accelStr[1], 60, PRIVATE);
		}

		Serial.println(accelStr[1]);
		accelStr[0] = accelStr[1];
		maxUnPubAccelMag = -1;
	}

	return published;
 }

 void setup()
 {
	Serial.begin(9600);
	//delay(10000);

	for (int x = 0; x < 2; ++x)
	{
		accelStr[x] = "Uninitialized";
		posStr[x] = "Uninitialized";
		lastGoodPosStr[x] = "Uninitialized";
	}

	Particle.function("pubBatt", publishBatteryStatus);
	Particle.function("setGpsRate", setGpsRate);
	Particle.function("publishNow", publishNow);
	Particle.variable("pubRateSec", &publishRateSec, INT);

	tracker.turnOnAccel();
	tracker.turnOnGPS();

	Particle.connect();
 }

 void loop()
 {
	tracker.updateGPS();

	updatePosition();
	updateAcceleration();

	if (isPublishing() &&
		(millis() - lastPublishMillis > static_cast<size_t>(publishRateSec) * 1000))
	{
		bool published = false;

		if (publishPositionIfDirty())
		{
			published = true;
		}

		if (publishAccelerationIfDirty())
		{
			published = true;
		}

		if (published)
		{
			lastPublishMillis = millis();
			shouldPublishNow = false;
		}
	}
 }
