#include <IoTT_LEDChain.h>

transitionType getTransitionTypeByName(String transName)
{
  if (transName == "soft") return soft;
  if (transName == "direct") return direct;
  if (transName == "merge") return merge;
  return direct; 
}

colorMode getColorModeByName(String colModeName)
{
  if (colModeName == "static") return constlevel;
  if (colModeName == "localblink") return localblinkpos;
  if (colModeName == "localblinkpos") return localblinkpos;
  if (colModeName == "localblinkneg") return localblinkneg;
  if (colModeName == "localrampup") return globalrampup;
  if (colModeName == "localrampdown") return globalrampdown;
  if (colModeName == "blink") return globalblinkpos;
  if (colModeName == "globalblink") return globalblinkpos;
  if (colModeName == "globalblinkpos") return globalblinkpos;
  if (colModeName == "globalblinkneg") return globalblinkneg;
  if (colModeName == "globalrampup") return globalrampup;
  if (colModeName == "globalrampdown") return globalrampdown;
  return constlevel; 
}

displayType getDisplayTypeByName(String displayTypeName)
{
  if (displayTypeName == "discrete") return discrete;
  if (displayTypeName == "linear") return linear;
  return discrete; 
}


actionType getLEDActionTypeByName(String actionName)
{ //blockdet, dccswitch, dccsignal, svbutton, analoginp, powerstat
  if (actionName == "block") return blockdet; 
  if (actionName == "switch") return dccswitch;
  if (actionName == "signal") return dccsignalnmra;
  if (actionName == "signaldyn") return dccsignaldyn;
  if (actionName == "signalstat") return dccsignalstat;
  if (actionName == "button") return svbutton;
  if (actionName == "analog") return analoginp;
  if (actionName == "power") return powerstat;
  if (actionName == "constant") return constantled;
  return unknown; 
}

IoTT_ColorDefinitions::IoTT_ColorDefinitions()
{
}

IoTT_ColorDefinitions::~IoTT_ColorDefinitions()
{
}

void IoTT_ColorDefinitions::loadColDefJSON(JsonObject thisObj)
{
	String colName = thisObj["Name"];
    strcpy(colorName, colName.c_str()); 
    if (thisObj.containsKey("HSVVal"))
    {
		byte HSV0 = (byte)thisObj["HSVVal"][0];
        byte HSV1 = (byte)thisObj["HSVVal"][1];
        byte HSV2 = (byte)thisObj["HSVVal"][2];
        HSVVal = CHSV(HSV0, HSV1, HSV2);
    }
    else if (thisObj.containsKey("RGBVal"))
    {
        byte RGB0 = (byte)thisObj["RGBVal"][0];
        byte RGB1 = (byte)thisObj["RGBVal"][1];
        byte RGB2 = (byte)thisObj["RGBVal"][2];
        RGBVal = CRGB(RGB0, RGB1, RGB2);
        HSVVal = rgb2hsv_approximate(RGBVal); //FastLED function
    }
}

//******************************************************************************************************************************************************************
IoTT_LEDCmdList::IoTT_LEDCmdList()
{
}

IoTT_LEDCmdList::~IoTT_LEDCmdList()
{
}

void IoTT_LEDCmdList::updateLEDs()
{
}


void IoTT_LEDCmdList::loadCmdListJSON(JsonObject thisObj)
{
//	Serial.println("Loading Cmd Seq");
	upToVal = thisObj["Val"];
	if (thisObj.containsKey("ColOn"))
		colOn = parentObj->parentObj->getColorByName(thisObj["ColOn"]);
	else
		colOn = NULL;
	if (thisObj.containsKey("ColOff"))
		colOff = parentObj->parentObj->getColorByName(thisObj["ColOff"]);
	else
		colOff = NULL;
	dispMode = getColorModeByName(thisObj["Mode"]);
	blinkRate = thisObj["Rate"];
	transType = getTransitionTypeByName(thisObj["Transition"]);
}



//******************************************************************************************************************************************************************
IoTT_LEDHandler::IoTT_LEDHandler()
{
}

IoTT_LEDHandler::~IoTT_LEDHandler()
{
	freeObjects();
}

void IoTT_LEDHandler::freeObjects()
{
	if (cmdListLen > 0)
	{
		for (uint16_t i = 0; i < cmdListLen; i++)
			delete cmdList[i];
		cmdListLen = 0;
		free(cmdList);
	}
}

void IoTT_LEDHandler::updateLocalBlinkValues()
{
}


void IoTT_LEDHandler::updateChainData(IoTT_LEDCmdList * cmdDef, IoTT_LEDCmdList * cmdDefLin, uint8_t distance)
{
	
	CHSV targetCol, targetColLin;
	bool flipBlink = false;
	bool useGlobal = true;
	switch (cmdDef->dispMode)
	{
		case constlevel: 
			targetCol = (cmdDef->colOn != NULL) ? cmdDef->colOn->HSVVal : CHSV(0,0,0); 
			break;
		case globalrampdown: 
		case globalblinkneg: 
			flipBlink = true;
		case globalrampup: 
		case globalblinkpos: 
			if (parentObj->getBlinkStatus() ^ flipBlink)
			{
				targetCol = (cmdDef->colOn != NULL) ? cmdDef->colOn->HSVVal : CHSV(0,0,0); 
				if (cmdDefLin != NULL)
					targetColLin = (cmdDefLin->colOn != NULL) ? cmdDefLin->colOn->HSVVal : CHSV(0,0,0); 
			}
			else 
			{
				targetCol = (cmdDef->colOff != NULL) ? cmdDef->colOff->HSVVal : CHSV(0,0,0); 
				if (cmdDefLin != NULL)
					targetColLin = (cmdDefLin->colOff != NULL) ? cmdDefLin->colOff->HSVVal : CHSV(0,0,0); 
			}
			break;
		case localrampdown: 
		case localblinkneg: 
			flipBlink = true;
		case localrampup: 
		case localblinkpos: 
			useGlobal = false;
			uint16_t timeElapsed = millis() - blinkTimer;
			if (millis() > blinkTimer)
			{
				blinkStatus = !blinkStatus;
				blinkTimer += cmdDef->blinkRate;
				timeElapsed -= blinkInterval;
				if (millis() > blinkTimer) //exception correction in case something is not initialized.
				{
					blinkTimer = millis() + blinkInterval;
					timeElapsed = 0;
				}
			}
			faderValue = (float)timeElapsed/(float)blinkInterval;  //positive slope ramp from 0 to 1
			
			if (blinkStatus ^ flipBlink) 
			{
				targetCol = (cmdDef->colOn != NULL) ? cmdDef->colOn->HSVVal : CHSV(0,0,0); 
				if (cmdDefLin != NULL)
					targetColLin = (cmdDefLin->colOn != NULL) ? cmdDefLin->colOn->HSVVal : CHSV(0,0,0); 
			}
			else 
			{
				targetCol = (cmdDef->colOff != NULL) ? cmdDef->colOff->HSVVal : CHSV(0,0,0); 
				if (cmdDefLin != NULL)
					targetColLin = (cmdDefLin->colOff != NULL) ? cmdDefLin->colOff->HSVVal : CHSV(0,0,0); 
			}
			break;
	}
	if (cmdDefLin != NULL)
	{
		targetCol.h = round(((targetCol.h * distance) + (targetColLin.h * (100-distance))) / distance);
		targetCol.s = round(((targetCol.s * distance) + (targetColLin.s * (100-distance))) / distance);
		targetCol.v = round(((targetCol.v * distance) + (targetColLin.v * (100-distance))) / distance);
	}
	
	targetCol.v = round(targetCol.v * parentObj->getBrightness()); //this is the final target color, now we calculate the next step on the way there, if needed

	if ((targetCol.h != currentColor.h) || (targetCol.s != currentColor.s) || (targetCol.v != currentColor.v))
	{
		uint16_t blinkPeriod;
		if (useGlobal)
			blinkPeriod = parentObj->blinkInterval;
		else
			blinkPeriod = cmdDef->blinkRate;
		uint16_t rateH;
		if (blinkPeriod > 0)
			rateH = round((255/(float)blinkPeriod)*(1000/(float)parentObj->ledUpdateInterval)); //val_units per LED refresh interval at given blink period
		else
			rateH = 255; //immediate change, period 0
//		Serial.printf("Period: %i Rate %i\n", blinkPeriod, rateH);
		int16_t hueChange = targetCol.h - currentColor.h;
		int16_t satChange = targetCol.s - currentColor.s;
		float satRatio = satChange / hueChange;
		int16_t valChange = targetCol.v - currentColor.v;
		float valRatio = valChange / hueChange;

		int newTarget_h = targetCol.h;
		int newTarget_v = targetCol.v;
		
		switch (cmdDef->transType)
		{
//			Serial.println(cmdDef->transType);
			case direct: //just go to the next value
				currentColor = targetCol;
				break;
			case soft: //reduce brightness, change hue and saturation, increase brightness
				rateH *= 4;
//			    Serial.printf("Setting Soft h %i s %i v %i to h %i s %i v %i rate %i\n", currentColor.h, currentColor.s, currentColor.v, targetCol.h, targetCol.s, targetCol.v, rateH); 
				if (currentColor.h != targetCol.h)
				{
					if (currentColor.v > rateH)
						currentColor.v -= rateH;
					else
						currentColor.v = 0;
					if (currentColor.v == 0)
					{
						currentColor.h = targetCol.h;
						currentColor.s = targetCol.s;
					}
				}
				else
				{
					if ((currentColor.v + rateH) < targetCol.v)
						currentColor.v += rateH;
					else
						currentColor.v = targetCol.v;
				}
				break;
			case merge: //change hue, saturation, brightness simultaneously
//			    Serial.printf("Setting Merge h %i s %i v %i to h %i s %i v %i rate %i\n", currentColor.h, currentColor.s, currentColor.v, targetCol.h, targetCol.s, targetCol.v, rateH); 
				uint16_t newTarget_h = targetCol.h;
				int myDeltaH = rateH;
				switch (hueChange)
				{
					case -260 ... -128: //turning down on long way, we better turn up
						newTarget_h += 255;
						break;
					case -127 ... -1: //turning down on direct way
						myDeltaH = -rateH;
						break; 
					case 0 ... 127: //turning up on direct way
						break;
					case 128 ... 260: //turning up, but more than half way, so we turn down
						newTarget_h -= 255;
						myDeltaH = -rateH;
						break;
				}
				if (myDeltaH >= 0)
					if ((currentColor.h + rateH) > targetCol.h)
						currentColor.h = targetCol.h;
					else
						currentColor.h += rateH;
				else
					if ((currentColor.h - rateH) < targetCol.h)
						currentColor.h = targetCol.h;
					else
						currentColor.h -= rateH;
				if (currentColor.h == targetCol.h)
				{
					currentColor.s = targetCol.s;
					currentColor.v = targetCol.v;
				}
				else
				{
					currentColor.s = currentColor.s + round(myDeltaH * satRatio);
					currentColor.v = currentColor.v + round(myDeltaH * valRatio);
				}
				break;
		}
		for (int i = 0; i < ledAddrListLen; i++)
			parentObj->setCurrColHSV(ledAddrList[i], currentColor);
	}
}

void IoTT_LEDHandler::updateBlockDet()
{
	IoTT_LEDCmdList * cmdDef = NULL;
	//get target color based on status
	uint8_t blockStatus = getBDStatus(ctrlAddrList[0]);
	if (lastValue != blockStatus)
	{
		lastValue = blockStatus;
		blinkTimer = millis();
	}
    for (int i = 0; i < cmdListLen; i++)
    {
		switch (blockStatus)
		{
			case 0: if (cmdList[i]->upToVal <= 0) cmdDef = cmdList[i]; break;
			case 1: if ((cmdList[i]->upToVal > 0) & (cmdList[i]->upToVal <= 1)) cmdDef = cmdList[i]; break;
		}
		if (cmdDef != NULL) break; //found cmdDef
	}
	//update chain LED's
	if (cmdDef != NULL)
		updateChainData(cmdDef);
}

void IoTT_LEDHandler::updateSwitchPos()
{
	IoTT_LEDCmdList * cmdDef = NULL;
	uint8_t swiStatus = getSwiStatus(ctrlAddrList[0]) >> 4;
//	Serial.printf("Checking Swi %i Stat %i\n", ctrlAddrList[0], swiStatus);
	int16_t nextVal = -1;
	int16_t nextInd = -1;
	if (lastValue != swiStatus)
	{
		lastValue = swiStatus;
		blinkTimer = millis();
	}
	for (int i = 0; i < cmdListLen; i++)
	{
//		Serial.printf("Testing Value %i Status %i in Loop %i\n", cmdList[i]->upToVal, swiStatus, i);
		if ((swiStatus <= cmdList[i]->upToVal) && (swiStatus > nextVal))
		{
			nextInd = i;
			nextVal = cmdList[i]->upToVal;
		}
	}
	if (nextInd >= 0)
	{
//		Serial.printf("Updating Switch %i to Position %i Cmd %i \n", ctrlAddrList[0], swiStatus, nextInd);
		cmdDef = cmdList[nextInd];
		updateChainData(cmdDef);
	}
}

void IoTT_LEDHandler::updateSwSignalPos(bool isDynamic)
{
	IoTT_LEDCmdList * cmdDef = NULL;
	int16_t nextVal = -1;
	int16_t nextInd = -1;
	uint16_t swiStatus = 0;
	uint8_t swiBitMask = 0x01;
	uint8_t swiChgMask = 0;
	if (isDynamic)
	{
		swiStatus = lastValue;
		uint32_t lastAct = 0;
		uint8_t dynSwi = 0;
		for (int i = 0; i < ctrlAddrListLen; i++) //check for the latest activity
		{
			uint32_t hlpAct = getLastSwitchActivity(ctrlAddrList[i]);
			if (hlpAct > lastAct)
			{
				dynSwi = i;
				lastAct = hlpAct;
			}
		}
		if (lastAct != 0) //we have activity
		{
			if (lastAct != lastActivity)
			{
				Serial.printf("Updating %i activity for Switch %i\n", dynSwi, ctrlAddrList[dynSwi]);
				swiStatus = 2 * dynSwi;
				if (((getSwiStatus(ctrlAddrList[dynSwi]) >> 4) & 0x02) > 0)
					swiStatus++;
				lastActivity = lastAct;
			}
		}
	}
	else
	{
		for (int i = 0; i < ctrlAddrListLen; i++)
		{
			if (((getSwiStatus(ctrlAddrList[i]) >> 4) & 0x02) > 0)
				swiStatus |= swiBitMask;
			swiBitMask <<= 1;
		}
	}
	if (lastValue != swiStatus)
	{
		lastValue = swiStatus; //this is the static value
		blinkTimer = millis();
	}
//	Serial.printf("Checking Swi %i Stat %i\n", ctrlAddrList[0], swiStatus);
	for (int i = 0; i < cmdListLen; i++)
	{
//		Serial.printf("Testing Value %i Status %i in Loop %i\n", cmdList[i]->upToVal, swiStatus, i);
		if ((swiStatus <= cmdList[i]->upToVal) && (swiStatus > nextVal))
		{
			nextInd = i;
			nextVal = cmdList[i]->upToVal;
		}
	}
	if (nextInd >= 0)
	{
//		Serial.printf("Updating Switch %i to Position %i Cmd %i \n", ctrlAddrList[0], swiStatus, nextInd);
		cmdDef = cmdList[nextInd];
		updateChainData(cmdDef);
	}
}

void IoTT_LEDHandler::updateSignalPos()
{
	IoTT_LEDCmdList * cmdDef = NULL;
	IoTT_LEDCmdList * cmdDefLin = NULL;
	uint16_t sigAddress = ctrlAddrList[0];
	uint16_t sigAspect = getSignalAspect(ctrlAddrList[0]);
	if (lastValue != sigAspect)
	{
		lastValue = sigAspect;
		blinkTimer = millis();
	}
//    Serial.printf("Checking Signal %i to Aspect %i  \n", sigAddress, sigAspect);
	int16_t nextVal = -1;
	int16_t nextInd = -1;
	int16_t prevInd = -1;
	uint8_t distance = 0;
	for (int i = 0; i < cmdListLen; i++)
	{
		if ((sigAspect <= cmdList[i]->upToVal) && (sigAspect > nextVal))
		{
			prevInd = nextInd;
			nextInd = i;
			nextVal = cmdList[i]->upToVal;
		}
	}
	if (nextInd >= 0)
	{
//    Serial.printf("Updating Signal %i to Aspect %i Cmd %i \n", sigAddress, sigAspect, nextInd);
		cmdDef = cmdList[nextInd];
		if ((prevInd >= 0) && (displType == linear))
		{
			cmdDefLin = cmdList[prevInd];
			distance = round(((sigAspect - cmdDefLin->upToVal) / (cmdDef->upToVal - cmdDefLin->upToVal)) * 100);
			updateChainData(cmdDef, cmdDefLin, distance);
		}
		else
			updateChainData(cmdDef);
	}
}

void IoTT_LEDHandler::updateButtonPos()
{
	IoTT_LEDCmdList * cmdDef = NULL;
	uint16_t btnNr = ctrlAddrList[0];
	uint16_t btnState = getButtonValue(btnNr);
	int16_t nextVal = -1;
	int16_t nextInd = -1;
	if (lastValue != btnState)
	{
		lastValue = btnState;
		blinkTimer = millis();
	}
	for (int i = 0; i < cmdListLen; i++)
	{
		if ((btnState <= cmdList[i]->upToVal) && (btnState > nextVal))
		{
			nextInd = i;
			nextVal = cmdList[i]->upToVal;
		}
	}
	if (nextInd >= 0)
	{
//    Serial.printf("Updating Button %i to Aspect %i Cmd %i \n", btnNr, btnState, nextInd);
		cmdDef = cmdList[nextInd];
		updateChainData(cmdDef);
	}
}

void IoTT_LEDHandler::updateAnalogValue()
{
	IoTT_LEDCmdList * cmdDef = NULL;
	IoTT_LEDCmdList * cmdDefLin = NULL;
	uint16_t analogNr = ctrlAddrList[0];
	uint16_t analogVal = getAnalogValue(analogNr);
	if (lastValue != analogVal)
	{
		lastValue = analogVal;
		blinkTimer = millis();
	}
	int16_t nextVal = -1;
	int16_t nextInd = -1;
	int16_t prevInd = -1;
	uint8_t distance = 0;
//    Serial.printf("Analog Input %i to Value %i \n", analogNr, analogVal);
	for (int i = 0; i < cmdListLen; i++)
	{
//		Serial.printf("Checking %i < %i \n", analogVal, cmdList[i]->upToVal);
		if ((analogVal <= cmdList[i]->upToVal) && (analogVal > nextVal))
		{
			prevInd = nextInd;
			nextInd = i;
			nextVal = cmdList[i]->upToVal;
		}
	}
	if (nextInd >= 0)
	{
//        Serial.printf("Updating Analog Input %i to Value %i Cmd %i \n", analogNr, analogVal, nextInd);
		cmdDef = cmdList[nextInd];
		if ((prevInd >= 0) && (displType == linear))
		{
			cmdDefLin = cmdList[prevInd];
			distance = round(((analogVal - cmdDefLin->upToVal) / (cmdDef->upToVal - cmdDefLin->upToVal)) * 100);
			updateChainData(cmdDef, cmdDefLin, distance);
		}
		else
			updateChainData(cmdDef);
	}
}

void IoTT_LEDHandler::updatePowerStatus()
{
	IoTT_LEDCmdList * cmdDef = NULL;
	int16_t nextVal = -1;
	int16_t nextInd = -1;
	if (lastValue != getPowerStatus())
	{
		lastValue = getPowerStatus();
		blinkTimer = millis();
	}
	for (int i = 0; i < cmdListLen; i++)
	{
		if ((getPowerStatus() <= cmdList[i]->upToVal) && (getPowerStatus() > nextVal))
		{
			nextInd = i;
			nextVal = cmdList[i]->upToVal;
		}
	}
	if (nextInd >= 0)
	{
//    Serial.printf("Updating Power Status to Status %i Cmd %i \n", getPowerStatus(), nextInd);
		cmdDef = cmdList[nextInd];
		updateChainData(cmdDef);
	}
}

void IoTT_LEDHandler::updateConstantLED()
{
	IoTT_LEDCmdList * cmdDef = cmdList[0];
	updateChainData(cmdDef);
//    Serial.printf("Updating Constant LED %i Cmd %i \n", 0, 0);
}

void IoTT_LEDHandler::updateLEDs()
{
    switch (ctrlSource)
    {
		case blockdet: updateBlockDet(); break;
		case dccswitch: updateSwitchPos();break;
		case dccsignalstat: updateSwSignalPos(false); break;
		case dccsignaldyn:  updateSwSignalPos(true); break;
		case dccsignalnmra: updateSignalPos(); break;
		case svbutton: updateButtonPos(); break;
		case analoginp: updateAnalogValue(); break;
		case powerstat: updatePowerStatus(); break;
		case constantled: updateConstantLED(); break;
    }
}

void IoTT_LEDHandler::loadLEDHandlerJSON(JsonObject thisObj)
{
	freeObjects();
//	Serial.println("Loading LED Object");
	if (thisObj.containsKey("LEDNums"))
	{
		JsonArray LEDNums = thisObj["LEDNums"];
		ledAddrListLen = LEDNums.size();
		ledAddrList = (uint16_t*) realloc (ledAddrList, ledAddrListLen * sizeof(uint16_t));
		for (int i=0; i<ledAddrListLen;i++)
			ledAddrList[i] = LEDNums[i];
	}
	if (thisObj.containsKey("CtrlAddr"))
	{
		JsonArray CtrlAddr = thisObj["CtrlAddr"];
		if (CtrlAddr.isNull())
			ctrlAddrListLen = 1;
		else
			ctrlAddrListLen = CtrlAddr.size();
		ctrlAddrList = (uint16_t*) realloc (ctrlAddrList, ctrlAddrListLen * sizeof(uint16_t));
		if (CtrlAddr.isNull())
			ctrlAddrList[0] = thisObj["CtrlAddr"];
		else
			for (int i=0; i<ctrlAddrListLen;i++)
				ctrlAddrList[i] = CtrlAddr[i];
	}
	if (thisObj.containsKey("CtrlSource"))
		ctrlSource = getLEDActionTypeByName(thisObj["CtrlSource"]);
	if (thisObj.containsKey("DisplayType"))
		displType = getTransitionTypeByName(thisObj["DisplayType"]);
	if (thisObj.containsKey("LEDCmd"))
	{
		JsonArray LEDCmd = thisObj["LEDCmd"];
		cmdListLen = LEDCmd.size();
        cmdList = (IoTT_LEDCmdList**) realloc (cmdList, cmdListLen * sizeof(IoTT_LEDCmdList*));
		for (int i=0; i<cmdListLen;i++)
		{
          IoTT_LEDCmdList * thisCmdEntry = new(IoTT_LEDCmdList);
          thisCmdEntry->parentObj = this;
          thisCmdEntry->loadCmdListJSON(LEDCmd[i]);
          cmdList[i] = thisCmdEntry;
		}
        
	}
}


//******************************************************************************************************************************************************************
IoTT_ledChain::IoTT_ledChain()
{
#ifdef useRTOS
	ledBaton = xSemaphoreCreateMutex();
#endif
}

IoTT_ledChain::~IoTT_ledChain()
{
	freeObjects();
}

void IoTT_ledChain::freeObjects()
{
	if (colorDefListLen > 0)
	{
		for (uint16_t i = 0; i < colorDefListLen; i++)
			delete colorDefinitionList[i];
		colorDefListLen = 0;
		free(colorDefinitionList);
	}
	if (LEDHandlerListLen > 0)
	{
		for (uint16_t i = 0; i < LEDHandlerListLen; i++)
			delete LEDHandlerList[i];
		LEDHandlerListLen = 0;
		free(LEDHandlerList);
	}
}

void IoTT_ledChain::loadLEDChainJSON(DynamicJsonDocument doc)
{
	freeObjects();
	Serial.println("Load Chain Params");
	if (doc.containsKey("ChainParams"))
    {
        chainLength = doc["ChainParams"]["NumLEDs"];
        currentBrightness = doc["ChainParams"]["Brightness"]["InitLevel"];
        brightnessControlType = getLEDActionTypeByName(doc["ChainParams"]["Brightness"]["CtrlSource"]);
        brightnessControlAddr = doc["ChainParams"]["Brightness"]["Addr"];
        initChain(chainLength);
		switch (brightnessControlType)
		{
			case analoginp: setAnalogValue(brightnessControlAddr, round(4095 * currentBrightness)); break;
			default: currentBrightness = 1.0; break;
		}
    }
	Serial.println("Load Colors");
    if (doc.containsKey("LEDCols"))
    {
        JsonArray LEDCols = doc["LEDCols"];
        colorDefListLen = LEDCols.size();
        colorDefinitionList = (IoTT_ColorDefinitions**) realloc (colorDefinitionList, colorDefListLen * sizeof(IoTT_ColorDefinitions*));
        for (int i=0; i < colorDefListLen; i++)
        {
			
          IoTT_ColorDefinitions * thisColorDefEntry = new(IoTT_ColorDefinitions);
          thisColorDefEntry->loadColDefJSON(LEDCols[i]);
          colorDefinitionList[i] = thisColorDefEntry;
        }
//        Serial.printf("%i colors loaded\n", colorDefListLen);
    }
	Serial.println("Load LED Defs");
    if (doc.containsKey("LEDDefs"))
    {
        JsonArray LEDDefs = doc["LEDDefs"];
        LEDHandlerListLen = LEDDefs.size();
        LEDHandlerList = (IoTT_LEDHandler**) realloc (LEDHandlerList, LEDHandlerListLen * sizeof(IoTT_LEDHandler*));
        for (int i=0; i < LEDHandlerListLen; i++)
        {
			IoTT_LEDHandler * thisLEDHandlerEntry = new(IoTT_LEDHandler);
			thisLEDHandlerEntry->parentObj = this;
			thisLEDHandlerEntry->loadLEDHandlerJSON(LEDDefs[i]);
			LEDHandlerList[i] = thisLEDHandlerEntry;
		}
//        Serial.printf("%i LED Defs loaded\n", LEDHandlerListLen);
	}
	Serial.println("Load LED Defs Complete");
}

IoTT_ColorDefinitions * IoTT_ledChain::getColorByName(String colName)
{
	for (int i=0; i<colorDefListLen;i++)
	{
		IoTT_ColorDefinitions * thisPointer = colorDefinitionList[i];
		if (String(thisPointer->colorName) == colName)
			return thisPointer;
	}
	return NULL;
}

uint16_t IoTT_ledChain::getChainLength()
{
	return chainLength;
}

CRGB * IoTT_ledChain::getChain()
{
	return ledChain;
}

float_t IoTT_ledChain::getBrightness()
{
	return currentBrightness;
}

void IoTT_ledChain::setBrightness(float_t newVal)
{
	currentBrightness = newVal;
}

uint8_t IoTT_ledChain::getBrightnessControlType()
{
	return brightnessControlType;
}

uint16_t IoTT_ledChain::getBrightnessControlAddr()
{
	return brightnessControlAddr;
}


CRGB * IoTT_ledChain::initChain(word numLEDs)
{
    ledChain = (CRGB*) realloc (ledChain, numLEDs * sizeof(CRGB));
    for (int i = 0; i < numLEDs; i++)
		setCurrColRGB(i, CHSV(0,255,0)); //initialize dark
	needUpdate = true;
	blinkTimer = millis() + blinkInterval;
	ledUpdateTimer = millis() + ledUpdateInterval;
	faderValue = 0;
	return ledChain;
}

void IoTT_ledChain::setCurrColRGB(uint16_t ledNr, CRGB newCol)
{
#ifdef useRTOS
    xSemaphoreTake(ledBaton, portMAX_DELAY);
#endif
	ledChain[ledNr] = newCol;
	needUpdate = true;
#ifdef useRTOS
    xSemaphoreGive(ledBaton);
#endif
}

void IoTT_ledChain::setCurrColHSV(uint16_t ledNr, CHSV newCol)
{
#ifdef useRTOS
    xSemaphoreTake(ledBaton, portMAX_DELAY);
#endif
	ledChain[ledNr] = newCol;
	needUpdate = true;
#ifdef useRTOS
    xSemaphoreGive(ledBaton);
#endif
}

void IoTT_ledChain::setBlinkRate(uint16_t blinkVal)
{
	blinkInterval = blinkVal;
}

void IoTT_ledChain::setRefreshInterval(uint16_t newInterval)
{
	ledUpdateInterval = newInterval;
}

bool IoTT_ledChain::getBlinkStatus()
{
	return blinkStatus;
}

float_t IoTT_ledChain::getFaderValue()
{
	if (blinkStatus)
	  return faderValue;
	else
	  return (1 - faderValue);
}

CRGB IoTT_ledChain::getCurrColRGB(uint16_t ledNr)
{
	return ledChain[ledNr];
}

CHSV IoTT_ledChain::getCurrColHSV(uint16_t ledNr)
{
	return rgb2hsv_approximate(ledChain[ledNr]);
}

void IoTT_ledChain::updateLEDs()
{
	switch (getBrightnessControlType())
	{
		case analoginp: setBrightness((float)getAnalogValue(getBrightnessControlAddr())/4095); break;
		default: setBrightness(1.0); break;
	}
	if (LEDHandlerListLen > 0)
		for (uint16_t i = 0; i < LEDHandlerListLen; i++)
			LEDHandlerList[i]->updateLEDs();
		
}

void IoTT_ledChain::processChain()
{
    uint16_t timeElapsed = millis() - blinkTimer;
	if (millis() > blinkTimer)
	{
		blinkStatus = !blinkStatus;
		blinkTimer += blinkInterval;
        timeElapsed -= blinkInterval;
		if (millis() > blinkTimer) //exception correction in case something is not initialized.
		{
		  blinkTimer = millis() + blinkInterval;
		  timeElapsed = 0;
	    }
	}
    faderValue = (float)timeElapsed/(float)blinkInterval;  //positive slope ramp from 0 to 1
    
	if (millis() > ledUpdateTimer)
	{
		ledUpdateTimer += ledUpdateInterval;
		if (millis() > ledUpdateTimer) //exception correction in case something is not initialized.
		  ledUpdateTimer = millis() + ledUpdateInterval;
		updateLEDs();
	}
	
	if (needUpdate)
	{
//		Serial.println("update LEDs");
#ifdef useRTOS
		xSemaphoreTake(ledBaton, portMAX_DELAY);
#endif
		FastLED.show();
		needUpdate = false;
#ifdef useRTOS
		xSemaphoreGive(ledBaton);
#endif
	}
}

