void mqttCallback(char* topic, byte* payload, unsigned int len) {
  
  if (String(topic) == topicGET) {
    #ifdef DEBUG
    Serial.print("Message arrived ");
    //Serial.println(topic);
    #endif
    char payload_string[len + 1];
    memcpy(payload_string, payload, len);
    payload_string[len] = '\0';

   int num=0;
   char *token;
   token = strtok(payload_string, ",");
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
      int num6=atoi(token);  // restart arduino
      if (num6==1) {
        reset();
       } 
    } 
      
      token = strtok(NULL, ",");
   }
  
 }


}



void initMQTT() {
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
}


void reconnectMQTT() {
  while (!mqtt.connected()) {
    #ifdef DEBUG
    Serial.print("MQTT: ");
    Serial.println(mqtt.state());    
    #endif     
    initMQTT(); 

    if (mqtt.connect(id, MTTTuser, MQTTpass)) { 
    mqtt.subscribe(topicGET);    
    mqtt.subscribe("get/ws");    //backup
    mqtt.loop();

    }
    else {
      #ifdef DEBUG
        Serial.println("MQTT FAIL");
      #endif      
    }
  }
}


void checkConnections() {
  if(!modem.isGprsConnected())
    setupGSM();
  if (!mqtt.connected()) 
    reconnectMQTT();
}

void setupGSM() {
  modem.init();
  while (!modem.gprsConnect(apn)) {
    modem.restart();
    delay(3000);
  }
    #ifdef DEBUG
    Serial.println("GPRS con");
    #endif   
}





void PostData() {    
  powerOn();
  battLevel = modem.getBattPercent();
  delay(100);
  sig=modem.getSignalQuality(); 

 sprintf(URL, FORMAT,id, windDir, wind_speed / 10, wind_speed % 10, windGustAvg / 10, windGustAvg % 10, tmp, wat, battLevel, sig, measureCount,resetReason);
    //#ifdef DEBUG
      //  Serial.println(URL);
    //#endif
      checkConnections();

    if (mqtt.publish(topicPUT, URL) ) {
    #ifdef DEBUG
    Serial.println("MQTT UPLOADED");
    #endif  
    mqtt.loop();
    AfterPost(); 
    }

    else {
      checkConnections();
    }
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
