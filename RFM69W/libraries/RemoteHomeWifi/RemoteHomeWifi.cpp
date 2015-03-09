#include <RemoteHomeWifi.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <EEPROM.h>
#include <Arduino.h>

#define ANSWER_READY "ready"
#define ANSWER_OK "OK"
#define ANSWER_LINKED "Linked"
#define ANSWER_FAIL "FAIL"
#define ANSWER_NO_CHANGE "no change"
#define ANSWER_DATA_OK "SEND OK"
#define AT_STARTDATA ">"

byte RemoteHomeWifi::nodeId;
String RemoteHomeWifi::inputString;         // a string to hold incoming data
String RemoteHomeWifi::outputString;        // a string to hold outgoing data
String RemoteHomeWifi::menuString;          // a string to hold additional menu items
String RemoteHomeWifi::pageHeadString;      // a string to hold additional <head> info
boolean RemoteHomeWifi::stringComplete = false;  // whether the string is complete
boolean RemoteHomeWifi::connectedToWifi = false;  // whether the device is connected to wifi or not
char previousChar = 0; //Needs to keep previous serial char to recognize stk500 sync command and jump to bootloader.
int eepromId = 0; //used to store the address of the eeprom for the eeprom common setup
int communicationChannelId = 0;
typedef void (*AppPtr_t) (void);
AppPtr_t GoToBootloader = (AppPtr_t)0x3F05;

const char AT_CMD_RST[] PROGMEM = "AT+RST";
const char AT_ADHOC_NET[] PROGMEM = "AT+CWMODE=3";
const char AT_JOIN_AP[] PROGMEM = "AT+CWMODE=1";
const char GET_IP_ADDRESS[] PROGMEM = "AT+CIFSR";
const char AT_SET_MULTIPLE_CONNECTION[] PROGMEM = "AT+CIPMUX=1";
const char AT_SET_SINGLE_CONNECTION[] PROGMEM = "AT+CIPMUX=0";
const char AT_SET_DATA_MODE[] PROGMEM = "AT+CIPMODE=1";
const char AT_SET_SERVER_PORT80[] PROGMEM = "AT+CIPSERVER=1,80";
const char AT_SET_SERVER_DISABLE[] PROGMEM = "AT+CIPSERVER=0";
const char AT_SET_SERVER_TIMEOUT2S[] PROGMEM = "AT+CIPSTO=2";
const char AT_JOIN_AP_PARAMS[] PROGMEM = "AT+CWJAP=\"";
const char AT_CHECK_AP_CONNECTION[] PROGMEM = "AT+CWJAP?";
const char AT_QUOTATION_MARK[] PROGMEM = "\"";
const char AT_PLUS_MARK[] PROGMEM = "+";
const char AT_QUOTATION_MARKS_WITH_COMMA[] PROGMEM = "\",\"";
const char AT_SEND_DATA[] PROGMEM = "AT+CIPSEND=";
const char AT_SEND_DATA_SINGLECH[] PROGMEM = "AT+CIPSEND";
const char AT_CREATE_TCP_CONNECTION[] PROGMEM = "AT+CIPSTART=1,\"TCP\",\"";
const char AT_CREATE_SINGLE_TCP_CONNECTION[] PROGMEM = "AT+CIPSTART=\"TCP\",\"";
const char AT_COMMA[] PROGMEM = ",";
const char AT_CLOSE_CONNECTION[] PROGMEM = "AT+CIPCLOSE=";
const char HTTP_RESPONSE_NOT_FOUND[] PROGMEM = "HTTP/1.0 404 Not Found\nConnection: close\n\n";
const char HTML_START[] PROGMEM = "HTTP/1.0 200 OK\nContent-Type: text/html\nConnection: close\n\n<html><head>";
const char HTML_START2[] PROGMEM = "<style>body{background-color:#2F4F4F;}p{background-color:orange;font-family:verdana;font-size:120%;}table{background-color:orange;}</style></head><body><p><a href='/'>Wifi config</a>&nbsp;|&nbsp;<a href='ca'>Device config</a>&nbsp;|&nbsp;<a href='cb'>Sketch upload</a>";
const char HTML_P_END[] PROGMEM = "</p>";
const char HTML_END[] PROGMEM = "</body></html>";
const char WIFI_CONFIG[] PROGMEM = "WIFI config";
const char DEVICE_CONFIG[] PROGMEM = "Device config";
const char CAPTION_WIFI_SSID[] PROGMEM = "Wifi SSID:";
const char CAPTION_WIFI_PWD[] PROGMEM = "Wifi password:";
const char CAPTION_SERVER_IP[] PROGMEM = "Server IP:";
const char CAPTION_SERVER_PORT[] PROGMEM = "Server port:";
const char CAPTION_PGM_PORT[] PROGMEM = "Programming port:";
const char CAPTION_DEVICE_ID[] PROGMEM = "Device ID:";
const char ACTION_S[] PROGMEM = "s";
const char ACTION_P[] PROGMEM = "p";
const char ACTION_R[] PROGMEM = "r";
const char ACTION_D[] PROGMEM = "d";
const char MAXSIZE_32[] PROGMEM = "32";
const char MAXSIZE_64[] PROGMEM = "64";
const char MAXSIZE_15[] PROGMEM = "15";
const char MAXSIZE_5[] PROGMEM = "5";
const char MAXSIZE_4[] PROGMEM = "4";
const char WIFI_CONFIG_ACTION[] PROGMEM = "cc";
const char DEVICE_CONFIG_ACTION[] PROGMEM = "ce";


RemoteHomeWifi::RemoteHomeWifi(HardwareSerial& serial) : _ser(serial) {
}

void RemoteHomeWifi::printString(const char str[]) {
  char c;
  if(!str) return;
  while((c = pgm_read_byte(str++))) _ser.print(c);
}

void RemoteHomeWifi::printStr(String &str) {
    _ser.print(str);
}
void RemoteHomeWifi::concatString(const char str[]) {
  char c;
  if(!str) return;
  while((c = pgm_read_byte(str++))) outputString += c;
}
int RemoteHomeWifi::countString(const char str[]) {
  int cnt;
  if(!str) return cnt;
  while(pgm_read_byte(str++)) cnt++;
  return cnt;
}
void RemoteHomeWifi::reset() {
  wdt_disable();
  inputString = "";
  stringComplete = false;
  wdt_enable(WDTO_1S);
  while(1);    
}
void RemoteHomeWifi::cleanSerialBuffer() {
    delay(5);
    while (_ser.available()) _ser.read();
}
boolean RemoteHomeWifi::sendDataToServer() {
    return establishConnectionToServer(false);
}
boolean RemoteHomeWifi::establishConnectionToServer(boolean progPort) {
     cleanSerialBuffer();
     if (progPort) {
        sendATCommand(AT_SET_SERVER_DISABLE, ANSWER_OK);
        sendATCommand(AT_CMD_RST, ANSWER_READY);
        waitToConnectToNetwork(20);
        sendATCommand(AT_SET_DATA_MODE, ANSWER_OK);
        sendATCommand(AT_SET_SINGLE_CONNECTION, ANSWER_OK);
        printString(AT_CREATE_SINGLE_TCP_CONNECTION);
     } else {
        communicationChannelId = 1;
        printString(AT_CREATE_TCP_CONNECTION);
     }
    char* ip = readIpAddrFromEEPROM(EEPROM_POSITION_SERVER_IP);
    _ser.print(ip);
    free(ip);
    printString(AT_QUOTATION_MARK);
    printString(AT_COMMA);
    char* port;
    if (progPort) port = readIntFromEEPROM(EEPROM_POSITION_SERVER_PROGPORT); else readIntFromEEPROM(EEPROM_POSITION_SERVER_PORT);
    _ser.print(port);
    _ser.println();
    if (_ser.find(ANSWER_LINKED)) {    
        if (!progPort) {
            prepareDataToSend(outputString.length());
            printStr(outputString);
            _ser.find(ANSWER_DATA_OK);
        } else {
            printString(AT_SEND_DATA_SINGLECH);
            _ser.println();
        }
        return true;
    } else return false;
}
void RemoteHomeWifi::prepareDataToSend(int len) {
    cleanSerialBuffer();
    printString(AT_SEND_DATA);
    _ser.print(communicationChannelId, DEC);
    printString(AT_COMMA);
    _ser.println(len, DEC);
    _ser.find(AT_STARTDATA);
}
void RemoteHomeWifi::sendDataNotFound() {
    prepareDataToSend(strlen(HTTP_RESPONSE_NOT_FOUND));
    printString(HTTP_RESPONSE_NOT_FOUND);
    _ser.find(ANSWER_DATA_OK);
}
void RemoteHomeWifi::sendPageWithMenuAndHeaderResponse() {
    int ln = outputString.length()+menuString.length()+pageHeadString.length()+strlen(HTML_START)+strlen(HTML_START2)+strlen(HTML_END)+strlen(HTML_P_END);
    prepareDataToSend(ln);
    printString(HTML_START);
    printStr(pageHeadString);
    printString(HTML_START2);    
    printStr(menuString);
    printString(HTML_P_END);
    printStr(outputString);
    printString(HTML_END);
    _ser.find(ANSWER_DATA_OK);
    printString(AT_CLOSE_CONNECTION);
    _ser.println(communicationChannelId, DEC);
    _ser.find(ANSWER_DATA_OK);
}
boolean RemoteHomeWifi::sendATCommand(const char cmd[], char* answer) {
    delay(10);
    for (int i=0; i<2; i++) {
        cleanSerialBuffer();
        printString(cmd);
        _ser.println();
        boolean result = _ser.find(answer);
        if (result) return true;
    }
    return false;
}
void RemoteHomeWifi::listenOnPort() {
  sendATCommand(AT_SET_MULTIPLE_CONNECTION, ANSWER_OK);
  sendATCommand(AT_SET_SERVER_PORT80, ANSWER_OK);
  sendATCommand(AT_SET_SERVER_TIMEOUT2S, ANSWER_OK);  
}
void RemoteHomeWifi::becomeAdHocNetwork() {    
  delay(10);
  cleanSerialBuffer();
  printString(AT_ADHOC_NET);
  _ser.println();
  _ser.findUntil(ANSWER_OK, ANSWER_NO_CHANGE);  
}
boolean RemoteHomeWifi::waitToConnectToNetwork(int attempts) {
  _ser.setTimeout(100);
  cleanSerialBuffer(); 
  while(attempts-- > 0) {
      if (sendATCommand(AT_CHECK_AP_CONNECTION, ANSWER_OK)) {
          setDefaultSerialTimeout();
          connectedToWifi = true;
          return true;
      }
  }
  setDefaultSerialTimeout();
  connectedToWifi = false;
  return false;
}
void RemoteHomeWifi::setSingleJoinNetwork() {
  cleanSerialBuffer();
  printString(AT_JOIN_AP);
  _ser.println();
  _ser.findUntil(ANSWER_OK, ANSWER_NO_CHANGE);  
}
void RemoteHomeWifi::setDefaultSerialTimeout() {
    _ser.setTimeout(2000);
}
String RemoteHomeWifi::getIPAddress() {
  cleanSerialBuffer();
  byte ipaddress[50];
  printString(GET_IP_ADDRESS);
  _ser.println();
  int read = _ser.readBytesUntil('O', ipaddress, 50);  
  String retIp;
  for(int k=0; k<read; k++){
      if (((ipaddress[k] > 47) && (ipaddress[k] < 58)) || (ipaddress[k] == '.')) retIp += String((char)ipaddress[k]);
      if ((ipaddress[k] == 10) || (ipaddress[k] == 13)) retIp += " ";
  }
  return retIp; 
}

boolean RemoteHomeWifi::joinNetwork(String ssid, String password) {
  cleanSerialBuffer();
  printString( AT_JOIN_AP_PARAMS);
  printStr(ssid);
  printString(AT_QUOTATION_MARKS_WITH_COMMA);
  printStr(password);
  printString(AT_QUOTATION_MARK);
  _ser.println();
  _ser.setTimeout(19000);
  boolean ret = _ser.find(ANSWER_OK);
  setDefaultSerialTimeout();
  delay(20);
  return ret;
}
void RemoteHomeWifi::setup() {
    cleanVariablesAfterProcessing();
    delay(1010);
    for (int i=0;i<3;i++) printString(AT_PLUS_MARK);
    delay(1010);
    sendATCommand(AT_CMD_RST, ANSWER_READY);
    if (!waitToConnectToNetwork(50)) {
        connectedToWifi = false;
        becomeAdHocNetwork();
    } else {
        setSingleJoinNetwork();
        connectedToWifi = true;
    }
    setDefaultSerialTimeout();
    listenOnPort();
}
char* RemoteHomeWifi::getNetworkName() {
    //static char netName[33];
    char * netName = (char *) calloc (33, 1);
    if (connectedToWifi) {
        cleanSerialBuffer(); 
        printString(AT_CHECK_AP_CONNECTION);
        _ser.println();
        _ser.find("\"");
        _ser.readBytesUntil((char)34, netName, 32); 
    }
    return netName;
}
void RemoteHomeWifi::manageSerialEvent() {
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
      if (inputString.length() > 0) {
          stringComplete = true;
          if (inputString.startsWith("+IPD,")) {
              communicationChannelId = inputString.substring(5).toInt();
              inputString = inputString.substring(inputString.indexOf("/")+1);
              cleanSerialBuffer();
          } else {
              inputString = "";
              stringComplete = false;
          }
      }
    } else {
      inputString += inChar;
    }
  }
}
void RemoteHomeWifi::cleanVariablesAfterProcessing() {
                outputString = "";
                pageHeadString = "";
                inputString="";
                stringComplete = false;
}
void RemoteHomeWifi::createTextBoxTableRow(const char* title, const char* action, char* value, const char* maxSize) {
    outputString += F("<tr><td>");
    concatString(title);
    outputString += F("</td><td><input name='");
    concatString(action);
    outputString += F("' type='text' maxSize='");
    concatString(maxSize);
    outputString += F("' value='");
    outputString += value;
    outputString += F("'/></td></tr>");
}
void RemoteHomeWifi::createTableWithForm(const char* title, const char* action) {
    outputString += F("<FORM action='");
    concatString(action);
    outputString += F("' method='GET'><table><tr><th colspan='2'>");
    concatString(title);
    outputString += F("</th></tr>");
}
void RemoteHomeWifi::endTableWithForm() {
    outputString += F("</table></form>");
}
void RemoteHomeWifi::createSubmitButton() {
     outputString += F("<tr><td colspan='2'><input type='submit' value='Configure'/></td></tr>");
}
void RemoteHomeWifi::skipInputToChar(char c) {
    inputString = inputString.substring(inputString.indexOf(c)+1);
}
void RemoteHomeWifi::saveIntToEEPROM(const int position) {
            skipInputToChar('=');
            int number = inputString.toInt();
            EEPROM.write(position, number/256);
            EEPROM.write(position+1, number%256); 
}
char* RemoteHomeWifi::readIntFromEEPROM(const int position) {
            unsigned int high=EEPROM.read(position);
            unsigned int low=EEPROM.read(position+1);
            unsigned int result = high*256+low; 
            String a = String(result);
            static char sp[6];
            a.toCharArray(sp, a.length()+1);
            return sp;
}
void RemoteHomeWifi::saveByteToEEPROM(const int position) {
            skipInputToChar('=');
            EEPROM.write(position, inputString.toInt());
}
char* RemoteHomeWifi::readByteFromEEPROM(const int position) {
            String a = String(EEPROM.read(position));
            static char id[4];
            a.toCharArray(id, a.length()+1);            
            return id;
}
void RemoteHomeWifi::saveIpAddrToEEPROM(const int position) {
            skipInputToChar('=');
            for (int i=0; i<4; i++) {
                if (i!=0) skipInputToChar('.');
                EEPROM.write(position+i, inputString.toInt());                
            }
}
char* RemoteHomeWifi::readIpAddrFromEEPROM(const int position) {
            char * ip = (char *) calloc (16, 1);
            String a = String(EEPROM.read(EEPROM_POSITION_SERVER_IP));
            for (int i=1; i<4; i++) a += "." + String(EEPROM.read(EEPROM_POSITION_SERVER_IP+i));
            a.toCharArray(ip, a.length()+1);
            return ip;
}
boolean RemoteHomeWifi::processCommonData() {
    if (stringComplete) {
        //filter out the module strings
        if (inputString.startsWith("Link") || inputString.startsWith("Unlink")  || inputString.startsWith("Error")) {
            //ignore
            cleanVariablesAfterProcessing();
            return false;            
        }
        if (inputString.startsWith(" ")) {
            //wifi config
            createTableWithForm(WIFI_CONFIG, WIFI_CONFIG_ACTION);
            char* netName = getNetworkName();
            createTextBoxTableRow(CAPTION_WIFI_SSID, ACTION_S, netName, MAXSIZE_32);
            free(netName);
            createTextBoxTableRow(CAPTION_WIFI_PWD, ACTION_P, "", MAXSIZE_64);
            createSubmitButton();
            endTableWithForm();
        } else if (inputString.startsWith("ca")) {
            //device config
            createTableWithForm(DEVICE_CONFIG, DEVICE_CONFIG_ACTION);
            char* ip = readIpAddrFromEEPROM(EEPROM_POSITION_SERVER_IP);
            createTextBoxTableRow(CAPTION_SERVER_IP, ACTION_S, ip, MAXSIZE_15);
            free(ip);
            createTextBoxTableRow(CAPTION_SERVER_PORT, ACTION_P, readIntFromEEPROM(EEPROM_POSITION_SERVER_PORT), MAXSIZE_5);
            createTextBoxTableRow(CAPTION_PGM_PORT, ACTION_R, readIntFromEEPROM(EEPROM_POSITION_SERVER_PROGPORT), MAXSIZE_5);            
            createTextBoxTableRow(CAPTION_DEVICE_ID, ACTION_D, readByteFromEEPROM(EEPROM_POSITION_NODE_ID), MAXSIZE_4);
            createSubmitButton();
            endTableWithForm();
        } else if (inputString.startsWith("cb")) {
            //sketch upload
            pageHeadString = F("<meta http-equiv='refresh' content=\"60;URL='/'\"/>");
            outputString = F("<p>Programming, the page is going to reload after 1 min.</p>");
            sendPageWithMenuAndHeaderResponse();
            delay(100);
            cleanVariablesAfterProcessing();
            establishConnectionToServer(true);
        } else if (inputString.startsWith("cc")) {
                //it is join network request cc?s=SSID&p=Password HTTP/1.1
                String ssid = inputString.substring(5,inputString.indexOf('&'));
                String password = inputString.substring(inputString.indexOf('&')+3,inputString.indexOf(' '));
                pageHeadString = F("<meta http-equiv='refresh' cotent=\"25;URL='cd'\"/>");
                outputString = F("<p>Connecting, please wait, the result is going to be displayed within 25 seconds...</p>");
                sendPageWithMenuAndHeaderResponse();
                if (!joinNetwork(ssid, password)) {
                    becomeAdHocNetwork();
                    listenOnPort();
                    connectedToWifi = false;
                }
                cleanVariablesAfterProcessing();
        } else if (inputString.startsWith("cd")) {
                if (waitToConnectToNetwork(1)) {
                    outputString = F("<p>Connected. IP: <b>");
                    outputString += getIPAddress();
                    outputString += F("</b><BR>Please reserve the IP in your router.</p>");
                    sendPageWithMenuAndHeaderResponse();
                    delay(1000);
                    setSingleJoinNetwork();
                    outputString = "";
                } else {
                    outputString = F("<p>Not connected, please try again.</p>");
                }
        } else if (inputString.startsWith("ce")) {
            //it is configure device: ce?s=192.168.1.2&p=8080&r=8081&d=1 HTTP/1.1
            saveIpAddrToEEPROM(EEPROM_POSITION_SERVER_IP);
            saveIntToEEPROM(EEPROM_POSITION_SERVER_PORT);
            saveIntToEEPROM(EEPROM_POSITION_SERVER_PROGPORT);
            saveByteToEEPROM(EEPROM_POSITION_NODE_ID);
            outputString = F("<p>Configured.</p>");
        } else {
            return true;
        }
        if (outputString.length()!=0) {
            sendPageWithMenuAndHeaderResponse();
        }
        cleanVariablesAfterProcessing();
        return false;                                
    }
    return true;
}
void RemoteHomeWifi::clearEEPROM() {
  for (int i=0; i<1024; i++) {
    EEPROM.write(i, 255);
  }
}
int RemoteHomeWifi::readVcc() {
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