#ifndef RemoteHome_h
#define RemoteHome_h
#include <Arduino.h>
#include <RFM69.h>

#define FREQUENCY RF69_433MHZ
#define EEPROM_POSITION_NODE_ID 1006 //Node Id eeprom position
#define EEPROM_POSITION_NETWORK_ID 1007 //Channel Id eeprom position
#define EEPROM_POSITION_KEY 1008 //Node Id eeprom position


class RemoteHome {
  public:
	static byte networkId;
	static byte nodeId;
	static char key[17];
	static RFM69 radio;
	static byte len;
	static char buff[100];
	static String inputString;
	static boolean stringComplete;
	static boolean reset_flag;
	String inputRadioData;

	RemoteHome();
	~RemoteHome();
	void reset();
	void setup();
	void setup(int nodeId, boolean overrideEEPROM);
	boolean sendRadioData();
	boolean sendRadioData(int destinationId);
	void manageSerialEvent(HardwareSerial &_ser);
	void sendOK();
	void sendERROR();
	void printOK(HardwareSerial &_ser);
	void printERROR(HardwareSerial &_ser);
	void printCommonConfig(HardwareSerial &_ser);
	boolean processCommonSerialData(HardwareSerial &_ser);
	void clearEEPROM();
	void manageReceivedData();
	boolean processCommonRadioData();
	int readVcc();
  };
#endif