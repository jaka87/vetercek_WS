bool netStatus() {
  int n = fona.getNetworkStatus();
  #ifdef DEBUG
  Serial.print(F("Network status ")); Serial.print(n); Serial.print(F(": "));
  if (n == 0) Serial.println(F("Not registered"));
  if (n == 1) Serial.println(F("Registered (home)"));
  if (n == 2) Serial.println(F("Not registered (searching)"));
  if (n == 3) Serial.println(F("Denied"));
  if (n == 4) Serial.println(F("Unknown"));
  if (n == 5) Serial.println(F("Registered roaming"));
  #endif
  
  if (!(n == 1 || n == 5)) return false;
  else return true;
}


void moduleSetup() {
  // Note: The SIM7000A baud rate seems to reset after being power cycled (SIMCom firmware thing)
  // SIM7000 takes about 3s to turn on but SIM7500 takes about 15s
  // Press reset button if the module is still turning on and the board doesn't find it.
  // When the module is on it should communicate right after pressing reset
  //fonaSS.begin(115200); // Default SIM7000 shield baud rate
  
  //Serial.println(F("Configuring to 9600 baud"));
  //fonaSS.println("AT+IPR=9600"); // Set baud rate
  //delay(100); // Short pause to let the command run
  fonaSS.begin(9600);
  if (! fona.begin(fonaSS)) {
    Serial.println(F("Couldn't find FONA"));
    while(1); // Don't proceed if it couldn't find the device
  }

}  

void parseResponse(char *payload_string) {
  int num=0;
   char *token;
   token = strtok(payload_string, ";");
   while(token != NULL)
   {
    num++;
    if (num==1) { whenSend=atoi(token); // how offten to send data
    }

    else if (num==2) { vaneOffset=atoi(token);  // wind vane offset
    }    

    else if (num==3) { windDelay=atoi(token);  // interval for wind measurement
    } 

    else if (num==4) { onOffTmp=atoi(token);  // turn on / off temperature measurement
    } 

    else if (num==5) {cutoffWind=atoi(token);  // cuttoff wind treshold
    } 

    else if (num==6) { sleepBetween=atoi(token);  // sleep between wind measurements
    } 
        
    else if (num==7) {
      int num7=atoi(token);  // restart arduino
      if (num7==1) {
        reset();
       } 
    } 
      
      token = strtok(NULL, ";");
   }
} 

#ifdef SEND_MQTT
void getMQTTmsg() {
    //digitalWrite(DTR, LOW);  //wake up

   uint8_t i = 0;
    if (fona.available()) {
      while (fona.available()) {
        replybuffer[i] = fona.read();
        i++;
      }
        //Serial.println(replybuffer);
      delay(300); // Make sure it prints and also allow other stuff to run properly
      if (strstr(replybuffer, "SMSUB:") != NULL ) {
        
        Serial.println(F("*** Received MQTT message! ***"));
        
        char *p = strtok(replybuffer, ",\"");
        char *topic_p = strtok(NULL, ",\"");
        char *payload_string = strtok(NULL, ",\"");

      parseResponse(payload_string);  
      }
    }

}
#endif 


void connectGPRS() {
    while (!netStatus()) {
      #ifdef DEBUG
        Serial.println(F("Failed to connect to cell network, retrying..."));
      #endif 
      delay(2000); // Retry every 2s
    }
    Serial.println(F("Connected to cell network!"));

#ifdef SEND_MQTT
    // Open wireless connection if not already activated
    if (!fona.wirelessConnStatus()) {
      while (!fona.openWirelessConnection(true)) {
        #ifdef DEBUG
          Serial.println(F("Failed to enable connection, retrying..."));
        #endif 
        delay(2000); // Retry every 2s
      }
      #ifdef DEBUG
        Serial.println(F("Enabled data!"));
      #endif 
    }

#else
  if (fona.enableGPRS(true)) {
  #ifdef DEBUG  
    Serial.println(F("Enabled data"));
  #endif 
  }
#endif 

}


void PostData() {    

//  #ifdef NBIOT
//    wakeUp();
//    delay(300);
//  fonaSS.begin(9600);
//  if (! fona.begin(fonaSS)) {
//    #ifdef DEBUG
//      Serial.println(F("Couldn't find FONA"));
//    #endif  
//    while(1); // Don't proceed if it couldn't find the device
//  }

  delay(300);

  if (sendBatTemp >= 10) {  // send data about battery and signal every 10 measurements
    sig=fona.getRSSI(); 
    delay(100);
    battLevel = readVcc(); // Get voltage %
    sendBatTemp=0;
}
  else {
    sendBatTemp=sendBatTemp+1;
}

#ifdef SEND_MQTT
 sprintf(URL, FORMAT,IMEI, windDir, wind_speed / 10, wind_speed % 10, windGustAvg / 10, windGustAvg % 10, tmp, wat, battLevel, sig, measureCount,resetReason);
    #ifdef DEBUG
      Serial.println(URL);
    #endif
  if (! fona.MQTT_connectionStatus()) {
      // Set up MQTT parameters (see MQTT app note for explanation of parameter values)
      fona.MQTT_setParameter("URL", broker, 1883);
      fona.MQTT_setParameter("USERNAME", MQTTuser);
      fona.MQTT_setParameter("PASSWORD", MQTTpass);
      fona.MQTT_setParameter("KEEPTIME", "150"); // Time to connect to server, 60s by default
      delay(100);
      Serial.println(F("Connecting to MQTT broker..."));
      if (! fona.MQTT_connect(true)) {
        Serial.println(F("Failed to connect to broker!"));
        connectGPRS();
      }
    }

    if (fona.MQTT_publish("ws", URL, strlen(URL), 0, 0)) {
        fona.MQTT_subscribe(topicGET, 0); // Topic name, QoS
    #ifdef DEBUG
      Serial.println("MQTT UPLOADED");
    #endif  
    AfterPost(); 
      }     
#endif 


#ifdef UDP

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
  
  if (temp > 0) {
    data[14]=1;
  } 
  else {
    data[14]=0;
  } 
  if (water > 0) {
    data[17]=1;
  } 
  else {
    data[17]=0;
  } 


bool isConnected = fona.UDPconnected();

    if (isConnected !=1) {
    int8_t GPRSstate = fona.GPRSstate();
      if (GPRSstate !=1) {
          connectGPRS();
       } 
     fona.UDPconnect("vetercek.com",6789);
     }     

  byte response[10];  
  if ( fona.UDPsend(data,sizeof(data),response,9)) { //25
  
  if (response[0] >0) { whenSend=response[0];}
  if (response[1] ==1 ) {  
    vaneOffset=(response[2]*100)+response[3];    // if byte is positive value
  } 
  else {  
    vaneOffset=-1*((response[2]*100)+response[3]);
  }

  if (response[4] >0) { windDelay=response[4]*100;}
  if (response[8] ==1) { reset(); }
  else if (response[8] == 2 or response[8]==13 or response[8]==38) { // if new settings for network prefference
    EEPROM.write(9, response[8]);   // write new data to EEPROM
    reset(); 
    }

  onOffTmp=response[5];
  cutoffWind=response[6];
  sleepBetween=response[7];

     #ifdef DEBUG
      Serial.println("SEND DONE");
     #endif
  AfterPost(); 
   } 

   else {
     fona.UDPclose();
   } 
   
#endif 


      
#ifdef HTTP
 sprintf(URL, FORMAT_URL,IMEI, windDir, wind_speed / 10, wind_speed % 10, windGustAvg / 10, windGustAvg % 10, tmp, wat, battLevel, sig, measureCount,resetReason);
    #ifdef DEBUG
      Serial.println(URL);
    #endif

int8_t state = fona.GPRSstate();
    if (state!=1) {
      connectGPRS();
    }

char response[50];  
  if (fona.postData(URL,response)) {
    #ifdef DEBUG
      Serial.println(F("complete HTTP POST..."));
    #endif
    parseResponse(response);
    AfterPost(); 
   }
#endif 



}


void AfterPost() {
    measureCount = 0;
    windAvr = 0;
    windGustAvg = 0;
    windDir = 0;
    water = 99;
    temp = 99;
    windAvgX = 0;
    windAvgY = 0;
    resetReason=0;
    memset(windGust, 0, sizeof(windGust)); // empty direction array
    //memset(tmp, 0, sizeof(tmp));
    //memset(wat, 0, sizeof(wat));
}



// send data to server
void SendData() {
#ifdef DEBUG
  Serial.println(timergprs);
#endif
    noInterrupts();
    timergprs = 0;
    interrupts();

  BeforePostCalculations();
  PostData();
#ifdef DEBUG
  Serial.println(timergprs);
#endif
  noInterrupts();
  timergprs = 0;
  interrupts();
}

void checkIMEI() {
   if (EEPROM.read(0)==1) {  // read from EEPROM if data in it

    #ifdef DEBUG
      Serial.println("IMEI from EEPROM");
         for (int i = 1; i < 9; i++){
             Serial.print(EEPROM.read(i));
         }
             Serial.println();
    #endif    
      for (int i = 0; i < 8; i++){
       data[i]=EEPROM.read(i+1);
       }
   }
   
   else {  // read from SIM module
    #ifdef DEBUG
      Serial.println("IMEI read!");
    #endif
      uint8_t imeiLen = fona.getIMEI(IMEI);  // imei to byte array
        delay(200);
        
      for(int i=0; i<16; i++)
        {
          idd[i]=(int)IMEI[i] - 48;
        }

      for (int i = 0; i < 8; i++){
        int multiply=i*2;
        if (i > 6) {
       data[i]=(idd[multiply]);
       }
        else {
       data[i]=((idd[multiply]*10)+idd[multiply+1]);
       }   
      }           


        EEPROM.write(0, 1);   // write new data to EEPROM
        for (int i = 0; i < 8; i++){
          EEPROM.write(i+1, data[i]);
         }
   }
}