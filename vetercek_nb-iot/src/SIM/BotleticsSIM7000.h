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
#ifndef BOTLETICS_MODEM_H
#define BOTLETICS_MODEM_H

/*
 * BOTLETICS_MODEM_DEBUG
 * Causes extensive debug output on the DebugStream;
 * set in the appropriate header.
 */

//#define BOTLETICS_MODEM_DEBUG

#include "includes/platform/Modem.h"

#define SIM7000  7
#define SIM7070  12

#define BOTLETICS_DEFAULT_TIMEOUT_MS 500
#define BOTLETICS_NO_RST_PIN 99
//#define SIM_ERROR -2
//#define SIM_UNKNOWN -1
//#define SIM_READY 0


class Botletics_modem : public BotleticsStreamType {
 public:
  Botletics_modem(int8_t);
  boolean begin(BotleticsStreamType &port);
  boolean checkAT();
  uint8_t type();

  // Stream
  int available(void);
  size_t write(uint8_t x);
  int read(void);
  int peek(void);
  void flush();



  // Power, battery, and ADC
  void powerOn(uint8_t BOTLETICS_PWRKEY);
  boolean powerDown(void);
  boolean getBattVoltage(uint16_t *v);

  // Functionality and operation mode settings
  boolean setFunctionality(uint8_t option); // AT+CFUN command
  boolean enableSleepMode(bool onoff); // AT+CSCLK command
  boolean set_eDRX(uint8_t mode, uint8_t connType, char * eDRX_val); // AT+CEDRXS command
  boolean setNetLED(bool onoff, uint8_t mode = 0, uint16_t timer_on = 64, uint16_t timer_off = 3000); // AT+CNETLIGHT and AT+SLEDS commands

  // SIM query
  uint8_t getNetworkStatus(void);
  uint8_t getRSSI(void);

  // IMEI
  uint8_t getIMEI(char *imei);


  // GPRS handling
  boolean enableGPRS(boolean onoff);
  boolean enableGPRS_old();
  int8_t GPRSstate(void);
  void setNetworkSettings(FStringPtr apn, FStringPtr username=0, FStringPtr password=0);
  int8_t getNetworkType(char *typeStringBuffer, size_t bufferLength);
  int8_t getBearerStatus(void);
  int8_t getNetworkInfo(void);
  bool getNetworkInfoLong(void);
  int8_t GPRSPDP(void);


  // Network connection (AT+CNACT)
  boolean openWirelessConnection(bool onoff);
  boolean wirelessConnStatus(void);


  // UDP raw connections
  boolean UDPconnect(char *server, uint16_t port);
  boolean UDPclose(void);
  uint8_t UDPconnected(void);
  boolean UDPsend(unsigned char *packet, uint8_t len, byte response[10],uint8_t charr);
  uint16_t UDPavailable(void);


  // Helper functions to verify responses.
  boolean expectReply(FStringPtr reply, uint16_t timeout = 10000);
  boolean sendCheckReply(const char *send, const char *reply, uint16_t timeout = BOTLETICS_DEFAULT_TIMEOUT_MS);
  boolean sendCheckReply(FStringPtr send, FStringPtr reply, uint16_t timeout = BOTLETICS_DEFAULT_TIMEOUT_MS);
  boolean sendCheckReply(const char* send, FStringPtr reply, uint16_t timeout = BOTLETICS_DEFAULT_TIMEOUT_MS);


 protected:
  int8_t _rstpin;
  uint8_t _type;

  char replybuffer[255];
  byte replybuffer2[10];
  FStringPtr apn;
  FStringPtr apnusername;
  FStringPtr apnpassword;
  boolean httpsredirect;
  FStringPtr useragent;
  FStringPtr ok_reply;

  // HTTP helpers

  void flushInput();
  uint16_t readRaw(uint16_t b);
  uint8_t readline(uint16_t timeout = BOTLETICS_DEFAULT_TIMEOUT_MS, boolean multiline = false);
  uint8_t readline2(uint16_t timeout, uint8_t characters);
  uint8_t getReply(const char *send, uint16_t timeout = BOTLETICS_DEFAULT_TIMEOUT_MS);
  uint8_t getReply(FStringPtr send, uint16_t timeout = BOTLETICS_DEFAULT_TIMEOUT_MS);
  uint8_t getReply(FStringPtr prefix, char *suffix, uint16_t timeout = BOTLETICS_DEFAULT_TIMEOUT_MS);
  uint8_t getReply(FStringPtr prefix, int32_t suffix, uint16_t timeout = BOTLETICS_DEFAULT_TIMEOUT_MS);
  uint8_t getReply(FStringPtr prefix, int32_t suffix1, int32_t suffix2, uint16_t timeout); // Don't set default value or else function call is ambiguous.
  uint8_t getReplyQuoted(FStringPtr prefix, FStringPtr suffix, uint16_t timeout = BOTLETICS_DEFAULT_TIMEOUT_MS);

  boolean sendCheckReply(FStringPtr prefix, char *suffix, FStringPtr reply, uint16_t timeout = BOTLETICS_DEFAULT_TIMEOUT_MS);
  boolean sendCheckReply(FStringPtr prefix, int32_t suffix, FStringPtr reply, uint16_t timeout = BOTLETICS_DEFAULT_TIMEOUT_MS);
  boolean sendCheckReply(FStringPtr prefix, int32_t suffix, int32_t suffix2, FStringPtr reply, uint16_t timeout = BOTLETICS_DEFAULT_TIMEOUT_MS);
  boolean sendCheckReplyQuoted(FStringPtr prefix, FStringPtr suffix, FStringPtr reply, uint16_t timeout = BOTLETICS_DEFAULT_TIMEOUT_MS);




  boolean parseReply(FStringPtr toreply,
          uint16_t *v, char divider  = ',', uint8_t index=0);
  boolean parseReplyFloat(FStringPtr toreply,
           float *f, char divider, uint8_t index);
  boolean parseReply(FStringPtr toreply,
          char *v, char divider  = ',', uint8_t index=0);
  boolean parseReplyQuoted(FStringPtr toreply,
          char *v, int maxlen, char divider, uint8_t index);

  boolean sendParseReply(FStringPtr tosend,
       FStringPtr toreply,
       uint16_t *v, char divider = ',', uint8_t index=0);

  boolean sendParseReplyFloat(FStringPtr tosend,
         FStringPtr toreply,
         float *f, char divider = ',', uint8_t index=0);


  BotleticsStreamType *mySerial;
};



class Botletics_modem_LTE : public Botletics_modem {

 public:
  Botletics_modem_LTE () : Botletics_modem(BOTLETICS_NO_RST_PIN) { _type = SIM7000; _type = SIM7070;  }

  boolean setPreferredMode(uint8_t mode);
  boolean setPreferredLTEMode(uint8_t mode);
  boolean setOperatingBand(const char * mode, uint8_t band);


};

#endif
