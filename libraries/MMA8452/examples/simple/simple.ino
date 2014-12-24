#include <Wire.h>
#include <MMA8452.h>

MMA8452 accelerometer;


void setup()
{
        pinMode(6, OUTPUT); 
        digitalWrite(6, HIGH);
	Serial.begin(115200);
	Serial.print("Initializing MMA8452Q: ");
        digitalWrite();
	Wire.begin();

	bool initialized = accelerometer.init(); 
	
	if (initialized)
	{
		Serial.println("ok");

		accelerometer.setDataRate(MMA_100hz);
		accelerometer.setRange(MMA_RANGE_2G);
	}
	else
	{
		Serial.println("failed. Check connections.");
		while (true) {};
	}
}

void loop()
{
	float x;
	float y;
	float z;

	accelerometer.getAcceleration(&x, &y, &z);

	Serial.print(x, 5);
	Serial.print(F(" "));
	Serial.print(y, 5);
	Serial.print(F(" "));
	Serial.print(z, 5);
	Serial.println();
	delay(1000);
}
