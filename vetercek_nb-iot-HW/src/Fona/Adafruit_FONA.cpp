// Modified version of Adafruit FONA library for Botletics hardware
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

#include "Adafruit_FONA.h"




Adafruit_FONA::Adafruit_FONA(int8_t rst)
{
  _rstpin = rst;

  // apn = F("FONAnet");
  apn = F("");
  apnusername = 0;
  apnpassword = 0;
  mySerial = 0;
  httpsredirect = false;
  useragent = F("FONA");
  ok_reply = F("OK");
}

uint8_t Adafruit_FONA::type(void) {
  return _type;
}


boolean Adafruit_FONA::checkAT() {
	if (sendCheckReply(F("AT"), ok_reply),5000)
		return true;
return false;
}

boolean Adafruit_FONA::begin(Stream &port) {
  mySerial = &port;

//  if (_rstpin != 99) { // Pulse the reset pin only if it's not an LTE module
//    DEBUG_PRINTLN(F("Resetting the module..."));
//    pinMode(_rstpin, OUTPUT);
//    digitalWrite(_rstpin, HIGH);
//    delay(10);
//    digitalWrite(_rstpin, LOW);
//    delay(100);
//    digitalWrite(_rstpin, HIGH);
//  }

  DEBUG_PRINTLN(F("try SIM700"));
  // give 7 seconds to reboot
  int16_t timeout = 7000;

  while (timeout > 0) {
    while (mySerial->available()) mySerial->read();
    if (sendCheckReply(F("AT"), ok_reply))
      break;
    while (mySerial->available()) mySerial->read();
    if (sendCheckReply(F("AT"), F("AT"))) 
      break;
    delay(500);
    timeout-=500;
  }

  if (timeout <= 0) {
#ifdef ADAFRUIT_FONA_DEBUG
    DEBUG_PRINTLN(F("No response, last attempt"));
#endif
  //pinMode(10, OUTPUT);
  //digitalWrite(10, LOW);
  //delay(1200); // For SIM7000 
  //digitalWrite(10, HIGH);
  //delay(1000);    
    
    sendCheckReply(F("AT"), ok_reply);
    delay(100);
    sendCheckReply(F("AT"), ok_reply);
    delay(100);
    sendCheckReply(F("AT"), ok_reply);
    delay(100);
  }

  // turn off Echo!
  sendCheckReply(F("ATE0"), ok_reply);
  delay(100);

  if (! sendCheckReply(F("ATE0"), ok_reply)) {
    return false;
  }

  // turn on hangupitude
  if (_rstpin != 99) sendCheckReply(F("AT+CVHU=0"), ok_reply);

  delay(100);
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN("ATI");

  mySerial->println("ATI");
  readline(500, true);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);




  if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7000A")) != 0) {
    _type = SIM7000A;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7000C")) != 0) {
    _type = SIM7000C;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7000E")) != 0) {
    _type = SIM7000E;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7000G")) != 0) {
    _type = SIM7000G;
  } 



  return true;
}


/********* Serial port ********************************************/

boolean Adafruit_FONA_LTE::setBaudrate(uint16_t baud) {
  return sendCheckReply(F("AT+IPR="), baud, ok_reply);
}


/********* POWER, BATTERY & ADC ********************************************/

/* returns value in mV (uint16_t) */
boolean Adafruit_FONA::getBattVoltage(uint16_t *v) {
    return sendParseReply(F("AT+CBC"), F("+CBC: "), v, ',', 2);
}

/* powers down the SIM module */
boolean Adafruit_FONA::powerDown(void) {
    if (! sendCheckReply(F("AT+CPOWD=1"), F("NORMAL POWER DOWN"))) // Normal power off
        return false;
  }

/********* NETWORK AND WIRELESS CONNECTION SETTINGS ***********************/

// Uses the AT+CFUN command to set functionality (refer to AT+CFUN in manual)
// 0 --> Minimum functionality
// 1 --> Full functionality
// 4 --> Disable RF
// 5 --> Factory test mode
// 6 --> Restarts module
// 7 --> Offline mode
boolean Adafruit_FONA::setFunctionality(uint8_t option) {
  return sendCheckReply(F("AT+CFUN="), option, ok_reply);
}

// 2  - Automatic
// 13 - GSM only
// 38 - LTE only
// 51 - GSM and LTE only
boolean Adafruit_FONA_LTE::setPreferredMode(uint8_t mode) {
  return sendCheckReply(F("AT+CNMP="), mode, ok_reply);
}

// 1 - CAT-M
// 2 - NB-IoT
// 3 - CAT-M and NB-IoT
boolean Adafruit_FONA_LTE::setPreferredLTEMode(uint8_t mode) {
  return sendCheckReply(F("AT+CMNB="), mode, ok_reply);
}

// Useful for choosing a certain carrier only
// For example, AT&T uses band 12 in the US for LTE CAT-M
// whereas Verizon uses band 13
// Mode: "CAT-M" or "NB-IOT"
// Band: The cellular EUTRAN band number
boolean Adafruit_FONA_LTE::setOperatingBand(const char * mode, uint8_t band) {
  char cmdBuff[24];

  sprintf(cmdBuff, "AT+CBANDCFG=\"%s\",%i", mode, band);

  return sendCheckReply(cmdBuff, ok_reply);
}

boolean Adafruit_FONA_LTE::setNetwork(uint16_t net, uint8_t band) {
  char cmdBuff[24];

  sprintf(cmdBuff, "AT+COPS=1,2,\"%i\",%i",net,band);

  return sendCheckReply(cmdBuff, ok_reply, 10000);
}

boolean Adafruit_FONA_LTE::setCOPS( uint8_t band) {
  char cmdBuff[24];
  sprintf(cmdBuff, "AT+COPS=%i",band);
  return sendCheckReply(cmdBuff, ok_reply, 3500);
   }


// Sleep mode reduces power consumption significantly while remaining registered to the network
// NOTE: USB port must be disconnected before this will take effect
boolean Adafruit_FONA::enableSleepMode(bool onoff) {
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
boolean Adafruit_FONA::set_eDRX(uint8_t mode, uint8_t connType, char * eDRX_val) {
  if (strlen(eDRX_val) > 4) return false;

  char auxStr[21];

  sprintf(auxStr, "AT+CEDRXS=%i,%i,\"%s\"", mode, connType, eDRX_val);

  return sendCheckReply(auxStr, ok_reply);
}




// Enable, disable, or set the blinking frequency of the network status LED
// Default settings are the following:
// Not connected to network --> 1,64,800
// Connected to network     --> 2,64,3000
// Data connection enabled  --> 3,64,300
boolean Adafruit_FONA::setNetLED(bool onoff, uint8_t mode, uint16_t timer_on, uint16_t timer_off) {
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




/********* IMEI **********************************************************/

uint8_t Adafruit_FONA::getIMEI(char *imei) {
  getReply(F("AT+GSN"));

  // up to 15 chars
  strncpy(imei, replybuffer, 15);
  imei[15] = 0;

  readline(); // eat 'OK'

  return strlen(imei);
}

/********* NETWORK *******************************************************/

uint8_t Adafruit_FONA::getNetworkStatus(void) {
  uint16_t status;

  if (_type >= SIM7000A) {
    if (! sendParseReply(F("AT+CGREG?"), F("+CGREG: "), &status, ',', 1)) return 0;
  }
  else {
    if (! sendParseReply(F("AT+CREG?"), F("+CREG: "), &status, ',', 1)) return 0;
  }

  return status;
}


uint8_t Adafruit_FONA::getRSSI(void) {
  uint16_t reply;

  if (! sendParseReply(F("AT+CSQ"), F("+CSQ: "), &reply) ) return 0;

  return reply;
}



/********* TIME **********************************************************/






/********* GPRS **********************************************************/


boolean Adafruit_FONA::enableGPRS() {
	// DISCONNECT
      // disconnect all sockets
      sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 2000);
      // close bearer
      sendCheckReply(F("AT+SAPBR=0,1"), ok_reply, 1000);
		
		sendCheckReply(F("AT+CGATT=0"), ok_reply, 10000);

	// CONNECT
      // set bearer profile! connection type GPRS
      if (! sendCheckReply(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""), ok_reply, 10000))
        return false;
      // } // UNCOMMENT FOR LTE ONLY!

      delay(200); // This seems to help the next line run the first time
      
      // set bearer profile access point name
      if (apn) {
        // Send command AT+SAPBR=3,1,"APN","<apn value>" where <apn value> is the configured APN value.
        if (! sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"APN\","), apn, ok_reply, 10000))
          return false;

        // if (_type < SIM7000A) { // UNCOMMENT FOR LTE ONLY!
          // send AT+CSTT,"apn","user","pass"
          flushInput();

          mySerial->print(F("AT+CSTT=\""));
          mySerial->print(apn);
          if (apnusername) {
            mySerial->print("\",\"");
            mySerial->print(apnusername);
          }
          if (apnpassword) {
            mySerial->print("\",\"");
            mySerial->print(apnpassword);
          }
          mySerial->println("\"");

          DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+CSTT=\""));
          DEBUG_PRINT(apn);
          
          if (apnusername) {
            DEBUG_PRINT("\",\"");
            DEBUG_PRINT(apnusername); 
          }
          if (apnpassword) {
            DEBUG_PRINT("\",\"");
            DEBUG_PRINT(apnpassword); 
          }
          DEBUG_PRINTLN("\"");
          
          if (! expectReply(ok_reply)) return false;
        // } // UNCOMMENT FOR LTE ONLY!
      
        // set username/password
        if (apnusername) {
          // Send command AT+SAPBR=3,1,"USER","<user>" where <user> is the configured APN username.
          if (! sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"USER\","), apnusername, ok_reply, 10000))
            return false;
        }
        if (apnpassword) {
          // Send command AT+SAPBR=3,1,"PWD","<password>" where <password> is the configured APN password.
          if (! sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"PWD\","), apnpassword, ok_reply, 10000))
            return false;
        }
      }

      // open bearer
      if (! sendCheckReply(F("AT+SAPBR=1,1"), ok_reply, 30000))
        return false;

      // if (_type < SIM7000A) { // UNCOMMENT FOR LTE ONLY!
        // bring up wireless connection
        if (! sendCheckReply(F("AT+CIICR"), ok_reply, 10000))
          return false;
        //if (! sendCheckReply(F("AT+CIFSR"), ok_reply, 10000)) //jaka - gets ip not ok status - parse reply
          //return false;          
          
  return true;
}

 

int8_t Adafruit_FONA::getNetworkInfo(void) {
  uint16_t state;
  sendParseReply(F("AT+COPS?"), F("COPS: "), &state, ',', 3);
  DEBUG_PRINT("cops: "); DEBUG_PRINTLN(state); // Print out server reply
  return state;
  //getReply(F("AT+COPS?"));
  //DEBUG_PRINT("cops: "); DEBUG_PRINTLN(replybuffer); // Print out server reply
}

int8_t Adafruit_FONA::GPRSstate(void) {
  uint16_t state;

  if (! sendParseReply(F("AT+CGATT?"), F("+CGATT: "), &state) )
    return -1;
    
   //if (! sendCheckReply(F("AT+CIFSR"), ok_reply, 5000)) //jaka
   //return -1;     

  return state;
}

int8_t Adafruit_FONA::GPRSPDP(void) {
  uint16_t state;

  if (! sendParseReply(F("AT+CGDCONT?"), F("+CGDCONT: "), &state) )
    return -1;
    
   //if (! sendCheckReply(F("AT+CIFSR"), ok_reply, 5000)) //jaka
   //return -1;     

  return state;
}

void Adafruit_FONA::setNetworkSettings(FONAFlashStringPtr apn,
              FONAFlashStringPtr username, FONAFlashStringPtr password) {
  this->apn = apn;
  this->apnusername = username;
  this->apnpassword = password;

  if (_type >= SIM7000A) sendCheckReplyQuoted(F("AT+CGDCONT=1,\"IP\","), apn, ok_reply, 10000);
}



/********* UDP FUNCTIONS  ************************************/


boolean Adafruit_FONA::UDPconnect(char *server, uint16_t port) {
  flushInput();
  char buffer[50];
  sprintf(buffer, "AT+CIPSTART=\"UDP\",\"%s\",\"%u\"", server, port);

  // close all old connections
  if (! sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 5000) ) return false;

  // single connection at a time
  if (! sendCheckReply(F("AT+CIPMUX=0"), ok_reply) ) return false;

  // manually read data set 1
  if (! sendCheckReply(F("AT+CIPRXGET=0"), ok_reply) ) return false;  //jaka

  // keep alvie
  //if (! sendCheckReply(F("AT+CIPTKA=1,7200,600,9"), ok_reply) ) return false;

  // mySerial->print(F("AT+CIPSTART=\"UDP\",\""));
  // mySerial->print(server);
  // mySerial->print(F("\",\""));
  // mySerial->print(port);
  // mySerial->println(F("\""));

  mySerial->println(buffer);

  if (! expectReply(ok_reply)) return false;
  if (! expectReply(F("CONNECT OK"))) return false;  //if ALREADY CONNECT

  // looks like it was a success (?)
  return true;
}

boolean Adafruit_FONA::UDPclose(void) {
      if (! sendCheckReply(F("AT+CIPCLOSE"),  F("CLOSE OK"), 3000))
        return false;
      if (! sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 3000))
        return false;	
  return true;
}

uint8_t Adafruit_FONA::UDPconnected(void) {
  if (! sendCheckReply(F("AT+CIPSTATUS"), ok_reply, 200) ) return 99;
  readline(200);
  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);
  byte res;

  if (strcmp(replybuffer, "STATE: IP INITIAL") == 0) { res=2;}
  //else if (strcmp(replybuffer, "STATE: IP START") == 0) { res=3;}
  //else if (strcmp(replybuffer, "STATE: IP CONFIG") == 0) { res=4;}
  //else if (strcmp(replybuffer, "STATE: IP GPRSACT") == 0) { res=5;}
  //else if (strcmp(replybuffer, "STATE: UDP STATUS") == 0) { res=6;}
  //else if (strcmp(replybuffer, "STATE: UDP CONNECTING") == 0) { res=7;}
  //else if (strcmp(replybuffer, "STATE: SERVER LISTENING") == 0) { res=8;;}
  //else if (strcmp(replybuffer, "STATE: UDP CLOSING") == 0) { res=11;}
  else if (strcmp(replybuffer, "STATE: UDP CLOSED") == 0) { res=12;}
  else if (strcmp(replybuffer, "STATE: PDP DEACT") == 0) { res=10;}
  else if (strcmp(replybuffer, "STATE: CONNECT OK") == 0) { res=1;}
  else { res=0;}
  
    DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(res);

  return res;
}

boolean Adafruit_FONA::UDPsend(unsigned char *packet, uint8_t len, byte response[10],uint8_t charr) {
     char buffer[20];
  sprintf(buffer, "AT+CIPSEND=%u", len);
  mySerial->println(buffer);
  DEBUG_PRINTLN(buffer);

  readline();

  if (replybuffer[0] != '>') return false;

  mySerial->write(packet, len);

uint8_t sendD = readline(5000); // return SEND OK
  DEBUG_PRINT(F("\t<--s ")); DEBUG_PRINTLN(replybuffer);
if (strcmp(replybuffer, "SEND OK") != 0) { return false;}
uint8_t receveD = readline2(5000,charr); // RETURN DATA


	DEBUG_PRINTLN("response :");   
     for (uint16_t i=0; i<charr;i++) {
		 
		if (replybuffer2[i]==128) {	 
		response[i]=0;
		DEBUG_PRINTLN(0);		
		}

		else {	 
		response[i]=replybuffer2[i];
		DEBUG_PRINTLN(replybuffer2[i]);		
		}
		

	}
	//DEBUG_PRINTLN(replybuffer2[0]);
  if (response[0] > 0 and response[1] < 4) return true;

  else return false;
}

uint16_t Adafruit_FONA::UDPavailable(void) {
  uint16_t avail;

  if (! sendParseReply(F("AT+CIPRXGET=4"), F("+CIPRXGET: 4,"), &avail, ',', 0) ) return false;


  DEBUG_PRINT (avail); DEBUG_PRINTLN(F(" bytes available"));


  return avail;
}





/********* HELPERS *********************************************/

boolean Adafruit_FONA::expectReply(FONAFlashStringPtr reply,
                                   uint16_t timeout) {
  readline(timeout);

  DEBUG_PRINT(F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
}

/********* LOW LEVEL *******************************************/

inline int Adafruit_FONA::available(void) {
  return mySerial->available();
}

inline size_t Adafruit_FONA::write(uint8_t x) {
  return mySerial->write(x);
}

inline int Adafruit_FONA::read(void) {
  return mySerial->read();
}

inline int Adafruit_FONA::peek(void) {
  return mySerial->peek();
}

inline void Adafruit_FONA::flush() {
  mySerial->flush();
}

void Adafruit_FONA::flushInput() {
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

uint16_t Adafruit_FONA::readRaw(uint16_t b) {
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

uint8_t Adafruit_FONA::readline(uint16_t timeout, boolean multiline) {
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
      replyidx++;
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

uint8_t Adafruit_FONA::readline2(uint16_t timeout, uint8_t characters) {
  uint16_t replyidx = 0;

  while (timeout--) {
    if (replyidx >= characters) {
      break;
    }

    while(mySerial->available() ) {
      int c =  mySerial->read();
      replybuffer2[replyidx] = c;
      replyidx++;
     if (replyidx == characters) {
      break;
		}
    }

    if (timeout == 0) {
      break;
    }
    delay(1);
  }
  replybuffer2[replyidx] = 0;  // null term
  return replyidx;
}

uint8_t Adafruit_FONA::getReply(const char *send, uint16_t timeout) {
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(send);


  mySerial->println(send);

  uint8_t l = readline(timeout);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return l;
}

uint8_t Adafruit_FONA::getReply(FONAFlashStringPtr send, uint16_t timeout) {
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(send);


  mySerial->println(send);

  uint8_t l = readline(timeout);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return l;
}

// Send prefix, suffix, and newline. Return response (and also set replybuffer with response).
uint8_t Adafruit_FONA::getReply(FONAFlashStringPtr prefix, char *suffix, uint16_t timeout) {
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(prefix); DEBUG_PRINTLN(suffix);


  mySerial->print(prefix);
  mySerial->println(suffix);

  uint8_t l = readline(timeout);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return l;
}

// Send prefix, suffix, and newline. Return response (and also set replybuffer with response).
uint8_t Adafruit_FONA::getReply(FONAFlashStringPtr prefix, int32_t suffix, uint16_t timeout) {
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(prefix); DEBUG_PRINTLN(suffix, DEC);


  mySerial->print(prefix);
  mySerial->println(suffix, DEC);

  uint8_t l = readline(timeout);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return l;
}

// Send prefix, suffix, suffix2, and newline. Return response (and also set replybuffer with response).
uint8_t Adafruit_FONA::getReply(FONAFlashStringPtr prefix, int32_t suffix1, int32_t suffix2, uint16_t timeout) {
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
uint8_t Adafruit_FONA::getReplyQuoted(FONAFlashStringPtr prefix, FONAFlashStringPtr suffix, uint16_t timeout) {
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

boolean Adafruit_FONA::sendCheckReply(const char *send, const char *reply, uint16_t timeout) {
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

boolean Adafruit_FONA::sendCheckReply(FONAFlashStringPtr send, FONAFlashStringPtr reply, uint16_t timeout) {
  if (! getReply(send, timeout) )
    return false;

  return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
}

boolean Adafruit_FONA::sendCheckReply(const char* send, FONAFlashStringPtr reply, uint16_t timeout) {
  if (! getReply(send, timeout) )
    return false;
  return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
}


// Send prefix, suffix, and newline.  Verify FONA response matches reply parameter.
boolean Adafruit_FONA::sendCheckReply(FONAFlashStringPtr prefix, char *suffix, FONAFlashStringPtr reply, uint16_t timeout) {
  getReply(prefix, suffix, timeout);
  return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
}

// Send prefix, suffix, and newline.  Verify FONA response matches reply parameter.
boolean Adafruit_FONA::sendCheckReply(FONAFlashStringPtr prefix, int32_t suffix, FONAFlashStringPtr reply, uint16_t timeout) {
  getReply(prefix, suffix, timeout);
  return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
}

// Send prefix, suffix, suffix2, and newline.  Verify FONA response matches reply parameter.
boolean Adafruit_FONA::sendCheckReply(FONAFlashStringPtr prefix, int32_t suffix1, int32_t suffix2, FONAFlashStringPtr reply, uint16_t timeout) {
  getReply(prefix, suffix1, suffix2, timeout);
  return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
}

// Send prefix, ", suffix, ", and newline.  Verify FONA response matches reply parameter.
boolean Adafruit_FONA::sendCheckReplyQuoted(FONAFlashStringPtr prefix, FONAFlashStringPtr suffix, FONAFlashStringPtr reply, uint16_t timeout) {
  getReplyQuoted(prefix, suffix, timeout);
  return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
}


boolean Adafruit_FONA::parseReply(FONAFlashStringPtr toreply,
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

boolean Adafruit_FONA::parseReply(FONAFlashStringPtr toreply,
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
boolean Adafruit_FONA::parseReplyQuoted(FONAFlashStringPtr toreply,
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

boolean Adafruit_FONA::sendParseReply(FONAFlashStringPtr tosend,
              FONAFlashStringPtr toreply,
              uint16_t *v, char divider, uint8_t index) {
  getReply(tosend);

  if (! parseReply(toreply, v, divider, index)) return false;

  readline(); // eat 'OK'

  return true;
}

boolean Adafruit_FONA::parseReplyFloat(FONAFlashStringPtr toreply,
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

boolean Adafruit_FONA::sendParseReplyFloat(FONAFlashStringPtr tosend,
              FONAFlashStringPtr toreply,
              float *f, char divider, uint8_t index) {
  getReply(tosend);

  if (! parseReplyFloat(toreply, f, divider, index)) return false;

  readline(); // eat 'OK'

  return true;
}


// needed for CBC and others




