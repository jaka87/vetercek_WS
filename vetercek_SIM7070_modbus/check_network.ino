bool checkNetwork() {  
  byte GSMstatus = 99;
  unsigned long startTime = millis();  
  byte zero_network = 0;
  
  // Maximum timer of 60 seconds
  unsigned long maxTime = 40000;

  do {
    GSMstatus = netStatus();
    
    // Check if more than 20 seconds have passed with no network or forbidden status
    if ((GSMstatus == 0 or GSMstatus == 3) and (millis() - startTime) > 20000) {
      fona.setCOPS(2); // de-register
      delay(2000);
      fona.setCOPS(0); // auto-register
      #ifdef DEBUG
        DEBUGSERIAL.println(F("dereg"));
      #endif
      startTime = millis(); 
    }
    
    // If no GSM signal
    else if (GSMstatus == 0) {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("nogsm"));
      #endif
      fona.setFunctionality(0); // AT+CFUN=0
      delay(3000);
      fona.setFunctionality(1); // AT+CFUN=1
      delay(10 * 1000);        
    }
    
    // If forbidden GSM status
    else if (GSMstatus == 3) {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("forbidden"));
      #endif
      fona.setPreferredMode(2);   
      startTime = millis(); 
    }
    
    // If no connection within 60 seconds
    else if (GSMstatus == 2 and millis() - startTime >= 60000) {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("NTWRK fail"));
        DEBUGSERIAL.println(GSMstatus);
      #endif
      fona.setFunctionality(0); // AT+CFUN=0
      delay(3000);
      fona.setFunctionality(1); // AT+CFUN=1     
      startTime = millis(); 
    }
    
    // General status messages
    else {
      #ifdef DEBUG
        DEBUGSERIAL.print(F("Src "));
        DEBUGSERIAL.println(GSMstatus);
      #endif     
      delay(1000);            
    }

    // Break if 60 seconds have passed
    if (millis() - startTime > maxTime) {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("Max time exceeded"));
      #endif
    reset(5);
    }
    
  } while (GSMstatus != 5 and GSMstatus != 1);

  return true; // Successfully connected to the network
}





bool checkGPRS() { 
    if (!fona.checkAT()) {
        #ifdef DEBUG
            DEBUGSERIAL.println(F("Module not responding"));
        #endif
        return false;
    }
    
    if (fona.GPRSstate() != 1) {
        #ifdef DEBUG
            DEBUGSERIAL.println(F("GPRS not attached"));
        #endif
        return false;
    }
    
    #ifdef DEBUG
        DEBUGSERIAL.println(F("GPRS attached"));
    #endif
    
    if (!hasValidIP()) {
        #ifdef DEBUG
            DEBUGSERIAL.println(F("No valid IP"));
        #endif
        return false;
    }
    
    #ifdef DEBUG
        DEBUGSERIAL.println(F("GPRS OK with IP"));
    #endif
    
    return true;
}


bool checkServer() {
  // Ensure PDP is active
  if (!checkGPRS()) {
    if (!connectGPRS()) {
      return false;
    }
  }

  // Try UDP connect
  for (int i = 0; i < 5; i++) {
    if (fona.UDPconnect(broker, BROKER_PORT)) {
      checkServernum = 0;   // ✅ reset on success

      sig = fona.getRSSI();
      battLevel = readVcc();

      #ifdef DEBUG
        DEBUGSERIAL.println(F("SRVR_OK"));
      #endif
      return true;
    }
    delay(500);
  }

  // Failed all retries
  checkServernum++;

  #ifdef DEBUG
    DEBUGSERIAL.print(F("vet_con_fail "));
    DEBUGSERIAL.println(checkServernum);
  #endif

  if (checkServernum == 2 || checkServernum == 3) {
      if (!checkGPRS()) { connectGPRS();  }
  }
  else if (checkServernum >= 4) {
    reset(12);            // hard recovery
  }

  return false;
}



void fail_to_send() {
  fona.UDPclose();
  failedSend++;

  #ifdef DEBUG
    DEBUGSERIAL.print(F("Fail_Send "));
    DEBUGSERIAL.println(failedSend);
  #endif

  if (failedSend >= 1) {
    reset(13);
  }
  
}



// Check if we have a valid IP address
bool hasValidIP() {
    char ip[16];
    if (fona.getIPAddress(ip, sizeof(ip))) {
        #ifdef DEBUG
            DEBUGSERIAL.print(F("IP Address: "));
            DEBUGSERIAL.println(ip);
        #endif
        return true;
    }
    
    #ifdef DEBUG
        DEBUGSERIAL.println(F("No valid IP address"));
    #endif
    return false;
}

// Wait for IP address with timeout
bool waitForIP(uint16_t timeoutMs = 15000) {
    unsigned long startTime = millis();
    while (millis() - startTime < timeoutMs) {
        if (hasValidIP()) {
            return true;
        }
        delay(500);
        //flushSerialBuffer(200);
    }
    return false;
}
