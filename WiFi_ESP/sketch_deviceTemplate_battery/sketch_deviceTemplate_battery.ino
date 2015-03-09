#include <EEPROM.h>
#include <RemoteHomeWifi.h>
#include <DHT.h>
#include <LowPower.h>

/* the EEPROM positions 1015 - 1024 are reserved by the library */
#define EEPROM_POSITION_REPORTING_PERIOD 1013 //Reporting period to send the status of the device. The value is in seconds. It is word - two bytes value
#define ALIVE_AFTER_STARTUP 60000 //How long after startup the module should go to sleep period.
#define WAIT_BEFORE_SLEEP 100 //How long after startup the module should be up before it will go to sleep again.
#define SENSOR_POWERPIN 4
#define DHTPIN 3 //connect pull up resistor 10K between the DHTPIN and POWER pin
#define DHTTYPE DHT22

const char CAPTION_PERIOD[] PROGMEM = "Reporting period:";
const char MAXSIZE_5[] PROGMEM = "5";
const char ACTION_E[] PROGMEM = "e";

RemoteHomeWifi remoteHome(Serial);
unsigned int period = 0;
long interval=ALIVE_AFTER_STARTUP;
int sleepTimer = 0;
unsigned long previousMillis=0;
DHT dht(DHTPIN, DHTTYPE);
float temperature = 0.0;
float humidity = 0.0;
int last_temperature = 0; //to save battery, send the value only when it is different from the previous value. Here the previous value is stored
int last_humidity = 0; //to save battery, send the value only when it is different from the previous value. Here the previous value is stored

void appendConfigTable() {
  /*
  Reserved values by the library:
      const char ACTION_S[] PROGMEM = "s";
      const char ACTION_P[] PROGMEM = "p";
      const char ACTION_R[] PROGMEM = "r";
      const char ACTION_D[] PROGMEM = "d";
  */
  remoteHome.createTextBoxTableRow(CAPTION_PERIOD, ACTION_E, remoteHome.readIntFromEEPROM(EEPROM_POSITION_REPORTING_PERIOD), MAXSIZE_5);
}
void saveConfigValues() {
  remoteHome.saveIntToEEPROM(EEPROM_POSITION_REPORTING_PERIOD);
  readValuesFromEEPROM();
}
void readValuesFromEEPROM() {
  byte high=EEPROM.read(EEPROM_POSITION_REPORTING_PERIOD);
  byte low=EEPROM.read(EEPROM_POSITION_REPORTING_PERIOD+1);
  period=word(high,low);
  if (period == 65535) period = 0;
}
void setup() {
  Serial.begin(115200);
  /* Setup the ESP module and if configured, it connects to network. If not configured, the add hoc network will be started. ) */
  remoteHome.setup();
  /* Set the sketch version. */
  remoteHome.version = F("1.0.0");
  /* Append the menu with the specific sketch functions */
  remoteHome.menuString = F("&nbsp;|&nbsp;<a href='t'>Status</a>");
  /* Register the function to add fields to configure page. */
  remoteHome.registerAppendConfigTable(appendConfigTable);
  /* Register the function to process the added configure menu. */
  remoteHome.registerSaveConfigValues(saveConfigValues);
  pinMode(SENSOR_POWERPIN, OUTPUT); //to save battery, the sensor is powerred on only when needed.
  digitalWrite(SENSOR_POWERPIN, LOW);
  readValuesFromEEPROM();
}

void loop() {
  /* let library to process its commands. If remoteHome.processCommonData() return true, this means that no command for the library, so it's sketch command. */ 
  if (remoteHome.processCommonData()) {
    /* This means that the HTTP GET has been fully read to the inputString so it's ready for processing. The library will skip the "GET /" string so the inputString starts with the character after the / - so in our case t */ 
    if (remoteHome.stringComplete) {
      //Let the sleep period to reset, so the next command could be processed before sleep
      previousMillis = millis();
      if (remoteHome.inputString.startsWith("t")) {
        readSensor();
        getHttpStatus();
      } else {
        if (remoteHome.inputString.endsWith("200 OK")) {
          //do nothing, it is the response of the server, when data was sent. Just ignore.
        } else {
          //Invalid data received, no handler to process this, send 404 NOT found
          remoteHome.sendDataNotFound();
        }
        remoteHome.cleanVariablesAfterProcessing();
        return;
      }
      //This will process the outputString - it will send the http answer with outputString.
      remoteHome.sendPageWithMenuAndHeaderResponse();
      //This will do cleanup of all variables - inputString, outputString stringComplete and serial buffer is also cleaned.
      remoteHome.cleanVariablesAfterProcessing();
    }
  }
  if ((period != 0) && (((unsigned long)(millis() - previousMillis)) >= interval)) {
    if (interval == ALIVE_AFTER_STARTUP) interval = WAIT_BEFORE_SLEEP; //OK it is after start, after start it is running 1 minute. After that, it is running 100 ms and then sleep again
    sleepTimer = 0;
    while (1) {
      remoteHome.disable();
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
      LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF); 
      if ((period) == (++sleepTimer)) {
        remoteHome.enable();
        readSensor();
        int humidityInt = humidity*100;
        int temperatureInt = temperature*100;
        if ((humidityInt != last_humidity) || (temperatureInt != last_temperature)) {
          remoteHome.waitToConnectToNetwork(20);      
          if (remoteHome.connectedToWifi) {
            remoteHome.listenOnPort();
            getStatus();
            boolean dataToServerSent = false;
            if (!remoteHome.sendDataToServer()) {
              //Ok, connection failure try to connect again 2nd times.
              dataToServerSent = remoteHome.sendDataToServer();
            } else dataToServerSent = true;              
            remoteHome.cleanVariablesAfterProcessing();
            if (dataToServerSent) {
                last_humidity = humidity*100;
                last_temperature = temperature*100;
            }
            previousMillis = millis();
            break; //returns to the main loop
          } else {
            sleepTimer = 0;
          }
        }
      }
    }
  }
}
void serialEvent() {
  /* OK, let the library to process the serial data. */
  remoteHome.manageSerialEvent();  
}
void readSensor() {
  digitalWrite(SENSOR_POWERPIN, HIGH);
  LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);
  dht.begin();
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  digitalWrite(SENSOR_POWERPIN, LOW);
}
void getHttpStatus() {
        //Send sensor status
        int bat = remoteHome.readVcc();
        char arrayTemp[10];
        dtostrf(temperature, 2, 2, arrayTemp);
        char arrayHum[10];
        dtostrf(humidity, 2, 2, arrayHum);
        String br = F("<BR/>");
        remoteHome.outputString = F("<p>Temperature:");
        remoteHome.outputString += String(arrayTemp);
        remoteHome.outputString += F(" deg. C");
        remoteHome.outputString += br;
        remoteHome.outputString += F("Humidity:");
        remoteHome.outputString += String(arrayHum);
        remoteHome.outputString += F(" %");
        remoteHome.outputString += br;        
        remoteHome.outputString += F("Voltage:");
        remoteHome.outputString += String(bat / 10, DEC) + "." + String(bat % 10, DEC);
        remoteHome.outputString += F(" V");
        remoteHome.outputString += br;        
        remoteHome.outputString += F("Period:");
        remoteHome.outputString += String(period, DEC);
        remoteHome.outputString += F(" *10 s</p>");
}
void getStatus() {
        String separator = F("&");
        //Send sensor status
        int bat = remoteHome.readVcc();
        char arrayTemp[10];
        dtostrf(temperature, 2, 2, arrayTemp);
        char arrayHum[10];
        dtostrf(humidity, 2, 2, arrayHum);        
        remoteHome.outputString = F("GET /?ServiceName=NetDvc");
        char* ip = remoteHome.readIpAddrFromEEPROM(EEPROM_POSITION_SERVER_IP);
        remoteHome.outputString += separator + "ip=" + ip + separator + "sp=" + remoteHome.readIntFromEEPROM(EEPROM_POSITION_SERVER_PORT);
        remoteHome.outputString += separator + "pp=" + remoteHome.readIntFromEEPROM(EEPROM_POSITION_SERVER_PROGPORT) + separator + "p=" + String(period, DEC);
        free(ip);
        remoteHome.outputString += separator + "v=" + remoteHome.version + separator + "n=" + String(remoteHome.nodeId, DEC) + separator + "t=" + String(arrayTemp) + separator + "h=" + String(arrayHum);
        remoteHome.outputString += separator + "b=" + String(bat / 10, DEC) + "." + String(bat % 10, DEC) ;
        remoteHome.outputString += F(" HTTP/1.1\r\nHost: sensor ");
        remoteHome.outputString += String(remoteHome.nodeId, DEC);
        remoteHome.outputString += F("\r\n\r\n");
}
