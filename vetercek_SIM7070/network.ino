void moduleSetup() {
Serial.begin(57600);

bool zagnano=fona.begin(Serial);


  if (!zagnano ){
    reset(9);
  }

    
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

  //if (GSMstate == 38 or GSMstate == 51) {
    //fona.setPreferredLTEMode(2);   
    //fona.setOperatingBand("NB-IOT",20); 
    //fona.set_eDRX(1, 5, "1001");    
  //}

//fona.println(F("AT+CMEE=1"));  //extend debugging
#ifdef DEBUG
  DEBUGSERIAL.println(F("modOK"));
#endif
}  



void changeNetwork_id(int network, byte technology) {
  //fona.setPreferredMode(51);
 fona.setNetwork(network,technology); 
  #ifdef DEBUG
    DEBUGSERIAL.println(F("network change"));
  #endif 
  //EEPROM.write(9, 51);
  delay(7000);
  connectGPRS();
}


byte netStatus() {
  byte n = fona.getNetworkStatus();
  return n;
}


void GSMerror() {      
    #ifdef DEBUG    
      DEBUGSERIAL.println(F("gerr1"));   
    #endif 
    bool checkAT = fona.checkAT();
    delay(50);
    dropConnection(1);
    if (fona.checkAT()) { simReset(); }
    else { reset(10); }

}


void connectGPRS() {
  bool GPRS=false;
  checkNetwork();
  unsigned long startTime=millis();    

  while (!GPRS && (millis() - startTime) < 30000) {
    #ifdef DEBUG
      DEBUGSERIAL.println(F("GPRS try"));
    #endif 
      dropConnection(0);
      delay(500);
      GPRS = fona.enableGPRS(true);
      delay(500);
  }

  if (!GPRS)  {
    #ifdef DEBUG
      DEBUGSERIAL.println(F("GPRS fail"));
    #endif     
    simReset();
  }
  
  else {
    #ifdef DEBUG
      DEBUGSERIAL.print(F("GPRS "));
      DEBUGSERIAL.println(GPRS);
    #endif 
  }
}

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

void PostData() {           

  if (measureCount < 5){ 
    if (EEPROM.read(39)==1 and EEPROM.read(62)>= 5){
         #ifdef DEBUG
          DEBUGSERIAL.println("EEPROM data");
          DEBUGSERIAL.println(EEPROM.read(62));
        #endif 
        const int dataSize = sizeof(data) / sizeof(data[0]);
        const int eepromStartAddress = 40; 
        for (int i = 0; i < dataSize; i++) {
          data[i] = EEPROM.read(eepromStartAddress + i);
        }
        EEPROM.write(39, 0); // do not read from eeprom
        data[23]=resetReason;
      }
     else  {gatherData();}
    }

  else  {gatherData();}



  byte response[13];  

 //try to send data 

  byte udp_send=fona.UDPsend(data,sizeof(data),response,26);
    #ifdef DEBUG 
      delay(20);
      DEBUGSERIAL.print("UDPsend"); 
      DEBUGSERIAL.println(udp_send); 
      delay(20);
    #endif 
if ( udp_send==1) { 
      
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
    if ( (response[7] < 4 and battLevel < 180 and battLevel > 170) or (batteryState==1 and response[7] < 4)) { // if low battery < 3.6V
       response[7]=4;
       batteryState=1;
    }
    else if (( response[7] < 8 and battLevel < 170 and battLevel > 17) or (batteryState==2 and response[7] < 8)) { // if low battery < 3.4V
       response[7]=8;
       batteryState=2;
    }

  // once battery gets charged change the battery state  
    if (  battLevel > 190 and batteryState==1) { batteryState=0; }// if battery > 3.8V
    else if (  battLevel >= 180 and battLevel >= 190 and batteryState==2) { batteryState=1; }// if battery > 3.6V
    
  
  
  #ifdef UZ_Anemometer
    if ( response[7]!= sleepBetween and UltrasonicAnemo==1 and response[7] > -1 and response[7] < 9 and sleepBetween != response[7]) { //change of sleep time
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


     #ifdef DEBUG
      DEBUGSERIAL.println(F("-->"));
     #endif
  
  AfterPost(); 
   } 

   else {
       fail_to_send();
   } 
  
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
    PDPcount=0;
    failedSend=0;
    sonicError=0;
    rainCount=0;
    pressure=0;
    humidity=0;
    checkServernum=0;
    memset(windGust, 0, sizeof(windGust)); // empty direction array

}





// send data to server
void SendData() {
  if (failedSend==0 and checkServernum==0){  BeforePostCalculations(1); }
  else {  BeforePostCalculations(0); }
  if (checkServer()) {  PostData(); }
  
}

void dropConnection(byte drop_type) { // 1 - full drop cnnection, 0 only drop gprs
      #ifdef DEBUG                                 
    DEBUGSERIAL.println("drp con start");
  #endif
  fona.activatePDP(0);  
  fona.enableGPRS(false);  
  if (drop_type==1){ 
    fona.setCOPS(2); //de-register
    delay(200);
    fona.setCOPS(0); //auto
    checkNetwork(); // wait till new network connection
    fona.setNetworkSettings(F(APN)); // after connection to new network APN shoud be entered

  } 
  delay(350);
      #ifdef DEBUG                                 
    DEBUGSERIAL.println("drp con stop");
  #endif
}



void checkIMEI() {
  char IMEI[15]; // Use this for device ID
   if (EEPROM.read(0)==1 and EEPROM.read(1)!=240) {  // read from EEPROM if data in it 
      for (int i = 0; i < 8; i++){
       data[i]=EEPROM.read(i+1);
        //#ifdef DEBUG                                 
        //  DEBUGSERIAL.println(EEPROM.read(i+1));
        //#endif
       }
   }
   
   else {  // read from SIM module
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
