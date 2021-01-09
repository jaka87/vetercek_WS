bool netStatus() {
  int n = fona.getNetworkStatus();
  
  Serial.print(F("Network status ")); Serial.print(n); Serial.print(F(": "));
  if (n == 0) Serial.println(F("Not registered"));
  if (n == 1) Serial.println(F("Registered (home)"));
  if (n == 2) Serial.println(F("Not registered (searching)"));
  if (n == 3) Serial.println(F("Denied"));
  if (n == 4) Serial.println(F("Unknown"));
  if (n == 5) Serial.println(F("Registered roaming"));

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
    if (num==1) {
      whenSend=atoi(token); // how offten to send data
    }

    else if (num==2) {
      vaneOffset=atoi(token);  // wind vane offset
    }    

    else if (num==3) {
      windDelay=atoi(token);  // interval for wind measurement
    } 

    else if (num==4) {
      onOffTmp=atoi(token);  // turn on / off temperature measurement
    } 

    else if (num==5) {
      cutoffWind=atoi(token);  // cuttoff wind treshold
    } 

    else if (num==6) {
      sleepBetween=atoi(token);  // sleep between wind measurements
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





void connectGPRS() {
    while (!netStatus()) {
      Serial.println(F("Failed to connect to cell network, retrying..."));
      delay(2000); // Retry every 2s
    }
    Serial.println(F("Connected to cell network!"));

#ifdef SEND_MQTT
    // Open wireless connection if not already activated
    if (!fona.wirelessConnStatus()) {
      while (!fona.openWirelessConnection(true)) {
        Serial.println(F("Failed to enable connection, retrying..."));
        delay(2000); // Retry every 2s
      }
      Serial.println(F("Enabled data!"));
    }

#else
  if (fona.enableGPRS(true)) {
  Serial.println(F("Enabled data"));
  }
#endif 

}


void PostData() {    

  //powerOn();
  delay(300);
  sig=fona.getRSSI(); 
  delay(100);
  battLevel = readVcc(); // Get voltage %

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
 sprintf(URL, FORMAT,IMEI, windDir, wind_speed / 10, wind_speed % 10, windGustAvg / 10, windGustAvg % 10, tmp, wat, battLevel, sig, measureCount,resetReason);
    #ifdef DEBUG
      Serial.println(URL);
    #endif

bool isConnected = fona.UDPconnected();

    if (isConnected !=1) {
        fona.UDPconnect("vetercek.com",6789);
      }     

  char response[50];  
  fona.UDPsend(URL,strlen(URL),response);
  Serial.println(response);
  parseResponse(response);
  AfterPost(); 
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
    Serial.println(F("complete HTTP POST..."));
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
    water = 0;
    temp = 0;
    windAvgX = 0;
    windAvgY = 0;
    resetReason=0;
    memset(windGust, 0, sizeof(windGust)); // empty direction array
    memset(tmp, 0, sizeof(tmp));
    memset(wat, 0, sizeof(wat));
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
