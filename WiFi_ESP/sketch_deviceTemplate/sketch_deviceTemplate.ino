#include <EEPROM.h>
#include <avr/wdt.h>
#include <RemoteHomeWifi.h>

RemoteHomeWifi remoteHome(Serial);

void setup() {
  // initialize serial
  Serial.begin(115200);
  remoteHome.setup();
  //wdt_enable(WDTO_4S);
  delay(100);
}

void loop() {
  wdt_reset();
  if (remoteHome.processCommonData()) {
    if (remoteHome.stringComplete) {
      if (remoteHome.inputString.startsWith("s")) {
        remoteHome.outputString = F("<p>Hello from the device.</p>");
      } else {
        //Invalid data received
        remoteHome.sendDataNotFound();
        remoteHome.inputString = "";
        remoteHome.stringComplete = false;
        return;
      }
      remoteHome.sendPageWithMenuAndHeaderResponse();
      remoteHome.cleanVariablesAfterProcessing();
    }
  }
}
void serialEvent() {
  remoteHome.manageSerialEvent();
}
