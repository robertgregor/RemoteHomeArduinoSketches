#include <EEPROM.h>
#include <RemoteHomeWifi.h>
#include <DHT.h>
#include <LowPower.h>

/* the EEPROM positions 1015 - 1024 are reserved by the library */
#define EEPROM_POSITION_REPORTING_PERIOD 1013 //Reporting period to send the status of the device. The value is in seconds. It is word - two bytes value
#define EEPROM_POSITION_IP_THERMOSTAT 0 //Thermostat IP address
#define EEPROM_POSITION_THERMOSTAT_SUBDEVICE_ID 4 //Thermostat subdevice ID
#define EEPROM_POSITION_IP_VENTILATOR 5 //Ventilator IP address
#define EEPROM_POSITION_VENTILATOR_SUBDEVICE_ID 9//Ventilator subdevice ID
#define EEPROM_POSITION_PORT_80 10//Port, where to report the temperature and humidity

#define ALIVE_AFTER_STARTUP 60000 //How long after startup the module should go to sleep period.
#define WAIT_BEFORE_SLEEP 100 //How long after startup the module should be up before it will go to sleep again.
#define SENSOR_POWERPIN 4
#define DHTPIN 3 //connect pull up resistor 10K between the DHTPIN and POWER pin
#define DHTTYPE DHT22

const char CAPTION_PERIOD[] PROGMEM = "Reporting period:";
const char MAXSIZE_5[] PROGMEM = "5";
const char ACTION_E[] PROGMEM = "e";
const char CAPTION_THERMOSTAT_IP[] PROGMEM = "Thermostat IP:";
const char MAXSIZE_15[] PROGMEM = "15";
const char ACTION_F[] PROGMEM = "f";
const char CAPTION_THERMOSTAT_SD[] PROGMEM = "Thermostat subdevice ID:";
const char MAXSIZE_1[] PROGMEM = "1";
const char ACTION_G[] PROGMEM = "g";
const char CAPTION_VENTILATOR_IP[] PROGMEM = "Ventilator IP:";
const char ACTION_H[] PROGMEM = "h";
const char CAPTION_VENTILATOR_SD[] PROGMEM = "Ventilator subdevice ID:";
const char ACTION_I[] PROGMEM = "i";

RemoteHomeWifi remoteHome(Serial);
unsigned int period = 0;
long interval=ALIVE_AFTER_STARTUP;
int sleepTimer = 0;
unsigned long previousMillis=0;
DHT dht(DHTPIN, DHTTYPE);
float temperature = 0.0;
float humidity = 0.0;

void appendConfigTable() {
  /*
  Reserved values by the library:
      const char ACTION_S[] PROGMEM = "s";
      const char ACTION_P[] PROGMEM = "p";
      const char ACTION_R[] PROGMEM = "r";
      const char ACTION_D[] PROGMEM = "d";
  */
  remoteHome.createTextBoxTableRow(CAPTION_PERIOD, ACTION_E, remoteHome.readIntFromEEPROM(EEPROM_POSITION_REPORTING_PERIOD), MAXSIZE_5);
  char* ipThermostat = remoteHome.readIpAddrFromEEPROM(EEPROM_POSITION_IP_THERMOSTAT);
  remoteHome.createTextBoxTableRow(CAPTION_THERMOSTAT_IP, ACTION_F, ipThermostat, MAXSIZE_15);
  free(ipThermostat);
  remoteHome.createTextBoxTableRow(CAPTION_THERMOSTAT_SD, ACTION_G, remoteHome.readByteFromEEPROM(EEPROM_POSITION_THERMOSTAT_SUBDEVICE_ID), MAXSIZE_1);
  char* ipVentilator = remoteHome.readIpAddrFromEEPROM(EEPROM_POSITION_IP_VENTILATOR);
  remoteHome.createTextBoxTableRow(CAPTION_VENTILATOR_IP, ACTION_H, ipVentilator, MAXSIZE_15);
  free(ipVentilator);
  remoteHome.createTextBoxTableRow(CAPTION_VENTILATOR_SD, ACTION_I, remoteHome.readByteFromEEPROM(EEPROM_POSITION_VENTILATOR_SUBDEVICE_ID), MAXSIZE_1);  
}
void saveConfigValues() {
  remoteHome.saveIntToEEPROM(EEPROM_POSITION_REPORTING_PERIOD);
  remoteHome.saveIpAddrToEEPROM(EEPROM_POSITION_IP_THERMOSTAT);
  remoteHome.saveByteToEEPROM(EEPROM_POSITION_THERMOSTAT_SUBDEVICE_ID);
  remoteHome.saveIpAddrToEEPROM(EEPROM_POSITION_IP_VENTILATOR);
  remoteHome.saveByteToEEPROM(EEPROM_POSITION_VENTILATOR_SUBDEVICE_ID);  
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
  if (!((EEPROM.read(EEPROM_POSITION_PORT_80) == 0) && EEPROM.read(EEPROM_POSITION_PORT_80+1) == 80)) {
      EEPROM.write(EEPROM_POSITION_PORT_80, 0);
      EEPROM.write(EEPROM_POSITION_PORT_80+1, 80);
  }
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
          remoteHome.cleanVariablesAfterProcessing();
        } else {
          //Invalid data received, no handler to process this, send 404 NOT found
          remoteHome.sendDataNotFound();
          remoteHome.cleanVariablesAfterProcessing();
        }
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
      if ((period/10) == (++sleepTimer)) {
        remoteHome.enable();
        readSensor();
        int humidityInt = humidity*100;
        int temperatureInt = temperature*100;
        remoteHome.waitToConnectToNetwork(20);      
        if (remoteHome.connectedToWifi) {
            remoteHome.listenOnPort();
            //ok send data to thermostat
            if (EEPROM.read(EEPROM_POSITION_IP_THERMOSTAT) != 255) {
                getShortStatus(EEPROM.read(EEPROM_POSITION_THERMOSTAT_SUBDEVICE_ID));
                if (!remoteHome.establishConnectionToServer(false, EEPROM_POSITION_IP_THERMOSTAT, EEPROM_POSITION_PORT_80)) {
                  //Ok, connection failure try to connect again 2nd times.
                  remoteHome.establishConnectionToServer(false, EEPROM_POSITION_IP_THERMOSTAT, EEPROM_POSITION_PORT_80);
                }              
                remoteHome.cleanVariablesAfterProcessing();                
            }
            //ok send data to ventilator
            if (EEPROM.read(EEPROM_POSITION_IP_VENTILATOR) != 255) {
                getShortStatus(EEPROM.read(EEPROM_POSITION_VENTILATOR_SUBDEVICE_ID));
                if (!remoteHome.establishConnectionToServer(false, EEPROM_POSITION_IP_VENTILATOR, EEPROM_POSITION_PORT_80)) {
                  //Ok, connection failure try to connect again 2nd times.
                  remoteHome.establishConnectionToServer(false, EEPROM_POSITION_IP_VENTILATOR, EEPROM_POSITION_PORT_80);
                }              
                remoteHome.cleanVariablesAfterProcessing();                
            }
            getStatus();
            if (!remoteHome.establishConnectionToServer(false, EEPROM_POSITION_SERVER_IP, EEPROM_POSITION_SERVER_PORT)) {
              //Ok, connection failure try to connect again 2nd times.
              remoteHome.establishConnectionToServer(false, EEPROM_POSITION_SERVER_IP, EEPROM_POSITION_SERVER_PORT);
            }              
            remoteHome.cleanVariablesAfterProcessing();
            previousMillis = millis();
            break; //returns to the main loop
        } else {
            sleepTimer = 0;
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
        remoteHome.outputString += F(" V</p>");
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
        char* tip = remoteHome.readIpAddrFromEEPROM(EEPROM_POSITION_IP_THERMOSTAT);
        remoteHome.outputString += separator + "tip=" + tip + separator + "ts=" + remoteHome.readByteFromEEPROM(EEPROM_POSITION_THERMOSTAT_SUBDEVICE_ID);
        free(tip);
        char* vip = remoteHome.readIpAddrFromEEPROM(EEPROM_POSITION_IP_VENTILATOR);
        remoteHome.outputString += separator + "vip=" + vip + separator + "vs=" + remoteHome.readByteFromEEPROM(EEPROM_POSITION_VENTILATOR_SUBDEVICE_ID);
        free(vip);
        remoteHome.outputString += separator + "v=" + remoteHome.version + separator + "n=" + String(remoteHome.nodeId, DEC) + separator + "t=" + String(arrayTemp) + separator + "h=" + String(arrayHum);
        remoteHome.outputString += separator + "b=" + String(bat / 10, DEC) + "." + String(bat % 10, DEC) ;
        remoteHome.outputString += F(" HTTP/1.1\r\nHost: sensor ");
        remoteHome.outputString += String(remoteHome.nodeId, DEC);
        remoteHome.outputString += F("\r\n\r\n");
}
void getShortStatus(byte subdeviceId) {
        String separator = F("&");
        char arrayTemp[10];
        dtostrf(temperature, 2, 2, arrayTemp);
        char arrayHum[10];
        dtostrf(humidity, 2, 2, arrayHum);        
        remoteHome.outputString = F("GET /?s=");
        remoteHome.outputString += String(subdeviceId, DEC)+separator+"t=";
        remoteHome.outputString += String(arrayTemp) + separator + "h=" + String(arrayHum);
        remoteHome.outputString += F(" HTTP/1.1\r\nHost: sensor ");
        remoteHome.outputString += String(remoteHome.nodeId, DEC);
        remoteHome.outputString += F("\r\n\r\n");
}
