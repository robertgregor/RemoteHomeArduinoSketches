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

The co2 sensor is the device, which measures the CO2 concentration. Please note, that CO2 sensor needs 3 minutes preheat period, so it is not possible to get the reading in realtime.
Radio commands:

Commands
--------------------------------------------------------------------------------------------
oh -     Factory heat. The sensor used - MG811 needs to be 24 hours heated to work properly. This operation should be done only one time, when the sensor is new. Otherwise, it could read incorrect data. So use this command first time.
        When you will submit this command, make sure that the battery is fully charged. You can touch the sensor after while, you will see, that it is hot. After the issuing this command, the board will go to the sleep mode for 24 hours, so it will not 
        answer to any command. At the end, it will send the status with indication, that initial heat of the sensor is done.
ocl -    Calibration low: The sensor will need to calibrate. It should calibrate first in normal air, it means 400ppm. Make sure to put the board with sensor to the outside air, there should be around 20 degrees and no wind. Then issue 
        this command, wait 3 minute for the sensor preheat, then the value is read and the status is sent. The board is in the sleep mode for these 3 minutes so it will not answer to any command.
och -    Calibration high: The sensor will need to calibrate at 40000ppm, this is 100% concentration of the CO2. Take the small cup, where the sensor fully fit. Put there soda and vineger. Wait for the CO2. Then issue 
        this command, wait 3 minute for the sensor preheat, then the value is read and the status is sent. The board is in the sleep mode for these 3 minutes so it will not answer to any command. Now the sensor is fully ready.
om=nnn - Set the frequency and put to the sleep mode. 1 means 10 seconds, 254 means 2540 seconds. Needs to be set to put the device to sleep mode!!!
	The preheat time of the sensor is 90 seconds, so the minimal frequency is 10 which is 1 minutes, 40 seconds (100 seconds)
        The good period for this sensor is 10 minutes = 60.
ocv=nnn - set the ventilator device Id, where the humidity should be sent. The range is 0 < nnn < 255.
ocvs=nnn - set the ventilator subdevice device Id, where the humidity should be sent. The range is 0 < nnn < 9.

Status format example: 1o|4|2.754|400 ppm|2.754|1.023|3.3|6.0|3600|1|0|1
       1o - fixed unique value
       4 - fixed value - subdevice Id
       2.754 - read voltage from the sensor
       400 ppm - Calculated ppm value - for details see MG811 datasheet
       2.754 - calibrated voltage at 400 ppm value
       1.023 - calibrated voltage at 40000 ppm value
       3.3 - voltage at the microprocessor
       6.0 - voltage at the sensor heating element. Should be 6 V if it is bellow, you have to charge the battery.
       3600 - sending period in seconds
       1 - 1st time 24 hours preheat indicator. 0 - no preheat was done. 1 - preheat executed.
       0 - Ventilator device Id - it is hardcodded and is the same as the device id of the CO2 sensor device.
       1 - Ventilator Sub deviceId - could be from 1 - 3.
       
Manual operation commands - for testing purpose only. Set first m=0 so the board will not sleep.
opo -   Switch the power to the sensor. This means the heating.
opf -   Switch off the power to the sensor.
os -    It will preheat the sensor for 3 minutes and then will send the value. 
-------------------------------------------------------------------------------------------- 



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
EEPROM_POSITION_NETWORK_ID 1007 //Network Id eeprom position
EEPROM_POSITION_KEY 1008 //Encryption key eeprom position
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
EEPROM_CO2_SENDING_STATUS_PERIOD 93 //Sending status period position eeprom position
EEPROM_400ppm 94 //2 bytes 400ppm calibration value
EEPROM_40000ppm 96 //2 bytes 40000ppm calibration value
EEPROM_HEAT 98 //Indicates, if the 24 hours preheat has been done
EEPROM_CO_DEVICE_ID 99 //Sub device ID, where to send the sensor.
EEPROM_CO_SUBDEVICE_ID 100 //Sub device ID, where to send the sensor.
LIGHT_SENSOR_MAX_VALUES 101 //Light sensor border values to switch on.
EEPROM_LIGHTSENSOR_SENDING_STATUS_PERIOD 109 //Sending status period position eeprom position
EEPROM_LIGHTSENSOR_SWITCH_DEVICE_ID 110 //Sub device ID, where to send the sensor.
EEPROM_LIGHTSENSOR_SWITCH_SUBDEVICE_ID 111 //Sub device ID, where to send the sensor.
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
#define EEPROM_CO2_SENDING_STATUS_PERIOD 93 //Sending status period position eeprom position
#define EEPROM_400ppm 94 //2 bytes 400ppm calibration value
#define EEPROM_40000ppm 96 //2 bytes 40000ppm calibration value
#define EEPROM_HEAT 98 //Indicates, if the 24 hours preheat has been done
#define EEPROM_CO2_DEVICE_ID 99 //Sub device ID, where to send the sensor.
#define EEPROM_CO2_SUBDEVICE_ID 100 //Sub device ID, where to send the sensor.
#define LIGHT_SENSOR_MAX_VALUES 101 //Light sensor border values to switch on.
#define CO2_SENSOR_POWER 7
#define CO2_SENSOR_ANALOG A1
#define LIGHT_SENSOR_ANALOG A0
#define PIR_SENSOR 3
#define EEPROM_LIGHTSENSOR_SENDING_STATUS_PERIOD 109 //Sending status period position eeprom position
#define EEPROM_LIGHTSENSOR_SWITCH_DEVICE_ID 110 //Sub device ID, where to send the sensor.
#define EEPROM_LIGHTSENSOR_SWITCH_SUBDEVICE_ID 111 //Sub device ID, where to send the sensor.


RemoteHome remoteHome;
SimpleTimer timer;
byte swCounter[3];
int pins[] = {5,6,4};
byte pinsPowerOnWhenStart[] = {0,0,0};
unsigned int counterValues[] = {180,180,180};
unsigned int currentCounterValues[] = {0,0,0};
unsigned int currentLightSensorValues[] = {0,0,0};
unsigned int lightSensorValues[] = {0,0,0};
float currentTemperatures[] = {0,0,0};
byte expectedTemperatures[] = {42,42,42};
byte currentTemperatureThresholds[] = {3,3,3};
unsigned int currentCO2concentration[] = {0,0,0};
unsigned int maxCO2concentration[] = {1500,1500,1500};
unsigned int co2concentrationTimeout[] = {180,180,180};
unsigned int currentHumidity[] = {0,0,0};
unsigned int maxHumidity[] = {30,30,30};
unsigned int humidityTimeout[] = {180,180,180};
boolean powerLostReported = false;
int pinCount = 3;
byte heatingSourceId = 0;
byte heatingSourceSubdeviceId = 1;
boolean heatingSourceOn = false;
float v400ppm = 2.6;   //MUST BE SET ACCORDING TO CALIBRATION
float v40000ppm = 2.0; //MUST BE SET ACCORDING TO CALIBRATION
float co2voltageFloat = 0;
int co2heaterVoltage = 0;
byte heatIndicator = 0;
byte coDeviceId = 0;
byte coSubDeviceId = 0;
int co2period = 60;
int co2currentPeriod = 0;
float co2SensorAnalog = 0;
boolean co2calibrateLowInProgress = false;
boolean co2calibrateHighInProgress = false;
int preheatId; //Id of the timer for preheat the sensor.
boolean preheatInProgress = false;
int lightSensorPeriod = 0;
int lightSensorCurrentPeriod = 0;
byte lightSensorDeviceId = 0;
byte lightSensorSubDeviceId = 0;

void setup() {
  wdt_enable(WDTO_2S);
  // initialize serial
  Serial.begin(115200);
  Serial.println("Started");
  for (int thisPin = 0; thisPin < pinCount; thisPin++) pinMode(pins[thisPin], OUTPUT);
  pinMode(CO2_SENSOR_POWER, OUTPUT);
  digitalWrite(CO2_SENSOR_POWER, LOW);
  pinMode(CO2_SENSOR_ANALOG, INPUT);
  pinMode(LIGHT_SENSOR_ANALOG, INPUT);
  pinMode(PIR_SENSOR, INPUT);
  remoteHome.setup();
  //Simple switch dectarations
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
  int heatingSourceIdEeprom = EEPROM.read(HEATING_SOURCE_ID);
  if (heatingSourceIdEeprom != 255) heatingSourceId = heatingSourceIdEeprom;  
  int heatingSourceSubdeviceIdEeprom = EEPROM.read(HEATING_SOURCE_SUBDEVICE_ID);
  if (heatingSourceSubdeviceIdEeprom != 255) heatingSourceSubdeviceId = heatingSourceSubdeviceIdEeprom;
  //CO2 declarations
  co2period = EEPROM.read(EEPROM_CO2_SENDING_STATUS_PERIOD);
  if (co2period == 255) co2period = 60;
  co2currentPeriod = co2period;
  unsigned int a=EEPROM.read(EEPROM_400ppm);
  unsigned int b=EEPROM.read(EEPROM_400ppm+1);
  unsigned int result = a*256+b;
  if (result != 65535) {
    v400ppm = result;
    v400ppm = v400ppm / 1000;
  }
  a=EEPROM.read(EEPROM_40000ppm);
  b=EEPROM.read(EEPROM_40000ppm+1);
  result = a*256+b;
  if (result != 65535) {
    v40000ppm = result;
    v40000ppm = v40000ppm / 1000;
  }
  heatIndicator = EEPROM.read(EEPROM_HEAT);
  if (heatIndicator == 255) heatIndicator = 0;
  coDeviceId = EEPROM.read(EEPROM_CO2_DEVICE_ID);
  if (coDeviceId == 255) coDeviceId = remoteHome.nodeId;;
  coSubDeviceId = EEPROM.read(EEPROM_CO2_SUBDEVICE_ID);
  if (coSubDeviceId == 255) coSubDeviceId = 2;
  co2currentPeriod = 9;  
  timer.setInterval(1000, runEachSec);
  timer.setInterval(10000, runEach10Sec);
  timer.setInterval(180000, runEach3mins);
  preheatId = timer.setInterval(86400000, preheatTimer); //24hours
  timer.disable(preheatId);
  lightSensorPeriod = EEPROM.read(EEPROM_LIGHTSENSOR_SENDING_STATUS_PERIOD);
  if (lightSensorPeriod == 255) lightSensorPeriod = 6;
  lightSensorCurrentPeriod = lightSensorPeriod;
  lightSensorDeviceId = EEPROM.read(EEPROM_LIGHTSENSOR_SWITCH_DEVICE_ID);
  if (lightSensorDeviceId == 255) lightSensorDeviceId = remoteHome.nodeId;
  lightSensorSubDeviceId = EEPROM.read(EEPROM_LIGHTSENSOR_SWITCH_SUBDEVICE_ID);
  if (lightSensorSubDeviceId == 255) lightSensorSubDeviceId = 1;
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
      }
      Serial.println();
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
    } else if ((char)remoteHome.radio.DATA[0] == 'v') {
      processInputVentilatorData();
    } else if ((char)remoteHome.radio.DATA[0] == 'o') {
      processCO2Data();
    } else if ((char)remoteHome.radio.DATA[0] == 'g') {
      processLightSensorData();
    } else {
      remoteHome.sendERROR();
    }
    remoteHome.manageReceivedData();
  }
}

void sendLightSensorStatus() {
              // Send the status
              String status = getLightSensorStatus();
              remoteHome.len = status.length();
              status.toCharArray(remoteHome.buff, remoteHome.len+1);
              remoteHome.sendRadioData();                                            
}

void processLightSensorData() {
      if ((char)remoteHome.radio.DATA[1] == 's') {
        remoteHome.manageReceivedData();
        sendLightSensorStatus();
      } else if (((char)remoteHome.radio.DATA[1] == 'm') && ((char)remoteHome.radio.DATA[2] == '=')) {
        String num = "";
        for (int i=3; i<255; i++) {
            if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
            num = num + (char)remoteHome.radio.DATA[i];
        }
        int recvNum = num.toInt();
        if ((recvNum >= 0) && (recvNum <= 254)) {
          lightSensorPeriod = recvNum;
          EEPROM.write(EEPROM_LIGHTSENSOR_SENDING_STATUS_PERIOD, (byte)lightSensorPeriod);
          remoteHome.sendOK();
          remoteHome.manageReceivedData();
          sendLightSensorStatus();
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
          lightSensorDeviceId = recvNum;
          EEPROM.write(EEPROM_LIGHTSENSOR_SWITCH_DEVICE_ID, lightSensorDeviceId);
          remoteHome.sendOK();
          remoteHome.manageReceivedData();
          sendLightSensorStatus();          
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
          lightSensorSubDeviceId = recvNum;
          EEPROM.write(EEPROM_LIGHTSENSOR_SWITCH_SUBDEVICE_ID, lightSensorSubDeviceId);
          remoteHome.sendOK();
          remoteHome.manageReceivedData();
          sendLightSensorStatus();          
        } else {
          remoteHome.sendERROR();
        }                
      } else {
        remoteHome.sendERROR();
      }
}

void processCO2Data() {
    if (preheatInProgress) {
      remoteHome.sendERROR();
      remoteHome.manageReceivedData();
      return;
    }
    if ((char)remoteHome.radio.DATA[1] == 's') {
       remoteHome.sendOK();
       remoteHome.manageReceivedData();
       co2currentPeriod = 9;
    } else if ((char)remoteHome.radio.DATA[1] == 'h') {
       remoteHome.sendOK();
       remoteHome.manageReceivedData();
       timer.enable(preheatId);
       digitalWrite(CO2_SENSOR_POWER, HIGH);
       preheatInProgress = true;
    } else if (((char)remoteHome.radio.DATA[1] == 'p') && ((char)remoteHome.radio.DATA[2] == 'o')) {
       digitalWrite(CO2_SENSOR_POWER, HIGH);
       remoteHome.sendOK();
    } else if (((char)remoteHome.radio.DATA[1] == 'p') && ((char)remoteHome.radio.DATA[2] == 'f')) {
       if (!preheatInProgress) digitalWrite(CO2_SENSOR_POWER, LOW);
       remoteHome.sendOK();
    } else if (((char)remoteHome.radio.DATA[1] == 'c') && ((char)remoteHome.radio.DATA[2] == 'l')) {
       remoteHome.sendOK();
       remoteHome.manageReceivedData();
       co2currentPeriod = 9;
       co2calibrateLowInProgress = true;
     } else if (((char)remoteHome.radio.DATA[1] == 'c') && ((char)remoteHome.radio.DATA[2] == 'h')) {
       remoteHome.sendOK();
       remoteHome.manageReceivedData();
       co2currentPeriod = 9;
       co2calibrateHighInProgress = true;       
     } else if (((char)remoteHome.radio.DATA[1] == 'c') && ((char)remoteHome.radio.DATA[2] == 'v') && ((char)remoteHome.radio.DATA[3] == '=')) {
       String num = "";
       for (int i=4; i<255; i++) {
           if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
           num = num + (char)remoteHome.radio.DATA[i];
       }
       int recvNum = num.toInt();
       if ((recvNum >= 0) && (recvNum < 255)) {
         coDeviceId = recvNum;
         EEPROM.write(EEPROM_CO2_DEVICE_ID, coDeviceId);
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
       if ((recvNum > 0) && (recvNum < 9)) {
         coSubDeviceId = recvNum;
         EEPROM.write(EEPROM_CO2_SUBDEVICE_ID, coSubDeviceId);
         remoteHome.sendOK();        
       } else {
         remoteHome.sendERROR();
       }
     } else if ((char)remoteHome.radio.DATA[1] == 'r') {
       remoteHome.manageReceivedData();
       sendAndProcessCO2status();
     } else if (((char)remoteHome.radio.DATA[1] == 'm') && ((char)remoteHome.radio.DATA[2] == '=')) {
       String num = "";
       for (int i=3; i<255; i++) {
          if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
          num = num + (char)remoteHome.radio.DATA[i];
       }
       co2period = num.toInt();
       if ((num.toInt() >= 10) && (num.toInt() <= 255)) {
         co2period = num.toInt();
         EEPROM.write(EEPROM_CO2_SENDING_STATUS_PERIOD, co2period);
         if (!preheatInProgress) digitalWrite(CO2_SENSOR_POWER, LOW);
         co2currentPeriod = co2period;
         remoteHome.sendOK();
       } else {
         remoteHome.sendERROR();
       }
     } else {
       remoteHome.sendERROR();
     }
     remoteHome.manageReceivedData();
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
void serialEvent() {
  remoteHome.manageSerialEvent(Serial);
}
float readCoSensor() {
  int co2value = 0;
  for (int i=0; i<5; i++) {
    co2value += analogRead(CO2_SENSOR_ANALOG);
    delay(30);
  }
  co2value=co2value/5;
  //convert to voltage
  return co2value*(3.3/1023.0);
}
void sendAndProcessCO2status() {
        //read CO2 sensor
        float co2voltageFloat = co2SensorAnalog;
        if (co2voltageFloat > v400ppm) co2voltageFloat = v400ppm;
        if (co2voltageFloat < v40000ppm) co2voltageFloat = v40000ppm;
        char co2voltage[10];
        dtostrf(co2voltageFloat, 1, 3, co2voltage); 
        // Calculate co2 from log10 formula (see sensor datasheet)
        float deltavs = v400ppm - v40000ppm;
        float A = deltavs/(-2.0);
        float B = 2.60206;
        float power = ((co2voltageFloat - v400ppm)/A) + B;
        char co2ppm[10];
        double ppm = pow(10.0,power);
        if (ppm < 400) ppm = 400;
        if (ppm > 40000) ppm = 40000;
        dtostrf(ppm, 1, 0, co2ppm);
        char v400[10];
        dtostrf(v400ppm, 1, 3, v400);
        char v40000[10];
        dtostrf(v40000ppm, 1, 3, v40000);
        String status = "1o|4|" + String(co2voltage) + "|" + String(co2ppm) + " ppm|" + String(v400) + "|" + String(v40000) + "|3.3|6.0|" + String(co2period*10, DEC) + "|" + String(heatIndicator, DEC) + "|" + String(coDeviceId, DEC) + "|" + String(coSubDeviceId, DEC); // + "|" + String(co2SensorAnalog, DEC);
        remoteHome.len = status.length();
        status.toCharArray(remoteHome.buff, remoteHome.len+1);
        remoteHome.sendRadioData();
        //Now send the new value of CO2 to the target device
        if (coDeviceId != 0) {
          if (coDeviceId == remoteHome.nodeId) {
              //the device is this just set the new value:
              currentCO2concentration[coSubDeviceId-1] = ppm / 10;
          } else {
              status = "o" + String(coSubDeviceId, DEC) + "co=" + String(co2ppm) + 10;
              remoteHome.len = status.length();
              status.toCharArray(remoteHome.buff, remoteHome.len+1); 
              remoteHome.sendRadioData(coDeviceId);
          }
        }
}
String getSwitchStatus(int sw) {
  return "1s|" + String(sw, DEC) + "|" + String(digitalRead(pins[sw-1]), DEC) + "|" + String(pinsPowerOnWhenStart[sw-1], DEC) + "|" + String(counterValues[sw-1]/60, DEC) + "|" + String(currentCounterValues[sw-1]/60, DEC);
}
String getLightSensorStatus() {
  return "7|" + String(1024 - analogRead(LIGHT_SENSOR_ANALOG), DEC) + "|3.3|" + String(lightSensorPeriod*10, DEC) + "|" + String(lightSensorDeviceId, DEC) + "|" + String(lightSensorSubDeviceId, DEC);
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
void preheatTimer() {
       EEPROM.write(EEPROM_HEAT, 1);
       heatIndicator = 1;
       preheatInProgress = false;
       co2currentPeriod = 9;
       timer.disable(preheatId);
}
void runEach10Sec() {
  //process co2 sensor period
  if (preheatInProgress) return;  
  if (co2currentPeriod == 9) {
      digitalWrite(CO2_SENSOR_POWER, HIGH);
      co2SensorAnalog = 0;
  }
  if (co2currentPeriod <= 2) {
    //OK, here perform the readingand add it to co2SensorAnalog. When final read is done, calculate the average.
    co2SensorAnalog += readCoSensor();
  }
  if (co2currentPeriod == 0) {
      co2SensorAnalog = co2SensorAnalog / 3; // make the average value, reading 3 times done...
      if (co2calibrateLowInProgress) {
        co2calibrateLowInProgress = false;
        unsigned int value = co2SensorAnalog * 1000;
        unsigned int a = value/256;
        unsigned int b = value % 256;
        EEPROM.write(EEPROM_400ppm,a);
        EEPROM.write(EEPROM_400ppm+1,b);
        v400ppm = co2SensorAnalog;
      }
      if (co2calibrateHighInProgress) {
        co2calibrateHighInProgress = false;        
        unsigned int value = co2SensorAnalog * 1000;
        unsigned int a = value/256;
        unsigned int b = value % 256;
        EEPROM.write(EEPROM_40000ppm,a);
        EEPROM.write(EEPROM_40000ppm+1,b);
        v40000ppm = co2SensorAnalog;        
      }
      //OK, send the status
      sendAndProcessCO2status();
      if (!preheatInProgress) digitalWrite(CO2_SENSOR_POWER, LOW);
      co2currentPeriod = co2period;      
  }
  co2currentPeriod--;
  //process light sensor period
  if (lightSensorPeriod-- == 0) {
    sendLightSensorStatus();
    if (lightSensorDeviceId > 0) {
      //OK send the current light sensor value also to the configured switch
      if (lightSensorDeviceId == remoteHome.nodeId) {
              //the device is this just set the new value:
              currentLightSensorValues[lightSensorSubDeviceId-1] = 1024 - analogRead(LIGHT_SENSOR_ANALOG);
      } else {
              String status = "1" + String(lightSensorSubDeviceId, DEC) + "cl=" + String(1024 - analogRead(LIGHT_SENSOR_ANALOG), DEC) + "\n";
              remoteHome.len = status.length();
              status.toCharArray(remoteHome.buff, remoteHome.len+1); 
              remoteHome.sendRadioData(lightSensorDeviceId);
      }
    }
    lightSensorCurrentPeriod = lightSensorPeriod;
  }
}
