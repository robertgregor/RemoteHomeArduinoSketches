/*
Pir sensor sketch

Radio commands (Works only 1st minutes after the power on, if interval is not 0). Please be sure to set the interval, othervise the unit doesn't switch to sleep mode and the battery will be exhausted quickly.

ps - return status 3|frequency|battery|enabled|state|switchDeviceId|switchSubDeviceId
	frequency - frequency in seconds to send status. Longer value, longer battery life
	battery voltage e.g. 2.9 or 3.0
        enabled - if enabled asynchronous message MOTION is sent, if the motion is detected. 0 - means, that this feature is disabled, 1 means that this feature is enabled. If 0, it is saving battery
        state - 0 = no movement, 1 = movement
        switchDeviceId - Device ID of the simple switch device, which should be switched on.
        switchSubDeviceId - SubDevice ID of the simple switch device, which should be switched on.
pm=nnn - set the frequency and put to the sleep mode. 1 means 10 seconds, 254 means 2540 seconds. Needs to be set to put the device to sleep mode!!!
	If not set, or set to 0, then no sleep - usefull for testing, but consume lot of power and batery will be quicly empty.
pcv=nnn - set the simple switch device Id, where the sensor detection is sent. The range is 0 < nnn < 255.
pcvs=nnn - set the simple switch sub device Id, where the sensor detection is sent. The range is 0 < nnn < 9.

pen - enable the sensor
pdi - disable the sensor

Asynchronous messages:
MOTION

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
EEPROM_POSITION_NODE_ID 1006 //Node Id eeprom position
EEPROM_POSITION_NETWORK_ID 1007 //Node Id eeprom position
EEPROM_POSITION_KEY 1008 //Node Id eeprom position
EEPROM_SENDING_STATUS_PERIOD 18 //Sending status period position eeprom position
EEPROM_SENSOR_ENABLED_FLAG 19 //Enabled / disabled
EEPROM_SWITCH_DEVICE_ID 20 //Sub device ID, where to send the sensor.
EEPROM_SWITCH_SUBDEVICE_ID 21 //Sub device ID, where to send the sensor.
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
#define EEPROM_SWITCH_DEVICE_ID 20 //Sub device ID, where to send the sensor.
#define EEPROM_SWITCH_SUBDEVICE_ID 21 //Sub device ID, where to send the sensor.
#define INTERRUPT_DELAY_COUNTER 12
#define MOTIONINT    1 //Interrupt 1 on D3
#define INPUTPIN     3 //D3
#define SENSOR_POWER 4 //D4

RemoteHome remoteHome;
long interval=ALIVE_AFTER_STARTUP;
byte period = 0;
int sleepTimer = 0;
unsigned long previousMillis=0;
byte enabled = 0;
boolean interrupt = false;
byte pirSensorSwitchDeviceId=0;
byte pirSensorSwitchSubDeviceId=1;
byte interruptDelayCounter = 0;

void setup() {
  // initialize serial
  Serial.begin(115200);
  Serial.println("Started");
  remoteHome.setup(); 
  period = EEPROM.read(EEPROM_SENDING_STATUS_PERIOD);
  if (period == 255) period = 0;
  enabled = EEPROM.read(EEPROM_SENSOR_ENABLED_FLAG);
  if (enabled == 255) enabled = 0;
  pinMode(SENSOR_POWER, OUTPUT);
  if (enabled == 1) digitalWrite(SENSOR_POWER, HIGH);
  pinMode(INPUTPIN, INPUT);
  pirSensorSwitchDeviceId = EEPROM.read(EEPROM_SWITCH_DEVICE_ID);
  if (pirSensorSwitchDeviceId == 255) pirSensorSwitchDeviceId = 0;
  if (pirSensorSwitchDeviceId != 0) digitalWrite(SENSOR_POWER, HIGH);
  pirSensorSwitchSubDeviceId = EEPROM.read(EEPROM_SWITCH_SUBDEVICE_ID);
  if (pirSensorSwitchSubDeviceId == 255) pirSensorSwitchSubDeviceId = 1;
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
      Serial.print("\tDevice type:PirSensor");
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
      digitalWrite(SENSOR_POWER, HIGH);
      remoteHome.printOK(Serial);
    } else if (lowerCaseInput.startsWith("at+di")) {
      EEPROM.write(EEPROM_SENSOR_ENABLED_FLAG, 0);
      enabled = 0;
      digitalWrite(SENSOR_POWER, LOW);
      remoteHome.printOK(Serial); 
    }
    remoteHome.inputString = "";
    remoteHome.stringComplete = false;
  }
  //check and manage radio
  if (remoteHome.processCommonRadioData()) {
    if ((char)remoteHome.radio.DATA[0] == 'p') {
      if ((char)remoteHome.radio.DATA[1] == 's') {
        remoteHome.manageReceivedData();
        sendPirSensorStatus();
      } else if (((char)remoteHome.radio.DATA[1] == 'm') && ((char)remoteHome.radio.DATA[2] == '=')) {
        int  num = getNumberFromInputChar(3);
        if ((num > 0) && (num <= 255)) {
          period = num;
          EEPROM.write(EEPROM_SENDING_STATUS_PERIOD, period);
          remoteHome.sendOK();
          remoteHome.manageReceivedData();
          sendPirSensorStatus();
        } else {
          remoteHome.sendERROR();
        }
      } else if (((char)remoteHome.radio.DATA[1] == 'e') && ((char)remoteHome.radio.DATA[2] == 'n')) {
        enabled = 1;
        digitalWrite(SENSOR_POWER, HIGH);
        EEPROM.write(EEPROM_SENSOR_ENABLED_FLAG,1);
        remoteHome.sendOK();
        remoteHome.manageReceivedData();
        sendPirSensorStatus();
      } else if (((char)remoteHome.radio.DATA[1] == 'd') && ((char)remoteHome.radio.DATA[2] == 'i')) {
        enabled = 0;
        if (pirSensorSwitchDeviceId == 0) digitalWrite(SENSOR_POWER, LOW);
        EEPROM.write(EEPROM_SENSOR_ENABLED_FLAG,0);
        remoteHome.sendOK(); 
        remoteHome.manageReceivedData();
        sendPirSensorStatus();
      } else if (((char)remoteHome.radio.DATA[1] == 'l') && ((char)remoteHome.radio.DATA[2] == '=')) {
        int  recvNum = getNumberFromInputChar(3);
        if ((recvNum >= 0) && (recvNum < 255)) {
          pirSensorSwitchDeviceId = recvNum;
          EEPROM.write(EEPROM_SWITCH_DEVICE_ID, pirSensorSwitchDeviceId);
          if ((pirSensorSwitchDeviceId == 0) && (enabled == 0)) digitalWrite(SENSOR_POWER, LOW);
          if (pirSensorSwitchDeviceId != 0) digitalWrite(SENSOR_POWER, HIGH);
          remoteHome.sendOK();        
          remoteHome.manageReceivedData();
          sendPirSensorStatus();
        } else {
          remoteHome.sendERROR();
        }
      } else if (((char)remoteHome.radio.DATA[1] == 'l') && ((char)remoteHome.radio.DATA[2] == 's') && ((char)remoteHome.radio.DATA[3] == '=')) {
        int  recvNum = getNumberFromInputChar(4);
        String num = "";
        if ((recvNum > 0) && (recvNum < 9)) {
          pirSensorSwitchSubDeviceId = recvNum;
          EEPROM.write(EEPROM_SWITCH_SUBDEVICE_ID, pirSensorSwitchSubDeviceId);
          remoteHome.sendOK();
          remoteHome.manageReceivedData();
          sendPirSensorStatus();          
        } else {
          remoteHome.sendERROR();
        }
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
        wdt_enable(WDTO_4S);
        processPirInterrupt();        
        remoteHome.radio.sleep();
        wdt_disable();
      }
      LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
      wdt_enable(WDTO_4S);
      if (interruptDelayCounter-- == 0) {
        interruptDelayCounter = INTERRUPT_DELAY_COUNTER;
        attachInterrupt(MOTIONINT, motionIRQ, RISING);
        interrupt = false;
      }
      if (interrupt) {
        processPirInterrupt();
      }      
      if ((period) == (++sleepTimer)) {
        sendPirSensorStatus();
        previousMillis = millis();
        break; //returns to the main loop
      }
    }
  }
}
void sendPirSensorStatus() {
        String status = getPirStatus();
        remoteHome.len = status.length();
        status.toCharArray(remoteHome.buff, remoteHome.len+1);       
        remoteHome.sendRadioData();
}
int getNumberFromInputChar(int startPosition) {
        String num = "";
        for (int i=startPosition; i<255; i++) {
           if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
           num = num + (char)remoteHome.radio.DATA[i];
        }
        return num.toInt();

}
void serialEvent() {
  remoteHome.manageSerialEvent(Serial);
}
String getPirStatus() {
        //Send sensor status
        int bat = remoteHome.readVcc();
        return "3|" + String(period*10, DEC) + "|" + String(bat / 10, DEC) + "." + String(bat % 10, DEC) + "|" + String(enabled, DEC) + "|" + String(digitalRead(INPUTPIN), DEC) + "|" + String(pirSensorSwitchDeviceId, DEC) + "|" + String(pirSensorSwitchSubDeviceId, DEC);
}
void motionIRQ() {
  interrupt = true;
}
void processPirInterrupt() {
       detachInterrupt(MOTIONINT);
       interrupt = false;       
       interruptDelayCounter = INTERRUPT_DELAY_COUNTER;
       if (enabled == 1) {          
          remoteHome.buff[0] = 'M';
          remoteHome.buff[1] = 'O';
          remoteHome.buff[2] = 'T';
          remoteHome.buff[3] = 'I';
          remoteHome.buff[4] = 'O';
          remoteHome.buff[5] = 'N';
          remoteHome.len = 6;
          remoteHome.sendRadioData();
          previousMillis = millis() + interval;
       }
       if (pirSensorSwitchDeviceId != 0) {
              String status = "l" + String(pirSensorSwitchSubDeviceId, DEC) + "of" + 10;
              remoteHome.len = status.length();
              status.toCharArray(remoteHome.buff, remoteHome.len+1); 
              remoteHome.sendRadioData(pirSensorSwitchDeviceId);
       }
}
