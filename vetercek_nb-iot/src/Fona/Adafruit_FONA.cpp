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

  if (_rstpin != 99) { // Pulse the reset pin only if it's not an LTE module
    DEBUG_PRINTLN(F("Resetting the module..."));
    pinMode(_rstpin, OUTPUT);
    digitalWrite(_rstpin, HIGH);
    delay(10);
    digitalWrite(_rstpin, LOW);
    delay(100);
    digitalWrite(_rstpin, HIGH);
  }

  DEBUG_PRINTLN(F("Attempting to open comm with ATs"));
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
    DEBUG_PRINTLN(F("Timeout: No response to AT... last ditch attempt."));
#endif
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



  if (prog_char_strstr(replybuffer, (prog_char *)F("SIM808 R14")) != 0) {
    _type = SIM808_V2;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM808 R13")) != 0) {
    _type = SIM808_V1;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM800 R13")) != 0) {
    _type = SIM800L;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIMCOM_SIM5320A")) != 0) {
    _type = SIM5320A;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIMCOM_SIM5320E")) != 0) {
    _type = SIM5320E;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7000A")) != 0) {
    _type = SIM7000A;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7000C")) != 0) {
    _type = SIM7000C;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7000E")) != 0) {
    _type = SIM7000E;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7000G")) != 0) {
    _type = SIM7000G;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7500A")) != 0) {
    _type = SIM7500A;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7500E")) != 0) {
    _type = SIM7500E;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7600A")) != 0) {
    _type = SIM7600A;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7600C")) != 0) {
    _type = SIM7600C;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM7600E")) != 0) {
    _type = SIM7600E;
  }


  if (_type == SIM800L) {
    // determine if L or H

  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN("AT+GMM");

    mySerial->println("AT+GMM");
    readline(500, true);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);


    if (prog_char_strstr(replybuffer, (prog_char *)F("SIM800H")) != 0) {
      _type = SIM800H;
    }
  }


  return true;
}


/********* Serial port ********************************************/
boolean Adafruit_FONA::setBaudrate(uint16_t baud) {
  return sendCheckReply(F("AT+IPREX="), baud, ok_reply);
}

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
  if (_type == SIM7500A || _type == SIM7500E) {
    if (! sendCheckReply(F("AT+CPOF"), ok_reply))
      return false;
  }
  else {
    if (! sendCheckReply(F("AT+CPOWD=1"), F("NORMAL POWER DOWN"))) // Normal power off
        return false;
  }
  

  return true;
}




/* returns the percentage charge of battery as reported by sim800 */
boolean Adafruit_FONA::getBattPercent(uint16_t *p) {
  return sendParseReply(F("AT+CBC"), F("+CBC: "), p, ',', 1);
}

boolean Adafruit_FONA::getADCVoltage(uint16_t *v) {
  return sendParseReply(F("AT+CADC?"), F("+CADC: 1,"), v);
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

// NOTE: Network must support PSM and modem needs to restart before it takes effect
boolean Adafruit_FONA::enablePSM(bool onoff) {
  return sendCheckReply(F("AT+CPSMS="), onoff, ok_reply);
}
// Set PSM with custom TAU and active time
// For both TAU and Active time, leftmost 3 bits represent the multiplier and rightmost 5 bits represent the value in bits.

// For TAU, left 3 bits:
// 000 10min
// 001 1hr
// 010 10hr
// 011 2s
// 100 30s
// 101 1min
// For Active time, left 3 bits:
// 000 2s
// 001 1min
// 010 6min
// 111 disabled

// Note: Network decides the final value of the TAU and active time. 
boolean Adafruit_FONA::enablePSM(bool onoff, char * TAU_val, char * activeTime_val) { // AT+CPSMS command
    if (strlen(activeTime_val) > 8) return false;
    if (strlen(TAU_val) > 8) return false;

    char auxStr[35];
    sprintf(auxStr, "AT+CPSMS=%i,,,\"%s\",\"%s\"", onoff, TAU_val, activeTime_val);

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

// Open or close wireless data connection
boolean Adafruit_FONA_LTE::openWirelessConnection(bool onoff) {
  if (!onoff) return sendCheckReply(F("AT+CNACT=0"), ok_reply); // Disconnect wireless
  else {
    getReplyQuoted(F("AT+CNACT=1,"), apn);
    readline(); // Eat 'OK'

    if (strcmp(replybuffer, "+APP PDP: ACTIVE") == 0) return true;
    else return false;
  }
}

// Query wireless connection status
boolean Adafruit_FONA_LTE::wirelessConnStatus(void) {
  getReply(F("AT+CNACT?"));
  // Format of response:
  // +CNACT: <status>,<ip_addr>
  if (strstr(replybuffer, "+CNACT: 1") == NULL) return false;
  return true;
}

/********* SIM ***********************************************************/

uint8_t Adafruit_FONA::unlockSIM(char *pin)
{
  char sendbuff[14] = "AT+CPIN=";
  sendbuff[8] = pin[0];
  sendbuff[9] = pin[1];
  sendbuff[10] = pin[2];
  sendbuff[11] = pin[3];
  sendbuff[12] = '\0';

  return sendCheckReply(sendbuff, ok_reply);
}

uint8_t Adafruit_FONA::getSIMCCID(char *ccid) {
  getReply(F("AT+CCID"));
  // up to 28 chars for reply, 20 char total ccid
  if (replybuffer[0] == '+') {
    // fona 3g?
    strncpy(ccid, replybuffer+8, 20);
  } else {
    // fona 800 or 800
    strncpy(ccid, replybuffer, 20);
  }
  ccid[20] = 0;

  readline(); // eat 'OK'

  return strlen(ccid);
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

boolean Adafruit_FONA::enableNTPTimeSync(boolean onoff, FONAFlashStringPtr ntpserver) {
  if (onoff) {
    if (! sendCheckReply(F("AT+CNTPCID=1"), ok_reply))
      return false;

    mySerial->print(F("AT+CNTP=\""));
    if (ntpserver != 0) {
      mySerial->print(ntpserver);
    } else {
      mySerial->print(F("pool.ntp.org"));
    }
    mySerial->println(F("\",0"));
    readline(FONA_DEFAULT_TIMEOUT_MS);
    if (strcmp(replybuffer, "OK") != 0)
      return false;

    if (! sendCheckReply(F("AT+CNTP"), ok_reply, 10000))
      return false;

    uint16_t status;
    readline(10000);
    if (! parseReply(F("+CNTP:"), &status))
      return false;
  } else {
    if (! sendCheckReply(F("AT+CNTPCID=0"), ok_reply))
      return false;
  }

  return true;
}

boolean Adafruit_FONA::getTime(char *buff, uint16_t maxlen) {
  getReply(F("AT+CCLK?"), (uint16_t) 10000);
  if (strncmp(replybuffer, "+CCLK: ", 7) != 0)
    return false;

  char *p = replybuffer+7;
  uint16_t lentocopy = min(maxlen-1, (int)strlen(p));
  strncpy(buff, p, lentocopy+1);
  buff[lentocopy] = 0;

  readline(); // eat OK

  return true;
}

/********* Real Time Clock ********************************************/

boolean Adafruit_FONA::readRTC(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *hr, uint8_t *min, uint8_t *sec, int8_t *tz) {
  getReply(F("AT+CCLK?"), (uint16_t) 10000); //Get RTC timeout 10 sec
  if (strncmp(replybuffer, "+CCLK: ", 7) != 0)
    return false;

  char *p = replybuffer+8;   // skip +CCLK: "
  // Parse date
  int reply = atoi(p);     // get year
  *year = (uint8_t) reply;   // save as year
  p+=3;              // skip 3 char
  reply = atoi(p);
  *month = (uint8_t) reply;
  p+=3;
  reply = atoi(p);
  *date = (uint8_t) reply;
  p+=3;
  reply = atoi(p);
  *hr = (uint8_t) reply;
  p+=3;
  reply = atoi(p);
  *min = (uint8_t) reply;
  p+=3;
  reply = atoi(p);
  *sec = (uint8_t) reply;
  p+=3;
  reply = atoi(p);
  *tz = reply;

  readline(); // eat OK

  return true;
}

boolean Adafruit_FONA::enableRTC(uint8_t i) {
  if (! sendCheckReply(F("AT+CLTS="), i, ok_reply))
    return false;
  return sendCheckReply(F("AT&W"), ok_reply);
}


/********* GPRS **********************************************************/


boolean Adafruit_FONA::enableGPRS(boolean onoff) {
  if (_type == SIM5320A || _type == SIM5320E || _type == SIM7500A || _type == SIM7500E || _type == SIM7600A || _type == SIM7600C || _type == SIM7600E) {
    if (onoff) {
      // disconnect all sockets
      sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 5000);

      if (! sendCheckReply(F("AT+CGATT=1"), ok_reply, 10000))
        return false;


      // set bearer profile access point name
      if (apn) {
        // Send command AT+CGSOCKCONT=1,"IP","<apn value>" where <apn value> is the configured APN name.
        if (! sendCheckReplyQuoted(F("AT+CGSOCKCONT=1,\"IP\","), apn, ok_reply, 10000))
          return false;

        // set username/password
        if (apnusername) {
          char authstring[100] = "AT+CGAUTH=1,1,\"";
          // char authstring[100] = "AT+CSOCKAUTH=1,1,\""; // For 3G
          char *strp = authstring + strlen(authstring);
          prog_char_strcpy(strp, (prog_char *)apnusername);
          strp+=prog_char_strlen((prog_char *)apnusername);
          strp[0] = '\"';
          strp++;
          strp[0] = 0;

          if (apnpassword) {
            strp[0] = ','; strp++;
            strp[0] = '\"'; strp++;
            prog_char_strcpy(strp, (prog_char *)apnpassword);
            strp+=prog_char_strlen((prog_char *)apnpassword);
            strp[0] = '\"';
            strp++;
            strp[0] = 0;
          }

          if (! sendCheckReply(authstring, ok_reply, 10000))
            return false;
        }
      }

      // connect in transparent mode
      if (! sendCheckReply(F("AT+CIPMODE=0"), ok_reply, 10000))  
        return false;
      // open network
      if (_type == SIM5320A || _type == SIM5320E) {
        if (! sendCheckReply(F("AT+NETOPEN=,,1"), F("Network opened"), 10000))
          return false;
      }
      else if (_type == SIM7500A || _type == SIM7500E || _type == SIM7600A || _type == SIM7600C || _type == SIM7600E) {
        if (! sendCheckReply(F("AT+NETOPEN"), ok_reply, 10000))
          return false;
      }
      readline(); // eat 'OK'
    } else {
      // close GPRS context
      if (_type == SIM5320A || _type == SIM5320E) {
        if (! sendCheckReply(F("AT+NETCLOSE"), F("Network closed"), 10000))
          return false;
      }
      else if (_type == SIM7500A || _type == SIM7500E || _type == SIM7600A || _type == SIM7600C || _type == SIM7600E) {
        getReply(F("AT+NETCLOSE"));
        getReply(F("AT+CHTTPSSTOP"));
        // getReply(F("AT+CHTTPSCLSE"));

        // if (! sendCheckReply(F("AT+NETCLOSE"), ok_reply, 10000))
       //   return false;
      //    if (! sendCheckReply(F("AT+CHTTPSSTOP"), F("+CHTTPSSTOP: 0"), 10000))
        //  return false;
        // if (! sendCheckReply(F("AT+CHTTPSCLSE"), ok_reply, 10000))
        //  return false;
      }

      readline(); // eat 'OK'
    }
  }
  else {
    if (onoff) {
      // if (_type < SIM7000A) { // UNCOMMENT FOR LTE ONLY!
        // disconnect all sockets
        sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 20000);

        if (! sendCheckReply(F("AT+CIPMODE=0"), ok_reply, 10000)) //jaka
          return false;

        //if (! sendCheckReply(F("AT+CGATT=1"), ok_reply, 10000))  //jaka
          //return false;

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
          
      // } // UNCOMMENT FOR LTE ONLY!

    } else {
      // disconnect all sockets
      if (! sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 20000))
        return false;

      // close bearer
      if (! sendCheckReply(F("AT+SAPBR=0,1"), ok_reply, 10000))
        return false;

      // if (_type < SIM7000A) { // UNCOMMENT FOR LTE ONLY!
        if (! sendCheckReply(F("AT+CGATT=0"), ok_reply, 10000))
          return false;
    // } // UNCOMMENT FOR LTE ONLY!

    }
  }
  return true;
}

 

void Adafruit_FONA::getNetworkInfo(void) {
  getReply(F("AT+CPSI?"));
  getReply(F("AT+COPS?"));
}

int8_t Adafruit_FONA::GPRSstate(void) {
  uint16_t state;

  if (! sendParseReply(F("AT+CGATT?"), F("+CGATT: "), &state) )
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





boolean Adafruit_FONA::postData(const char *URL, char *response) {
  // NOTE: Need to open socket/enable GPRS before using this function
  // char auxStr[64];

  // Make sure HTTP service is terminated so initialization will run
  sendCheckReply(F("AT+HTTPTERM"), ok_reply, 10000);

  // Initialize HTTP service
  if (! sendCheckReply(F("AT+HTTPINIT"), ok_reply, 10000))
    return false;

  // Set HTTP parameters
  if (! sendCheckReply(F("AT+HTTPPARA=\"CID\",1"), ok_reply, 10000))
    return false;

  // Specify URL
  char urlBuff[strlen(URL) + 22];

  sprintf(urlBuff, "AT+HTTPPARA=\"URL\",\"%s\"", URL);

  if (! sendCheckReply(urlBuff, ok_reply, 10000))
    return false;

    if (! sendCheckReply(F("AT+HTTPACTION=0"), ok_reply, 10000))
      return false;


  // Parse response status and size
  uint16_t status, datalen;
  readline(10000);
  if (! parseReply(F("+HTTPACTION:"), &status, ',', 1))
    return false;
  if (! parseReply(F("+HTTPACTION:"), &datalen, ',', 2))
    return false;

  DEBUG_PRINT("HTTP status: "); DEBUG_PRINTLN(status);
  DEBUG_PRINT("Data length: "); DEBUG_PRINTLN(datalen);

  if (status != 200) return false;

  getReply(F("AT+HTTPREAD"));

  readline(10000);
  DEBUG_PRINT("\t<--- "); DEBUG_PRINTLN(replybuffer); // Print out server reply

   strcpy(response, replybuffer);

  // Terminate HTTP service
  sendCheckReply(F("AT+HTTPTERM"), ok_reply, 10000);

  return true;
}



/********* SIM7000 MQTT FUNCTIONS  ************************************/
// Set MQTT parameters
// Parameter tags can be "CLIENTID", "URL", "KEEPTIME", "CLEANSS", "USERNAME",
// "PASSWORD", "QOS", "TOPIC", "MESSAGE", or "RETAIN"
boolean Adafruit_FONA_LTE::MQTT_setParameter(const char* paramTag, const char* paramValue, uint16_t port) {
  char cmdStr[50];

  if (strcmp(paramTag, "CLIENTID") == 0 || strcmp(paramTag, "URL") == 0 || strcmp(paramTag, "TOPIC") == 0 || strcmp(paramTag, "MESSAGE") == 0) {
    if (port == 0) sprintf(cmdStr, "AT+SMCONF=\"%s\",\"%s\"", paramTag, paramValue); // Quoted paramValue
    else sprintf(cmdStr, "AT+SMCONF=\"%s\",\"%s\",\"%i\"", paramTag, paramValue, port);
    if (! sendCheckReply(cmdStr, ok_reply)) return false;
  }
  else {
    sprintf(cmdStr, "AT+SMCONF=\"%s\",%s", paramTag, paramValue); // Unquoted paramValue
    if (! sendCheckReply(cmdStr, ok_reply)) return false;
  }
  
  return true;
}

// Connect or disconnect MQTT
boolean Adafruit_FONA_LTE::MQTT_connect(bool yesno) {
  if (yesno) return sendCheckReply(F("AT+SMCONN"), ok_reply, 5000);
  else return sendCheckReply(F("AT+SMDISC"), ok_reply);
}

// Query MQTT connection status
boolean Adafruit_FONA_LTE::MQTT_connectionStatus(void) {
  if (! sendCheckReply(F("AT+SMSTATE?"), F("+SMSTATE: 1"))) return false;
  return true;
}

// Subscribe to specified MQTT topic
// QoS can be from 0-2
boolean Adafruit_FONA_LTE::MQTT_subscribe(const char* topic, byte QoS) {
  char cmdStr[32];
  sprintf(cmdStr, "AT+SMSUB=\"%s\",%i", topic, QoS);

  if (! sendCheckReply(cmdStr, ok_reply)) return false;
  return true;
}

// Unsubscribe from specified MQTT topic
boolean Adafruit_FONA_LTE::MQTT_unsubscribe(const char* topic) {
  char cmdStr[32];
  sprintf(cmdStr, "AT+SMUNSUB=\"%s\"", topic);
  if (! sendCheckReply(cmdStr, ok_reply)) return false;
  return true;
}

// Publish to specified topic
// Message length can be from 0-512 bytes
// QoS can be from 0-2
// Server hold message flag can be 0 or 1
boolean Adafruit_FONA_LTE::MQTT_publish(const char* topic, const char* message, uint16_t contentLength, byte QoS, byte retain) {
  char cmdStr[40];
  sprintf(cmdStr, "AT+SMPUB=\"%s\",%i,%i,%i", topic, contentLength, QoS, retain);

  getReply(cmdStr, 300);
  if (strstr(replybuffer, ">") == NULL) return false; // Wait for "> " to send message
  if (! sendCheckReply(message, ok_reply, 5000)) return false; // Now send the message

  return true;
}

// Change MQTT data format to hex
// Enter "true" if you want hex, "false" if you don't
boolean Adafruit_FONA_LTE::MQTT_dataFormatHex(bool yesno) {
  if (yesno) sendCheckReply(F("AT+SMPUBHEX="), yesno, ok_reply);
}


/********* UDP FUNCTIONS  ************************************/


boolean Adafruit_FONA::UDPconnect(char *server, uint16_t port) {
  flushInput();


  // close all old connections
  if (! sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 5000) ) return false;

  // single connection at a time
  if (! sendCheckReply(F("AT+CIPMUX=0"), ok_reply) ) return false;

  // manually read data set 1
  if (! sendCheckReply(F("AT+CIPRXGET=0"), ok_reply) ) return false;  //jaka

  // keep alvie
  //if (! sendCheckReply(F("AT+CIPTKA=1,7200,600,9"), ok_reply) ) return false;


  DEBUG_PRINT(F("AT+CIPSTART=\"UDP\",\""));
  DEBUG_PRINT(server);
  DEBUG_PRINT(F("\",\""));
  DEBUG_PRINT(port);
  DEBUG_PRINTLN(F("\""));


  mySerial->print(F("AT+CIPSTART=\"UDP\",\""));
  mySerial->print(server);
  mySerial->print(F("\",\""));
  mySerial->print(port);
  mySerial->println(F("\""));

  if (! expectReply(ok_reply)) return false;
  if (! expectReply(F("CONNECT OK"))) return false;

  // looks like it was a success (?)
  return true;
}

boolean Adafruit_FONA::UDPclose(void) {
      if (! sendCheckReply(F("AT+CIPCLOSE"),  F("CLOSE OK"), 3000))
        return false;
      //if (! sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 3000))
        //return false;	
  return true;
}

uint8_t Adafruit_FONA::UDPconnected(void) {
  if (! sendCheckReply(F("AT+CIPSTATUS"), ok_reply, 200) ) return false;
  readline(200);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  //return (strcmp(replybuffer, "STATE: CONNECT OK") == 0);
  
  if (strcmp(replybuffer, "STATE: CONNECT OK") == 0) { return 1;}
  else if (strcmp(replybuffer, "STATE:  UDP CLOSED") == 0) { return 5;}
  else if (strcmp(replybuffer, "STATE: PDP DEACT:") == 0) { return 10;}
  else { return 0;}
}

boolean Adafruit_FONA::UDPsend(unsigned char *packet, uint8_t len, byte response[10],uint8_t charr) {

  DEBUG_PRINT(F("AT+CIPSEND="));
  DEBUG_PRINTLN(len);
#ifdef ADAFRUIT_FONA_DEBUG
  for (uint16_t i=0; i<len; i++) {
  //DEBUG_PRINT(F(" 0x"));
  DEBUG_PRINT(packet[i], HEX);
  }
#endif
  DEBUG_PRINTLN();


  mySerial->print(F("AT+CIPSEND="));
  mySerial->println(len);
  readline();

  if (replybuffer[0] != '>') return false;

  mySerial->write(packet, len);

  readline(5000); // return SEND OK
  readline2(5000,charr); // RETURN DATA

	DEBUG_PRINTLN("response :");   
     for (uint16_t i=0; i<charr;i++) {
		response[i]=replybuffer2[i];
		DEBUG_PRINTLN(replybuffer2[i]);
	}
	//DEBUG_PRINTLN(replybuffer2[0]);
  if (replybuffer2[0] > 0 and replybuffer2[1] < 2) return true;

  else return false;
}

uint16_t Adafruit_FONA::UDPavailable(void) {
  uint16_t avail;

  if (! sendParseReply(F("AT+CIPRXGET=4"), F("+CIPRXGET: 4,"), &avail, ',', 0) ) return false;


  DEBUG_PRINT (avail); DEBUG_PRINTLN(F(" bytes available"));


  return avail;
}





/********* HTTP LOW LEVEL FUNCTIONS  ************************************/

boolean Adafruit_FONA::HTTP_init() {
  return sendCheckReply(F("AT+HTTPINIT"), ok_reply);
}

boolean Adafruit_FONA::HTTP_term() {
  return sendCheckReply(F("AT+HTTPTERM"), ok_reply);
}

void Adafruit_FONA::HTTP_para_start(FONAFlashStringPtr parameter,
                                    boolean quoted) {
  flushInput();


  DEBUG_PRINT(F("\t---> "));
  DEBUG_PRINT(F("AT+HTTPPARA=\""));
  DEBUG_PRINT(parameter);
  DEBUG_PRINTLN('"');


  mySerial->print(F("AT+HTTPPARA=\""));
  mySerial->print(parameter);
  if (quoted)
    mySerial->print(F("\",\""));
  else
    mySerial->print(F("\","));
}

boolean Adafruit_FONA::HTTP_para_end(boolean quoted) {
  if (quoted)
    mySerial->println('"');
  else
    mySerial->println();

  return expectReply(ok_reply);
}

boolean Adafruit_FONA::HTTP_para(FONAFlashStringPtr parameter,
                                 const char *value) {
  HTTP_para_start(parameter, true);
  mySerial->print(value);
  return HTTP_para_end(true);
}

boolean Adafruit_FONA::HTTP_para(FONAFlashStringPtr parameter,
                                 FONAFlashStringPtr value) {
  HTTP_para_start(parameter, true);
  mySerial->print(value);
  return HTTP_para_end(true);
}

boolean Adafruit_FONA::HTTP_para(FONAFlashStringPtr parameter,
                                 int32_t value) {
  HTTP_para_start(parameter, false);
  mySerial->print(value);
  return HTTP_para_end(false);
}

boolean Adafruit_FONA::HTTP_data(uint32_t size, uint32_t maxTime) {
  flushInput();


  DEBUG_PRINT(F("\t---> "));
  DEBUG_PRINT(F("AT+HTTPDATA="));
  DEBUG_PRINT(size);
  DEBUG_PRINT(',');
  DEBUG_PRINTLN(maxTime);


  mySerial->print(F("AT+HTTPDATA="));
  mySerial->print(size);
  mySerial->print(",");
  mySerial->println(maxTime);

  return expectReply(F("DOWNLOAD"));
}

boolean Adafruit_FONA::HTTP_action(uint8_t method, uint16_t *status,
                                   uint16_t *datalen, int32_t timeout) {
  // Send request.
  if (! sendCheckReply(F("AT+HTTPACTION="), method, ok_reply))
    return false;

  // Parse response status and size.
  readline(timeout);
  if (! parseReply(F("+HTTPACTION:"), status, ',', 1))
    return false;
  if (! parseReply(F("+HTTPACTION:"), datalen, ',', 2))
    return false;

  return true;
}

boolean Adafruit_FONA::HTTP_readall(uint16_t *datalen) {
  getReply(F("AT+HTTPREAD"));
  if (! parseReply(F("+HTTPREAD:"), datalen, ',', 0))
    return false;

  return true;
}

boolean Adafruit_FONA::HTTP_ssl(boolean onoff) {
  return sendCheckReply(F("AT+HTTPSSL="), onoff ? 1 : 0, ok_reply);
}

/********* HTTP HIGH LEVEL FUNCTIONS ***************************/





void Adafruit_FONA::setUserAgent(FONAFlashStringPtr useragent) {
  this->useragent = useragent;
}

void Adafruit_FONA::setHTTPSRedirect(boolean onoff) {
  httpsredirect = onoff;
}

/********* HTTP HELPERS ****************************************/

boolean Adafruit_FONA::HTTP_setup(char *url) {
  // Handle any pending
  HTTP_term();

  // Initialize and set parameters
  if (! HTTP_init())
    return false;
  if (! HTTP_para(F("CID"), 1))
    return false;
  if (! HTTP_para(F("UA"), useragent))
    return false;
  if (! HTTP_para(F("URL"), url))
    return false;

  // HTTPS redirect
  if (httpsredirect) {
    if (! HTTP_para(F("REDIR"), 1))
      return false;

    if (! HTTP_ssl(true))
      return false;
  }

  return true;
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




