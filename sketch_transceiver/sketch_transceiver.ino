/*

Transceiver sketch

Serial commands:

at+ddd=Cmd - specific device command ddd = device Id, Cmd = command
at+c=XXX Configure the channel - Network. Receiver should have the same channel than all the devices. There could be channels 1 >= XXX >= 254
at+p=thisIsEncryptKey - for AES encryption, this command set the symetric key for encryption between the transceiver and the device. Should be exactly 16 characters
at+r=nnn - Returns the RSSI of the specific device.
at+res Reset the device
at+def Reset the device and set the factory default values of EEPROM
at+memid=XXX To set the eeprom value. This command preset the address
at+memval=XXX This command set the XXX value at the position memid in the EEPROM
at+s Returns status. Example: +Device ID:255<TAB character>Channel:5<TAB character>Password:LukiKukiBuki0103<TAB character>Device type:Transceiver

Asynchronous commands:
+XXX data - XXX device ID.

EEPROM bank
EEPROM_POSITION_NODE_ID 0 //Node Id eeprom position
EEPROM_POSITION_NETWORK_ID 1 //Node Id eeprom position
EEPROM_POSITION_KEY 2 //Node Id eeprom position
*/

#include <EEPROM.h>
#include <avr/wdt.h>
#include <LowPower.h>
#include <RFM69.h>
#include <SPI.h>
#include <RemoteHome.h>

RemoteHome remoteHome;

void setup() {
  // initialize serial
  Serial.begin(115200);
  Serial.println("Started");
  remoteHome.setup(255);
  delay(100);
}

void loop() {
  wdt_reset();
  if (remoteHome.processCommonSerialData(Serial)) {
    String lowerCaseInput = remoteHome.inputString;
    lowerCaseInput.toLowerCase();
    if (lowerCaseInput.startsWith("at+s")) {
      remoteHome.printCommonConfig(Serial);
      Serial.print("\tDevice type:Transceiver");     
      Serial.println();
    } else if (lowerCaseInput.startsWith("at+r=")) {
      byte deviceIdToSend = remoteHome.inputString.substring(5).toInt();
      char buff[2];
      buff[0] = 'n';
      buff[1] = 'n';
      if (sendRadioData(deviceIdToSend,buff,2)) {
        Serial.println(remoteHome.radio.readRSSI(), DEC);
      }
    } else if (lowerCaseInput.startsWith("at+")) {
      char buff[200];
      if ((remoteHome.inputString.length() > 3) && (remoteHome.inputString.indexOf('=') != -1)) {
        //remoteHome.inputString = remoteHome.inputString + "\n";
        int d = (remoteHome.inputString.substring(3, remoteHome.inputString.indexOf('='))).toInt();
        String dataToSend = remoteHome.inputString.substring(remoteHome.inputString.indexOf('=')+1);
        if (d > 254) {
          remoteHome.printERROR(Serial);
        } else {
          if (dataToSend != "=") {
            dataToSend = dataToSend + "\n";
            byte len = dataToSend.length();
            dataToSend.toCharArray(buff, len);
            sendRadioData(d, buff, len);
          } else {
            remoteHome.printERROR(Serial);
          }
        }
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
     Serial.print("+");
     Serial.print(remoteHome.radio.SENDERID, DEC);
     Serial.print(" ");
     for (byte i = 0; i < remoteHome.radio.DATALEN; i++) {
       Serial.print((char)remoteHome.radio.DATA[i]);
     }
     Serial.println();
     remoteHome.manageReceivedData();
  }
}
void serialEvent() {
  remoteHome.manageSerialEvent(Serial);
}
boolean sendRadioData(int dev, const void* buff, byte len) {
  int retryCnt = 0;
  while (1) {
      if (len == 0) return true;
      if (remoteHome.radio.sendWithRetry(dev, buff, len, 0, 30)) {
        len = 0;
        return true;
      } else {
        if (++retryCnt > 8) {
          Serial.println("ERROR [Not reachable]");
          len = 0;
          return false;
        } else {
          delay(random(50, 200));
        }
      }
  }
}
