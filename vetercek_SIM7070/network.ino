void moduleSetup() {
  Serial.begin(57600);
  bool zagnano=fona.begin(Serial);
  if (!zagnano ){   reset(9); }

  fona.enableSleepMode(true);
  delay(100);
  fona.setPreferredMode(GSMstate);
  delay(3000);
  fona.setNetLED(true,3,64,5000);
  delay(100);
  fona.setNetworkSettings(F(APN)); // APN
  delay(100);

  if (GSMstate!=13){
    fona.set_eDRX(1, 5, "1001");
    delay(100);
  }

  #ifdef DEBUG
    DEBUGSERIAL.println(F("modOK"));
  #endif
}  



void changeNetwork_id(int network, byte technology) {
 fona.setNetwork(network,technology); 
  #ifdef DEBUG
    DEBUGSERIAL.println(F("network change"));
  #endif 
  delay(7000);
  connectGPRS();
}


byte netStatus() {
  byte n = fona.getNetworkStatus();
  return n;
}



bool connectGPRS() {
    // First, ensure network is registered
    if (!checkNetwork()) {
        #ifdef DEBUG
            DEBUGSERIAL.println(F("Network not registered"));
        #endif
        return false;
    }
    
    #ifdef DEBUG
        DEBUGSERIAL.println(F("Connecting GPRS..."));
    #endif
    
    unsigned long startTime = millis();
    bool gprsAttached = false;
    
    // Try to enable GPRS
    while (!gprsAttached && (millis() - startTime) < 30000) {
        gprsAttached = fona.enableGPRS(true);
        
        if (!gprsAttached) {
            #ifdef DEBUG
                DEBUGSERIAL.print(F("GPRS enable attempt failed, retrying..."));
            #endif
            delay(2000);
            //flushSerialBuffer(200);
        }
    }
    
    if (!gprsAttached) {
        #ifdef DEBUG
            DEBUGSERIAL.println(F("GPRS enable failed after timeout"));
        #endif
        return false;
    }
    
    #ifdef DEBUG
        DEBUGSERIAL.println(F("GPRS attached, waiting for IP..."));
    #endif
    
    // CRITICAL: Wait for IP address to be assigned
    if (waitForIP(15000)) {
        #ifdef DEBUG
            DEBUGSERIAL.println(F("GPRS connected with valid IP"));
        #endif
        return true;
    }
    
    #ifdef DEBUG
        DEBUGSERIAL.println(F("GPRS attached but no IP assigned"));
    #endif
    
    // Try to reactivate PDP context
    #ifdef DEBUG
        DEBUGSERIAL.println(F("Reactivating PDP context..."));
    #endif
    
    fona.enableGPRS(false);
    //flushSerialBuffer(200);
    delay(2000);
    
    // Try one more time
    gprsAttached = fona.enableGPRS(true);
    if (gprsAttached) {
        return waitForIP(10000);
    }
    
    return false;
}


#ifdef OPENVPN

void gatherData() {
  data[2]  = windDir / 100;
  data[3]  = windDir % 100;
  data[4]  = wind_speed / 10;
  data[5]  = wind_speed % 10;
  data[6]  = windGustAvg / 10;
  data[7]  = windGustAvg % 10;

  data[9]  = abs(temp * 100) / 100;
  data[10] = abs(int(temp * 100)) % 100;

  data[12] = abs(water * 100) / 100;
  data[13] = abs(int(water * 100)) % 100;

  data[14] = battLevel;
  data[15] = sig;
  data[16] = measureCount;
  data[17] = resetReason;
  data[18] = SolarCurrent;

  // air temperature sign
  if (temp > 0) data[8] = 1;
  else          data[8] = 0;

  // rain / water logic
  if (rainCount > -1 && enableRain == 1) {
    data[11] = 10;
    data[12] = rainCount;
    data[13] = 0;
  }
  else if (water > 0) {
    data[11] = 1;
    data[12] = abs(water * 100) / 100;
    data[13] = abs(int(water * 100)) % 100;
  }
  else {
    data[11] = 0;
    data[12] = abs(water * 100) / 100;
    data[13] = abs(int(water * 100)) % 100;
  }

  // pressure
  if (enableBmp == 1) {
    data[19] = pressure / 100;
    data[20] = pressure % 100;
  } else {
    data[19] = sonicError;
  }

  // humidity
  if (enableHum == 1) {
    data[21] = humidity;
  }
}
#else

void gatherData() {
  data[8]=windDir/100;
  data[9]=windDir%100;
  data[10]=wind_speed/10;
  data[11]=wind_speed%10;
  data[12]=windGustAvg/10;
  data[13]=windGustAvg%10;
  data[15]=abs(temp*100)/100;
  data[16]=abs(int(temp*100))%100;
  data[18]=abs(water*100)/100;
  data[19]=abs(int(water*100))%100;
  data[20]=battLevel;
  data[21]=sig;
  data[22]=measureCount;
  data[23]=resetReason;
  data[24]=SolarCurrent;

  if (temp > 0) { data[14]=1; } // if positive or negative air temperature
  else {    data[14]=0; } 

  if (rainCount > -1 and enableRain==1) { // if rain instead of water
    data[17]=10;
    data[18]=rainCount;
    data[19]=0;    
  } 
  
  else if (water > 0) { //if positive or negative water temperature
    data[17]=1;
    data[18]=abs(water*100)/100;
    data[19]=abs(int(water*100))%100;
  } 
  else {
    data[17]=0;
    data[18]=abs(water*100)/100;
    data[19]=abs(int(water*100))%100;
  } 

  if (enableBmp==1) { // if send pressure value
    data[25]=pressure/100;
    data[26]=pressure%100;
  } 
  else { 
    data[25]=sonicError;    
  } 

  if (enableHum==1) { // if send humidity value
    data[27]=humidity;
  } 

}
#endif



void parseResponse(byte response[13]) {
        
  if (response[1] ==1 ) {  
    vaneOffset=(response[2]*100)+response[3];    // if byte is positive value
  } 
  else {  
    vaneOffset=-1*((response[2]*100)+response[3]);
  }

  if (response[4] >0) { windDelay=response[4]*100;}
  
  if (response[8] ==1) { reset(3); } // reset
  else if (response[8]==100) { EEPROM.write(10, 0); reset(3); } // water
  else if (response[8]==101) { EEPROM.write(10, 1); reset(3); } 
  else if (response[8]==110) { EEPROM.write(11, 2); reset(3); } // solar
  else if (response[8]==111) { EEPROM.write(11, 1); reset(3); } 
  else if (response[8]==120) { EEPROM.write(12, 0); reset(3); } //ultrasonic
  else if (response[8]==121) { EEPROM.write(12, 1); reset(3); }   
  else if (response[8]==130) { EEPROM.write(13, 0); reset(3); }   //pressure
  else if (response[8]==131) { EEPROM.write(13, 1); reset(3); } 
  else if (response[8]==40) { EEPROM.write(14, 10); stopSleepChange=3; }  // UZ sleep on / off
  else if (response[8]==41) { EEPROM.write(14, 11); stopSleepChange=0; } 
  else if (response[8]==160) { EEPROM.write(16, 1); enableHum=1; }  // humidity on off
  else if (response[8]==161) { EEPROM.write(16, 0); enableHum=0; } 
  else if (response[8]==27) { EEPROM.write(27, 1); } //turn on toggle mobile network
  else if (response[8]==28) { EEPROM.write(27, 0); } //turn off toggle mobile network
  else if (response[8] == 102 ) { GSMstate=2; moduleSetup(); } // temporarry change network - auto
  else if (response[8] == 113 ) { GSMstate=13; moduleSetup(); } // temporarry change network - 2G
  else if (response[8] == 138 ) { GSMstate=38; moduleSetup(); } // temporarry change network - nb-iot
  else if (response[8] == 2 or response[8]==13 or response[8]==38 or response[8]==51) { // if new settings for network prefference
    EEPROM.write(9, response[8]);   // write new data to EEPROM
    reset(3); 
    }

  else if (response[8] == 99 or response[8] == 97 or  response[8] == 98) { // connect to custom network
      if (response[8] == 97){
        EEPROM.write(20, response[9]);
        EEPROM.write(21, response[10]);
        EEPROM.write(22, response[11]);
   #ifdef DEBUG                                 
    DEBUGSERIAL.println("net1");
  #endif
      }
      else if (response[8] == 98){
        EEPROM.write(23, response[9]);
        EEPROM.write(24, response[10]);
        EEPROM.write(25, response[11]);
   #ifdef DEBUG                                 
    DEBUGSERIAL.println("net2");
  #endif        
      }

    else {
    byte lastbyte = response[11];
    byte firstpart;
    byte secondpart;
    if (lastbyte <10) {
      secondpart=lastbyte;
      firstpart=0;
    }
    else {
      secondpart = lastbyte%10; 
      firstpart  = (lastbyte/10)%10;
    }
    int networkid=(response[9]*1000)+(response[10]*10)+firstpart;
     #ifdef DEBUG
      DEBUGSERIAL.println(F("custom"));
      DEBUGSERIAL.println(networkid);
      DEBUGSERIAL.println(secondpart);
     #endif
     if(response[9]!=0 and response[10]!=0 and response[11]!=0){
        changeNetwork_id(networkid,secondpart);
      }
     } 
    }
   
  onOffTmp=response[5];
  cutoffWind=response[6];

  // if low battery increase sleep time
//    if ( (response[7] < 4 and battLevel < 180 and battLevel > 170) or (batteryState==1 and response[7] < 4)) { // if low battery < 3.6V
//       response[7]=4;
//       batteryState=1;
//    }
//    else if (( response[7] < 8 and battLevel < 170 and battLevel > 17) or (batteryState==2 and response[7] < 8)) { // if low battery < 3.4V
//       response[7]=8;
//       batteryState=2;
//    }

  // once battery gets charged change the battery state  
//    if (  battLevel > 190 and batteryState==1) { batteryState=0; }// if battery > 3.8V
//    else if (  battLevel >= 180 and battLevel >= 190 and batteryState==2) { batteryState=1; }// if battery > 3.6V
//    
  
  
  #ifdef UZ_Anemometer
    if ( response[7]!= sleepBetween and response[7] > -1 and response[7] < 9 and sleepBetween != response[7]) { //change of sleep time
      changeSleep=1;
      sleepBetween=response[7];
    }
  #else
    if ( (response[7] > -1 and response[7] < 9 and sleepBetween != response[7])) { 
      sleepBetween=response[7];
    }  
  #endif

  if (response[0] >0 and sleepBetween==0) { whenSend=response[0]*2;} // when sleep is 0 updates =2x
  else if (response[0] >0 ) { whenSend=response[0];}
  
} 









void AfterPost() {
    fona.UDPclose();
    measureCount = 0;
    windAvr = 0;
    windGustAvg = 0;
    windDir = 0;
    water = 99;
    temp = 99;
    windAvgX = 0;
    windAvgY = 0;
    resetReason=0;
    failedSend=0;
    sonicError=0;
    rainCount=0;
    pressure=0;
    humidity=0;
    checkServernum=0;
    memset(windGust, 0, sizeof(windGust)); // empty direction array

}




bool SendData() {
    static byte failLevel = 0;
    bool serverConnected = false;
    
    if (failedSend == 0 && checkServernum == 0) {  
        BeforePostCalculations(1); 
    } else {  
        BeforePostCalculations(0); 
    }
    
    gatherData();
    
    while (failLevel <= 3) {
        
        // Apply recovery for current level
        if (failLevel == 0) {
            // Level 0: Normal operation
            #ifdef DEBUG
                DEBUGSERIAL.println(F("Level 0: Normal send attempt"));
            #endif
            // Only flush if we're recovering from previous failure
            if (failedSend > 0 || checkServernum > 0) {
                //flushSerialBuffer(100);
            }
        }
        else if (failLevel == 1) {
            // Level 1: Restart GPRS
            #ifdef DEBUG
                DEBUGSERIAL.println(F("Level 1: Restarting GPRS..."));
            #endif
            
            // Close UDP socket and disable GPRS
            fona.UDPclose();
            //flushSerialBuffer(200);
            delay(200);
            
            fona.enableGPRS(false);
            //flushSerialBuffer(200);
            delay(2000);
            
            // Try to connect GPRS with timeout
            unsigned long startTime = millis();
            bool gprsConnected = false;
            while (!gprsConnected && (millis() - startTime) < 30000) {
                gprsConnected = connectGPRS();
                if (!gprsConnected) {
                    delay(2000);
                }
            }
            
            if (!checkGPRS()) {
                #ifdef DEBUG
                    DEBUGSERIAL.println(F("GPRS restart failed"));
                #endif
                failLevel++;
                continue;
            }
            
            #ifdef DEBUG
                DEBUGSERIAL.println(F("GPRS restart successful"));
            #endif
        }
        else if (failLevel == 2) {
            // Level 2: Reset GSM module
            #ifdef DEBUG
                DEBUGSERIAL.println(F("Level 2: Resetting GSM module..."));
            #endif
            
            // Cleanup before reset
            fona.UDPclose();
            //flushSerialBuffer(200);
            delay(500);
            
            fona.enableGPRS(false);
            //flushSerialBuffer(200);
            delay(1000);
            
            // Reset GSM module
            fona.reset();
            delay(5000);
            
            // Re-initialize
            moduleSetup();
            delay(2000);
            
            
            // Try to connect GPRS with timeout
            unsigned long startTime = millis();
            bool gprsConnected = false;
            while (!gprsConnected && (millis() - startTime) < 30000) {
                gprsConnected = connectGPRS();
                if (!gprsConnected) {
                    delay(2000);
                }
            }
            
            if (!checkGPRS()) {
                #ifdef DEBUG
                    DEBUGSERIAL.println(F("GSM reset failed"));
                #endif
                failLevel++;
                continue;
            }
            
            #ifdef DEBUG
                DEBUGSERIAL.println(F("GSM reset successful"));
            #endif
        }
        else if (failLevel == 3) {
            // Level 3: Board reset
            #ifdef DEBUG
                DEBUGSERIAL.println(F("Level 3: Board reset..."));
            #endif
            reset(13);
            return false;
        }
        
        // Always close any existing UDP connection before trying to reconnect
        fona.UDPclose();
        //flushSerialBuffer(200);
        delay(100);
        
        // Now try to connect to server with retries
        serverConnected = false;
        for (int connectAttempt = 0; connectAttempt < 3; connectAttempt++) {
            #ifdef DEBUG
                DEBUGSERIAL.print(F("Server connect attempt "));
                DEBUGSERIAL.print(connectAttempt + 1);
                DEBUGSERIAL.print(F("... "));
            #endif
            
            if (fona.UDPconnect(broker, BROKER_PORT)) {
                serverConnected = true;
                
                // Update battery and signal when connected
                sig = fona.getRSSI();
                battLevel = readVcc();
                
                #ifdef DEBUG
                    DEBUGSERIAL.print(F("OK (Sig: "));
                    DEBUGSERIAL.print(sig);
                    DEBUGSERIAL.print(F(", Batt: "));
                    DEBUGSERIAL.print(battLevel);
                    DEBUGSERIAL.println(F(")"));
                #endif
                break;
            }
            
            #ifdef DEBUG
                DEBUGSERIAL.println(F("FAILED"));
            #endif
            delay(2000);
            //flushSerialBuffer(200);  // Only flush after failed attempt
        }
        
        if (!serverConnected) {
            #ifdef DEBUG
                DEBUGSERIAL.println(F("Server connection failed, escalating"));
            #endif
            failLevel++;
            continue;
        }

       
        // Try to send data with retries
        byte result = trySendWithRetries(failLevel);
        
        if (result == 1) {
            // SUCCESS!
            failLevel = 0;
            failedSend = 0;
            checkServernum = 0;
            return true;
        }
        
        // Send failed, escalate
        #ifdef DEBUG
            DEBUGSERIAL.print(F("Send failed with result: "));
            DEBUGSERIAL.println(result);
        #endif
        
        failLevel++;
        fona.UDPclose();
        //flushSerialBuffer(200);
        delay(1000);
    }
    
    return false;
}


byte trySendWithRetries(byte &failLevel) {
    const byte MAX_RETRIES = 5;
    byte result = 0;
    
    for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
        byte response[13];
        
        #ifdef DEBUG
            DEBUGSERIAL.print(F("UDPsend attempt "));
            DEBUGSERIAL.print(attempt + 1);
            DEBUGSERIAL.print(F("... "));
        #endif
        
        result = fona.UDPsend(data, sizeof(data), response, 26);
        
        #ifdef DEBUG
            DEBUGSERIAL.println(result);
        #endif
        
        if (result == 1) {
            // Success!
            sendError = 0;
            parseResponse(response);
            AfterPost();
            return 1;
        }
        
        if (result == 5) { // No response
            #ifdef DEBUG
                DEBUGSERIAL.println(F("No response, checking GPRS..."));
            #endif
        
            if (!checkGPRS()) {
                #ifdef DEBUG
                    DEBUGSERIAL.println(F("GPRS not attached, restarting GPRS..."));
                #endif
                failLevel = 1; // escalate to GPRS restart
                break; // exit retries, go to failLevel handling
            }
            else {
                // GPRS OK, just retry UDPsend
                int delayTime = 1000 * (attempt + 1);
                delay(delayTime);
                continue;
            }
        }
        
        // Any other error, break out
        #ifdef DEBUG
            DEBUGSERIAL.println(F("Fatal error, stopping retries"));
        #endif
        break;
    }
    
    #ifdef DEBUG
        DEBUGSERIAL.println(F("All UDP retries failed"));
    #endif
    
    return result;
}
