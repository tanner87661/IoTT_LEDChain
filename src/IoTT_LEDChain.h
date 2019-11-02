#ifndef IoTT_LEDChain_h
#define IoTT_LEDChain_h

#include "Arduino.h"
#include <FastLED.h>
#include <ArduinoJSON.h>
#include <IoTT_ButtonTypeDef.h>
#include <IoTT_DigitraxBuffers.h>

#define useRTOS

enum transitionType : byte {soft=0, direct=1, merge=2};
enum colorMode : byte {constlevel=0, localblinkpos=1, localblinkneg=2, globalblinkpos=3, globalblinkneg=4, localrampup=5, localrampdown=6, globalrampup=7, globalrampdown=8};
enum displayType : byte {discrete=0, linear=1};

class IoTT_ledChain;
class IoTT_LEDHandler;

class IoTT_ColorDefinitions
{
	public:
		IoTT_ColorDefinitions();
		~IoTT_ColorDefinitions();
		void loadColDefJSON(JsonObject thisObj);
	private:
	public:
		char colorName[50];
		CRGB RGBVal;
		CHSV HSVVal;
	private:
};

class IoTT_LEDCmdList
{
	public:
		IoTT_LEDCmdList();
		~IoTT_LEDCmdList();
		void loadCmdListJSON(JsonObject thisObj);
		void updateLEDs();
	private:
	public:
		IoTT_LEDHandler* parentObj = NULL;
		uint16_t upToVal = 0;
		IoTT_ColorDefinitions* colOn = NULL;
		IoTT_ColorDefinitions* colOff = NULL;
		uint8_t dispMode = 0;
		uint16_t blinkRate;
		uint8_t transType;
	
};

class IoTT_LEDHandler
{
	public:
		IoTT_LEDHandler();
		~IoTT_LEDHandler();
		void loadLEDHandlerJSON(JsonObject thisObj);
		void updateLEDs();
		void updateLocalBlinkValues();
	private:
		void freeObjects();
		void updateBlockDet();
		void updateSwitchPos();
		void updateSwSignalPos(bool isDynamic = true);
		void updateSignalPos();
		void updateButtonPos();
		void updateAnalogValue();
		void updatePowerStatus();
		void updateConstantLED();
		void updateChainData(IoTT_LEDCmdList * cmdDef, IoTT_LEDCmdList * cmdDefLin = NULL, uint8_t distance = 0);
	public:
		IoTT_ledChain* parentObj = NULL;
	public:
		IoTT_LEDCmdList** cmdList = NULL;
		uint16_t cmdListLen = 0;
		uint16_t * ledAddrList = NULL;
		uint8_t ledAddrListLen = 0;
		uint16_t * ctrlAddrList = NULL;
		uint8_t ctrlAddrListLen = 0;
		uint8_t ctrlSource = 0;
		uint8_t displType = 0;

		CHSV currentColor = CHSV(0,0,0);
		uint16_t blinkInterval = 500;
		uint32_t blinkTimer = millis();
		bool     blinkStatus = false;
		float_t  faderValue = 0;
		uint16_t lastValue = 0;
		uint32_t lastActivity = 0;
};

class IoTT_ledChain
{
	public:
		IoTT_ledChain();
		~IoTT_ledChain();
		void loadLEDChainJSON(DynamicJsonDocument doc);
		uint16_t getChainLength();
		CRGB * getChain();
		float_t getBrightness();
		void setBrightness(float_t newVal);
		uint8_t getBrightnessControlType();
		uint16_t getBrightnessControlAddr();
		IoTT_ColorDefinitions* getColorByName(String colName);

	private:
		void freeObjects();
		void updateLEDs();

		uint16_t chainLength = 0;
		CRGB * ledChain = NULL;

		IoTT_ColorDefinitions** colorDefinitionList = NULL;
		uint16_t colorDefListLen = 0;
		
		IoTT_LEDHandler** LEDHandlerList = NULL;
		uint16_t LEDHandlerListLen = 0;

		float currentBrightness = 1;
		uint8_t brightnessControlType = -1; //not defined
		uint16_t brightnessControlAddr = 0;

	public:
		uint16_t blinkInterval = 500;
		uint32_t blinkTimer;
		uint32_t ledUpdateTimer;
		uint16_t ledUpdateInterval = 50;
		bool     blinkStatus;
		float_t  faderValue;
		bool needUpdate;
		uint8_t ledPin;
		SemaphoreHandle_t ledBaton;

	public: 
		CRGB *  initChain(word numLEDs);
		void processChain();
		void setCurrColRGB(uint16_t ledNr, CRGB newCol);
		void setCurrColHSV(uint16_t ledNr, CHSV newCol);
		void setBlinkRate(uint16_t blinkVal);
		void setRefreshInterval(uint16_t newInterval); //1/frame rate in millis()
		bool getBlinkStatus();
		float_t getFaderValue();
		CRGB getCurrColRGB(uint16_t ledNr);
		CHSV getCurrColHSV(uint16_t ledNr);
};

//extern void onUpdateLED() __attribute__ ((weak));

#endif
