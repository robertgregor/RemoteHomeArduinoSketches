/*

Simple switch sketch

Radio commands:

Switch commands
--------------------------------------------------------------------------------------------
lxo - light 1 on
lxof - light x on and off after timeout period
lxot - light x on and off after 3 minutes doesnt matter the timeout period.
lxf - light x off
lxco - configure light x to swith on when applied power
lxcf - configure light x to stay off when applied power
lxcm=nnn - configure light x timeout nnn = 0 to 255 minutes (0 means forewer) minutes
lxcb=nnn - configure border value of the light sensor. If the light intensity is under this value, the light will switch on, when it receive the commands lxof or lxot. The value is 0 .. 1024, 0 means total dark.
lxcl=nnn - receive current light intensity value from the light sensor. The value is 0 .. 1024, 0 means total dark.
lxs - return status 1s|switchId|status|power|configuredTimeout|currentTimeout|configuredLightIntensity|currentLightIntensity
        switchID - number of switch ( It is in fact value x )
	status - 0 off, 1 on
	power - 0 on when applied power, 1 stay off when applied power	
	configuredTimeout - no of minutes, when lx will go off, 0 forewer
	currentTimeout - current light 1 timeout in minutes, 0 - never switch off. E.g. 3 means, that after 3 minutes the relay will switch off

x - switch number
Asynchronous messages (It is sent, when the status of the device change):
1s|switchId|status|power|configuredTimeout|currentTimeout|configuredLightIntensity|currentLightIntensity
LP - Lost power
--------------------------------------------------------------------------------------------

Thermostat commands
--------------------------------------------------------------------------------------------
txcu=nn.nn - receive current temperature from temperature sensor nn.nn is the float
txcte=nnn - configure expected temperature. Should be in form float, decimal character is ".", e.g. 21.5
txctt=nnn - configure threshold. Should be 0 < nnn < 9. If the expected temperature is e.g. 21 and threshold is 2, then the switch is on, when the current temperature is bellow 20.8 and will switch off, when it is higher than 21.2
txch=nnn - configure heating device Id. If there is a device, which needs to be switched on ( e.g. heating unit ), then configure it. This value is common for all the switches on the same board. nnn = 0 to 254. 0 means no heating device Id.
txcs=nnn - configure heating sub device Id. If there is a device, which needs to be switched on ( e.g. heating unit ), then configure it. If there is a subdevice board to control more outputs configure subdevice Id. nnn = 1 to 8
txs - return status 1t|thermostatID|status|currentTemperature|ExpectedTemperature|Threshold|HeatingDeviceId|HeatingDeviceSubdeviceId
        thermostatID - number of thermostat ( It is in fact value x )
        status - 0 off, 1 on
        currentTemperature - current temperature send by the temperature sensor
        ExpectedTemperature - expected temperature
        Threshold - Threshold. Should be 0 < nnn < 9. If the expected temperature is e.g. 21 and threshold is 2, then the switch is on, when the current temperature is bellow 20.8 and will switch off, when it is higher than 21.2
        HeatingDeviceId - heating device Id. If there is a device, which needs to be switched on ( e.g. heating unit ), then configure it. This value is common for all the switches on the same board. nnn = 1 to 254
        HeatingDeviceSubdeviceId - heating sub device Id. If there is a device, which needs to be switched on ( e.g. heating unit ), then configure it. If there is a subdevice board to control more outputs configure subdevice Id. nnn = 1 to 8

x - thermostat number
Asynchronous messages (It is sent, when the status of the device change):
1t|thermostatID|status|currentTemperature|ExpectedTemperature|Threshold|HeatingDeviceId|HeatingDeviceSubdeviceId
LP - Lost power
--------------------------------------------------------------------------------------------

Ventilation controller commands ( Used together with CO2 / humidity sensors )
--------------------------------------------------------------------------------------------
vxch=nnn - receive current humidity from humidity sensor nnn is the value 0 - 100
vxchx=nnn - configure max. humidity concentration. If the received concentration is higher, then the max. the relay will switch ON. ( For the defined period vxchm=nnn. )
vxchm=nnn - configure the humidity switch ON timeout in minutes.
vxco=nnnn - receive current CO2 from CO2 sensor nnnn is the int
vxcox=nnn - configure max. CO2 concentration. If the received concentration is higher, then the max. the relay will switch ON. ( For the defined period vxcom=nnn. )
vxcom=nnn - configure the CO2 switch ON timeout.
vxo - Switch on forewer
vxf - Switch off forewer
vxs - return status 1v|ventilationID|status|currentHumidity|currentCo2|maxHumidity|maxCo2|humidityTimeout|Co2Timeout|currentCounter
        ventilationID - number of ventilation ( It is in fact value x )
        status - 0 off, 1 on
        currentHumidity - current humidity received by the humidity sensor
        currentCo2 - current Co2 received by the Co2 sensor
        maxHumidity - Max. humidity value configured with the vxchx command
        maxCo2 - Max. Co2 value configured with the vxcox command
        humidityTimeout - humidity timeout value configured with the vxchx command
        Co2Timeout - Co2 timeout value configured with the vxcox command
        currentCounter - current timeout in minutes. E.g. 3 means, that after 3 minutes the relay will switch off. 0 means never switch off. 

x - thermostat number
Asynchronous messages (It is sent, when the status of the device change):
1v|ventilationID|status|currentHumidity|currentCo2|maxHumidity|maxCo2|humidityTimeout|Co2Timeout|currentCounter
LP - Lost power
--------------------------------------------------------------------------------------------

Motor controller
--------------------------------------------------------------------------------------------
bxt=nnn - configure number of seconds, when the blind is moving fully from the open position to closed position.
bxu - blinds up moving to fully open position it means 0 %
bxd - blinds down moving to fully closed position it means 100 %
bxp - blinds stop stop movements
bxc=nnn - Move to the nnn position. nnn is the percentage between open and close. 0 - fully open, 100 - fully closed
bxm=nnn - Timeout value - number of configured timeout in 100 msec. This value is the 100 % time to open the blinds
bxs - return status 1b|blindId|down|up|timeout value|current timeout value|expected opening|current opening
        blindID - number of blind ( It is in fact value x )
	down - 1 if currently moving down
	up - 1 if currently moving up
	timeout value - number of configured timeout in 100 msec. This value is the 100 % time to open the blinds
	current timeout value - number of configured timeout in 100 msec. This value is the 100 % time to open the blinds
        expected opening - Blinds opening in %. This is expected value. When the blind reach that value, it will stop and will send the asynchronous command.
	current opening - Blinds opening in %. 0 means fully open, 100 means fully closed. If the blinds moving, it display current position. If it is not moving, this value should match the value of the expected temperature.

x - pair of the relays number. In this mode, always 2 relays form 1 controller. So to use this mode, 2 relay board or 8 relay board should be used. So e.g. b2u command means, that blind's motor is connected to relays 3 and 4.
Asynchronous messages (It is sent, when the status of the device change):
1b|blindId|down|up|timeout value|current timeout value|expected opening|current opening
LP - Lost power

--------------------------------------------------------------------------------------------

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
at+pcn=X - set the number of pins, the hardware is capable to manage. X could be 1, 2 and 8.

EEPROM bank
EEPROM_POSITION_NODE_ID 1006 //Node Id eeprom position
EEPROM_POSITION_NETWORK_ID 1007 //Node Id eeprom position
EEPROM_POSITION_KEY 1008 //Node Id eeprom position
PIN_COUNT 18 //Number of pins, that the sw has
PIN_POWER_ON 19 //Power on values - eight bytes for corresponding switch number. If 0 no switch on, when power applied, if 1 switch on, when power applied.
PIN_COUNTER 27 //counter values - counter timeouts - ised with lxof command. Meaning is number of minutes.
PIN_EXPECTED_TEMPERATURES 35 //expected temperatures
PIN_TEMPERATURE_THRESHOLDS 43 //thresholds for the temperatures
HEATING_SOURCE_ID 51 //heating source id
HEATING_SOURCE_SUBDEVICE_ID 52 //heating source subdevice id
MAX_CO2_CONCENTRATION 53 //maximum concentration of CO2
CO2_TIMEOUT 69 //CO2 timeout
MAX_HUMIDITY_CONCENTRATION 77 //maximum humidity
HUMIDITY_TIMEOUT 85 //humidity timeout
MOTOR_CONTROLLER_COUNTERS 93 //motor controller counters
LIGHT_SENSOR_MAX_VALUES 101 //Light sensor border values to switch on.
*/


#include <EEPROM.h>
#include <avr/wdt.h>
#include <LowPower.h>
#include <RFM69.h>
#include <SPI.h>
#include <RemoteHome.h>
#include <SimpleTimer.h>

#define PIN_COUNT 18 //Number of pins, that the sw has
#define PIN_POWER_ON 19 //Power on values 8 bytes
#define PIN_COUNTER 27 //counter values 8 bytes
#define PIN_EXPECTED_TEMPERATURES 35 //counter 8 values
#define PIN_TEMPERATURE_THRESHOLDS 43 //counter 8 values
#define HEATING_SOURCE_ID 51 //heating source id
#define HEATING_SOURCE_SUBDEVICE_ID 52 //heating source subdevice id
#define MAX_CO2_CONCENTRATION 53 //maximum concentration of CO2 8 word values it means 16 bytes total
#define CO2_TIMEOUT 69 //CO2 timeout 8 values
#define MAX_HUMIDITY_CONCENTRATION 77 //maximum humidity 8 values
#define HUMIDITY_TIMEOUT 85 //humidity timeout 8 values
#define MOTOR_CONTROLLER_COUNTERS 93 //Counters for motor controller These are 4 word values - it means each counter takes 2 bytes 8 bytes total
#define LIGHT_SENSOR_MAX_VALUES 101 //Light sensor border values to switch on.

RemoteHome remoteHome;
SimpleTimer timer;
byte swCounter[8];
int pins[] = {5,6,A0,4,3,A1,7,8};
byte pinsPowerOnWhenStart[] = {0,0,0,0,0,0,0,0};
unsigned int counterValues[] = {180,180,180,180,180,180,180,180};
unsigned int currentCounterValues[] = {0,0,0,0,0,0,0,0};
unsigned int currentLightSensorValues[] = {0,0,0,0,0,0,0,0};
unsigned int lightSensorValues[] = {0,0,0,0,0,0,0,0};
float currentTemperatures[] = {0,0,0,0,0,0,0,0};
byte expectedTemperatures[] = {42,42,42,42,42,42,42,42};
byte currentTemperatureThresholds[] = {3,3,3,3,3,3,3,3};
unsigned int motorControllerCounters[] = {100,100,100,100};
unsigned int currentMotorControllerCounters[] = {0,0,0,0};
byte motorControllerCurrentClose[] = {0,0,0,0};
unsigned int currentCO2concentration[] = {0,0,0,0,0,0,0,0};
unsigned int maxCO2concentration[] = {1500,1500,1500,1500,1500,1500,1500,1500};
unsigned int co2concentrationTimeout[] = {180,180,180,180,180,180,180,180};
unsigned int currentHumidity[] = {0,0,0,0,0,0,0,0};
unsigned int maxHumidity[] = {30,30,30,30,30,30,30,30};
unsigned int humidityTimeout[] = {180,180,180,180,180,180,180,180};
boolean powerLostReported = false;
int pinCount = 8;
byte heatingSourceId = 0;
byte heatingSourceSubdeviceId = 1;
boolean heatingSourceOn = false;

void setup() {
  wdt_enable(WDTO_2S);
  // initialize serial
  Serial.begin(115200);
  Serial.println("Started");
  for (int thisPin = 0; thisPin < pinCount; thisPin++) pinMode(pins[thisPin], OUTPUT);
  remoteHome.setup();
  int pinCountEeprom = EEPROM.read(PIN_COUNT);
  if (pinCountEeprom != 255) pinCount = pinCountEeprom;
  for (int i=0; i<pinCount; i++) {
    pinsPowerOnWhenStart[i] = EEPROM.read(PIN_POWER_ON + i);
    if (pinsPowerOnWhenStart[i] == 255) pinsPowerOnWhenStart[i] = 0;
    if (pinsPowerOnWhenStart[i] == 1) digitalWrite(pins[i],HIGH);
    counterValues[i] = EEPROM.read(PIN_COUNTER + i);
    if (counterValues[i] == 255) counterValues[i] = 3;
    counterValues[i] = counterValues[i]*60;
    lightSensorValues[i] = EEPROM.read(LIGHT_SENSOR_MAX_VALUES + i)*10;
    if (lightSensorValues[i] == 2550) lightSensorValues[i] = 0;
    expectedTemperatures[i] = EEPROM.read(PIN_EXPECTED_TEMPERATURES + i);
    if (expectedTemperatures[i] == 255) expectedTemperatures[i] = 42;
    currentTemperatureThresholds[i] = EEPROM.read(PIN_TEMPERATURE_THRESHOLDS + i);
    if (currentTemperatureThresholds[i] == 255) currentTemperatureThresholds[i] = 3;
    maxCO2concentration[i] = EEPROM.read(MAX_CO2_CONCENTRATION + i*2) << 8;
    maxCO2concentration[i] = maxCO2concentration[i] + EEPROM.read(MAX_CO2_CONCENTRATION + i*2 + 1);
    if (maxCO2concentration[i] == 65535) maxCO2concentration[i] = 1500;
    maxHumidity[i] = EEPROM.read(MAX_HUMIDITY_CONCENTRATION + i);
    if (maxHumidity[i] == 255) maxHumidity[i] = 30;
    co2concentrationTimeout[i] = EEPROM.read(CO2_TIMEOUT + i);
    if (co2concentrationTimeout[i] == 255) co2concentrationTimeout[i] = 3;
    co2concentrationTimeout[i] = co2concentrationTimeout[i]*60;
    humidityTimeout[i] = EEPROM.read(HUMIDITY_TIMEOUT + i);
    if (humidityTimeout[i] == 255) humidityTimeout[i] = 3;
    humidityTimeout[i] = humidityTimeout[i]*60;
  }
  for (int i=0; i<8; i+=2) {
    int tmp = EEPROM.read(MOTOR_CONTROLLER_COUNTERS + i) << 8;
    tmp = tmp + EEPROM.read(MOTOR_CONTROLLER_COUNTERS + i + 1);
    if (tmp == 65535) motorControllerCounters[i/2] = 100; else motorControllerCounters[i/2] = tmp;
  }
  int heatingSourceIdEeprom = EEPROM.read(HEATING_SOURCE_ID);
  if (heatingSourceIdEeprom != 255) heatingSourceId = heatingSourceIdEeprom;  
  int heatingSourceSubdeviceIdEeprom = EEPROM.read(HEATING_SOURCE_SUBDEVICE_ID);
  if (heatingSourceSubdeviceIdEeprom != 255) heatingSourceSubdeviceId = heatingSourceSubdeviceIdEeprom;
  timer.setInterval(1000, runEachSec);
  timer.setInterval(100, runEach100ms);
  timer.setInterval(180000, runEach3mins);
  timer.setInterval(2000, runEachTwoSec);
  delay(100);
}

void loop() {
  wdt_reset();
  timer.run();
  if (remoteHome.readVcc() < 32) {
      powerLost();
  }
  if (remoteHome.processCommonSerialData(Serial)) {
    String lowerCaseInput = remoteHome.inputString;
    lowerCaseInput.toLowerCase();
    if (lowerCaseInput.startsWith("at+s")) {
      remoteHome.printCommonConfig(Serial);
      Serial.print("\tDevice type:SimpleSwitch");
      Serial.print("\tPinCount:");       Serial.print(pinCount,DEC);
      for (int i=0; i<pinCount; i++) {
        char array[10];
        dtostrf(currentTemperatures[i], 2, 2, array);
        char arrayExp[10];
        float expTmp = expectedTemperatures[i]/2;
        dtostrf(expTmp, 2, 1, arrayExp);
        Serial.print("\tSW");Serial.print(i+1, DEC);Serial.print(":");
        Serial.print(digitalRead(pins[i]),DEC);Serial.print(",");
        Serial.print(pinsPowerOnWhenStart[i],DEC);Serial.print(",");
        Serial.print(counterValues[i]/60,DEC);Serial.print(",");
        Serial.print(currentCounterValues[i]/60,DEC);Serial.print(",");
        Serial.print(array);Serial.print(",");
        Serial.print(arrayExp);Serial.print(",");
        Serial.print("0.");
        Serial.print(currentTemperatureThresholds[i],DEC);Serial.print(",");
        Serial.print(heatingSourceId,DEC);Serial.print(",");
        Serial.print(heatingSourceSubdeviceId,DEC);Serial.print(",");
        if ((i==0) || (i==1)) { Serial.print(motorControllerCounters[0],DEC);Serial.print(",");Serial.print(motorControllerCurrentClose[0],DEC); }
        if ((i==2) || (i==3)) { Serial.print(motorControllerCounters[1],DEC);Serial.print(",");Serial.print(motorControllerCurrentClose[1],DEC); }
        if ((i==4) || (i==5)) { Serial.print(motorControllerCounters[2],DEC);Serial.print(",");Serial.print(motorControllerCurrentClose[2],DEC); }
        if ((i==6) || (i==7)) { Serial.print(motorControllerCounters[3],DEC);Serial.print(",");Serial.print(motorControllerCurrentClose[3],DEC); }
      }
      Serial.println();
    } else if (lowerCaseInput.startsWith("at+pcn=")) {
      int pinC = lowerCaseInput.substring(7).toInt();
      if (pinC == 1) {
        pinCount = pinC;
        EEPROM.write(PIN_COUNT, (byte)pinC);
        remoteHome.printOK(Serial);
      } else if (pinC == 2) {
        pinCount = pinC;
        EEPROM.write(PIN_COUNT, (byte)pinC);
        remoteHome.printOK(Serial);
      } else if (pinC == 8) {
        pinCount = pinC;
        EEPROM.write(PIN_COUNT, (byte)pinC);
        remoteHome.printOK(Serial);
      } else {
        remoteHome.printERROR(Serial);
      }
    } else {
      remoteHome.printERROR(Serial);
    }
    remoteHome.inputString = "";
    remoteHome.stringComplete = false;
  }
  //check and manage radio
  if (remoteHome.processCommonRadioData()) {
    if ((char)remoteHome.radio.DATA[0] == 't') {
      processInputTemperatureData();
    } else if ((char)remoteHome.radio.DATA[0] == 'l') {
      processInputLightData();
    } else if ((char)remoteHome.radio.DATA[0] == 'b') {
      processInputBlindsData();
    } else if ((char)remoteHome.radio.DATA[0] == 'v') {
      processInputVentilatorData();
    } else {
      remoteHome.sendERROR();
    }
    remoteHome.manageReceivedData();
  }
}

void sendTemperatureStatus(int sw) {
              // Send the status
              String status = getThermostatStatus(sw);
              remoteHome.len = status.length();
              status.toCharArray(remoteHome.buff, remoteHome.len+1);
              remoteHome.sendRadioData();                                            
}
void processInputTemperatureData() {
      int sw = remoteHome.inputRadioData.substring(1).toInt();
      if ((sw > 0) && (sw < (pinCount+1))) {
        if ((char)remoteHome.radio.DATA[2] == 'c') {
          if (((char)remoteHome.radio.DATA[3] == 't') && ((char)remoteHome.radio.DATA[4] == 'e') && ((char)remoteHome.radio.DATA[5] == '=')) {
              String tempStr = remoteHome.inputRadioData.substring(6);
              byte baseNbr = tempStr.substring(0,tempStr.indexOf(".")).toInt()*2;
              byte decimal = 0;
              if (tempStr.indexOf(".") > 0) decimal = tempStr.substring(tempStr.indexOf(".")+1,tempStr.indexOf(".")+2).toInt(); // support . as separator
              if (tempStr.indexOf(",") > 0) decimal = tempStr.substring(tempStr.indexOf(",")+1,tempStr.indexOf(",")+2).toInt(); // support , as separator
              if ((decimal > 2) && (decimal < 8)) {
                baseNbr += 1;
              } else if ((7 < decimal) && (decimal < 10)) {
                baseNbr += 2;
              } 
              expectedTemperatures[sw-1] = (byte)baseNbr;
              EEPROM.write(PIN_EXPECTED_TEMPERATURES + (sw - 1), expectedTemperatures[sw-1]);
              processTemperature(sw);
              remoteHome.sendOK();
              remoteHome.manageReceivedData();
              sendTemperatureStatus(sw);
          } else if (((char)remoteHome.radio.DATA[3] == 'u') && ((char)remoteHome.radio.DATA[4] == '=')) {
              String tempStr = remoteHome.inputRadioData.substring(5);
              char carray[tempStr.length() + 1];
              tempStr.toCharArray(carray, sizeof(carray));
              currentTemperatures[sw-1] = atof(carray);
              processTemperature(sw);
              remoteHome.manageReceivedData();
              sendTemperatureStatus(sw);
          } else if (((char)remoteHome.radio.DATA[3] == 't') && ((char)remoteHome.radio.DATA[4] == 't') && ((char)remoteHome.radio.DATA[5] == '=')) {
              int t = remoteHome.inputRadioData.substring(6).toInt();
              if ((t>=0) && (t<=9)) {
                currentTemperatureThresholds[sw-1] = t;
                EEPROM.write(PIN_TEMPERATURE_THRESHOLDS + (sw - 1), t);
                processTemperature(sw);
                remoteHome.sendOK();
                remoteHome.manageReceivedData();
                sendTemperatureStatus(sw);                
              } else {
                remoteHome.sendERROR();
              }            
          } else if (((char)remoteHome.radio.DATA[3] == 'h') && ((char)remoteHome.radio.DATA[4] == '=')) {
              int t = remoteHome.inputRadioData.substring(5).toInt();
              if ((t>=0) && (t<255)) {
                heatingSourceId = t;
                EEPROM.write(HEATING_SOURCE_ID, t);
                remoteHome.sendOK();
                remoteHome.manageReceivedData();
                sendTemperatureStatus(sw);
              } else {
                remoteHome.sendERROR();
              }            
          } else if (((char)remoteHome.radio.DATA[3] == 's') && ((char)remoteHome.radio.DATA[4] == '=')) {
              int t = remoteHome.inputRadioData.substring(5).toInt();
              if ((t>0) && (t<9)) {
                heatingSourceSubdeviceId = t;
                EEPROM.write(HEATING_SOURCE_SUBDEVICE_ID, t);
                remoteHome.sendOK();
                remoteHome.manageReceivedData();
                sendTemperatureStatus(sw);
              } else {
                remoteHome.sendERROR();
              }
          } else {
            remoteHome.sendERROR();
          }            
        } else if ((char)remoteHome.radio.DATA[2] == 's') {
          remoteHome.manageReceivedData();
          sendTemperatureStatus(sw);
        } else {
          remoteHome.sendERROR();
        }
      } else {
        remoteHome.sendERROR();
      }
}
void sendVentilatorStatus(int sw) {
              // Send the status
              String status = getVentilatorStatus(sw);
              remoteHome.len = status.length();
              status.toCharArray(remoteHome.buff, remoteHome.len+1);
              remoteHome.sendRadioData();                                            
}
void processInputVentilatorData() {
      int sw = remoteHome.inputRadioData.substring(1).toInt();
      if ((sw > 0) && (sw < (pinCount+1))) {
        if ((char)remoteHome.radio.DATA[2] == 'c') {
          if (((char)remoteHome.radio.DATA[3] == 'h') && ((char)remoteHome.radio.DATA[4] == '=')) {
              int tempNbr = remoteHome.inputRadioData.substring(5).toInt();
              if ((tempNbr >= 0) && (tempNbr <= 100)) {
                 currentHumidity[sw-1] = tempNbr;
                 processHumidity(sw);
                 remoteHome.manageReceivedData();
                 // Send the status
                 sendVentilatorStatus(sw);
              }
          } else if (((char)remoteHome.radio.DATA[3] == 'h') && ((char)remoteHome.radio.DATA[4] == 'x') && ((char)remoteHome.radio.DATA[5] == '=')) {
              int tempNbr = remoteHome.inputRadioData.substring(6).toInt();
              if ((tempNbr >= 0) && (tempNbr <= 100)) {
                 maxHumidity[sw-1] = tempNbr;
                 EEPROM.write(MAX_HUMIDITY_CONCENTRATION + (sw - 1), tempNbr);
                 processHumidity(sw);
                 remoteHome.sendOK();
                 remoteHome.manageReceivedData();
                 // Send the status
                 sendVentilatorStatus(sw);
              } else {
                 remoteHome.sendERROR();
              }
          } else if (((char)remoteHome.radio.DATA[3] == 'h') && ((char)remoteHome.radio.DATA[4] == 'm') && ((char)remoteHome.radio.DATA[5] == '=')) {
              int tempNbr = remoteHome.inputRadioData.substring(6).toInt();
              if ((tempNbr >= 0) && (tempNbr <= 255)) {
                 humidityTimeout[sw-1] = tempNbr*60;
                 EEPROM.write(HUMIDITY_TIMEOUT + (sw - 1), tempNbr);
                 remoteHome.sendOK();
                 remoteHome.manageReceivedData();
                 // Send the status
                 sendVentilatorStatus(sw);     
              } else {
                 remoteHome.sendERROR();
              }
          } else if (((char)remoteHome.radio.DATA[3] == 'o') && ((char)remoteHome.radio.DATA[4] == '=')) {
              int tempNbr = remoteHome.inputRadioData.substring(5).toInt();
              if ((tempNbr >= 0) && (tempNbr <= 40000)) {
                 currentCO2concentration[sw-1] = tempNbr;
                 processCo2(sw);
                 remoteHome.manageReceivedData();
                 // Send the status
                 sendVentilatorStatus(sw);     
              }
          } else if (((char)remoteHome.radio.DATA[3] == 'o') && ((char)remoteHome.radio.DATA[4] == 'x') && ((char)remoteHome.radio.DATA[5] == '=')) {
              int tempNbr = remoteHome.inputRadioData.substring(6).toInt();
              if ((tempNbr >= 0) && (tempNbr <= 40000)) {
                 maxCO2concentration[sw-1] = tempNbr;
                 EEPROM.write(MAX_CO2_CONCENTRATION + (sw-1)*2, highByte(tempNbr));
                 EEPROM.write(MAX_CO2_CONCENTRATION + ((sw-1)*2)+1, lowByte(tempNbr));
                 processCo2(sw);
                 remoteHome.sendOK();
                 remoteHome.manageReceivedData();
                 // Send the status
                 sendVentilatorStatus(sw);     
              } else {
                 remoteHome.sendERROR();
              }
          } else if (((char)remoteHome.radio.DATA[3] == 'o') && ((char)remoteHome.radio.DATA[4] == 'm') && ((char)remoteHome.radio.DATA[5] == '=')) {
              unsigned int tempNbr = remoteHome.inputRadioData.substring(6).toInt();
              if ((tempNbr >= 0) && (tempNbr <= 40000)) {
                 co2concentrationTimeout[sw-1] = tempNbr*60;
                 EEPROM.write(CO2_TIMEOUT + (sw - 1), tempNbr);
                 remoteHome.sendOK();      
                 remoteHome.manageReceivedData();
                 // Send the status
                 sendVentilatorStatus(sw);     
              } else {
                 remoteHome.sendERROR();
              }
          } else {
            remoteHome.sendERROR();
          }            
        } else if ((char)remoteHome.radio.DATA[2] == 'o') {
          currentCounterValues[sw-1] = 0;
          digitalWrite(pins[sw-1], HIGH);
          remoteHome.sendOK();
          remoteHome.manageReceivedData();
          sendVentilatorStatus(sw);
        } else if ((char)remoteHome.radio.DATA[2] == 'f') {
          currentCounterValues[sw-1] = 0;
          digitalWrite(pins[sw-1], LOW);
          remoteHome.sendOK();
          remoteHome.manageReceivedData();
          sendVentilatorStatus(sw);
        } else if ((char)remoteHome.radio.DATA[2] == 's') {
          remoteHome.manageReceivedData();
          sendVentilatorStatus(sw);
        } else {
          remoteHome.sendERROR();
        }
      } else {
        remoteHome.sendERROR();
      }
}
void sendLightStatus(int sw) {
              // Send the status
              String status = getSwitchStatus(sw);
              remoteHome.len = status.length();
              status.toCharArray(remoteHome.buff, remoteHome.len+1);
              remoteHome.sendRadioData();                                            
}
void processInputLightData() {
      int sw = remoteHome.inputRadioData.substring(1).toInt();
      if ((sw > 0) && (sw < (pinCount+1))) {
        if ((char)remoteHome.radio.DATA[2] == 'o') {
          if ((char)remoteHome.radio.DATA[3] == 'f') {
            currentCounterValues[sw-1] = counterValues[sw-1];
            if (lightSensorValues[sw-1] >= currentLightSensorValues[sw-1]) {
              //OK, so the light sensor is darker than the configured border value switch on
              digitalWrite(pins[sw-1], HIGH);
            }
          } else if ((char)remoteHome.radio.DATA[3] == 't') {
            currentCounterValues[sw-1] = 185;
            if (lightSensorValues[sw-1] >= currentLightSensorValues[sw-1]) {
              //OK, so the light sensor is darker than the configured border value switch on
              digitalWrite(pins[sw-1], HIGH);
            }
          } else {
            digitalWrite(pins[sw-1], HIGH);
          }
          remoteHome.sendOK();
          remoteHome.manageReceivedData();
          sendLightStatus(sw);
        } else if ((char)remoteHome.radio.DATA[2] == 'f') {
          digitalWrite(pins[sw-1], LOW);
          currentCounterValues[sw-1] = 0;
          remoteHome.sendOK();
          remoteHome.manageReceivedData();
          sendLightStatus(sw);
        } else if ((char)remoteHome.radio.DATA[2] == 's') {
          remoteHome.manageReceivedData();
          sendLightStatus(sw);
        } else if ((char)remoteHome.radio.DATA[2] == 'c') {
          if ((char)remoteHome.radio.DATA[3] == 'o') {
            pinsPowerOnWhenStart[sw-1] = 1;
            EEPROM.write(PIN_POWER_ON + (sw - 1), 1);
            remoteHome.sendOK();
            remoteHome.manageReceivedData();
            sendLightStatus(sw);        
          } else if ((char)remoteHome.radio.DATA[3] == 'f') {
            pinsPowerOnWhenStart[sw-1] = 0;
            EEPROM.write(PIN_POWER_ON + (sw - 1), 0);
            remoteHome.sendOK();
            remoteHome.manageReceivedData();
            sendLightStatus(sw);        
          } else if (((char)remoteHome.radio.DATA[3] == 'l') && ((char)remoteHome.radio.DATA[4] == '=')) {
              int t = remoteHome.inputRadioData.substring(5).toInt();
              if ((t>=0) && (t<1024)) {
                currentLightSensorValues[sw-1] = t;
                remoteHome.manageReceivedData();
                sendLightStatus(sw);
              }       
          } else if (((char)remoteHome.radio.DATA[3] == 'b') && ((char)remoteHome.radio.DATA[4] == '=')) {
              int t = remoteHome.inputRadioData.substring(5).toInt();
              if ((t>=0) && (t<1024)) {
                lightSensorValues[sw-1] = t;
                EEPROM.write(LIGHT_SENSOR_MAX_VALUES + (sw - 1), t/10);
                remoteHome.sendOK();
                remoteHome.manageReceivedData();
                sendLightStatus(sw);
              }       
          } else if (((char)remoteHome.radio.DATA[3] == 'm') && ((char)remoteHome.radio.DATA[4] == '=')) {
              int t = remoteHome.inputRadioData.substring(5).toInt();
              if ((t>=0) && (t<256)) {
                counterValues[sw-1] = t*60;
                EEPROM.write(PIN_COUNTER + (sw - 1), t);
                remoteHome.sendOK();
                remoteHome.manageReceivedData();
                sendLightStatus(sw);        
              } else {
                remoteHome.sendERROR();
              }
          } else {
            remoteHome.sendERROR();
          }
        } else {
          remoteHome.sendERROR();
        }
      } else {
        remoteHome.sendERROR();
      }
}
void sendBlindStatus(int sw) {
              // Send the status
              String status = getBlindsStatus(sw);
              remoteHome.len = status.length();
              status.toCharArray(remoteHome.buff, remoteHome.len+1);
              remoteHome.sendRadioData();                                            
}
void stopBlinds(int sw) {
        if (currentMotorControllerCounters[sw-1] != 0) {
              float remainingPercent = ((float)currentMotorControllerCounters[sw-1] * 100) / (float)motorControllerCounters[sw-1];
              currentMotorControllerCounters[sw-1] = 0;
              if (digitalRead(pins[(sw-1)*2]) == 1) {
                motorControllerCurrentClose[sw-1] = motorControllerCurrentClose[sw-1] - remainingPercent;
              } else {
                motorControllerCurrentClose[sw-1] = motorControllerCurrentClose[sw-1] + remainingPercent;
              }
              digitalWrite(pins[(sw-1)*2], LOW);
              digitalWrite(pins[((sw-1)*2)+1], LOW);
        }
}
void processInputBlindsData() {
      int sw = remoteHome.inputRadioData.substring(1).toInt();
      if (((pinCount == 2) && (sw == 1)) || 
           ((pinCount == 4) && ((sw == 1) || (sw == 2))) || 
           ((pinCount == 6) && ((sw >= 1) && (sw <= 3))) || 
           ((pinCount == 8) && ((sw >= 1) && (sw <= 4)))) {
        if ((char)remoteHome.radio.DATA[2] == 'u') { // blinds up
            stopBlinds(sw);
            if (motorControllerCounters[sw-1] != 0) {
              if (0 == motorControllerCurrentClose[sw-1]) {
                //it is on possition - do nothing
                remoteHome.sendOK();
                remoteHome.manageReceivedData();
                sendBlindStatus(sw);            
              } else if (0 < motorControllerCurrentClose[sw-1]) {
                // go up
                int difference = motorControllerCurrentClose[sw-1];
                currentMotorControllerCounters[sw-1] = (motorControllerCounters[sw-1] * difference) / 100;
                digitalWrite(pins[(sw-1)*2], LOW);
                digitalWrite(pins[((sw-1)*2)+1], HIGH);
                motorControllerCurrentClose[sw-1] = 0;
                remoteHome.sendOK();
                remoteHome.manageReceivedData();
                sendBlindStatus(sw);
              }
            } else {
              remoteHome.sendERROR();
            }
        } else if ((char)remoteHome.radio.DATA[2] == 'd') { // blinds down
            stopBlinds(sw);
            if (motorControllerCounters[sw-1] != 0) {
              if (100 == motorControllerCurrentClose[sw-1]) {
                //it is on possition - do nothing
                remoteHome.sendOK();
                remoteHome.manageReceivedData();
                sendBlindStatus(sw);            
              } else if (100 > motorControllerCurrentClose[sw-1]) {
                // go down
                int difference = 100 - motorControllerCurrentClose[sw-1];
                currentMotorControllerCounters[sw-1] = (motorControllerCounters[sw-1] * difference) / 100;
                digitalWrite(pins[(sw-1)*2], HIGH);
                digitalWrite(pins[((sw-1)*2)+1], LOW);
                motorControllerCurrentClose[sw-1] = 100;
                remoteHome.sendOK();
                remoteHome.manageReceivedData();
                sendBlindStatus(sw);
              }
            } else {
              remoteHome.sendERROR();
            }
        } else if ((char)remoteHome.radio.DATA[2] == 'p') { // blinds stop
            stopBlinds(sw);
            remoteHome.sendOK();
            remoteHome.manageReceivedData();
            sendBlindStatus(sw);            
        } else if (((char)remoteHome.radio.DATA[2] == 'c') && ((char)remoteHome.radio.DATA[3] == '=')) { // set to prefferred position
          int tmp = remoteHome.inputRadioData.substring(4).toInt();
          if ((tmp >= 0) && (tmp <= 100)) {
            if (tmp == motorControllerCurrentClose[sw-1]) {
              //do nothing it is already on position
            } else if (tmp > motorControllerCurrentClose[sw-1]) {
              // go down
              int difference = tmp - motorControllerCurrentClose[sw-1];
              currentMotorControllerCounters[sw-1] = (motorControllerCounters[sw-1] * difference) / 100;
              digitalWrite(pins[(sw-1)*2], HIGH);
              digitalWrite(pins[((sw-1)*2)+1], LOW);
              motorControllerCurrentClose[sw-1] = tmp;
            } else {
              // go up
              int difference = motorControllerCurrentClose[sw-1] - tmp;
              currentMotorControllerCounters[sw-1] = (motorControllerCounters[sw-1] * difference) / 100;
              digitalWrite(pins[(sw-1)*2], LOW);
              digitalWrite(pins[((sw-1)*2)+1], HIGH);
              motorControllerCurrentClose[sw-1] = tmp;
            }
            remoteHome.sendOK();
            remoteHome.manageReceivedData();
            sendBlindStatus(sw);            
          } else {
            remoteHome.sendERROR();
          }
        } else if (((char)remoteHome.radio.DATA[2] == 'm') && ((char)remoteHome.radio.DATA[3] == '=')) { // blinds set 100 % close time in 100 ms
          int tmp = remoteHome.inputRadioData.substring(4).toInt();
          if ((tmp > 0) && (tmp <= 65535)) {
            motorControllerCounters[sw-1] = tmp;
            EEPROM.write(MOTOR_CONTROLLER_COUNTERS + ((sw-1)*2), highByte(motorControllerCounters[sw-1]));
            EEPROM.write(MOTOR_CONTROLLER_COUNTERS + (((sw-1)*2) + 1), lowByte(motorControllerCounters[sw-1]));
            remoteHome.sendOK();
            remoteHome.manageReceivedData();
            sendBlindStatus(sw);
          } else {
            remoteHome.sendERROR();
          }
        } else if ((char)remoteHome.radio.DATA[2] == 's') {
            remoteHome.manageReceivedData();
            sendBlindStatus(sw);
        } else {
          remoteHome.sendERROR();
        }
      } else {
        remoteHome.sendERROR();
      }
}
void serialEvent() {
  remoteHome.manageSerialEvent(Serial);
}
String getSwitchStatus(int sw) {
  return "1s|" + String(sw, DEC) + "|" + String(digitalRead(pins[sw-1]), DEC) + "|" + String(pinsPowerOnWhenStart[sw-1], DEC) + "|" + String(counterValues[sw-1]/60, DEC) + "|" + String(currentCounterValues[sw-1]/60, DEC) + "|" + String(lightSensorValues[sw-1], DEC) + "|" + String(currentLightSensorValues[sw-1], DEC);
}
String getBlindsStatus(int sw) {
  int currentClose = motorControllerCurrentClose[sw-1];
  if (currentMotorControllerCounters[sw-1] != 0) {
    float remainingPercent = ((float)(currentMotorControllerCounters[sw-1] * 100)) / ((float)motorControllerCounters[sw-1]);
    if (digitalRead(pins[(sw-1)*2]) == 1) {
      currentClose = motorControllerCurrentClose[sw-1] - remainingPercent;
    } else {
      currentClose = motorControllerCurrentClose[sw-1] + remainingPercent;
    }
  }
  return "1b|" + String(sw, DEC) + "|" + String(digitalRead(pins[(sw-1)*2]), DEC) + "|" + String(digitalRead(pins[((sw-1)*2)+1]), DEC) + "|" + String(motorControllerCounters[sw-1], DEC) + "|" + String(currentMotorControllerCounters[sw-1], DEC) + "|" + String(motorControllerCurrentClose[sw-1], DEC) + "|" + String(currentClose, DEC);
}
String getThermostatStatus(int sw) {
  char array[10];
  dtostrf(currentTemperatures[sw-1], 2, 2, array);
  char arrayExp[10];
  float expTmp = ((float)expectedTemperatures[sw-1])/2;
  dtostrf(expTmp, 2, 1, arrayExp);
  return "1t|" + String(sw, DEC) + "|" + String(digitalRead(pins[sw-1]), DEC) + "|" + String(array) + "|" + String(arrayExp) + "|0." + String(currentTemperatureThresholds[sw-1], DEC) + "|" + String(heatingSourceId, DEC) + "|" + String(heatingSourceSubdeviceId, DEC);
}

String getVentilatorStatus(int sw) {
  return "1v|" + String(sw, DEC) + "|" + String(digitalRead(pins[sw-1]), DEC) + "|" + String(currentHumidity[sw-1], DEC) + "|" + String(currentCO2concentration[sw-1], DEC) + "|" + String(maxHumidity[sw-1], DEC) + "|" + String(maxCO2concentration[sw-1], DEC) + "|" + String(humidityTimeout[sw-1]/60, DEC) + "|" + String(co2concentrationTimeout[sw-1]/60, DEC) + "|" + String(currentCounterValues[sw-1]/60, DEC);
}
void switchOnHeatingSource() {
  if (heatingSourceId > 0) {
     String data = "l" + String(heatingSourceSubdeviceId, DEC) + "ot\n";
     remoteHome.len = data.length();
     data.toCharArray(remoteHome.buff, remoteHome.len+1); 
     remoteHome.sendRadioData(heatingSourceId);  
   }
}
void processTemperature(int sw) {
     float expTemp = ((float)expectedTemperatures[sw-1])/2;
     float thr = ((float)currentTemperatureThresholds[sw-1])/10;
     if ((expTemp > 0) && (currentTemperatures[sw-1] >= (expTemp + thr))) {
          digitalWrite(pins[sw-1], LOW);
          heatingSourceOn = false;
     }
     if ((expTemp > 0) && (currentTemperatures[sw-1] <= (expTemp - thr))) {
          digitalWrite(pins[sw-1], HIGH);
          heatingSourceOn = true;
     }
}
void processHumidity(int sw) {
     if (currentHumidity[sw-1] > maxHumidity[sw-1]) {
        if (currentCounterValues[sw-1] < humidityTimeout[sw-1]) {
            currentCounterValues[sw-1] = humidityTimeout[sw-1];
            digitalWrite(pins[sw-1], HIGH);
        }
     }
}
void processCo2(int sw) {
     if (currentCO2concentration[sw-1] > maxCO2concentration[sw-1]) {
        if (currentCounterValues[sw-1] < co2concentrationTimeout[sw-1]) {
            currentCounterValues[sw-1] = co2concentrationTimeout[sw-1];
            digitalWrite(pins[sw-1], HIGH);
        }
     }
}
void powerLost() {
      if (!powerLostReported) {
          remoteHome.buff[0] = 'L';
          remoteHome.buff[1] = 'P';
          remoteHome.len = 2;
          remoteHome.sendRadioData();
          powerLostReported = true;
      }
}
void runEach3mins() {
  if (heatingSourceOn) {
    switchOnHeatingSource();
  }
}
void runEach100ms() {
  for (int i=0; i<4; i++) {
    if (currentMotorControllerCounters[i] > 0) {
      if (--currentMotorControllerCounters[i] == 0) {
        digitalWrite(pins[i*2], LOW);
        digitalWrite(pins[(i*2)+1], LOW);
        // Send the status
        sendBlindStatus(i+1);
      }
    }
  }
}
void runEachTwoSec() {
  for (int i=0; i<4; i++) {
    if (currentMotorControllerCounters[i] > 0) {
        sendBlindStatus(i+1);
    }
  }
}
void runEachSec() {
  for (int i=0; i<8; i++) {
    if (currentCounterValues[i] > 0) {
      if (--currentCounterValues[i] == 0) {
        digitalWrite(pins[i], LOW);
        // Send the status
        sendLightStatus(i+1);
      }
    }
  }
}
