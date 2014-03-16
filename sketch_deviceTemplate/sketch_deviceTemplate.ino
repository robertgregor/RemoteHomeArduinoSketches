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
  remoteHome.setup();
  delay(100);
}

void loop() {
  wdt_reset();
  if (remoteHome.processCommonSerialData(Serial)) {
    String lowerCaseInput = remoteHome.inputString;
    lowerCaseInput.toLowerCase();
    if (lowerCaseInput.startsWith("at+s")) {
      remoteHome.printCommonConfig(Serial);
      Serial.print("\tDevice type:Template");     
      Serial.println();
    }
    remoteHome.inputString = "";
    remoteHome.stringComplete = false;
  }
  //check and manage radio
  if (remoteHome.processCommonRadioData()) {
    if ((char)remoteHome.radio.DATA[0] == 's') {
      remoteHome.buff[0] = '8';
      remoteHome.buff[1] = '|';
      remoteHome.len = 2;  
    } else {
      remoteHome.sendERROR();
    }
    remoteHome.manageReceivedData();
  }
}
void serialEvent() {
  remoteHome.manageSerialEvent(Serial);
}
