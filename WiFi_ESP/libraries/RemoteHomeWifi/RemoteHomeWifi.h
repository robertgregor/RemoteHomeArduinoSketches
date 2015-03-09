#include <Arduino.h>

#ifndef RemoteHomeWifi_h
#define RemoteHomeWifi_h
#define EEPROM_POSITION_NODE_ID 1015 //Node Id eeprom position
#define EEPROM_POSITION_SERVER_IP 1016 //Server IP address, 4 bytes
#define EEPROM_POSITION_SERVER_PORT 1020 //Server port 2 bytes
#define EEPROM_POSITION_SERVER_PROGPORT 1022 //Server port 2 bytes

class RemoteHomeWifi {
  public:
	static byte nodeId;
	static String inputString; //here is stored the GET command received
	static String outputString; //Here store the response of the GET, will be send with the response.
        static String menuString; //Here should go additional menu strings - i.e. <a> html tags
        static String pageHeadString; //Additional header strings - i.e. refresh
        static String version; // Here is the version of the sketch
        static String status;
	static boolean stringComplete; //Flag to know, that the GET command has been fully readed - the \n character was read.
        static boolean connectedToWifi; //if true, it is connected to the home wifi network.
        HardwareSerial& _ser; //The serial object

	RemoteHomeWifi(HardwareSerial& serial);
	void setup();
        void enable();
        void disable();
        void printCrLf();
        char* getNetworkName(); //return created with malloc. so you need to call free.
        void sendResponseOK();
        void sendDataNotFound();
        void sendPageWithMenuAndHeaderResponse();
        void cleanVariablesAfterProcessing();
        void createTextBoxTableRow(const char* title, const char* action, char* value, const char* maxSize);
        void createTableWithForm(const char* title, const char* action);
        void concatString(const char str[]);        
        void skipInputToChar(char c);
        void endTableWithForm();
        void createSubmitButton();
        void saveByteToEEPROM(const int position);
        char* readByteFromEEPROM(const int position); //return created with static
        void saveIntToEEPROM(const int position);
        char* readIntFromEEPROM(const int position); //return created with static
        void saveIpAddrToEEPROM(const int position);
        char* readIpAddrFromEEPROM(const int position); //return created with malloc. so you need to call free.
        void setMultipleConnection();
	boolean sendDataToServer();
        boolean establishConnectionToServer(boolean progPort, const int ip, const int port);
	void manageSerialEvent();
	boolean processCommonData();
        boolean sendATCommand(const char cmd[], char answer[]);
	void clearEEPROM();
	int readVcc();
        void registerAppendConfigTable ( void (*fp)() );
        void registerSaveConfigValues ( void (*fp)() );
        void printStr(String &str);
        void printString(const char str[]);
        void prepareDataToSend(int len);
        int countString(const char str[]);
        void cleanSerialBuffer();
        void listenOnPort();
        void becomeAdHocNetwork();
        boolean waitToConnectToNetwork(int attempts);
        void setSingleJoinNetwork();
        String getIPAddress();
        boolean joinNetwork(String ssid, String password, String ip);
        void setDefaultSerialTimeout();
        void closeConnection();
};
#endif