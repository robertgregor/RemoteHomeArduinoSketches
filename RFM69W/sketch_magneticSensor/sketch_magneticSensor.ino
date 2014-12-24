/*
Magnetic sensor sketch

Radio commands (Works only 1st minutes after the power on, if interval is not 0). Please be sure to set the interval, othervise the unit doesn't switch to sleep mode and the battery will be exhausted quickly.

s - return status 4|frequency|battery|enabled|state
	frequency - frequency in seconds to send status. Longer value, longer battery life
	battery voltage e.g. 2.9 or 3.0
        enabled - if enabled asynchronous messages ENABLED / DISABLED are sent. 0 - disabed, 1 - enabled
        state - 0 = open, 1 = closed
m=nnn - set the frequency and put to the sleep mode. 1 means 10 seconds, 254 means 2540 seconds. Needs to be set to put the device to sleep mode!!!
	If not set, or set to 0, then no sleep - usefull for testing, but consume lot of power and batery will be quicly empty.
en - enable the sensor
di - disable the sensor

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
at+en - enable the sensor
at+di - disable the sensor

EEPROM bank
EEPROM_POSITION_NODE_ID 0 //Node Id eeprom position
EEPROM_POSITION_NETWORK_ID 1 //Node Id eeprom position
EEPROM_POSITION_KEY 2 //Node Id eeprom position
EEPROM_SENDING_STATUS_PERIOD 18 //Sending status period position eeprom position
EEPROM_SENSOR_ENABLED_FLAG 19 //Enabled / disabled
*/ 

#include <EEPROM.h>
#include <avr/wdt.h>
#include <LowPower.h>
#include <RFM69.h>
#include <SPI.h>
#include <RemoteHome.h>
 

#define EEPROM_SENDING_STATUS_PERIOD 18 //Sending status period position eeprom position
#define ALIVE_AFTER_STARTUP 60000 //How long after startup the module should go to sleep period.
#define EEPROM_SENSOR_ENABLED_FLAG 19 //Enabled / disabled
#define MOTIONINT     1 //Interrupt 1 on D3
#define INPUTPIN     3 //D3 

RemoteHome remoteHome;
long interval=ALIVE_AFTER_STARTUP;
byte period = 0;
int sleepTimer = 0;
unsigned long previousMillis=0;
byte enabled = 0;
byte motionDetected=0;
boolean processingRadio=false;

void setup() {
  // initialize serial
  Serial.begin(115200);
  Serial.println("Started");
  remoteHome.setup(); 
  period = EEPROM.read(EEPROM_SENDING_STATUS_PERIOD);
  if (period == 255) period = 0;
  enabled = EEPROM.read(EEPROM_SENSOR_ENABLED_FLAG);
  if (enabled == 255) enabled = 0;
  pinMode(INPUTPIN,  INPUT_PULLUP);
  attachInterrupt(MOTIONINT, motionIRQ, CHANGE);
  delay(100);
  previousMillis = millis();
}

void loop() {
  wdt_reset();
  if (remoteHome.processCommonSerialData(Serial)) {
    String lowerCaseInput = remoteHome.inputString;
    lowerCaseInput.toLowerCase();
    if (lowerCaseInput.startsWith("at+s")) {
      remoteHome.printCommonConfig(Serial);
      Serial.print("\tDevice type:MagneticSensor");
      Serial.print("\tPeriod:");
      Serial.print(period, DEC);
      Serial.print("\tBattery:");
      Serial.print(remoteHome.readVcc(), DEC);
      Serial.print("\tEnabled:");
      Serial.print(enabled, DEC);
      Serial.print("\tSensor:");
      Serial.print(digitalRead(INPUTPIN), DEC); 
      Serial.println();
    } else if (lowerCaseInput.startsWith("at+m=")) {
      period = lowerCaseInput.substring(5).toInt();
      EEPROM.write(EEPROM_SENDING_STATUS_PERIOD, period);
      remoteHome.printOK(Serial); 
    } else if (lowerCaseInput.startsWith("at+en")) {
      EEPROM.write(EEPROM_SENSOR_ENABLED_FLAG, 1);
      enabled = 1;
      remoteHome.printOK(Serial);
    } else if (lowerCaseInput.startsWith("at+di")) {
      EEPROM.write(EEPROM_SENSOR_ENABLED_FLAG, 0);
      enabled = 0;
      remoteHome.printOK(Serial); 
    }
    remoteHome.inputString = "";
    remoteHome.stringComplete = false;
  }
  //check and manage radio
  processingRadio = true;
  if (remoteHome.processCommonRadioData()) {
    if ((char)remoteHome.radio.DATA[0] == 's') {
      remoteHome.manageReceivedData();
      String status = getStatus();
      remoteHome.len = status.length();
      status.toCharArray(remoteHome.buff, remoteHome.len+1);       
    } else if (((char)remoteHome.radio.DATA[0] == 'm') && ((char)remoteHome.radio.DATA[1] == '=')) {
      String num = "";
      for (int i=2; i<255; i++) {
          if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
          num = num + (char)remoteHome.radio.DATA[i];
      }
      period = num.toInt();
      EEPROM.write(EEPROM_SENDING_STATUS_PERIOD, period);
      remoteHome.sendOK();
    } else if (((char)remoteHome.radio.DATA[0] == 'e') && ((char)remoteHome.radio.DATA[1] == 'n')) {
      enabled = 1;
      EEPROM.write(EEPROM_SENSOR_ENABLED_FLAG,1);
      remoteHome.sendOK();
    } else if (((char)remoteHome.radio.DATA[0] == 'd') && ((char)remoteHome.radio.DATA[1] == 'i')) {
      enabled = 0;
      EEPROM.write(EEPROM_SENSOR_ENABLED_FLAG,0);
      remoteHome.sendOK(); 
    } else {
      if (remoteHome.radio.DATA[0] != 0) {
        remoteHome.sendERROR();
      }
    }
    remoteHome.manageReceivedData();
  }
  processingRadio = false;
  if ((period != 0) && (((unsigned long)(millis() - previousMillis)) >= interval)) {
    if (interval == ALIVE_AFTER_STARTUP) interval = 200; //OK it is after start, after start it is running 1 minute. After that, it is running 200 ms and then sleep again
    sleepTimer = 0;
    while (1) {
      processingRadio = true;
      remoteHome.radio.sleep();
      processingRadio = false;
      wdt_disable();
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
      remoteHome.radio.sleep();
      LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
      wdt_enable(WDTO_4S);
      if ((period) == (++sleepTimer)) {
        processingRadio = true;
        String status = getStatus();
        remoteHome.len = status.length();
        status.toCharArray(remoteHome.buff, remoteHome.len+1);       
        remoteHome.sendRadioData();
        processingRadio = false;
        previousMillis = millis();
        break; //returns to the main loop
      }
    }
  }
}
void serialEvent() {
  remoteHome.manageSerialEvent(Serial);
}

String getStatus() {
        //Send sensor status
        int bat = remoteHome.readVcc();
        return "4|" + String(period*10, DEC) + "|" + String(bat / 10, DEC) + "." + String(bat % 10, DEC) + "|" + String(enabled, DEC) + "|" + String(digitalRead(INPUTPIN), DEC);
}
void motionIRQ() {
       wdt_enable(WDTO_4S);  
       byte motion = digitalRead(INPUTPIN) + 1;
       if ((enabled == 1) && !processingRadio && (motion != motionDetected)) {
       motionDetected = motion;
        if (motionDetected == 2) {
          //Open
          remoteHome.buff[0] = 'O';
          remoteHome.buff[1] = 'P';
          remoteHome.buff[2] = 'E';
          remoteHome.buff[3] = 'N';
          remoteHome.buff[4] = 'E';
          remoteHome.buff[5] = 'D';
          remoteHome.len = 6;
        } else if (motionDetected == 1) {
          //Close
          remoteHome.buff[0] = 'C';
          remoteHome.buff[1] = 'L';
          remoteHome.buff[2] = 'O';
          remoteHome.buff[3] = 'S';
          remoteHome.buff[4] = 'E';
          remoteHome.buff[5] = 'D';
          remoteHome.len = 6;
        }
        remoteHome.sendRadioData();
        previousMillis = millis() + interval;
       }
}
