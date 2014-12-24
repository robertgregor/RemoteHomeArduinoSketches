/*
Temperature and humidity meter sketch

Radio commands (Works only 1st minutes after the power on, if interval is not 0). Please be sure to set the interval, othervise the unit doesn't switch to sleep mode and the battery will be exhausted quickly.

s - return status 10|temperature|humidity|battery|frequency|thermostatId|thermostatSubDeviceId|humidityId|humiditySubDeviceId
	temperature format - always sign, 2 decimals degree celsius, dot, 2 decimals, e.g 23.32 or -02.32 or 02.20
	humidity format - 2 decimals, dot, 2 decimals, e.g 30.32 or 02.20
	battery voltage e.g. 2.9 or 3.0
	frequency - frequency in seconds to send status. Longer value, longer battery life
        thermostatId - thermostat device Id, where the temperature meter is sending the temperature
        thermostatSubDeviceId - thermostat sub device Id, where the temperature meter is sending the temperature
m=nnn - set the frequency and put to the sleep mode. 1 means 10 seconds, 254 means 2540 seconds. Needs to be set to put the device to sleep mode!!!
	If not set, or set to 0, then no sleep - usefull for testing, but consume lot of power and batery will be quicly empty.
t=nnn - set the thermostat device Id, where the temperature should be sent. The range is 0 < nnn < 255.
ts=nnn - set the thermostat subdevice device Id, where the temperature should be sent. The range is 0 < nnn < 9.
h=nnn - set the ventilator device Id, where the humidity should be sent. The range is 0 < nnn < 255.
hs=nnn - set the ventilator subdevice device Id, where the humidity should be sent. The range is 0 < nnn < 9.


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
at+t=XXX - set the thermostat device Id, where the temperature should be sent. The range is 0 < nnn < 255.
at+ts=XXX - set the thermostat sub device Id, where the temperature should be sent. The range is 0 < nnn < 255.
at+h=XXX - set the ventilator device Id, where the humidity should be sent. The range is 0 < nnn < 255.
at+hs=XXX - set the ventilator sub device Id, where the humidity should be sent. The range is 0 < nnn < 255.

EEPROM bank
EEPROM_POSITION_NODE_ID 0 //Node Id eeprom position
EEPROM_POSITION_NETWORK_ID 1 //Node Id eeprom position
EEPROM_POSITION_KEY 2 //Node Id eeprom position
EEPROM_SENDING_STATUS_PERIOD 18 //Sending status period position eeprom position
EEPROM_THERMOSTAT_ID 19 //Thermostat Id, where to send the temperature
EEPROM_THERMOSTAT_SUBDEVICE_ID 20 //Thermostat subdevice Id, where to send the temperature
EEPROM_HUMIDITY_ID 21 //Humidity Id, where to send the temperature
EEPROM_HUMIDITY_SUBDEVICE_ID 22 //Humidity subdevice Id, where to send the temperature
*/



#include <EEPROM.h>
#include <avr/wdt.h>
#include <LowPower.h>
#include <RFM69.h>
#include <SPI.h>
#include <RemoteHome.h>
#include <DHT.h>

 

#define EEPROM_SENDING_STATUS_PERIOD 18 //Sending status period position eeprom position
#define EEPROM_THERMOSTAT_ID 19 //Thermostat Id, where to send the temperature
#define EEPROM_THERMOSTAT_SUBDEVICE_ID 20 //Thermostat Id, where to send the temperature
#define EEPROM_HUMIDITY_ID 21 //Humidity Id, where to send the humidity
#define EEPROM_HUMIDITY_SUBDEVICE_ID 22 //Humidity Id, where to send the humidity
#define ALIVE_AFTER_STARTUP 60000 //How long after startup the module should go to sleep period.
#define WAIT_BEFORE_SLEEP 100 //How long after startup the module should be up before it will go to sleep again.
#define TEMP_SENSOR_POWER 4
#define DHTPIN 3
#define DHTTYPE DHT22

RemoteHome remoteHome;
long interval=ALIVE_AFTER_STARTUP;
byte period = 0;
byte thermostatId = 0;
byte thermostatSubdeviceId = 0;
byte humidityId = 0;
byte humiditySubdeviceId = 0;
int sleepTimer = 0;
unsigned long previousMillis=0;
DHT dht(DHTPIN, DHTTYPE);
float temperature = 1.0;
float humidity = 1.0;

void setup() {
  wdt_enable(WDTO_2S);
  // initialize serial
  Serial.begin(115200);
  Serial.println("Started");
  remoteHome.setup();
  pinMode(TEMP_SENSOR_POWER, OUTPUT);
  digitalWrite(TEMP_SENSOR_POWER, LOW);  
  period = EEPROM.read(EEPROM_SENDING_STATUS_PERIOD);
  if (period == 255) period = 0;
  thermostatId = EEPROM.read(EEPROM_THERMOSTAT_ID);
  if (thermostatId == 255) thermostatId = 0;
  thermostatSubdeviceId = EEPROM.read(EEPROM_THERMOSTAT_SUBDEVICE_ID);
  if (thermostatSubdeviceId == 255) thermostatSubdeviceId = 1;
  humidityId = EEPROM.read(EEPROM_HUMIDITY_ID);
  if (humidityId == 255) humidityId = 0;
  humiditySubdeviceId = EEPROM.read(EEPROM_HUMIDITY_SUBDEVICE_ID);
  if (humiditySubdeviceId == 255) humiditySubdeviceId = 1;
  delay(100);
}

void loop() {
  wdt_reset();
  if (remoteHome.processCommonSerialData(Serial)) {
    String lowerCaseInput = remoteHome.inputString;
    lowerCaseInput.toLowerCase();
    if (lowerCaseInput.startsWith("at+s")) {
      readSensor();
      remoteHome.printCommonConfig(Serial);
      Serial.print("\tDevice type:TemperatureMeter");
      Serial.print("\tPeriod:");
      Serial.print(period*10, DEC);
      Serial.print("\tBattery:");
      Serial.print(remoteHome.readVcc(), DEC);
      Serial.print("\tTemperature:");
      Serial.print(temperature, 2);     
      Serial.print("\tHumidity:");
      Serial.print(humidity, 2);     
      Serial.print("\tThermostat ID:");
      Serial.print(thermostatId, DEC);     
      Serial.print("\tThermostat subdevice ID:");
      Serial.print(thermostatSubdeviceId, DEC);     
      Serial.print("\tHumidity ID:");
      Serial.print(humidityId, DEC);     
      Serial.print("\tHumidity subdevice ID:");
      Serial.print(humiditySubdeviceId, DEC);     
      Serial.println();
    } else if (lowerCaseInput.startsWith("at+m=")) {
      period = lowerCaseInput.substring(5).toInt();
      EEPROM.write(EEPROM_SENDING_STATUS_PERIOD, period);
      remoteHome.printOK(Serial); 
    } else if (lowerCaseInput.startsWith("at+t=")) {
      thermostatId = lowerCaseInput.substring(5).toInt();
      EEPROM.write(EEPROM_THERMOSTAT_ID, thermostatId);
      remoteHome.printOK(Serial); 
    } else if (lowerCaseInput.startsWith("at+ts=")) {
      thermostatSubdeviceId = lowerCaseInput.substring(5).toInt();
      EEPROM.write(EEPROM_THERMOSTAT_SUBDEVICE_ID, thermostatSubdeviceId);
      remoteHome.printOK(Serial); 
    } else if (lowerCaseInput.startsWith("at+h=")) {
      thermostatId = lowerCaseInput.substring(5).toInt();
      EEPROM.write(EEPROM_THERMOSTAT_ID, thermostatId);
      remoteHome.printOK(Serial); 
    } else if (lowerCaseInput.startsWith("at+hs=")) {
      thermostatSubdeviceId = lowerCaseInput.substring(5).toInt();
      EEPROM.write(EEPROM_THERMOSTAT_SUBDEVICE_ID, thermostatSubdeviceId);
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
      readSensor();
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
    } else if (((char)remoteHome.radio.DATA[0] == 't') && ((char)remoteHome.radio.DATA[1] == '=')) {
      String num = "";
      for (int i=2; i<255; i++) {
          if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
          num = num + (char)remoteHome.radio.DATA[i];
      }
      int recvNum = num.toInt();
      if ((recvNum >= 0) && (recvNum < 255)) {
        thermostatId = recvNum;
        EEPROM.write(EEPROM_THERMOSTAT_ID, thermostatId);
        remoteHome.sendOK();        
      } else {
        remoteHome.sendERROR();
      }
    } else if (((char)remoteHome.radio.DATA[0] == 't') && ((char)remoteHome.radio.DATA[1] == 's') && ((char)remoteHome.radio.DATA[2] == '=')) {
      String num = "";
      for (int i=3; i<255; i++) {
          if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
          num = num + (char)remoteHome.radio.DATA[i];
      }
      int recvNum = num.toInt();
      if ((recvNum > 0) && (recvNum < 255)) {
        thermostatSubdeviceId = recvNum;
        EEPROM.write(EEPROM_THERMOSTAT_SUBDEVICE_ID, thermostatSubdeviceId);
        remoteHome.sendOK();        
      } else {
        remoteHome.sendERROR();
      }
    } else if (((char)remoteHome.radio.DATA[0] == 'h') && ((char)remoteHome.radio.DATA[1] == '=')) {
      String num = "";
      for (int i=2; i<255; i++) {
          if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
          num = num + (char)remoteHome.radio.DATA[i];
      }
      int recvNum = num.toInt();
      if ((recvNum >= 0) && (recvNum < 255)) {
        humidityId = recvNum;
        EEPROM.write(EEPROM_HUMIDITY_ID, humidityId);
        remoteHome.sendOK();        
      } else {
        remoteHome.sendERROR();
      }
    } else if (((char)remoteHome.radio.DATA[0] == 'h') && ((char)remoteHome.radio.DATA[1] == 's') && ((char)remoteHome.radio.DATA[2] == '=')) {
      String num = "";
      for (int i=3; i<255; i++) {
          if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
          num = num + (char)remoteHome.radio.DATA[i];
      }
      int recvNum = num.toInt();
      if ((recvNum > 0) && (recvNum < 255)) {
        humiditySubdeviceId = recvNum;
        EEPROM.write(EEPROM_HUMIDITY_SUBDEVICE_ID, humiditySubdeviceId);
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
        readSensor();
        //send to thermostat
        if (thermostatId > 0) {
          String status = getThermostatData();
          remoteHome.len = status.length();
          status.toCharArray(remoteHome.buff, remoteHome.len+1); 
          remoteHome.sendRadioData(thermostatId);
        }               
        if (humidityId > 0) {
          String status = getHumidityData();
          remoteHome.len = status.length();
          status.toCharArray(remoteHome.buff, remoteHome.len+1); 
          remoteHome.sendRadioData(humidityId);
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
void readSensor() {
  digitalWrite(TEMP_SENSOR_POWER, HIGH);
  LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);
  dht.begin();
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  digitalWrite(TEMP_SENSOR_POWER, LOW);
}
String getStatus() {
        //Send sensor status
        int bat = remoteHome.readVcc();
        char arrayTemp[10];
        dtostrf(temperature, 2, 2, arrayTemp);
        char arrayHum[10];
        dtostrf(humidity, 2, 2, arrayHum);
        return "10|" + String(arrayTemp) + "|" + String(arrayHum) + "|" + String(bat / 10, DEC) + "." + String(bat % 10, DEC) + "|" + String(period*10, DEC) + "|" + String(thermostatId, DEC) + "|" + String(thermostatSubdeviceId, DEC) + "|" + String(humidityId, DEC) + "|" + String(humiditySubdeviceId, DEC);
}
String getThermostatData() {
        //Send temperature
        char array[10];
        dtostrf(temperature, 2, 2, array);
        return "t" + String(thermostatSubdeviceId, DEC) + "cu=" + String(array) + "\n";
}
String getHumidityData() {
        //Send humidity
        char array[10];
        dtostrf(humidity, 2, 2, array);
        return "v" + String(humiditySubdeviceId, DEC) + "ch=" + String(array) + "\n";
}
