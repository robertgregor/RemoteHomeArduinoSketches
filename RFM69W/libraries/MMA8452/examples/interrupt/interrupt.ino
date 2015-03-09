#include <Wire.h>
#include <MMA8452.h>
#include <avr/wdt.h>
#include <SimpleTimer.h>
#include <PinChangeInt.h>

MMA8452 accelerometer;
volatile bool interrupt;

void setup()
{
        pinMode(6, OUTPUT); 
        digitalWrite(6, HIGH);
	Serial.begin(115200);
	Serial.print("Initializing MMA8452Q: ");
	pinMode(4, INPUT);
        pinMode(3, INPUT);
	Wire.begin();
	bool initialized = accelerometer.init(); 
	
	if (initialized)
	{
		Serial.println("ok");

		// config
		accelerometer.setDataRate(MMA_1_56hz);	// 800hz doesn't trigger an interrupt
		accelerometer.setRange(MMA_RANGE_2G);
		accelerometer.setInterruptsEnabled(MMA_DATA_READY);
		accelerometer.configureInterrupts(false, false);
		accelerometer.setInterruptPins(true, true, true, true, true, true);
                PCintPort::attachInterrupt(3, accelerometerInterruptHandler, FALLING); 
                PCintPort::attachInterrupt(4, accelerometerInterruptHandler2, FALLING); 
	}
	else
	{
		Serial.println("failed. Check connections.");
		while (true) {};
	}
}

void loop()
{
	if (interrupt)
	{
                Serial.println("Interrupt");
		interrupt = false;
		bool wakeStateChanged;
		bool movementOccurred;
		bool landscapePortrait;
		bool tap;
		bool freefall;
		bool dataReady;
		accelerometer.getInterruptEvent(&wakeStateChanged, &movementOccurred, &landscapePortrait, &tap, &freefall, &dataReady);

		// if (dataReady)
		// {
			// clear flag so new data will be read
			uint16_t x, y, z;
			accelerometer.getRawData(&x, &y, &z);

			cli();
                        Serial.print(x, DEC);
                        Serial.print(y, DEC);
                        Serial.print(z, DEC);
			sei();
		// }
	}
}

void accelerometerInterruptHandler()
{
	interrupt = true;
}
void accelerometerInterruptHandler2()
{
	interrupt = true;
}
