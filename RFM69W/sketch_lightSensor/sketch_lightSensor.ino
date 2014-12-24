/*
Light meter sketch

Radio commands (Works only 1st minutes after the power on, if interval is not 0). Please be sure to set the interval, othervise the unit doesn't switch to sleep mode and the battery will be exhausted quickly.

gs - return status 7|light|battery|frequency
	light - the value of the light intensity in the range 0 < 1024. 0 means total dark, 1024 means maximum light.
	battery voltage e.g. 2.9 or 3.0
	frequency - frequency in seconds to send status. Longer value, longer battery life
gm=nnn - set the frequency and put to the sleep mode. 1 means 10 seconds, 254 means 2540 seconds. Needs to be set to put the device to sleep mode!!!
	If not set, or set to 0, then no sleep - usefull for testing, but consume lot of power and batery will be quicly empty.
gcv=nnn - set the switch device Id, where the light intensity should be sent. The range is 0 < nnn < 255.
gcvs=nnn - set the ventilator subdevice device Id, where the light intensity should be sent. The range is 0 < nnn < 9.

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
EEPROM_POSITION_NETWORK_ID 1007 //Network Id eeprom position
EEPROM_POSITION_KEY 1008 //Encryption key eeprom position
EEPROM_SENDING_STATUS_PERIOD 18 //Sending status period position eeprom position
EEPROM_SWITCH_DEVICE_ID 19 //Sub device ID, where to send the sensor.
EEPROM_SWITCH_SUBDEVICE_ID 20 //Sub device ID, where to send the sensor.
*/ 



#include <EEPROM.h>
#include <avr/wdt.h>
#include <LowPower.h>
#include <RFM69.h>
#include <SPI.h>
#include <RemoteHome.h>
 

#define EEPROM_SENDING_STATUS_PERIOD 18 //Sending status period position eeprom position
#define EEPROM_SWITCH_DEVICE_ID 19 //Sub device ID, where to send the sensor.
#define EEPROM_SWITCH_SUBDEVICE_ID 20 //Sub device ID, where to send the sensor.
#define ALIVE_AFTER_STARTUP 60000 //How long after startup the module should go to sleep period.
#define WAIT_BEFORE_SLEEP 100 //How long after startup the module should be up before it will go to sleep again.
#define LIGHT_SENSOR_POWER 5
#define LIGHT_SENSOR_ANALOG A0

RemoteHome remoteHome;
long interval=ALIVE_AFTER_STARTUP;
int period = 0;
byte lightDeviceId = 0;
byte lightSubDeviceId = 0;
int sleepTimer = 0;
unsigned long previousMillis=0;

void setup() {
  wdt_enable(WDTO_2S);
  // initialize serial
  Serial.begin(115200);
  Serial.println("Started");
  remoteHome.setup();
  pinMode(LIGHT_SENSOR_POWER, OUTPUT);
  digitalWrite(LIGHT_SENSOR_POWER, LOW);  
  period = EEPROM.read(EEPROM_SENDING_STATUS_PERIOD);
  if (period == 255) period = 0;
  lightDeviceId = EEPROM.read(EEPROM_SWITCH_DEVICE_ID);
  if (lightDeviceId == 255) lightDeviceId = 0;
  lightSubDeviceId = EEPROM.read(EEPROM_SWITCH_SUBDEVICE_ID);
  if (lightSubDeviceId == 255) lightSubDeviceId = 1;

  delay(100);
}

void loop() {
  wdt_reset();
  if (remoteHome.processCommonSerialData(Serial)) {
    String lowerCaseInput = remoteHome.inputString;
    lowerCaseInput.toLowerCase();
    if (lowerCaseInput.startsWith("at+s")) {
      remoteHome.printCommonConfig(Serial);
      Serial.print("\tDevice type:Light sensor");
      Serial.print("\tPeriod:");
      Serial.print(period*10, DEC);
      Serial.print("\tBattery:");
      Serial.print(remoteHome.readVcc(), DEC);
      Serial.print("\tLight:");
      Serial.print(readLight());    
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
    previousMillis = millis();
    if ((char)remoteHome.radio.DATA[0] == 'g') {
      if ((char)remoteHome.radio.DATA[1] == 's') {
        remoteHome.manageReceivedData();
        String status = getStatus();
        remoteHome.len = status.length();
        status.toCharArray(remoteHome.buff, remoteHome.len+1); 
      } else if (((char)remoteHome.radio.DATA[1] == 'm') && ((char)remoteHome.radio.DATA[2] == '=')) {
        String num = "";
        for (int i=3; i<255; i++) {
            if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
            num = num + (char)remoteHome.radio.DATA[i];
        }
        int recvNum = num.toInt();
        if ((recvNum >= 0) && (recvNum <= 254)) {
          period = recvNum;
          EEPROM.write(EEPROM_SENDING_STATUS_PERIOD, (byte)period);
          remoteHome.sendOK();
        } else {
          remoteHome.sendERROR();
        }
      } else if (((char)remoteHome.radio.DATA[1] == 'c') && ((char)remoteHome.radio.DATA[2] == 'v') && ((char)remoteHome.radio.DATA[3] == '=')) {
        String num = "";
        for (int i=4; i<255; i++) {
            if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
            num = num + (char)remoteHome.radio.DATA[i];
        }
        int recvNum = num.toInt();
        if ((recvNum >= 0) && (recvNum < 254)) {
          lightDeviceId = recvNum;
          EEPROM.write(EEPROM_SWITCH_DEVICE_ID, lightDeviceId);
          remoteHome.sendOK();
        } else {
          remoteHome.sendERROR();
        }        
      } else if (((char)remoteHome.radio.DATA[1] == 'c') && ((char)remoteHome.radio.DATA[2] == 'v') && ((char)remoteHome.radio.DATA[3] == 's') && ((char)remoteHome.radio.DATA[4] == '=')) {        
        String num = "";
        for (int i=5; i<255; i++) {
            if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
            num = num + (char)remoteHome.radio.DATA[i];
        }
        int recvNum = num.toInt();
        if ((recvNum >= 0) && (recvNum < 9)) {
          lightSubDeviceId = recvNum;
          EEPROM.write(EEPROM_SWITCH_SUBDEVICE_ID, lightSubDeviceId);
          remoteHome.sendOK();
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
    if (interval == ALIVE_AFTER_STARTUP) interval = WAIT_BEFORE_SLEEP; //OK it is after start, after start it is running 1 minute. After that, it is running 100 ms and then sleep again
    sleepTimer = 0;
    while (1) {
      remoteHome.radio.sleep();
      wdt_disable();
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
      LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
      wdt_enable(WDTO_2S);
      if ((period) == (++sleepTimer)) {
        if (lightDeviceId > 0) {
          String status = getLightData();
          remoteHome.len = status.length();
          status.toCharArray(remoteHome.buff, remoteHome.len+1); 
          remoteHome.sendRadioData(lightDeviceId);
        }
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
long readLight() {
  int tmp;
  digitalWrite(LIGHT_SENSOR_POWER, HIGH);
  LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF);
  tmp = analogRead(LIGHT_SENSOR_ANALOG);
  digitalWrite(LIGHT_SENSOR_POWER, LOW);
  return 1024 - tmp;
}
String getStatus() {
        //Send sensor status
        int bat = remoteHome.readVcc();
        long lightVal = readLight();
        return "7|" + String(lightVal, DEC) + "|" + String(bat / 10, DEC) + "." + String(bat % 10, DEC) + "|" + String(period*10, DEC) + "|" + String(lightDeviceId, DEC) + "|" + String(lightSubDeviceId, DEC);
}
String getLightData() {
  return "l" + String(lightSubDeviceId, DEC) + "cl=" + String(readLight(),DEC) + "\n"; 
}
