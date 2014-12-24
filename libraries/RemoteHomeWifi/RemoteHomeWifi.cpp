#include <RemoteHomeWifi.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <EEPROM.h>
#include <Arduino.h>

#define ANSWER_READY "ready"
#define ANSWER_OK "OK"
#define ANSWER_FAIL "FAIL"
#define ANSWER_NO_CHANGE "no change"
#define ANSWER_CONNECTED "STATUS:4"
#define ANSWER_DATA_OK "SEND OK"
#define AT_STARTDATA ">"

byte RemoteHomeWifi::nodeId;
String RemoteHomeWifi::inputString = "";         // a string to hold incoming data
String RemoteHomeWifi::outputString = "";        // a string to hold outgoing data
boolean RemoteHomeWifi::stringComplete = false;  // whether the string is complete
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
const char AT_SET_SERVER_PORT80[] PROGMEM = "AT+CIPSERVER=1,80";
const char AT_SET_SERVER_TIMEOUT1S[] PROGMEM = "AT+CIPSTO=2";
const char AT_JOIN_AP_PARAMS[] PROGMEM = "AT+CWJAP=\"";
const char AT_CHECK_AP_CONNECTION[] PROGMEM = "AT+CWJAP?";
const char AT_QUOTATION_MARK[] PROGMEM = "\"";
const char AT_QUOTATION_MARKS_WITH_COMMA[] PROGMEM = "\",\"";
const char AT_SEND_DATA[] PROGMEM = "AT+CIPSEND=";
const char AT_COMMA[] PROGMEM = ",";
const char AT_CLOSE_CONNECTION[] PROGMEM = "AT+CIPCLOSE=";
const char HTTP_RESPONSE_OK[] PROGMEM = "HTTP/1.1 200 OK";
const char HTTP_RESPONSE_NOT_FOUND[] PROGMEM = "HTTP/1.1 404 Not Found";
const char HTTP_RESPONSE_CONNECTION_CLOSE[] PROGMEM = "Connection: close";
const char PAGE1[] PROGMEM = "<html><head><style>body{background-color:#2F4F4F;}</style></head><body><FORM action='";
const char PAGE2[] PROGMEM = "' method='GET'><table style='background-color:orange;'><tr><th colspan='2'>";
const char PAGE3[] PROGMEM = "</th></tr>";
const char WIFI_CFG[] PROGMEM = "<tr><td>Wifi SSID:</td><td><input name='s' type='text' maxSize='32'/></td></tr><tr><td>Wifi password:</td><td><input name='p' type='text' maxSize='64'/></td></tr><tr><td colspan='2'><input type='submit' value='Configure'/></td></tr>";
const char PAGE4[] PROGMEM = "</table></form></body></html>";

RemoteHomeWifi::RemoteHomeWifi(){}

RemoteHomeWifi::~RemoteHomeWifi(){}

void printString(HardwareSerial &_ser, const char str[]) {
  char c;
  if(!str) return;
  while((c = pgm_read_byte(str++))) _ser.print(c);
}
int countString(const char str[]) {
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

void cleanSerialBuffer(HardwareSerial &_ser) {
    delay(5);
    while (_ser.available()) _ser.read();
}
boolean RemoteHomeWifi::sendDataToServer() {
    return true;
}
void sendDataStart1(HardwareSerial &_ser, int dataLength) {
    delay(5);
    cleanSerialBuffer(_ser);
    printString(_ser, AT_SEND_DATA);
    _ser.print(communicationChannelId, DEC);
    printString(_ser, AT_COMMA);
    _ser.println(dataLength, DEC);
    _ser.find(AT_STARTDATA);    
}
void sendDataStart2(HardwareSerial &_ser) {
    delay(5);
    printString(_ser, HTTP_RESPONSE_CONNECTION_CLOSE);  
    _ser.println();
    delay(5);
    _ser.println();
    delay(5);    
}
void sendDataStartOK(HardwareSerial &_ser, int dataLength) {
    sendDataStart1(_ser, dataLength);
    printString(_ser, HTTP_RESPONSE_OK);
    _ser.println();
    sendDataStart2(_ser);
}
void sendDataEnd(HardwareSerial &_ser) {
    _ser.setTimeout(3000);
    _ser.find(ANSWER_DATA_OK);
    delay(20);
    _ser.setTimeout(1000);
    printString(_ser, AT_CLOSE_CONNECTION);
    _ser.println(communicationChannelId, DEC);
    _ser.find(ANSWER_DATA_OK);
    delay(20);    
}
void RemoteHomeWifi::sendDataNotFound(HardwareSerial &_ser) {
    sendDataStart1(_ser, 45);
    printString(_ser, HTTP_RESPONSE_NOT_FOUND);
    _ser.println();
    sendDataStart2(_ser);
    sendDataEnd(_ser);
}
void RemoteHomeWifi::sendData(HardwareSerial &_ser, String data) {
    cleanSerialBuffer(_ser);
    sendDataStartOK(_ser, data.length()+38);
    _ser.print(data);
    sendDataEnd(_ser);
}
boolean sendATCommand(HardwareSerial &_ser, const char cmd[], char* answer) {
    delay(10);
    for (int i=0; i<2; i++) {
        cleanSerialBuffer(_ser);
        printString(_ser, cmd);
        _ser.println();
        boolean result = _ser.find(answer);
        if (result) return true;
    }
    return false;
}
void listenOnPort(HardwareSerial &_ser) {
  sendATCommand(_ser, AT_SET_MULTIPLE_CONNECTION, ANSWER_OK);
  sendATCommand(_ser, AT_SET_SERVER_PORT80, ANSWER_OK);
  sendATCommand(_ser, AT_SET_SERVER_TIMEOUT1S, ANSWER_OK);     
}
void becomeAdHocNetwork(HardwareSerial &_ser) {
  delay(10);
  cleanSerialBuffer(_ser);
  printString(_ser, AT_ADHOC_NET);
  _ser.println();
  _ser.findUntil(ANSWER_OK, ANSWER_NO_CHANGE);  
}
boolean waitToConnectToNetwork(HardwareSerial &_ser, int attempts) {
  _ser.setTimeout(100);
  cleanSerialBuffer(_ser); 
  while(attempts-- > 0) {
      if (sendATCommand(_ser, AT_CHECK_AP_CONNECTION, ANSWER_OK)) {
          _ser.setTimeout(1000);
          return true;
      }
  }
  _ser.setTimeout(1000);
  return false;
}
void setSingleJoinNetwork(HardwareSerial &_ser) {
  cleanSerialBuffer(_ser);
  printString(_ser, AT_JOIN_AP);
  _ser.println();
  _ser.findUntil(ANSWER_OK, ANSWER_NO_CHANGE);  
}
String getIPAddress(HardwareSerial &_ser) {
  cleanSerialBuffer(_ser);
  byte ipaddress[50];
  printString(_ser, GET_IP_ADDRESS);
  _ser.println();
  int read = _ser.readBytesUntil('O', ipaddress, 50);  
  String retIp;
  for(int k=0; k<read; k++){
      if (((ipaddress[k] > 47) && (ipaddress[k] < 58)) || (ipaddress[k] == '.')) retIp += String((char)ipaddress[k]);
      if ((ipaddress[k] == 10) || (ipaddress[k] == 13)) retIp += " ";
  }
  return retIp; 
}
boolean joinNetwork(HardwareSerial &_ser, String ssid, String password) {
  cleanSerialBuffer(_ser);
  printString(_ser, AT_JOIN_AP_PARAMS);
  _ser.print(ssid);
  printString(_ser, AT_QUOTATION_MARKS_WITH_COMMA);
  _ser.print(password);
  printString(_ser, AT_QUOTATION_MARK);
  _ser.println();
  _ser.setTimeout(19000);
  boolean ret = _ser.find(ANSWER_OK);
  _ser.setTimeout(100);
  delay(20);
  return ret;
}
void RemoteHomeWifi::setup(HardwareSerial &_ser) {
    _ser.setTimeout(200);
    cleanSerialBuffer(_ser);
    if (!waitToConnectToNetwork(_ser, 50)) {
        becomeAdHocNetwork(_ser);
    } else {
        setSingleJoinNetwork(_ser);
    }
    _ser.setTimeout(100);
    listenOnPort(_ser);
}
void RemoteHomeWifi::manageSerialEvent(HardwareSerial &_ser) {
  while (_ser.available()) {
    // get the new byte:
    char inChar = (char)_ser.read();    
    if (((inChar == 32) && (previousChar == 48)) || ((inChar == 0xFF) && (previousChar == 0x00))) {
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
              cleanSerialBuffer(_ser);
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
boolean RemoteHomeWifi::processCommonData(HardwareSerial &_ser) {
    if (stringComplete) {
        //filter out the module strings
        if (inputString.startsWith("Link") || inputString.startsWith("Unlink")  || inputString.startsWith("Error")) {
            //ignore
            inputString="";
            stringComplete = false;
            return false;            
        }
        String answer = "";
        if (inputString.startsWith(" ")) {
            answer = PAGE1;
            answer += "c";
            answer += PAGE2;
            answer += F("WIFI network configuration");
            answer += PAGE3;
            answer += WIFI_CFG;
            answer += PAGE4;
        } else if (inputString.startsWith("c")) {
                //it is join network request n?s=Bob&p=LukiKukiBuki010388& HTTP/1.1
                String ssid = inputString.substring(4,inputString.indexOf('&'));
                String password = inputString.substring(inputString.indexOf('&')+3,inputString.indexOf(' '));
                sendData(_ser, F("<html><head><meta http-equiv='refresh' content=\"25;URL='z'\"/></head><body>Connecting, wait ... (The result will be displayed here in 25 seconds)</body></html>"));
                if (!joinNetwork(_ser, ssid, password)) {
                    becomeAdHocNetwork(_ser);
                    listenOnPort(_ser);
                }
                answer = "";
        } else if (inputString.startsWith("z")) {
                if (waitToConnectToNetwork(_ser, 1)) {
                    answer = F("<html><body>Connected. IP: <b>");
                    answer += getIPAddress(_ser);
                    answer += F("</b><BR>Please reserve the IP in your router. Now reconnect to your network.</body></html>");
                    sendData(_ser, answer);
                    delay(1000);
                    setSingleJoinNetwork(_ser);
                    answer = "";
                } else {
                    answer = F("<html><body>Not connected. Go <a href=\"\">here</a> and put correct values.</body></html>");
                }
        }
        if (answer.length()!=0) {
            _ser.println("aaaa:"+answer);
            sendData(_ser, answer);
        }
        inputString="";
        stringComplete = false;
        return false;                                
    }
    return true;
}
void RemoteHomeWifi::printCommonConfig(HardwareSerial &_ser) {
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