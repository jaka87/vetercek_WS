// Adaptation of Adafruit FONA library for Botletics hardware
// Original text below:

/***************************************************
  This is a library for our Adafruit FONA Cellular Module

  Designed specifically to work with the Adafruit FONA
  ----> http://www.adafruit.com/products/1946
  ----> http://www.adafruit.com/products/1963

  These displays use TTL Serial to communicate, 2 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include "BotleticsSIM7000.h"




Botletics_modem::Botletics_modem(int8_t rst)
{
  _rstpin = rst;

  apn = F("");
  mySerial = 0;
  ok_reply = F("OK");
}

uint8_t Botletics_modem::type(void) {
  return _type;
}

boolean Botletics_modem::checkAT() {
	if (sendCheckReply(F("AT"), ok_reply),300)
		return true;
return false;
}

boolean Botletics_modem::begin(Stream &port) {
  mySerial = &port;

  DEBUG_PRINTLN(F("Attempting to open comm with ATs"));
  // give 6 seconds to reboot
  int16_t timeout = 6000;
  int16_t timeoutSerial = 8000;

  while (timeoutSerial > 0 and !mySerial->available()) {
    delay(1000);
    timeoutSerial-=1000;
    	  //digitalWrite(13, HIGH);   // turn the LED on
		  //delay(100);              // wait
		  //digitalWrite(13, LOW);    // turn the LED
  }

  if (timeoutSerial==0) {
	  digitalWrite(10, LOW);
	  delay(1500);
	  digitalWrite(10, HIGH);
	  delay(6000);
  }

  while (timeout > 0) {
    while (mySerial->available()) mySerial->read();
    if (sendCheckReply(F("AT"), ok_reply))
      break;
    while (mySerial->available()) mySerial->read();
    if (sendCheckReply(F("AT"), F("AT")))
      break;
    delay(1000);
    timeout-=1000;
  }
  
  // turn off Echo!
  sendCheckReply(F("ATE0"), ok_reply);
  delay(100);

  if (! sendCheckReply(F("ATE0"), ok_reply)) {
    return false;
  }

  // turn on hangupitude
  delay(100);
  flushInput();


  // DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN("ATI");
  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN("AT+GMR"); // This definitely should have the module name, but ATI may not

  // mySerial->println("ATI");
  mySerial->println("AT+GMR");
  readline(500, true);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);




   if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7000")) != 0) {
    _type = SIM7000;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7070")) != 0) {
    _type = SIM7070;
		 if (prog_char_strstr(replybuffer, (prog_char *)F("1951B08SIM7070")) != 0) {
		 _type2 = 1;
		 }
		 else  {
		 _type2 = 2;
		 }

}


  return true;
}




/********* POWER, BATTERY & ADC ********************************************/

/* returns value in mV (uint16_t) */
boolean Botletics_modem::getBattVoltage(uint16_t *v) {

    return sendParseReply(F("AT+CBC"), F("+CBC: "), v, ',', 2);
}

/* powers down the SIM module */
boolean Botletics_modem::powerDown(void) {

    if (! sendCheckReply(F("AT+CPOWD=1"), F("NORMAL POWER DOWN"))) // Normal power off
        return false;


  return true;
}






/********* NETWORK AND WIRELESS CONNECTION SETTINGS ***********************/

// Uses the AT+CFUN command to set functionality (refer to AT+CFUN in manual)
// 0 --> Minimum functionality
// 1 --> Full functionality
// 4 --> Disable RF
// 5 --> Factory test mode
// 6 --> Restarts module
// 7 --> Offline mode
boolean Botletics_modem::setFunctionality(uint8_t option) {
  return sendCheckReply(F("AT+CFUN="), option, ok_reply);
}

boolean Botletics_modem::reset() {
  return sendCheckReply(F("AT+CFUN=1,1"), ok_reply);
}

// 2  - Automatic
// 13 - GSM only
// 38 - LTE only
// 51 - GSM and LTE only
boolean Botletics_modem_LTE::setPreferredMode(uint8_t mode) {
  return sendCheckReply(F("AT+CNMP="), mode, ok_reply);
}

// 1 - CAT-M
// 2 - NB-IoT
// 3 - CAT-M and NB-IoT
boolean Botletics_modem_LTE::setPreferredLTEMode(uint8_t mode) {
  return sendCheckReply(F("AT+CMNB="), mode, ok_reply);
}

// Useful for choosing a certain carrier only
// For example, AT&T uses band 12 in the US for LTE CAT-M
// whereas Verizon uses band 13
// Mode: "CAT-M" or "NB-IOT"
// Band: The cellular EUTRAN band number
boolean Botletics_modem_LTE::setOperatingBand(const char * mode, uint8_t band) {
  char cmdBuff[24];

  sprintf(cmdBuff, "AT+CBANDCFG=\"%s\",%i", mode, band);

  return sendCheckReply(cmdBuff, ok_reply);
}

boolean Botletics_modem_LTE::setNetwork(uint16_t net, uint8_t band) {
  char cmdBuff[24];
  sprintf(cmdBuff, "AT+COPS=4,2,\"%i\",%i",net,band);
  return sendCheckReply(cmdBuff, ok_reply, 15000);
   }

boolean Botletics_modem_LTE::setCOPS( uint8_t band) {
  char cmdBuff[24];
  sprintf(cmdBuff, "AT+COPS=%i",band);
  return sendCheckReply(cmdBuff, ok_reply, 3500);
   }

boolean Botletics_modem_LTE::activatePDP( uint8_t band) {
  char cmdBuff[24];
  sprintf(cmdBuff, "AT+CGACT=%i,1",band);
  return sendCheckReply(cmdBuff, ok_reply, 3500);
   }

// Sleep mode reduces power consumption significantly while remaining registered to the network
// NOTE: USB port must be disconnected before this will take effect
boolean Botletics_modem::enableSleepMode(bool onoff) {
  return sendCheckReply(F("AT+CSCLK="), onoff, ok_reply);
}

// Set e-RX parameters
// Mode options:
// 0  Disable the use of eDRX
// 1  Enable the use of eDRX
// 2  Enable the use of eDRX and auto report
// 3  Disable the use of eDRX(Reserved)

// Connection type options:
// 4 - CAT-M
// 5 - NB-IoT

// See AT command manual for eDRX values (options 0-15)

// NOTE: Network must support eDRX mode
boolean Botletics_modem::set_eDRX(uint8_t mode, uint8_t connType, char * eDRX_val) {
  if (strlen(eDRX_val) > 4) return false;

  char auxStr[21];

  sprintf(auxStr, "AT+CEDRXS=%i,%i,\"%s\"", mode, connType, eDRX_val);

  return sendCheckReply(auxStr, ok_reply);
}

boolean Botletics_modem::setNetLED(bool onoff, uint8_t mode, uint16_t timer_on, uint16_t timer_off) {
  if (onoff) {
    if (! sendCheckReply(F("AT+CNETLIGHT=1"), ok_reply)) return false;
      delay(100);
    if (! sendCheckReply(F("AT+CSGS=2"), ok_reply)) return false;

    if (mode > 0) {
      char auxStr[24];

      sprintf(auxStr, "AT+SLEDS=%i,%i,%i", mode, timer_on, timer_off);

      return sendCheckReply(auxStr, ok_reply);
    }
    else {
      return false;
    }
  }
  else {
    return sendCheckReply(F("AT+CNETLIGHT=0"), ok_reply);
  }
}



/********* NETWORK *******************************************************/

uint8_t Botletics_modem::getNetworkStatus(void) {
  uint16_t status;

    if (! sendParseReply(F("AT+CGREG?"), F("+CGREG: "), &status, ',', 1)) return 0;

  DEBUG_PRINT (F("\t network status ")); DEBUG_PRINTLN(status);

  return status;
}


uint8_t Botletics_modem::getRSSI(void) {
  uint16_t reply;

  if (! sendParseReply(F("AT+CSQ"), F("+CSQ: "), &reply) ) return 0;

  return reply;
}






/********* GPRS **********************************************************/


boolean Botletics_modem::enableGPRS(boolean onoff) {
 
    // getNetworkInfo();

    if (! openWirelessConnection(onoff)) return false;
    // if (! wirelessConnStatus()) return false;
 
  return true;
}

int8_t Botletics_modem::getNetworkType(char *typeStringBuffer, size_t bufferLength)
{
  uint16_t type;

  if (! sendParseReply(F("AT+CNSMOD?"), F("+CNSMOD:"), &type, ',', 1))
    return -1;

  if (typeStringBuffer != NULL)
  {
    switch (type)
    {
      case 0:
        strncpy(typeStringBuffer, "no service", bufferLength);
        break;
      case 1:
        strncpy(typeStringBuffer, "GSM", bufferLength);
        break;
      case 3:
        strncpy(typeStringBuffer, "EGPRS", bufferLength);
        break;
      case 7:
        strncpy(typeStringBuffer, "LTE M1", bufferLength);
        break;
      case 9:
        strncpy(typeStringBuffer, "LTE NB", bufferLength);
        break;
      default:
        strncpy(typeStringBuffer, "unknown", bufferLength);
        break;
    }
  }

  return (int8_t)type;
}

// Returns bearer status
// -1 Command returned with an error
// 0 Bearer is connecting
// 1 Bearer is connected
// 2 Bearer is closing
// 3 Bearer is closed
int8_t Botletics_modem::getBearerStatus(void)
{
  uint16_t state;

  if (! sendParseReply(F("AT+SAPBR=2,1"), F("+SAPBR: "), &state, ',', 1))
    return -1;

  return (int8_t)state;
}


int8_t Botletics_modem::getNetworkInfo(void) {
  uint16_t state;
  sendParseReply(F("AT+COPS?"), F("COPS: "), &state, ',', 3);
  DEBUG_PRINT("cops: "); DEBUG_PRINTLN(state); // Print out server reply
  return state;
}

bool Botletics_modem::getNetworkInfoLong(void) {
  if (!sendCheckReply(F("AT+COPS=?"), ok_reply, 2000))
     return false;

  return true;
}



int8_t Botletics_modem::GPRSstate(void) {
  uint16_t state;

  if (! sendParseReply(F("AT+CGATT?"), F("+CGATT: "), &state) )
    return -1;
  DEBUG_PRINT("GPRSstate: "); DEBUG_PRINTLN(state); 

  return state;
}

void Botletics_modem::setNetworkSettings(FStringPtr apn) {
  this->apn = apn;
  if (_type >= SIM7000) sendCheckReplyQuoted(F("AT+CGDCONT=1,\"IP\","), apn, ok_reply, 10000);
  //if (_type >= SIM7000) sendCheckReplyQuoted(F("AT+CNCFG=0,1,"), apn, ok_reply, 10000);
}


// Open or close wireless data connection
boolean Botletics_modem::openWirelessConnection(bool onoff) {
    if (!onoff) { // Disconnect wireless
        if (!sendCheckReply(F("AT+CNACT=0,0"), ok_reply)) return false;

        // Wait briefly for the unsolicited DEACTIVE message (optional)
        unsigned long start = millis();
        while (millis() - start < 2000) { // 2s timeout
            readline(500, true);
            if (_type == SIM7070 && strstr(replybuffer, "+APP PDP: 0,DEACTIVE")) break;
        }
    }
    else { // Activate PDP
        if (!sendCheckReply(F("AT+CNACT=0,1"), ok_reply)) return false;

        // Wait for unsolicited ACTIVE message (SIM7070)
        if (_type == SIM7070) {
            unsigned long start = millis();
            bool activeSeen = false;
            while (millis() - start < 5000) { // 5s timeout
                readline(500, true);
                if (strstr(replybuffer, "+APP PDP: 0,ACTIVE")) {
                    activeSeen = true;
                    break;
                }
            }
            if (!activeSeen) return false;
        }
        else {
            delay(1000); // SIM7000: short delay is enough
        }
    }

    return true;
}


// Query wireless connection status
boolean Botletics_modem::wirelessConnStatus(void) {
  getReply(F("AT+CNACT?"));
  // Format of response:
  // +CNACT: <status>,<ip_addr>  (ex.SIM7000)
  // +CNACT: <pdpidx>,<status>,<ip_addr>  (ex.SIM7070)
	      DEBUG_PRINT("\t<--- "); DEBUG_PRINTLN(replybuffer);
    if (strstr(replybuffer, "+CNACT: 0,1") == NULL) return false;

  return true;
}


int8_t Botletics_modem::GPRSPDP(void) {
  uint16_t state;

  if (! sendParseReply(F("AT+CGDCONT?"), F("+CGDCONT: "), &state) )
    return -1;
  DEBUG_PRINT("GPRSPDP: "); DEBUG_PRINTLN(state); 


  return state;
}


/********* UDP FUNCTIONS  ************************************/


boolean Botletics_modem::UDPconnect(char *server, uint16_t port) {
  flushInput();
  char buffer[50]; // Make sure the buffer is large enough to hold the entire string

	  //sendCheckReply(F("AT+CACLOSE=0"),  F("OK"), 300);
     //sendCheckReply(F("AT"),  F("OK"), 300);
     sprintf(buffer, "AT+CAOPEN=0,0,\"UDP\",\"%s\",\"%u\"", server, port);
      if (! sendCheckReply(buffer,  F("+CAOPEN: 0,0"), 5000))
        return false;	  
	  
	  // looks like it was a success (?)
	  return true;
  
}

boolean Botletics_modem::UDPclose(void) {
      if (! sendCheckReply(F("AT+CACLOSE=0"),  F("OK"), 500))
        return false;
  return true;
  
}

uint8_t Botletics_modem::UDPconnected(void) {
  getReply(F("AT+CASTATE?"));
	      DEBUG_PRINT("\t<--- "); DEBUG_PRINTLN(replybuffer);
    if (strstr(replybuffer, "+CASTATE: 0,1") == NULL) return false;
}


uint8_t Botletics_modem::UDPsend(unsigned char *packet, uint8_t len, byte response[12], uint8_t charr) {  
    uint8_t howmany;
    char buffer[20];
    byte replybuffer2[24];  // Local buffer - saves SRAM

    flushInput();

    sprintf(buffer, "AT+CASEND=0,%u", len);
    mySerial->println(buffer);
    DEBUG_PRINTLN(buffer);

    // Dynamic wait for '>' prompt - more efficient than fixed timeout
    unsigned long startTime = millis();
    bool gotPrompt = false;
    
    while (millis() - startTime < 2000) { // Max 2 seconds for prompt
        if (mySerial->available()) {
            char c = mySerial->read();
            if (c == '>') {
                gotPrompt = true;
                DEBUG_PRINTLN(F("\t<--- Got '>' prompt"));
                break;
            }
        }
        delay(1); // Small delay to prevent busy-waiting
    }
    
    if (!gotPrompt) {
        DEBUG_PRINTLN(F("\t<--- No prompt received"));
        return 3;
    }
    
    // Send packet immediately after prompt
    mySerial->write(packet, len);


	// Wait for SEND OK or OK for up to 3 seconds
	startTime = millis();
	bool gotSendOk = false;
		 
	while (millis() - startTime < 5000) {
		 if (mySerial->available()) {
			  uint8_t l = readline(150); // Increased timeout slightly for complete lines
			  
			  if (l > 0) {
					DEBUG_PRINT(F("\t<--- ")); DEBUG_PRINTLN(replybuffer);
					
					// More robust checking
					char* sendOkPos = strstr(replybuffer, "SEND OK");
					char* okPos = strstr(replybuffer, "OK");
					
					if (sendOkPos != NULL) {
						 gotSendOk = true;
						 break;
					}
					// Check for standalone "OK" (not part of another word)
					else if (okPos != NULL) {
						 // Make sure it's "OK" at end or surrounded by non-alphanumeric chars
						 uint8_t okIndex = okPos - replybuffer;
						 
						 // Check if OK is at the end of string
						 if (okIndex + 2 == l) { // "OK" is last 2 chars
							  gotSendOk = true;
							  break;
						 }
						 // Or check if next char after "OK" is not alphanumeric
						 else if (okIndex + 2 < l) {
							  char nextChar = replybuffer[okIndex + 2];
							  if (!isalnum(nextChar)) {
									gotSendOk = true;
									break;
							  }
						 }
					}
					
					// Optional: Also check for errors
					if (strstr(replybuffer, "ERROR") != NULL) {
						 DEBUG_PRINTLN(F("\t<--- ERROR received"));
						 return 4; // Fail fast on error
					}
			  }
		 }
		 
		 // Small delay to prevent busy-waiting
		 delay(10);
	}
		 
	if (!gotSendOk) {
		 DEBUG_PRINTLN(F("\t<--- No SEND OK received"));
		 return 4;
	}

	// Wait for data indication (network response)
	startTime = millis();
	bool gotDataInd = false;

	while (millis() - startTime < 8000) { // Max 8 seconds for network response
		 if (mySerial->available()) {
			  uint8_t l = readline(100);
			  if (l > 0) {
					DEBUG_PRINT(F("\t<--- ")); DEBUG_PRINTLN(replybuffer);
					// Use strstr instead of strcmp for more flexible matching
					if (strstr(replybuffer, "+CADATAIND: 0") != NULL) {
						 gotDataInd = true;
						 break;
					}
			  }
		 }
		 delay(10);
	}

    if (!gotDataInd) {
        DEBUG_PRINTLN(F("\t<--- No data indication received"));
        return 5;
    }

    // Handle different firmware versions
    if (_type2 == 2) { // different firmware version
        uint8_t sendD3 = readline(1000); // buffer full
        DEBUG_PRINT(F("\t<--- ")); DEBUG_PRINTLN(replybuffer);
    }

    // Request received data
    mySerial->println(F("AT+CARECV=0,25"));
	uint16_t replyidx = 0;
	uint16_t timeout = 2000;
	while (timeout--) {
		 if (replyidx >= charr) break;
		 
		 while(mySerial->available()) {
			  replybuffer2[replyidx++] = mySerial->read();
			  if (replyidx == charr) break;
		 }
		 
		 if (timeout == 0) break;
		 delay(1);
	}
	replybuffer2[replyidx] = 0;
	uint8_t receveD = replyidx;



    // Parse response based on format
    if (replybuffer2[12]==50 && replybuffer2[13]==44){ // "2," in ASCII
        howmany=13;
    } else {
        howmany=12;
    }
    
    // Extract response data
    for (uint16_t i=0; i<charr; i++) {
        if (i > howmany) {
            response[i-(howmany+1)] = replybuffer2[i];
        }
    }
    
    // Check if response is valid
    if (response[0] > 0 && response[1] < 4) return 1;
    else return 6;
}


/********* HELPERS *********************************************/

boolean Botletics_modem::expectReply(FStringPtr reply,
                                   uint16_t timeout) {
  readline(timeout);

  DEBUG_PRINT(F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
}

/********* LOW LEVEL *******************************************/

inline int Botletics_modem::available(void) {
  return mySerial->available();
}

inline size_t Botletics_modem::write(uint8_t x) {
  return mySerial->write(x);
}

inline int Botletics_modem::read(void) {
  return mySerial->read();
}

inline int Botletics_modem::peek(void) {
  return mySerial->peek();
}

inline void Botletics_modem::flush() {
  mySerial->flush();
}

void Botletics_modem::flushInput() {
    // Read all available serial input to flush pending data.
    uint16_t timeoutloop = 0;
    while (timeoutloop++ < 40) {
        while(available()) {
            read();
            timeoutloop = 0;  // If char was received reset the timer
        }
        delay(1);
    }
}

uint16_t Botletics_modem::readRaw(uint16_t b) {
  uint16_t idx = 0;

  while (b && (idx < sizeof(replybuffer)-1)) {
    if (mySerial->available()) {
      replybuffer[idx] = mySerial->read();
      idx++;
      b--;
    }
  }
  replybuffer[idx] = 0;

  return idx;
}

uint8_t Botletics_modem::readline(uint16_t timeout, boolean multiline) {
  uint16_t replyidx = 0;

  while (timeout--) {
    if (replyidx >= 254) {
      //DEBUG_PRINTLN(F("SPACE"));
      break;
    }

    while(mySerial->available()) {
      char c =  mySerial->read();
      if (c == '\r') continue;
      if (c == 0xA) {
        if (replyidx == 0)   // the first 0x0A is ignored
          continue;

        if (!multiline) {
          timeout = 0;         // the second 0x0A is the end of the line
          break;
        }
      }
      replybuffer[replyidx] = c;
      //DEBUG_PRINT(c, HEX); DEBUG_PRINT("#"); DEBUG_PRINTLN(c);

      if (++replyidx >= 254)
        break;
    }

    if (timeout == 0) {
      //DEBUG_PRINTLN(F("TIMEOUT"));
      break;
    }
    delay(1);
  }
  replybuffer[replyidx] = 0;  // null term
  return replyidx;
}



uint8_t Botletics_modem::getReply(const char *send, uint16_t timeout) {
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(send);


  mySerial->println(send);

  uint8_t l = readline(timeout);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return l;
}

uint8_t Botletics_modem::getReply(FStringPtr send, uint16_t timeout) {
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(send);


  mySerial->println(send);

  uint8_t l = readline(timeout);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return l;
}

// Send prefix, suffix, and newline. Return response (and also set replybuffer with response).
uint8_t Botletics_modem::getReply(FStringPtr prefix, char *suffix, uint16_t timeout) {
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(prefix); DEBUG_PRINTLN(suffix);


  mySerial->print(prefix);
  mySerial->println(suffix);

  uint8_t l = readline(timeout);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return l;
}

// Send prefix, suffix, and newline. Return response (and also set replybuffer with response).
uint8_t Botletics_modem::getReply(FStringPtr prefix, int32_t suffix, uint16_t timeout) {
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(prefix); DEBUG_PRINTLN(suffix, DEC);


  mySerial->print(prefix);
  mySerial->println(suffix, DEC);

  uint8_t l = readline(timeout);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return l;
}

// Send prefix, suffix, suffix2, and newline. Return response (and also set replybuffer with response).
uint8_t Botletics_modem::getReply(FStringPtr prefix, int32_t suffix1, int32_t suffix2, uint16_t timeout) {
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(prefix);
  DEBUG_PRINT(suffix1, DEC); DEBUG_PRINT(','); DEBUG_PRINTLN(suffix2, DEC);


  mySerial->print(prefix);
  mySerial->print(suffix1);
  mySerial->print(',');
  mySerial->println(suffix2, DEC);

  uint8_t l = readline(timeout);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return l;
}

// Send prefix, ", suffix, ", and newline. Return response (and also set replybuffer with response).
uint8_t Botletics_modem::getReplyQuoted(FStringPtr prefix, FStringPtr suffix, uint16_t timeout) {
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(prefix);
  DEBUG_PRINT('"'); DEBUG_PRINT(suffix); DEBUG_PRINTLN('"');


  mySerial->print(prefix);
  mySerial->print('"');
  mySerial->print(suffix);
  mySerial->println('"');

  uint8_t l = readline(timeout);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return l;
}

boolean Botletics_modem::sendCheckReply(const char *send, const char *reply, uint16_t timeout) {
  if (! getReply(send, timeout) )
    return false;
/*
  for (uint8_t i=0; i<strlen(replybuffer); i++) {
  DEBUG_PRINT(replybuffer[i], HEX); DEBUG_PRINT(" ");
  }
  DEBUG_PRINTLN();
  for (uint8_t i=0; i<strlen(reply); i++) {
    DEBUG_PRINT(reply[i], HEX); DEBUG_PRINT(" ");
  }
  DEBUG_PRINTLN();
  */
  return (strcmp(replybuffer, reply) == 0);
}

boolean Botletics_modem::sendCheckReply(FStringPtr send, FStringPtr reply, uint16_t timeout) {
  if (! getReply(send, timeout) )
    return false;

  return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
}

boolean Botletics_modem::sendCheckReply(const char* send, FStringPtr reply, uint16_t timeout) {
  if (! getReply(send, timeout) )
    return false;
  return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
}


// Send prefix, suffix, and newline.  Verify modem response matches reply parameter.
boolean Botletics_modem::sendCheckReply(FStringPtr prefix, char *suffix, FStringPtr reply, uint16_t timeout) {
  getReply(prefix, suffix, timeout);
  return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
}

// Send prefix, suffix, and newline.  Verify modem response matches reply parameter.
boolean Botletics_modem::sendCheckReply(FStringPtr prefix, int32_t suffix, FStringPtr reply, uint16_t timeout) {
  getReply(prefix, suffix, timeout);
  return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
}

// Send prefix, suffix, suffix2, and newline.  Verify modem response matches reply parameter.
boolean Botletics_modem::sendCheckReply(FStringPtr prefix, int32_t suffix1, int32_t suffix2, FStringPtr reply, uint16_t timeout) {
  getReply(prefix, suffix1, suffix2, timeout);
  return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
}

// Send prefix, ", suffix, ", and newline.  Verify modem response matches reply parameter.
boolean Botletics_modem::sendCheckReplyQuoted(FStringPtr prefix, FStringPtr suffix, FStringPtr reply, uint16_t timeout) {
  getReplyQuoted(prefix, suffix, timeout);
  return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
}


boolean Botletics_modem::parseReply(FStringPtr toreply,
          uint16_t *v, char divider, uint8_t index) {
  char *p = prog_char_strstr(replybuffer, (prog_char*)toreply);  // get the pointer to the voltage
  if (p == 0) return false;
  p+=prog_char_strlen((prog_char*)toreply);
  //DEBUG_PRINTLN(p);
  for (uint8_t i=0; i<index;i++) {
    // increment dividers
    p = strchr(p, divider);
    if (!p) return false;
    p++;
    //DEBUG_PRINTLN(p);

  }
  *v = atoi(p);

  return true;
}

boolean Botletics_modem::parseReply(FStringPtr toreply,
          char *v, char divider, uint8_t index) {
  uint8_t i=0;
  char *p = prog_char_strstr(replybuffer, (prog_char*)toreply);
  if (p == 0) return false;
  p+=prog_char_strlen((prog_char*)toreply);

  for (i=0; i<index;i++) {
    // increment dividers
    p = strchr(p, divider);
    if (!p) return false;
    p++;
  }

  for(i=0; i<strlen(p);i++) {
    if(p[i] == divider)
      break;
    v[i] = p[i];
  }

  v[i] = '\0';

  return true;
}

// Parse a quoted string in the response fields and copy its value (without quotes)
// to the specified character array (v).  Only up to maxlen characters are copied
// into the result buffer, so make sure to pass a large enough buffer to handle the
// response.
boolean Botletics_modem::parseReplyQuoted(FStringPtr toreply,
          char *v, int maxlen, char divider, uint8_t index) {
  uint8_t i=0, j;
  // Verify response starts with toreply.
  char *p = prog_char_strstr(replybuffer, (prog_char*)toreply);
  if (p == 0) return false;
  p+=prog_char_strlen((prog_char*)toreply);

  // Find location of desired response field.
  for (i=0; i<index;i++) {
    // increment dividers
    p = strchr(p, divider);
    if (!p) return false;
    p++;
  }

  // Copy characters from response field into result string.
  for(i=0, j=0; j<maxlen && i<strlen(p); ++i) {
    // Stop if a divier is found.
    if(p[i] == divider)
      break;
    // Skip any quotation marks.
    else if(p[i] == '"')
      continue;
    v[j++] = p[i];
  }

  // Add a null terminator if result string buffer was not filled.
  if (j < maxlen)
    v[j] = '\0';

  return true;
}

boolean Botletics_modem::sendParseReply(FStringPtr tosend,
              FStringPtr toreply,
              uint16_t *v, char divider, uint8_t index) {
  getReply(tosend);

  if (! parseReply(toreply, v, divider, index)) return false;

  readline(); // eat 'OK'

  return true;
}

boolean Botletics_modem::parseReplyFloat(FStringPtr toreply,
          float *f, char divider, uint8_t index) {
  char *p = prog_char_strstr(replybuffer, (prog_char*)toreply);  // get the pointer to the voltage
  if (p == 0) return false;
  p+=prog_char_strlen((prog_char*)toreply);
  //DEBUG_PRINTLN(p);
  for (uint8_t i=0; i<index;i++) {
    // increment dividers
    p = strchr(p, divider);
    if (!p) return false;
    p++;
    //DEBUG_PRINTLN(p);

  }
  *f = atof(p);

  return true;
}

// needed for CBC and others

boolean Botletics_modem::sendParseReplyFloat(FStringPtr tosend,
              FStringPtr toreply,
              float *f, char divider, uint8_t index) {
  getReply(tosend);

  if (! parseReplyFloat(toreply, f, divider, index)) return false;

  readline(); // eat 'OK'

  return true;
}






