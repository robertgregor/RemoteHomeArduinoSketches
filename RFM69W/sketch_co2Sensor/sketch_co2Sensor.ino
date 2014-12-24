/*

CO2 sensor sketch

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
	If not set, or set to 0, then no sleep - usefull for testing, but consume lot of power and batery will be drained. The preheat time of the sensor is 3 minute, so the minimal frequency is 30 which is 5 minutes (300 seconds)
        Also note that preheat consume around 200 mA, so in case of e.g. 5 minutes period, the sensor will waste the battery quite fast. The good period for this sensor is 30 minutes = 180. The battery has 5000 mAh it is around 500 cycles which means 
        250 hours of the working time, it is around 10 days. If you will measure one time per hour the working time is 20 days.        
ocv=nnn - set the ventilator device Id, where the humidity should be sent. The range is 0 < nnn < 255.
ocvs=nnn - set the ventilator subdevice device Id, where the humidity should be sent. The range is 0 < nnn < 9.

Status format example: 1o|1|2.754|400 ppm|2.754|1.023|3.3|6.0|0|1|0|1
       1o - fixed unique value
       1 - Subdevice Id. This is fixed also depends on the hardware
       2.754 - read voltage from the sensor
       400 ppm - Calculated ppm value - for details see MG811 datasheet
       2.754 - calibrated voltage at 400 ppm value
       1.023 - calibrated voltage at 40000 ppm value
       3.3 - voltage at the microprocessor
       6.0 - voltage at the sensor heating element. Should be 6 V if it is bellow, you have to charge the battery.
       0 - sending period in seconds
       1 - 1st time 24 hours preheat indicator. 0 - no preheat was done. 1 - preheat executed.
       0 - Ventilator device Id
       1 - Ventilator Sub deviceId
       
Manual operation commands - for testing purpose only. Set first m=0 so the board will not sleep.
opo -   Switch the power to the sensor. This means the heating.
opf -   Switch off the power to the sensor.
os -    It will preheat the sensor for 3 minutes and then will send the value. 
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
EEPROM_POSITION_NODE_ID 0 //Node Id eeprom position
EEPROM_POSITION_NETWORK_ID 1 //Node Id eeprom position
EEPROM_POSITION_KEY 2 //Node Id eeprom position
EEPROM_SENDING_STATUS_PERIOD 18 //Sending status period position eeprom position
EEPROM_400ppm 19 //2 bytes 400ppm calibration value
EEPROM_40000ppm 21 //2 bytes 40000ppm calibration value
EEPROM_HEAT 23 //Indicates, if the 24 hours preheat has been done 
EEPROM_CO_ID 24 //Device ID, where to send the sensor.
EEPROM_CO_SUBDEVICE_ID 25 //Sub device ID, where to send the sensor.
*/


#include <EEPROM.h>
#include <avr/wdt.h>
#include <LowPower.h>
#include <RFM69.h>
#include <SPI.h>
#include <RemoteHome.h>
 

#define EEPROM_SENDING_STATUS_PERIOD 18 //Sending status period position eeprom position
#define EEPROM_400ppm 19 //2 bytes 400ppm calibration value
#define EEPROM_40000ppm 21 //2 bytes 40000ppm calibration value
#define EEPROM_HEAT 23 //Indicates, if the 24 hours preheat has been done
#define EEPROM_CO_ID 24 //Device ID, where to send the sensor.
#define EEPROM_CO_SUBDEVICE_ID 25 //Sub device ID, where to send the sensor.
#define ALIVE_AFTER_STARTUP 60000 //How long after startup the module should go to sleep period.
#define SENSOR_POWER 3
#define SENSOR_ANALOG A0
#define MAIN_BAT_ANALOG A1

RemoteHome remoteHome;
long interval=ALIVE_AFTER_STARTUP;
byte period = 0;
int sleepTimer = 0;
unsigned long previousMillis=0;
float v400ppm = 2.20;   //MUST BE SET ACCORDING TO CALIBRATION
float v40000ppm = 1.20; //MUST BE SET ACCORDING TO CALIBRATION
float co2voltageFloat = 0;
int co2heaterVoltage = 0;
byte heatIndicator = 0;
byte coDeviceId = 0;
byte coSubDeviceId = 0;

void setup() {
  // initialize serial
  Serial.begin(115200);
  Serial.println("Started");
  pinMode(SENSOR_POWER, OUTPUT);
  digitalWrite(SENSOR_POWER, LOW);
  pinMode(SENSOR_ANALOG, INPUT);
  remoteHome.setup(); 
  period = EEPROM.read(EEPROM_SENDING_STATUS_PERIOD);
  if (period == 255) period = 0;
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
  coDeviceId = EEPROM.read(EEPROM_CO_ID);
  if (coDeviceId == 255) coDeviceId = 0;
  coSubDeviceId = EEPROM.read(EEPROM_CO_SUBDEVICE_ID);
  if (coSubDeviceId == 255) coSubDeviceId = 1;  
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
      Serial.print("\tDevice type:CO2 sensor");
      Serial.print("\tPeriod:");
      Serial.print(period, DEC);
      Serial.print("\tSensor power:");
      Serial.print(digitalRead(SENSOR_POWER), DEC);
      Serial.print("\tBattery:");
      Serial.print(remoteHome.readVcc(), DEC);
      Serial.print("\tVentilator device Id:");
      Serial.print(coDeviceId, DEC);
      Serial.print("\tVentilator sub device Id:");
      Serial.print(coSubDeviceId, DEC);      
      Serial.println();
    } else if (lowerCaseInput.startsWith("at+om=")) {
      period = lowerCaseInput.substring(5).toInt();
      if ((period >= 30) && (period <= 255)) {
          EEPROM.write(EEPROM_SENDING_STATUS_PERIOD, period);
          remoteHome.printOK(Serial);
      } else {
          remoteHome.printERROR(Serial);
      }
    }
    remoteHome.inputString = "";
    remoteHome.stringComplete = false;
  }
  //check and manage radio
  if (remoteHome.processCommonRadioData()) {
    if ((char)remoteHome.radio.DATA[0] == 'o') {
     if ((char)remoteHome.radio.DATA[1] == 's') {
       remoteHome.sendOK();
       remoteHome.manageReceivedData();
       digitalWrite(SENSOR_POWER, HIGH);
       wdt_disable();
       for (int i=0; i<18;i++) {
         LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
         LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
       }
       wdt_enable(WDTO_4S);
       readSensor();
       readHeaterVoltage();
       digitalWrite(SENSOR_POWER, LOW);
       String status = getStatus();       
       remoteHome.len = status.length();
       status.toCharArray(remoteHome.buff, remoteHome.len+1); 
       remoteHome.manageReceivedData();
     } else if ((char)remoteHome.radio.DATA[1] == 'h') {
       remoteHome.sendOK();
       remoteHome.manageReceivedData();
       digitalWrite(SENSOR_POWER, HIGH);
       wdt_disable();
       for (int i=0; i<8640;i++) {  // 8640 is 24 hours
         LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
         LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
       }
       wdt_enable(WDTO_4S);
       EEPROM.write(EEPROM_HEAT, 1);
       heatIndicator = 1;
       readSensor();
       readHeaterVoltage();
       digitalWrite(SENSOR_POWER, LOW);
       String status = getStatus();              
       remoteHome.len = status.length();
       status.toCharArray(remoteHome.buff, remoteHome.len+1); 
       remoteHome.manageReceivedData();      
     } else if (((char)remoteHome.radio.DATA[1] == 'p') && ((char)remoteHome.radio.DATA[2] == 'o')) {
       digitalWrite(SENSOR_POWER, HIGH);
       remoteHome.sendOK();
     } else if (((char)remoteHome.radio.DATA[1] == 'p') && ((char)remoteHome.radio.DATA[2] == 'f')) {
       digitalWrite(SENSOR_POWER, LOW);
       remoteHome.sendOK();
     } else if (((char)remoteHome.radio.DATA[1] == 'c') && ((char)remoteHome.radio.DATA[2] == 'l')) {
       remoteHome.sendOK();
       remoteHome.manageReceivedData();
       digitalWrite(SENSOR_POWER, HIGH);
       wdt_disable();
       for (int i=0; i<9;i++) {
         LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
         LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
       }
       wdt_enable(WDTO_4S);
       readSensor();
       unsigned int value = co2voltageFloat * 1000;
       unsigned int a = value/256;
       unsigned int b = value % 256;
       EEPROM.write(EEPROM_400ppm,a);
       EEPROM.write(EEPROM_400ppm+1,b);
       v400ppm = co2voltageFloat;
       readSensor();
       readHeaterVoltage();
       digitalWrite(SENSOR_POWER, LOW);
       String status = getStatus();              
       remoteHome.len = status.length();
       status.toCharArray(remoteHome.buff, remoteHome.len+1); 
       remoteHome.manageReceivedData();   
     } else if (((char)remoteHome.radio.DATA[1] == 'c') && ((char)remoteHome.radio.DATA[2] == 'v') && ((char)remoteHome.radio.DATA[3] == '=')) {
       String num = "";
       for (int i=4; i<255; i++) {
           if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
           num = num + (char)remoteHome.radio.DATA[i];
       }
       int recvNum = num.toInt();
       if ((recvNum >= 0) && (recvNum < 255)) {
         coDeviceId = recvNum;
         EEPROM.write(EEPROM_CO_ID, coDeviceId);
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
       if ((recvNum > 0) && (recvNum < 255)) {
         coSubDeviceId = recvNum;
         EEPROM.write(EEPROM_CO_SUBDEVICE_ID, coSubDeviceId);
         remoteHome.sendOK();        
       } else {
         remoteHome.sendERROR();
       }
     } else if (((char)remoteHome.radio.DATA[1] == 'c') && ((char)remoteHome.radio.DATA[2] == 'h')) {
       remoteHome.sendOK();
       remoteHome.manageReceivedData();
       digitalWrite(SENSOR_POWER, HIGH);
       wdt_disable();
       for (int i=0; i<9;i++) {
         LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
         LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
       }
       wdt_enable(WDTO_4S);
       readSensor();
       unsigned int value = co2voltageFloat * 1000;
       unsigned int a = value/256;
       unsigned int b = value % 256;
       EEPROM.write(EEPROM_40000ppm,a);
       EEPROM.write(EEPROM_40000ppm+1,b);
       v40000ppm = co2voltageFloat;
       readSensor();
       readHeaterVoltage();
       digitalWrite(SENSOR_POWER, LOW);
       String status = getStatus();              
       remoteHome.len = status.length();
       status.toCharArray(remoteHome.buff, remoteHome.len+1); 
       remoteHome.manageReceivedData();
     } else if ((char)remoteHome.radio.DATA[1] == 'r') {
       remoteHome.manageReceivedData();
       readSensor();
       readHeaterVoltage();
       String status = getStatus();
       remoteHome.len = status.length();
       status.toCharArray(remoteHome.buff, remoteHome.len+1); 
       remoteHome.manageReceivedData();      
     } else if (((char)remoteHome.radio.DATA[1] == 'm') && ((char)remoteHome.radio.DATA[2] == '=')) {
       String num = "";
       for (int i=3; i<255; i++) {
          if ((remoteHome.radio.DATA[i] == 10) || (remoteHome.radio.DATA[i] == 13)) break;
          num = num + (char)remoteHome.radio.DATA[i];
       }
       period = num.toInt();
       if ((period >= 30) && (period <= 255)) {
         EEPROM.write(EEPROM_SENDING_STATUS_PERIOD, period);
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
    if (interval == ALIVE_AFTER_STARTUP) interval = 200; //OK it is after start, after start it is running 1 minute. After that, it is running 200 ms and then sleep again
    sleepTimer = 0;
    while (1) {
      remoteHome.radio.sleep();
      wdt_disable();
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
      LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
      wdt_enable(WDTO_4S);
      if ((period) == (++sleepTimer)) {
        digitalWrite(SENSOR_POWER, HIGH);
        wdt_disable();
        for (int i=0; i<9;i++) {
          LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
          LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
        }
        wdt_enable(WDTO_4S);
        readSensor();
        readHeaterVoltage();
        digitalWrite(SENSOR_POWER, LOW);
        if (coDeviceId > 0) {
          String status = getCO2Data();
          remoteHome.len = status.length();
          status.toCharArray(remoteHome.buff, remoteHome.len+1); 
          remoteHome.sendRadioData(coDeviceId);
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
  int co2value = 0;
  for (int i=0; i<3; i++) {
    co2value += analogRead(SENSOR_ANALOG);
    delay(30);
  }
  co2value=co2value/3;
  //convert to voltage
  co2voltageFloat = co2value*(3.3/1023.0);
}
void readHeaterVoltage() {
  float tmp = analogRead(MAIN_BAT_ANALOG);
  tmp = (tmp*(3.3/1023.0))*2;
  co2heaterVoltage = tmp*10;
}
String getStatus() {
        //Send sensor status
        int bat = remoteHome.readVcc();
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
        return "1o|1|" + String(co2voltage) + "|" + String(co2ppm) + " ppm|" + String(v400) + "|" + String(v40000) + "|" + String(bat / 10, DEC) + "." + String(bat % 10, DEC) + "|" + String(co2heaterVoltage / 10, DEC) + "." + String(co2heaterVoltage % 10, DEC) + "|" + String(period*10, DEC) + "|" + String(heatIndicator, DEC) + "|" + String(coSubDeviceId, DEC) + "|" + String(coDeviceId, DEC);
}
String getCO2Data() {
        //Send CO2
        if (co2voltageFloat > v400ppm) co2voltageFloat = v400ppm;
        if (co2voltageFloat < v40000ppm) co2voltageFloat = v40000ppm;        
        // Calculate co2 from log10 formula (see sensor datasheet)
        float deltavs = v400ppm - v40000ppm;
        float A = deltavs/(-2.0);
        float B = 2.60206;
        float power = ((co2voltageFloat - v400ppm)/A) + B;
        char co2ppm[10];
        double ppm = pow(10,power);
        if (ppm < 400) ppm = 400;
        if (ppm > 40000) ppm = 40000;        
        dtostrf(ppm, 1, 0, co2ppm);        
        return "o" + String(coSubDeviceId, DEC) + "co=" + String(co2ppm) + "\n";
}

