#include <EEPROM.h>
#include <avr/wdt.h>
#include <LowPower.h>
#include <RFM69.h>
#include <SPI.h>
#include <RemoteHome.h>
 

#define EEPROM_SENDING_STATUS_PERIOD 18 //Sending status period position eeprom position
#define ALIVE_AFTER_STARTUP 60000 //How long after startup the module should go to sleep period.

#define EEPROM_SENSOR_ENABLED_FLAG 19 //Node Id eeprom position
#define MOTIONINT     1 //Interrupt 1 on D3
#define INPUTPIN     3 //D3 

RemoteHome remoteHome;
long interval=ALIVE_AFTER_STARTUP;
byte period = 0;
int sleepTimer = 0;
unsigned long previousMillis=0;
byte enabled = 0;
byte motionDetected=0; 

void setup() {
  // initialize serial
  Serial.begin(115200);
  Serial.println("Started");
  remoteHome.setup(); 
  period = EEPROM.read(EEPROM_SENDING_STATUS_PERIOD);
  if (period == 255) period = 0;
  enabled = EEPROM.read(EEPROM_SENSOR_ENABLED_FLAG);
  if (enabled == 255) enabled = 0;
  pinMode(INPUTPIN, INPUT_PULLUP);
  attachInterrupt(MOTIONINT, motionIRQ, CHANGE);
  delay(100);
  
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
  if (remoteHome.processCommonRadioData()) {
    if ((char)remoteHome.radio.DATA[0] == 's') {
      remoteHome.manageReceivedData();
      sendStatus();
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
        sendStatus();
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
void sendStatus() {
        //Send sensor status
        remoteHome.buff[0] = '8';
        remoteHome.buff[1] = '|';
        remoteHome.buff[2] = 48 + period;
        remoteHome.buff[3] = '|';        
        int bat = remoteHome.readVcc();
        remoteHome.buff[4] = 48 + bat / 10;
        remoteHome.buff[5] = '.';
        remoteHome.buff[6] = 48 + bat % 10;
        remoteHome.buff[7] = '|';
        remoteHome.buff[8] = 48 + enabled;
        remoteHome.buff[9] = '|';        
        remoteHome.buff[10] = 48 + digitalRead(INPUTPIN);
        remoteHome.len = 11;  
}
void motionIRQ() {
       wdt_enable(WDTO_4S);  
       byte motion = digitalRead(INPUTPIN) + 1;
       if ((enabled == 1) && (motion != motionDetected)) {
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
          Serial.println("OPENED");
        } else if (motionDetected == 1) {
          //Close
          remoteHome.buff[0] = 'C';
          remoteHome.buff[1] = 'L';
          remoteHome.buff[2] = 'O';
          remoteHome.buff[3] = 'S';
          remoteHome.buff[4] = 'E';
          remoteHome.buff[5] = 'D';
          remoteHome.len = 6;
          Serial.println("CLOSED");
        }
        remoteHome.sendRadioData();
        previousMillis = millis() + interval;
       }
}
