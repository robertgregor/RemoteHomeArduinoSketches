#include <RemoteHomeWifi.h>
#include <avr/eeprom.h>
#include <EEPROM.h>
#include <Arduino.h>

#define ENABLE_PIN 2
#define LED 13
#define ANSWER_READY "ready"
#define ANSWER_OK "OK"
#define ANSWER_ERROR "ERROR"
#define ANSWER_NO_AP "No AP"
#define ANSWER_FAIL "FAIL"
#define ANSWER_NO_CHANGE "no change"
#define ANSWER_DATA_OK "SEND OK"
#define AT_STARTDATA ">"

byte RemoteHomeWifi::nodeId;
String RemoteHomeWifi::inputString;         // a string to hold incoming data
String RemoteHomeWifi::outputString;        // a string to hold outgoing data
String RemoteHomeWifi::menuString;          // a string to hold additional menu items
String RemoteHomeWifi::pageHeadString;      // a string to hold additional <head> info
String RemoteHomeWifi::version;              // a string to hold the version of the software
boolean RemoteHomeWifi::stringComplete = false;  // whether the string is complete
boolean RemoteHomeWifi::connectedToWifi = false;  // whether the device is connected to wifi or not
char previousChar = 0; //Needs to keep previous serial char to recognize stk500 sync command and jump to bootloader.
int eepromId = 0; //used to store the address of the eeprom for the eeprom common setup
int communicationChannelId = 0;
typedef void (*AppPtr_t) (void);
AppPtr_t GoToBootloader = (AppPtr_t)0x3F05;
/* pointers to sketch functions */
void (*fpAppendConfigTable)() = 0;
void (*fpSaveConfigValues)() = 0;

const char CRLF[] PROGMEM = "\r\n";
const char AT_CMD_RST[] PROGMEM = "AT+RST";
const char AT_ADHOC_NET[] PROGMEM = "AT+CWMODE=3";
const char AT_JOIN_AP[] PROGMEM = "AT+CWMODE=1";
const char AT_ENABLE_DHCP[] PROGMEM = "AT+CWDHCP=1,0";
const char AT_DISABLE_DHCP[] PROGMEM = "AT+CWDHCP=1,1";
const char AT_SET_IP_ADDR[] PROGMEM = "AT+CIPSTA=\"";
const char GET_IP_ADDRESS[] PROGMEM = "AT+CIFSR";
const char AT_SET_MULTIPLE_CONNECTION[] PROGMEM = "AT+CIPMUX=1";
const char AT_SET_SINGLE_CONNECTION[] PROGMEM = "AT+CIPMUX=0";
const char AT_SET_DATA_MODE[] PROGMEM = "AT+CIPMODE=1";
const char AT_SET_IPMODE_MODE[] PROGMEM = "AT+CIPMODE=0";
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
const char HTTP_RESPONSE_NOT_FOUND[] PROGMEM = "HTTP/1.0 404 Not Found\r\nConnection: close\r\n\r\n";
const char HTML_START[] PROGMEM = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<html><head>";
const char HTML_START2[] PROGMEM = "<style>body{background-color:#2F4F4F;}p{background-color:orange;font-family:verdana;font-size:120%;}small{color:white;}table{background-color:orange;}</style></head><body><p><a href='/'>Wifi config</a>&nbsp;|&nbsp;<a href='ca'>Device config</a>&nbsp;|&nbsp;<a href='cb'>Sketch upload</a>";
const char HTML_P_END[] PROGMEM = "</p>";
const char HTML_VERSION[] PROGMEM = "<small>Version: ";
const char HTML_END[] PROGMEM = "</small></body></html>";
const char WIFI_CONFIG[] PROGMEM = "WIFI config";
const char DEVICE_CONFIG[] PROGMEM = "Device config";
const char CAPTION_WIFI_SSID[] PROGMEM = "Wifi SSID:";
const char CAPTION_WIFI_PWD[] PROGMEM = "Wifi password:";
const char CAPTION_WIFI_IP[] PROGMEM = "Static IP addr:";
const char CAPTION_SERVER_IP[] PROGMEM = "Server IP:";
const char CAPTION_SERVER_PORT[] PROGMEM = "Server port:";
const char CAPTION_PGM_PORT[] PROGMEM = "Programming port:";
const char CAPTION_DEVICE_ID[] PROGMEM = "Device ID:";
const char ACTION_S[] PROGMEM = "cs";
const char ACTION_P[] PROGMEM = "cp";
const char ACTION_I[] PROGMEM = "ci";
const char ACTION_R[] PROGMEM = "cr";
const char ACTION_D[] PROGMEM = "cd";
const char MAXSIZE_32[] PROGMEM = "32";
const char MAXSIZE_64[] PROGMEM = "64";
const char MAXSIZE_15[] PROGMEM = "15";
const char MAXSIZE_5[] PROGMEM = "5";
const char MAXSIZE_4[] PROGMEM = "4";
const char WIFI_CONFIG_ACTION[] PROGMEM = "cc";
const char DEVICE_CONFIG_ACTION[] PROGMEM = "ce";

RemoteHomeWifi::RemoteHomeWifi(HardwareSerial& serial) : _ser(serial) {
}
/* ESP enable pin - set to high so the module start to work */
void RemoteHomeWifi::enable() {
    _ser.setTimeout(2000);
    digitalWrite(ENABLE_PIN, HIGH);
    _ser.find(ANSWER_READY);
    setDefaultSerialTimeout();
    //delay(350);
}
/* ESP enable pin - set to low so the module stop to work */
void RemoteHomeWifi::disable() {
    digitalWrite(ENABLE_PIN, LOW);
}
/* This will print the string from program memory to the serial port.*/
void RemoteHomeWifi::printString(const char str[]) {
  char c;
  if(!str) return;
  while((c = pgm_read_byte(str++))) {
      _ser.print(c);
  }
}
/* This will print CR LF*/
void RemoteHomeWifi::printCrLf() {
    printString(CRLF);
}
void RemoteHomeWifi::printStr(String &str) {
    _ser.print(str);
}
/* This will concat string stored in program memory to the outputString. */
void RemoteHomeWifi::concatString(const char str[]) {
  char c;
  if(!str) return;
  while((c = pgm_read_byte(str++))) outputString += c;
}
/* This will count the string stored in program memory. */
int RemoteHomeWifi::countString(const char str[]) {
  int cnt;
  if(!str) return cnt;
  while(pgm_read_byte(str++)) cnt++;
  return cnt;
}
/* This will read all the bytes from the input serial buffer. */
void RemoteHomeWifi::cleanSerialBuffer() {
    delay(10);
    while (_ser.available() > 0) {
        while (_ser.available() > 0) _ser.read();
        delay(10);
    }
}
/* This will send the data, stored in the output string to the server. */
boolean RemoteHomeWifi::sendDataToServer() {
    return establishConnectionToServer(false, EEPROM_POSITION_SERVER_IP, EEPROM_POSITION_SERVER_PORT);
}
/* This will establish the connection to the http server. For the programming, the transparent data mode needs to be used. If the parameter is true, then the programming mode is used. */
boolean RemoteHomeWifi::establishConnectionToServer(boolean progPort, const int ip, const int port) {
    cleanSerialBuffer();
    _ser.setTimeout(10000);
    if (progPort) {
        sendATCommand(AT_SET_SERVER_DISABLE, "\n");
        sendATCommand(AT_CMD_RST, ANSWER_READY);
        waitToConnectToNetwork(20);        
        sendATCommand(AT_SET_SINGLE_CONNECTION, ANSWER_OK);
        sendATCommand(AT_SET_DATA_MODE, ANSWER_OK);
        printString(AT_CREATE_SINGLE_TCP_CONNECTION);
    } else {
        communicationChannelId = 1;
        printString(AT_CREATE_TCP_CONNECTION);
    }
    _ser.print(EEPROM.read(ip), DEC);
    for (int i=1;i<4;i++) {
        _ser.print('.');
        _ser.print(EEPROM.read(ip+i), DEC);
    }
    printString(AT_QUOTATION_MARK);
    printString(AT_COMMA);
    char* portValue;
    portValue = readIntFromEEPROM(port);
    _ser.print(portValue);   
    printCrLf();
    if (_ser.find(ANSWER_OK)) {       
        if (!progPort) {
            prepareDataToSend(outputString.length());
            printStr(outputString);
            _ser.find(ANSWER_DATA_OK);
            setDefaultSerialTimeout();
            closeConnection();
            cleanSerialBuffer();
        } else {
            sendATCommand(AT_SEND_DATA_SINGLECH, AT_STARTDATA);
        }
        setDefaultSerialTimeout();
        return true;
    } else {
        setDefaultSerialTimeout();
        return false;
    }
}
/* This will send to ESP the AT commands to send the data. The method waits for the > character.*/
void RemoteHomeWifi::prepareDataToSend(int len) {
    cleanSerialBuffer();
    printString(AT_SEND_DATA);
    _ser.print(communicationChannelId, DEC);   
    printString(AT_COMMA);
    _ser.print(len, DEC);
    printCrLf();
    _ser.find(AT_STARTDATA);
}
/* This will send 404 error - not found. */
void RemoteHomeWifi::sendDataNotFound() {
    prepareDataToSend(strlen(HTTP_RESPONSE_NOT_FOUND));
    printString(HTTP_RESPONSE_NOT_FOUND);
    _ser.find(ANSWER_DATA_OK);
}
/* This will send complete html page with http headers and everything. */
void RemoteHomeWifi::sendPageWithMenuAndHeaderResponse() {
    int ln = outputString.length()+menuString.length()+pageHeadString.length()+version.length()+strlen(HTML_START)+strlen(HTML_START2)+strlen(HTML_VERSION)+strlen(HTML_END)+strlen(HTML_P_END);
    prepareDataToSend(ln);
    printString(HTML_START);
    printStr(pageHeadString);
    printString(HTML_START2);    
    printStr(menuString);
    printString(HTML_P_END);
    printStr(outputString);
    printString(HTML_VERSION);
    printStr(version);
    printString(HTML_END);
    _ser.find(ANSWER_DATA_OK);   
    closeConnection();
}
/* This method will close the established TCP connection */
void RemoteHomeWifi::closeConnection() {
    printString(AT_CLOSE_CONNECTION);
    _ser.print(communicationChannelId, DEC);
    printCrLf();
    _ser.find(ANSWER_OK);
}
/* This will send the AT command and waits for the response. Returns true, if the response received withing the defined timeout. */
boolean RemoteHomeWifi::sendATCommand(const char cmd[], char answer[]) {
    delay(10);
    for (int i=0; i<2; i++) {
        cleanSerialBuffer();
        printString(cmd);
        printCrLf();      
        boolean result = _ser.find(answer);
        if (result) return true;
    }
    return false;
}
/* These commands instruct the ESP module to listen on port 80 to form http server. */
void RemoteHomeWifi::listenOnPort() {
  setMultipleConnection();
  sendATCommand(AT_SET_SERVER_PORT80, ANSWER_OK);
  sendATCommand(AT_SET_SERVER_TIMEOUT2S, ANSWER_OK);  
}
/*This will set the multiple connections*/
void RemoteHomeWifi::setMultipleConnection() {
  sendATCommand(AT_SET_IPMODE_MODE, ANSWER_OK);  
  sendATCommand(AT_SET_MULTIPLE_CONNECTION, ANSWER_OK);  
}
/* This will form the adhoc network. */
void RemoteHomeWifi::becomeAdHocNetwork() {    
  delay(10);
  cleanSerialBuffer();
  printString(AT_ADHOC_NET);
  printCrLf();
  _ser.findUntil(ANSWER_OK, ANSWER_NO_CHANGE);  
}
/* This will wait to connect to the network. The module remember last connected network and after startup, it tries to connect there. Returns true, if it connects. */
boolean RemoteHomeWifi::waitToConnectToNetwork(int attempts) {
  _ser.setTimeout(200);
  cleanSerialBuffer(); 
  while(attempts-- > 0) {
      printString(AT_CHECK_AP_CONNECTION);
      printCrLf();
      if (!(_ser.findUntil(ANSWER_NO_AP, ANSWER_ERROR))) {
          setDefaultSerialTimeout();
          connectedToWifi = true;
          return true;
      }
  }
  setDefaultSerialTimeout();
  connectedToWifi = false;
  return false;
}
/* This will tell ESP to form the wifi client and join the AP. */
void RemoteHomeWifi::setSingleJoinNetwork() {
  cleanSerialBuffer();
  printString(AT_JOIN_AP);
  printCrLf();
  _ser.findUntil(ANSWER_OK, ANSWER_NO_CHANGE);  
}
/* Default serial timeout used in all AT commands. */
void RemoteHomeWifi::setDefaultSerialTimeout() {
    _ser.setTimeout(3500);
}
/* This will get the IP address, assigned by the wifi router. */
String RemoteHomeWifi::getIPAddress() {
  cleanSerialBuffer();
  printString(GET_IP_ADDRESS);
  printCrLf();
  _ser.readStringUntil('\r\n');
  return _ser.readStringUntil('\r\n\r\n');
}
/* This will tell ESP module to connect to the WiFi network. */
boolean RemoteHomeWifi::joinNetwork(String ssid, String password, String ip) {
  cleanSerialBuffer();
  if (ip.length() == 0) {
      //enable dhcp
      sendATCommand(AT_ENABLE_DHCP, ANSWER_OK);
  } else {
      //disable dhcp and set ip
      sendATCommand(AT_DISABLE_DHCP, ANSWER_OK);
      printString(AT_SET_IP_ADDR);
      printStr(ip);
      printString(AT_QUOTATION_MARK);
      printString(CRLF);
      _ser.find(ANSWER_OK);
  }
  printString( AT_JOIN_AP_PARAMS);
  printStr(ssid);
  printString(AT_QUOTATION_MARKS_WITH_COMMA);
  printStr(password);
  printString(AT_QUOTATION_MARK);
  printCrLf();
  _ser.setTimeout(19000);
  boolean ret = _ser.find(ANSWER_OK);
  setDefaultSerialTimeout();
  delay(20);
  return ret;
}
/* Initial setup. */
void RemoteHomeWifi::setup() {
    nodeId = EEPROM.read(EEPROM_POSITION_NODE_ID);
    if (nodeId == 255) nodeId = 0;
    pinMode(ENABLE_PIN, OUTPUT);
    enable();
    cleanVariablesAfterProcessing();
    delay(1010);
    for (int i=0;i<3;i++) printString(AT_PLUS_MARK);
    delay(1010);
    sendATCommand(AT_CMD_RST, ANSWER_READY);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);
    if (!waitToConnectToNetwork(50)) {
        connectedToWifi = false;
        becomeAdHocNetwork();
        digitalWrite(LED, LOW);
        delay(1000);
        digitalWrite(LED, HIGH);
        delay(1000);
        digitalWrite(LED, LOW);
    } else {
        setSingleJoinNetwork();
        connectedToWifi = true;
        digitalWrite(LED, LOW);
    }
    setDefaultSerialTimeout();
    listenOnPort();
}
/* Returns joined network name. */
char* RemoteHomeWifi::getNetworkName() {
    //static char netName[33];
    char * netName = (char *) calloc (33, 1);
    if (connectedToWifi) {
        cleanSerialBuffer(); 
        printString(AT_CHECK_AP_CONNECTION);
        printCrLf();
        _ser.find("\"");
        _ser.readBytesUntil((char)34, netName, 32); 
    }
    return netName;
}
/* Cleans all variables. */
void RemoteHomeWifi::cleanVariablesAfterProcessing() {
                outputString = "";
                pageHeadString = "";
                inputString="";
                stringComplete = false;
}
/* Manage incomming data. It will read the bytes from ESP, if found the bootloader sync commands, 
 * redirects directly to bootloader, new sketch is loaded. Or it will store to input string the GET command for processing. 
 */
void RemoteHomeWifi::manageSerialEvent() {
    char inChar;
    while (_ser.available() > 0) {
        // get the new byte:
        inChar = (char)_ser.read();
        if ((inChar == 32) && (previousChar == 48)) {
            GoToBootloader();
            return;
        }        
        // if the incoming character is a newline, set a flag
        // so the main loop can do something with it:
        previousChar = inChar; 
        if (inChar == '+') {
            inputString = _ser.readStringUntil('\n');
            _ser.find("\r\n\r\n");
            communicationChannelId = inputString.substring(inputString.indexOf(",")+1).toInt();
            inputString.remove(0, inputString.indexOf("/")+1);            
            stringComplete = true;              
        }
    }
}
/* Creates the label and text box withing the html table form. */
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
/* Creates the html form with the table. */
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
    inputString.remove(0, inputString.indexOf(c)+1);
}
void RemoteHomeWifi::saveIntToEEPROM(const int position) {
            skipInputToChar('=');
            int number = inputString.toInt();
            EEPROM.write(position, highByte(number));
            EEPROM.write(position+1, lowByte(number)); 
}
char* RemoteHomeWifi::readIntFromEEPROM(const int position) {
            byte high=EEPROM.read(position);
            byte low=EEPROM.read(position+1);
            unsigned int result=word(high,low); 
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
            String a = String(EEPROM.read(position));
            for (int i=1; i<4; i++) a += "." + String(EEPROM.read(position+i));
            a.toCharArray(ip, a.length()+1);
            return ip;
}
void RemoteHomeWifi::registerAppendConfigTable ( void (*fpa)() ) {
    fpAppendConfigTable = fpa;
}
void RemoteHomeWifi::registerSaveConfigValues ( void (*fps)() ) {
    fpSaveConfigValues = fps;
}
/* This will process library data. It will return true, if nothing was processing -> the data are for the sketch, otherwise, it returns false, so the data are processed here
 * and nothing should be processed by the sketch.
 */
boolean RemoteHomeWifi::processCommonData() {
    if (stringComplete) {
        if (inputString.startsWith(" ")) {
            //wifi config
            createTableWithForm(WIFI_CONFIG, WIFI_CONFIG_ACTION);
            char* netName = getNetworkName();
            createTextBoxTableRow(CAPTION_WIFI_SSID, ACTION_S, netName, MAXSIZE_32);
            free(netName);
            createTextBoxTableRow(CAPTION_WIFI_PWD, ACTION_P, "", MAXSIZE_64);
            createTextBoxTableRow(CAPTION_WIFI_IP, ACTION_I, "", MAXSIZE_15);
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
            if (0 != fpAppendConfigTable) (*fpAppendConfigTable)();
            createSubmitButton();
            endTableWithForm();
        } else if (inputString.startsWith("cb")) {
            //sketch upload
            pageHeadString = F("<meta http-equiv='refresh' content=\"60;URL='/'\"/>");
            outputString = F("<p>Programming, the page is going to reload after 1 min.</p>");
            sendPageWithMenuAndHeaderResponse();
            delay(30);
            cleanVariablesAfterProcessing();
            if (establishConnectionToServer(true, EEPROM_POSITION_SERVER_IP, EEPROM_POSITION_SERVER_PROGPORT)) {
                delay(10);
                _ser.print((byte)1);
                if (!_ser.find("a")) {
                    setup();
                }               
            }
        } else if (inputString.startsWith("cc")) {
                //it is join network request cc?s=SSID&p=Password&i=192.168.1.30 HTTP/1.1
                skipInputToChar('=');
                String ssid = inputString.substring(0,inputString.indexOf('&'));
                skipInputToChar('=');
                String password = inputString.substring(0,inputString.indexOf('&'));
                skipInputToChar('=');
                String ip = inputString.substring(0,inputString.indexOf(' '));
                pageHeadString = F("<meta http-equiv='refresh' content=\"25;URL='");
                if (ip.length() != 0) {
                    pageHeadString += F("http://");
                    pageHeadString += ip;
                    pageHeadString += F("/");
                }
                pageHeadString += F("cd'\"/>");
                outputString = F("<p>Connecting, please wait, the result is going to be displayed within 25 seconds...</p>");
                sendPageWithMenuAndHeaderResponse();
                if (!joinNetwork(ssid, password, ip)) {
                    becomeAdHocNetwork();
                    listenOnPort();
                    connectedToWifi = false;
                }
                cleanVariablesAfterProcessing();
        } else if (inputString.startsWith("cd")) {
                if (waitToConnectToNetwork(1)) {
                    outputString = F("<p>Connected:<b>");
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
            //it is configure device: ce?s=192.168.1.2&p=8080&r=8081&d=1&e=0 HTTP/1.1
            saveIpAddrToEEPROM(EEPROM_POSITION_SERVER_IP);
            saveIntToEEPROM(EEPROM_POSITION_SERVER_PORT);
            saveIntToEEPROM(EEPROM_POSITION_SERVER_PROGPORT);
            saveByteToEEPROM(EEPROM_POSITION_NODE_ID);
            nodeId = EEPROM.read(EEPROM_POSITION_NODE_ID);
            if (0 != fpSaveConfigValues) (*fpSaveConfigValues)();
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
/* reads the voltage on the microcontroller. Returns the int e.g. 33, it means 3.3 V. It use internal 1.1 voltage reference. */
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