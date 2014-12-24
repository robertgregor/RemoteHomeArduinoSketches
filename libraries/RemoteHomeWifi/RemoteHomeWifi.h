#ifndef RemoteHomeWifi_h
#define RemoteHomeWifi_h

#include <Arduino.h>

class RemoteHomeWifi {
  public:
	static byte nodeId;
	static String inputString;
	static String outputString;
	static boolean stringComplete;

	RemoteHomeWifi();
	~RemoteHomeWifi();
	void reset();
	void setup(HardwareSerial &_ser);
	boolean sendDataToServer();
	void sendData(HardwareSerial &_ser, String data);
        void sendDataNotFound(HardwareSerial &_ser);
	void manageSerialEvent(HardwareSerial &_ser);
	void sendOK();
	void sendERROR();
	void printCommonConfig(HardwareSerial &_ser);
	boolean processCommonData(HardwareSerial &_ser);
	void clearEEPROM();
	int readVcc();
  };
#endif