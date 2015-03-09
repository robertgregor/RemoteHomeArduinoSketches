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
	static String inputString;
	static String outputString;
        static String menuString;
        static String pageHeadString;
	static boolean stringComplete;
        static boolean connectedToWifi;
        HardwareSerial& _ser;

	RemoteHomeWifi(HardwareSerial& serial);
	void reset();
	void setup();
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
	boolean sendDataToServer();
        boolean establishConnectionToServer(boolean progPort);
	void manageSerialEvent();
	boolean processCommonData();
        boolean sendATCommand(const char cmd[], char* answer);
	void clearEEPROM();
	int readVcc();
  private:
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
        boolean joinNetwork(String ssid, String password);
        void setDefaultSerialTimeout();
};
#endif