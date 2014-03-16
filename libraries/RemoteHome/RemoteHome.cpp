#include <RemoteHome.h>
#include <avr/wdt.h>
#include <RFM69registers.h>
#include <RFM69.h>
#include <SPI.h>
#include <EEPROM.h>


byte RemoteHome::networkId;
byte RemoteHome::nodeId;
char RemoteHome::key[17] = "thisIsEncryptKey"; //has to be same 16 characters/bytes on all nodes, not more not less!
RFM69 RemoteHome::radio;
byte RemoteHome::len;
char RemoteHome::buff[100];
boolean RemoteHome::reset_flag = false;
String RemoteHome::inputString = "";         // a string to hold incoming data
boolean RemoteHome::stringComplete = false;  // whether the string is complete

char previousChar = 0; //Needs to keep previous serial char to recognize stk500 sync command and jump to bootloader.
String inputRadioData = "";
byte inputSenderId;
unsigned long keepRadioMillis = 0;
int eepromId = 0; //used to store the address of the eeprom for the eeprom common setup
typedef void (*AppPtr_t) (void);
AppPtr_t GoToBootloader = (AppPtr_t)0x3F05;

RemoteHome::RemoteHome(){}

RemoteHome::~RemoteHome(){}

void RemoteHome::reset() {
  wdt_disable();
  inputString = "";
  stringComplete = false;
  wdt_enable(WDTO_1S);
  while(1);    
}
void RemoteHome::setup() {
  setup(1);
}
void RemoteHome::setup(int node) {
  wdt_disable();
  delay(400); //needs to be there to stabilise power supply before the radio module is initialized.
  wdt_enable(WDTO_4S);
  nodeId = EEPROM.read(EEPROM_POSITION_NODE_ID);
  if (nodeId == 255) nodeId = node;
  networkId = EEPROM.read(EEPROM_POSITION_NETWORK_ID);
  if (networkId == 255) networkId = 1;	
  if (EEPROM.read(EEPROM_POSITION_KEY) != 255) {
    for (int i=0; i<16; i++) {
      key[i] = EEPROM.read(i+EEPROM_POSITION_KEY);
    }
  }
  radio.initialize(FREQUENCY,nodeId,networkId);
  radio.encrypt(key);
  //radio.setHighPower(); //must uncomment only for RFM69HW!
  inputString.reserve(100);
}
boolean RemoteHome::sendRadioData() {
	sendRadioData(255); //Transceiver
}
boolean RemoteHome::sendRadioData(int destinationId) {
  int retryCnt = 0;
  while (1) {
      if (len == 0) return true;
      boolean st = radio.sendWithRetry(destinationId, buff, len);
      if (st) {
        len = 0;
        return true;
      } else {
        if (++retryCnt > 8) {
          len = 0;
          return false;
        } else {
          delay(random(35, 100));
        }
      }
  }
}
void RemoteHome::manageSerialEvent(HardwareSerial &_ser) {
  while (_ser.available()) {
    // get the new byte:
    char inChar = (char)_ser.read();    
    if ((inChar == 32) && (previousChar == 48)) {
        noInterrupts();
        GoToBootloader();
        break;
    } 
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    previousChar = inChar;
    if ((inChar == 10) || (inChar == 13)) {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
}
void RemoteHome::sendOK() {
      buff[0] = 'O';
      buff[1] = 'K';
      len = 2;
}
void RemoteHome::sendERROR() {
      buff[0] = 'E';
      buff[1] = 'R';
      buff[2] = 'R';
      buff[3] = 'O';
      buff[4] = 'R';
      len = 5;
}
void RemoteHome::printOK(HardwareSerial &_ser) {
  _ser.println("OK");
}
void RemoteHome::printERROR(HardwareSerial &_ser) {
  _ser.println("ERROR");
}
void RemoteHome::printCommonConfig(HardwareSerial &_ser) {
      _ser.print("+Device ID:");
      _ser.print(nodeId, DEC);     
      _ser.print("\tChannel:");
      _ser.print(networkId, DEC);     
      _ser.print("\tPassword:");
      for (int i=0; i<16; i++) {
        _ser.print(key[i]);
      }
}
void RemoteHome::clearEEPROM() {
  for (int i=0; i<1024; i++) {
    EEPROM.write(i, 255);
  }
}
void RemoteHome::manageReceivedData() {
    if (radio.ACK_REQUESTED) {
      radio.sendACK();
    }
    if (len > 0) {
      sendRadioData();
      radio.DATA[0] = 0;   
    }
    if (reset_flag) {
      reset_flag = false;
      reset();
    }
}
/*
	return true - if data should be managed
*/
boolean RemoteHome::processCommonRadioData() {
  if (((unsigned long)(millis() - keepRadioMillis)) >= 1000) { // do not manage the same data within one second
      inputRadioData = "";
      inputSenderId = 0;
      keepRadioMillis = millis();
  }
  if (radio.receiveDone()) {
    String data((const char *)radio.DATA);
    if ((data != inputRadioData) || (inputSenderId != radio.SENDERID)) {
      inputRadioData = data;
      inputSenderId = radio.SENDERID;
      keepRadioMillis = millis();
      if (((char)radio.DATA[0] == 'p') && ((char)radio.DATA[1] == 'n')) {
        sendOK();
	  manageReceivedData();
	  return false;
      } else if (((char)radio.DATA[0] == 'n') && ((char)radio.DATA[1] == 'n')) {
        len = 0;
	  manageReceivedData();
	  return false;
      } else if (((char)radio.DATA[0] == 'r') && ((char)radio.DATA[1] == 'e') && ((char)radio.DATA[2] == 's')) {
        sendOK();
        reset_flag = true;
	  manageReceivedData();
	  return false;
      } else if (((char)radio.DATA[0] == 'd') && ((char)radio.DATA[1] == 'e') && ((char)radio.DATA[2] == 'f')) {
        sendOK();
        clearEEPROM();
        reset_flag = true;
	  manageReceivedData();
	  return false;
      } else {
	  return true;
      }
    } else {
      len = 0;
      manageReceivedData();
      return false;
    }    
  } else {
    return false;
  }
}
/*
	return true - if data should be managed
*/
boolean RemoteHome::processCommonSerialData(HardwareSerial &_ser) {
  if (stringComplete) {
    String lowerCaseInput = inputString;
    lowerCaseInput.toLowerCase();
    if (inputString == "") {
      //do nothing, could happen if the end of line is CR & LF
      inputString = "";
      stringComplete = false;
      return false; 
    } else if (lowerCaseInput.startsWith("at+c=")) {
      byte networkId_temp = inputString.substring(5).toInt();
      if ((networkId_temp < 1) || (networkId_temp > 255)) {
        printERROR(_ser);
        inputString = "";
        stringComplete = false;
        return false;
      } else {
        EEPROM.write(EEPROM_POSITION_NETWORK_ID, networkId_temp);
        printOK(_ser);
        inputString = "";
        stringComplete = false;
	networkId = networkId_temp;
        reset();
        return false;
      }
    } else if (lowerCaseInput.startsWith("at+n=")) {
      byte nodeId_temp = inputString.substring(5).toInt();
      if ((nodeId_temp < 1) || (nodeId_temp > 254)) {
        printERROR(_ser);
        inputString = "";
        stringComplete = false;
        return false;
      } else {
         EEPROM.write(EEPROM_POSITION_NODE_ID, nodeId_temp);
         printOK(_ser);
         inputString = "";
         stringComplete = false;
         nodeId = nodeId_temp;
         reset();
         return false;
      }
    } else if (lowerCaseInput.startsWith("at+res")) {
      printOK(_ser);
      inputString = "";
      stringComplete = false;
      reset();
      return false; 
    } else if (lowerCaseInput.startsWith("at+memid=")) {
      eepromId = inputString.substring(9).toInt();
      printOK(_ser);
      inputString = "";
      stringComplete = false;
      return false; 
    } else if (lowerCaseInput.startsWith("at+memval=")) {
      EEPROM.write(eepromId, inputString.substring(10).toInt());
      printOK(_ser);
      inputString = "";
      stringComplete = false;
      return false; 
    } else if (lowerCaseInput.startsWith("at+def")) {
      clearEEPROM();
      printOK(_ser);
      inputString = "";
      stringComplete = false;
      reset();
      return false; 
    } else if (lowerCaseInput.startsWith("at+p=")) {
      String pwd = inputString.substring(5);
      if (pwd.length() == 16) {
        for (int i=0; i<16; i++) {
          key[i] = inputString.charAt(i+5);
          EEPROM.write(i+EEPROM_POSITION_KEY, key[i]);
        }
        printOK(_ser);
        inputString = "";
        stringComplete = false;
        reset();
        return false; 
      } else {
        printERROR(_ser);
        inputString = "";
        stringComplete = false;
        return false; 
      }
    } else {
      return true;
    }
  } else {
	return false;
  }
}
int RemoteHome::readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else    
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif
  delay(2);  // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both
  long result = (high<<8) | low;
  result = 11253L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result;
}