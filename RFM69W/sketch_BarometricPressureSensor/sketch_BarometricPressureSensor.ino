/*
BarometricPressure meter sketch

Radio commands (Works only 1st minute after the power on, if interval is not 0). Please be sure to set the interval, othervise the unit doesn't switch to sleep mode and the battery will be exhausted quickly.

s - return status 13|absPressurehPa|relativePressurehPa|temperature|configuredAltitude|battery|frequency
	 absolute pressure in mb (hPa)
         relative pressure in mb (hPa) (The relative pressure depends on the altitude, which has to be measured using GPS and configured to the sensor.)
         temperature in degrees Celsius
         configured altitude of the device to compute relative pressure. If 0, relative pressure is also 0.
	 battery voltage e.g. 2.9 or 3.0
	 frequency - frequency in seconds to send status. Longer value, longer battery life
m=nnn -  set the frequency and put to the sleep mode. 1 means 10 seconds, 254 means 2540 seconds. Needs to be set to put the device to sleep mode!!!
	 If not set, or set to 0, then no sleep - usefull for testing, but consume lot of power and batery will be quicly empty.
ca=nnn - set the altitude of the device to be able to measure the the relative pressure. Value is in meters under the sea level.

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
EEPROM_ALTITUDE 19 //Altitude in meters, 2 bytes
*/ 



#include <EEPROM.h>
#include <avr/wdt.h>
#include <LowPower.h>
#include <RFM69.h>
#include <SPI.h>
#include <RemoteHome.h>
#include <Wire.h>
#include <SFE_BMP180.h>
 

#define EEPROM_SENDING_STATUS_PERIOD 18 //Sending status period position eeprom position
#define EEPROM_ALTITUDE 19 //Altitude in meters, 2 bytes
#define ALIVE_AFTER_STARTUP 60000 //How long after startup the module should go to sleep period.
#define WAIT_BEFORE_SLEEP 100 //How long after startup the module should be up before it will go to sleep again.
#define BAROMETRIC_SENSOR_POWER 6
#define DHT12_SENSOR_POWER 3
#define DHT12_SENSOR 4

RemoteHome remoteHome;
SFE_BMP180 pressure;
long interval=ALIVE_AFTER_STARTUP;
byte period = 0;
unsigned int altitude;
int sleepTimer = 0;
unsigned long previousMillis=0;

void setup() {
  wdt_enable(WDTO_2S);
  // initialize serial
  Serial.begin(115200);
  pinMode(BAROMETRIC_SENSOR_POWER, OUTPUT);
  pinMode(DHT12_SENSOR_POWER, OUTPUT);
  pinMode(DHT12_SENSOR, OUTPUT);
  digitalWrite(DHT12_SENSOR_POWER, LOW);
  digitalWrite(DHT12_SENSOR, LOW);
  digitalWrite(BAROMETRIC_SENSOR_POWER, HIGH);
  LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);  
  boolean initOK = pressure.begin();
  digitalWrite(BAROMETRIC_SENSOR_POWER, LOW);  
  if (initOK) {
    Serial.println("Started");
  } else {
    Serial.println("Initialization of sensor failed.");
  }
  remoteHome.setup();
  period = EEPROM.read(EEPROM_SENDING_STATUS_PERIOD);
  if (period == 255) period = 0;
  altitude = (EEPROM.read(EEPROM_ALTITUDE)*256) + (EEPROM.read(EEPROM_ALTITUDE+1));
  if (altitude == 65535) altitude = 0;
  delay(100);
}

void loop() {
  wdt_reset();
  if (remoteHome.processCommonSerialData(Serial)) {
    String lowerCaseInput = remoteHome.inputString;
    lowerCaseInput.toLowerCase();
    if (lowerCaseInput.startsWith("at+s")) {
      double temperature;
      double absPressure;
      double relPressure = 0;
      digitalWrite(BAROMETRIC_SENSOR_POWER, HIGH);
      LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
      temperature = readTemperature();
      absPressure = readPressure(temperature);        
      if (altitude != 0) {
         relPressure = pressure.sealevel(absPressure,altitude);
      }  
      digitalWrite(BAROMETRIC_SENSOR_POWER, LOW);
      remoteHome.printCommonConfig(Serial);
      Serial.print("\tDevice type:BarometricSensor");
      Serial.print("\tPeriod:");
      Serial.print(period*10, DEC);
      Serial.print("\tAltitude:");
      Serial.print(altitude, DEC);
      Serial.print("\tBattery:");
      Serial.print(remoteHome.readVcc(), DEC);
      Serial.print("\tAbsolute pressure:");
      Serial.print(absPressure, 5);     
      Serial.print("\tRelative pressure:");
      Serial.print(relPressure, 5);     
      Serial.print("\tTemperature:");
      Serial.print(temperature, 2);     
      Serial.println();
    } else if (lowerCaseInput.startsWith("at+m=")) {
      period = lowerCaseInput.substring(5).toInt();
      EEPROM.write(EEPROM_SENDING_STATUS_PERIOD, period);
      remoteHome.printOK(Serial);
    } else if (lowerCaseInput.startsWith("at+ca=")) {
      altitude = lowerCaseInput.substring(6).toInt();
      EEPROM.write(EEPROM_ALTITUDE, highByte(altitude));
      EEPROM.write(EEPROM_ALTITUDE+1, lowByte(altitude));
      remoteHome.printOK(Serial);
    }
    remoteHome.inputString = "";
    remoteHome.stringComplete = false;
  }
  //check and manage radio
  if (remoteHome.processCommonRadioData()) {
    previousMillis = millis();
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
      int recvNum = num.toInt();
      if ((recvNum >= 0) && (recvNum <= 254)) {
        period = recvNum;
        EEPROM.write(EEPROM_SENDING_STATUS_PERIOD, period);
        remoteHome.sendOK();
      } else {
        remoteHome.sendERROR();
      }
    } else if (((char)remoteHome.radio.DATA[0] == 'c') && ((char)remoteHome.radio.DATA[1] == 'a') && ((char)remoteHome.radio.DATA[2] == '=')) {
      String num = "";
      for (int i=2; i<255; i++) {
          if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
          num = num + (char)remoteHome.radio.DATA[i];
      }
      int recvNum = num.toInt();
      if ((recvNum >= 0) && (recvNum <= 65535)) {
        altitude = recvNum;
        EEPROM.write(EEPROM_ALTITUDE, highByte(altitude));
        EEPROM.write(EEPROM_ALTITUDE+1, lowByte(altitude));
        remoteHome.sendOK();
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
double readTemperature() {
  char status;
  double temperature;
  status = pressure.startTemperature();
  if (status != 0) {
    // Wait for the measurement to complete:
    delay(status);
    status = pressure.getTemperature(temperature);
    if (status==0) {
      //temperature reading failed
      temperature = 255;
    }
  }
  return temperature;
}
double readPressure(double temperature) {
  char status;
  double pressureVal;
  status = pressure.startPressure(3);
  if (status != 0) {
    // Wait for the measurement to complete:
    delay(status);
    status = pressure.getPressure(pressureVal, temperature);
    if (status==0) {
      //pressure reading failed
      pressureVal = 255;
    }
  }
  return pressureVal;
}
String getStatus() {
        //Send sensor status        
        double temperature;
        double absPressure;
        double relPressure = 0;
        digitalWrite(BAROMETRIC_SENSOR_POWER, HIGH);
        LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
        temperature = readTemperature();
        absPressure = readPressure(temperature);        
        if (altitude != 0) {
           relPressure = pressure.sealevel(absPressure,altitude);
        }  
        digitalWrite(BAROMETRIC_SENSOR_POWER, LOW);
        char temperatureArray[6];
        dtostrf(temperature, 2, 1, temperatureArray);
        char absPressureArray[9];
        dtostrf(absPressure, 5, 3, absPressureArray);
        char relPressureArray[9];
        dtostrf(relPressure, 5, 3, relPressureArray);
        char batVoltage[4];
        dtostrf(remoteHome.readVcc()/10.0, 1, 1, batVoltage);
        return "13|" + String(absPressureArray) + "|" + String(relPressureArray) + "|" + String(temperatureArray) + "|" + String(altitude, DEC) + "|" + String(batVoltage) + "|" + String(period*10, DEC);
}

