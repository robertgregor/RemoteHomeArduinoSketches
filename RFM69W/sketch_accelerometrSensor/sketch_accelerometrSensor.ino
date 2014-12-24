/*
Pir sensor sketch

Radio commands (Works only 1st minutes after the power on, if interval is not 0). Please be sure to set the interval, othervise the unit doesn't switch to sleep mode and the battery will be exhausted quickly.

s - return status 9|battery|enableMotion|enableOrientation|frequency|accelerationX|accelerationY|accelerationZ|ORIENTATION|BACK|ZTILTLOCKOUT
	battery voltage e.g. 2.9 or 3.0
        enableMotion 0 - disable motion detection, 1 enable motion detection. If enabled, asychronous messages MOTION|X|Y|Z is sent when any motion happen.
        enableOrientation 0 - disable orientation detection, 1 enable orientation detection. If enabled, asychronous messages ORIENTATION|BACK|ZTILTLOCKOUT is sent when any orientation change happen.
	frequency - frequency in seconds to send status. Longer value, longer battery life
        accelerationX|accelerationY|accelerationZ - is the G of the movement in axis X Y Z. The values are float, decimal separator is "."
        ORIENTATION - current orientation. Could be: PU, PD, LR, LL
        BACK - orientation back or normal. Could be: B, N 
        ZTILTLOCKOUT - orientation Z axis. Could be: Z, N
m=nnn - set the frequency and put to the sleep mode. 1 means 10 seconds, 254 means 2540 seconds. Needs to be set to put the device to sleep mode!!!
	If not set, or set to 0, then no sleep - usefull for testing, but consume lot of power and batery will be quicly empty.
enmot - enable motion detection mode. If the orientation detection mode is set, then it is disabled.
enori - enable orientation detection mode. If the motion detection mode is set, then it is disabled.
di    - disable the accelerometer sensor

Asynchronous messages:
MOTION|X|Y|Z|ORIENTATION|BACK|ZTILTLOCKOUT is sent, when motion is detected. X Y Z is the G of the movement in axis X Y Z
Orientation change values:
                            ORIENTATION:
               			"PU" - portrait up
       			        "PD" - portrait down
       		        	"LR" - landscape right
              			"LL" - landscape left
                            BACK:
                                "B" - back
                                "N" - normal
                            ZTILTLOCKOUT:
                                "Z" - ztiltlockout
                                "N" - normal
ORIENTATION|X|Y|Z|ORIENTATION|BACK|ZTILTLOCKOUT is sent, when orientation change is detected. X Y Z is the G of the movement in axis X Y Z
Orientation change values:
                            ORIENTATION:
               			"PU" - portrait up
       			        "PD" - portrait down
       		        	"LR" - landscape right
              			"LL" - landscape left
                            BACK:
                                "B" - back
                                "N" - normal
                            ZTILTLOCKOUT:
                                "Z" - ztiltlockout
                                "N" - normal

pn - check weather the device is up and runing (ping command) - returns OK
nn - check weather the device is up and runing (ping command) - returns nothing - used by the transceiver to measure the RSSI.
res - reset the device
def - reset the device and place the EEPROM device to factory default

Serial commands:
at+c=XXX Configure the channel - Network. Receiver should have the same channel than all the devices. There could be channels 1 >= XXX >= 254
at+n=XXX Configure the node Id - Network. Node ID should be unique and identify the node in the network. There could be channels 1 >= XXX >= 254. The receiver has always nodeId = 255.
at+res Reset the device
at+def Reset the device and set the factory default values of EEPROM
at+memid=XXX To set the eeprom value. This command preset the address
at+memval=XXX This command set the XXX value at the position memid in the EEPROM
at+p=thisIsEncryptKey - for AES encryption, this command set the symetric key for encryption between the transceiver and the device. Should be exactly 16 characters
at+s - print the status of the device. Values are separated by \t character.
at+m=XXX - set the frequency and put to the sleep mode. 1 means 10 seconds, 254 means 2540 seconds. Needs to be set to put the device to sleep mode!!!
	   If not set, or set to 0, then no sleep - usefull for testing, but consume lot of power and batery will be quicly empty.

EEPROM bank
EEPROM_POSITION_NODE_ID 1006 //Node Id eeprom position
EEPROM_POSITION_NETWORK_ID 1007 //Node Id eeprom position
EEPROM_POSITION_KEY 1008 //Node Id eeprom position
EEPROM_SENDING_STATUS_PERIOD 18 //Sending status period position eeprom position
EEPROM_ENABLE_ORIENTATION 19 //Enable orientation change eeprom position
EEPROM_ENABLE_MOTION 20 //Enable motion detection mode eeprom position
*/

#include <EEPROM.h>
#include <PinChangeInt.h>
#include <LowPower.h>
#include <RFM69.h>
#include <SPI.h>
#include <RemoteHome.h>
#include <avr/wdt.h>
#include <Wire.h>
#include <MMA8452.h>

#define ARDUINO 1
#define EEPROM_SENDING_STATUS_PERIOD 18 //Sending status period position eeprom position
#define EEPROM_ENABLE_ORIENTATION 19 //Enable orientation change eeprom position
#define EEPROM_ENABLE_MOTION 20 //Enable motion detection mode eeprom position
#define ALIVE_AFTER_STARTUP 60000 //How long after startup the module should go to sleep period.
#define ACC_INT2 3 //INT0 MMA connected to pin 3
#define ACC_INT1 4 //INT1 MMA connected to pin 4
#define ACCELEROMETER_SENSOR_POWER 6
RemoteHome remoteHome;
long interval=ALIVE_AFTER_STARTUP;
byte period = 0;
int sleepTimer = 0;
unsigned long previousMillis=0;
MMA8452 accelerometer; 
String oldOrientation = "";
volatile boolean interrupt = false;
volatile boolean firstInterrupt = false;
int enableMotion = 0;
int enableOri = 0;

void setup() {
  // initialize serial
  Serial.begin(115200);
  pinMode(ACCELEROMETER_SENSOR_POWER, OUTPUT);
  digitalWrite(ACCELEROMETER_SENSOR_POWER, HIGH);
  pinMode(ACC_INT1, INPUT);
  remoteHome.setup();
  Wire.begin();
  period = EEPROM.read(EEPROM_SENDING_STATUS_PERIOD);
  if (period == 255) period = 0;
  if (accelerometer.init()) {                
		accelerometer.setPowerMode(MMA_HIGH_RESOLUTION);
		accelerometer.setRange(MMA_RANGE_2G);
		accelerometer.setAutoSleep(true, 0x11, MMA_SLEEP_50hz);
		accelerometer.setWakeOnInterrupt();
		accelerometer.setMotionDetectionMode(MMA_MOTION, MMA_ALL_AXIS);
		accelerometer.setMotionTreshold(0x11);
                accelerometer.enableOrientationChange(true, true);
		accelerometer.configureInterrupts(false, false);
		accelerometer.setInterruptPins(true, true, true, true, true, true);
                accelerometer.setInterruptsEnabled(0);
                PCintPort::attachInterrupt(ACC_INT1, accelerometerInterruptHandler, FALLING);
                enableMotion = EEPROM.read(EEPROM_ENABLE_MOTION);
                if (enableMotion == 255) enableMotion = 0;
                if (enableMotion == 1) {
                    firstInterrupt = true;
                    accelerometer.setInterruptsEnabled(MMA_FREEFALL_MOTION);                  
                }
                enableOri = EEPROM.read(EEPROM_ENABLE_ORIENTATION);
                if (enableOri == 255) enableOri = 0;
                if (enableOri == 1) {
                    accelerometer.setInterruptsEnabled(MMA_ORIENTATION_CHANGE);                  
                }
                Serial.println("Started");
  } else {
		Serial.println("Failed to start accelerometer.");
  } 
}

void sendStatus() {
              // Send the status
              String status = getStatus();
              remoteHome.len = status.length();
              status.toCharArray(remoteHome.buff, remoteHome.len+1);
              remoteHome.sendRadioData();                                            
} 

void loop() {
  wdt_reset();
  processInterrupt();
  if (remoteHome.processCommonSerialData(Serial)) {
    String lowerCaseInput = remoteHome.inputString;
    lowerCaseInput.toLowerCase();
    if (lowerCaseInput.startsWith("at+s")) {
      remoteHome.printCommonConfig(Serial);
      Serial.print("\tDevice type:Accelerometer");
      Serial.print("\tPortrait:");
      Serial.print(getOrientation());
      Serial.print("\tAcceleration:");
      Serial.print(getAccXYZ());
      Serial.println();
    } else if (lowerCaseInput.startsWith("at+m=")) {
      period = lowerCaseInput.substring(5).toInt();
      EEPROM.write(EEPROM_SENDING_STATUS_PERIOD, period);
      remoteHome.printOK(Serial); 
    }  
    remoteHome.inputString = "";
    remoteHome.stringComplete = false;
  }
  //check and manage radio
  if (remoteHome.processCommonRadioData()) {
    if ((char)remoteHome.radio.DATA[0] == 's') {
        sendStatus();
    } else if (((char)remoteHome.radio.DATA[0] == 'e') && ((char)remoteHome.radio.DATA[1] == 'n') && ((char)remoteHome.radio.DATA[2] == 'm') && ((char)remoteHome.radio.DATA[3] == 'o') && ((char)remoteHome.radio.DATA[4] == 't')) {
        firstInterrupt = true;
        accelerometer.setInterruptsEnabled(MMA_FREEFALL_MOTION);
        EEPROM.write(EEPROM_ENABLE_MOTION,1);
        EEPROM.write(EEPROM_ENABLE_ORIENTATION,0);
        enableMotion = 1;
        enableOri = 0;
        remoteHome.sendOK();
        remoteHome.manageReceivedData();
        sendStatus();
    } else if (((char)remoteHome.radio.DATA[0] == 'd') && ((char)remoteHome.radio.DATA[1] == 'i')) {
        accelerometer.setInterruptsEnabled(0);
        EEPROM.write(EEPROM_ENABLE_MOTION,0);
        EEPROM.write(EEPROM_ENABLE_ORIENTATION,0);
        enableMotion = 0;
        enableOri = 0;
        remoteHome.sendOK(); 
        remoteHome.manageReceivedData();
        sendStatus();
    } else if (((char)remoteHome.radio.DATA[0] == 'e') && ((char)remoteHome.radio.DATA[1] == 'n') && ((char)remoteHome.radio.DATA[2] == 'o') && ((char)remoteHome.radio.DATA[3] == 'r') && ((char)remoteHome.radio.DATA[4] == 'i')) {
        accelerometer.setInterruptsEnabled(MMA_ORIENTATION_CHANGE);
        EEPROM.write(EEPROM_ENABLE_MOTION,0);
        EEPROM.write(EEPROM_ENABLE_ORIENTATION,1);
        enableMotion = 0;
        enableOri = 1;
        remoteHome.sendOK();  
        remoteHome.manageReceivedData();
        sendStatus();
    } else if (((char)remoteHome.radio.DATA[0] == 'm') && ((char)remoteHome.radio.DATA[1] == '=')) {
      String num = "";
      for (int i=2; i<255; i++) {
          if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
          num = num + (char)remoteHome.radio.DATA[i];
      }
      int recvNum = num.toInt();
      if ((recvNum >= 0) && (recvNum <= 254)) {
        period = recvNum;
        EEPROM.write(EEPROM_SENDING_STATUS_PERIOD, period);
        remoteHome.sendOK();    
        remoteHome.manageReceivedData();
        sendStatus();
      } else {
        remoteHome.sendERROR();
      }
    } else {
        remoteHome.sendERROR();
    }
    remoteHome.manageReceivedData();
  }
  if ((period != 0) && (((unsigned long)(millis() - previousMillis)) >= interval)) {
    if (interval == ALIVE_AFTER_STARTUP) interval = 200; //OK it is after start, after start it is running 1 minute. After that, it is running 200 ms and then sleep again
    sleepTimer = 0;
    while (1) {
      remoteHome.radio.sleep();
      wdt_disable();
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
      if (interrupt) {
        if (sleepTimer > 0) --sleepTimer;
      }
      processInterrupt();
      remoteHome.radio.sleep();
      LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
      processInterrupt();
      wdt_enable(WDTO_4S);
      if ((period) == (++sleepTimer)) {
        String status = getStatus();
        remoteHome.len = status.length();
        status.toCharArray(remoteHome.buff, remoteHome.len+1); 
        remoteHome.sendRadioData(); 
        previousMillis = millis();
        break; //returns to the main loop
      }
    }
  }
}
void serialEvent() {
  remoteHome.manageSerialEvent(Serial);
}

void accelerometerInterruptHandler() {
    interrupt = true;
}
void processInterrupt() {
    while (interrupt) {
     noInterrupts();
     interrupt = false;
     interrupts();
		bool wakeStateChanged;
		bool transient;
		bool landscapePortrait;
		bool tap;
		bool freefallMotion;
		bool dataReady;
                String transientData = "";
                String orientationData = "";
                String tapData = "";
                String motionData = "";
                String readyData = "";
		accelerometer.getInterruptEvent(&wakeStateChanged, &transient, &landscapePortrait, &tap, &freefallMotion, &dataReady);
                if (wakeStateChanged) {
                        mma8452_mode_t mode = accelerometer.getMode();
		}
                if (transient) {
			transientData = "TRANSIENT|" + getAccXYZ() + "|" + getOrientation();
		}
                if (landscapePortrait) {
                  orientationData = "ORIENTATION|" + getAccXYZ() + "|" + getOrientation();
                  if (oldOrientation != orientationData) {
                      oldOrientation = orientationData;
                  } else {
                      // it is the same, do not send anything
                      orientationData = "";
                  }
                }
                if (tap) {
                      bool singleTap;
                      bool doubleTap;
	              bool x;
	              bool y;
	              bool z;
	              bool negX;
	              bool negY;
	              bool negZ;
	              accelerometer.getTapDetails(&singleTap, &doubleTap, &x, &y, &z, &negX, &negY, &negZ);
                      if (singleTap || doubleTap) {
		        if (doubleTap) {
                          tapData =  "DOUBLETAP|";
                        } else {
                          tapData =  "SINGLETAP|";
                        }
		        if (x) {
                                tapData = tapData + "X|" + (negX ? "LEFT|" : "RIGHT");
		        }
                	if (y) {
                                tapData = tapData + "Y|" + (negY ? "DOWN|" : "UP");
		        }
		        if (z) {
                                tapData = tapData + "Z|" + (negZ ? "OUT|" : "IN");
		        }
	              }
                }
                if (freefallMotion) {
                      motionData = "MOTION|" + getAccXYZ()+"|"+getOrientation();
                }
                if (dataReady) {
                      readyData = "ACCDATA|" + getAccXYZ()+"|"+getOrientation();
                }
                if (firstInterrupt) {
                  //after enabling accelerometer interrupts, it is always raised. So ignore the first one.
                  firstInterrupt = false;
                  return;
                }
                sendInterruptData(transientData);
                sendInterruptData(orientationData);
                sendInterruptData(tapData);
                sendInterruptData(motionData);
                sendInterruptData(readyData);
    }
}
void sendInterruptData(String &data) {
                if (data.length() != 0) {
                    //Serial.println(data);
                    remoteHome.len = data.length();
                    data.toCharArray(remoteHome.buff, remoteHome.len+1); 
                    remoteHome.sendRadioData();
                    data = "";
                }  
}
String getAccXYZ() {
                      float x, y, z;
                      accelerometer.getAcceleration(&x, &y, &z);
                      char arrayX[10];
                      dtostrf(x, 1, 5, arrayX);  
                      char arrayY[10];
                      dtostrf(y, 1, 5, arrayY);  
                      char arrayZ[10];
                      dtostrf(z, 1, 5, arrayZ);  
                      return String(arrayX) + "|" + String(arrayY) + "|" + String(arrayZ);
}
String getOrientation() {
                String orientationData = "";
                bool orientationChanged;
               	bool zTiltLockout;
              	mma8452_orientation_t orientation;
               	bool back; 
                accelerometer.getPortaitLandscapeStatus(&orientationChanged, &zTiltLockout, &orientation, &back);
              		switch (orientation) {
       			    case MMA_PORTRAIT_UP:
               			orientationData = "PU";
        			break;
	        	    case MMA_PORTRAIT_DOWN:
       			        orientationData = "PD";
       				break;
        		    case MMA_LANDSCAPE_RIGHT:
       		        	orientationData = "LR";
			        break;
       			    case MMA_LANDSCAPE_LEFT:
              			orientationData = "LL";
	        		break;
                        }
                        if (back) orientationData += "|B"; else orientationData += "|N";
                        if (zTiltLockout) orientationData += "|Z"; else orientationData += "|N";
                return orientationData; 
}
String getStatus() {
        //Send sensor status
        int bat = remoteHome.readVcc();
        return "9|" + String(bat / 10, DEC) + "." + String(bat % 10, DEC) + "|" + String(enableMotion, DEC) + "|" + String(enableOri, DEC) + "|" + String(period*10, DEC) + "|" + getAccXYZ() + "|" + getOrientation(); 
}

