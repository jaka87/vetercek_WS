byte netStatus() {
  byte n = fona.getNetworkStatus();
  #ifdef DEBUG
  if (n == 0) DEBUGSERIAL.println(F("NR"));
  if (n == 1) DEBUGSERIAL.println(F("Reg"));
  if (n == 2) DEBUGSERIAL.println(F("Src"));
  if (n == 3) DEBUGSERIAL.println(F("NO"));
  if (n == 5) DEBUGSERIAL.println(F("ROK"));
  #endif

  return n;

}


  void GSMerror() {   
  #ifdef DEBUG    
    DEBUGSERIAL.println(F("gsm error"));   
  #endif    
  //fona.enableGPRS();   
  powerOn();    
  wakeUp();   
  //digitalWrite(RESET, LOW);     
  //delay(350);   
  //digitalWrite(RESET, HIGH);    
  delay(5000);    
  moduleSetup();          
  connectGPRS();

       #ifdef DEBUG
        DEBUGSERIAL.println(F("vetercek"));
     #endif   
  fona.UDPconnect("vetercek.com",6789);   
      
}

void moduleSetup() {
  // Note: The SIM7000A baud rate seems to reset after being power cycled (SIMCom firmware thing)
  // SIM7000 takes about 3s to turn on but SIM7500 takes about 15s
  // Press reset button if the module is still turning on and the board doesn't find it.
  // When the module is on it should communicate right after pressing reset
  delay(3000);
  //fonaSS->begin(9600);
  //fona.println(F("AT+IPR=57600"); // Set baud rate
  fonaSS->begin(57600);

  while (! fona.begin(*fonaSS)) {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("No F"));
      #endif   
  }

  fona.println("AT+CIPMUX=0"); // single ip
  fona.setNetLED(true,3,64,5000);
  delay(100);
  //fona.setFunctionality(0); // AT+CFUN=0
  //delay(3000);
  //fona.setFunctionality(1); // AT+CFUN=1
  fona.setNetworkSettings(F(APN)); // APN
  delay(100);

  fona.setPreferredMode(GSMstate); 
  delay(100);
//  if (GSMstate == 38) {
//    fona.setPreferredLTEMode(2);   
//    fona.setOperatingBand("NB-IOT",20); 
//  }
  
  //fona.setOperatingBand("NB-IOT",20); // AT&T uses band 12
  fona.enableSleepMode(true);
  delay(100);
  if (GSMstate==38) {
    fona.set_eDRX(1, 5, "1001");    
  }
  delay(100);
  //fona.enablePSM(false);
  //fona.enablePSM(true, "00100001", "00100011");
  delay(100);
}  



void connectGPRS() {

int8_t info=fona.getNetworkInfo();  // check if connected to network, else try 2G
if ((info <1 or info > 10) and GSMstate==2)  {
  fona.setPreferredMode(13);   
}

unsigned long startTime=millis();  
byte runState=0;

    while (netStatus() != 5 and netStatus() != 1) {
      if (millis() - startTime >= 8000 and netStatus() == 0 and runState==0)  {
       #ifdef DEBUG
        DEBUGSERIAL.println(F("err RstC1"));
        DEBUGSERIAL.println(netStatus());
       #endif
      moduleSetup();
      runState=1;
      }
       
      else if (millis() - startTime >= 20000 and netStatus() == 0 and runState==1)  {
       #ifdef DEBUG
        DEBUGSERIAL.println(F("err RstC2"));
       #endif
      GSMerror();
      runState=0;
      startTime=millis(); 
      }

      #ifdef DEBUG
        DEBUGSERIAL.println(F("RetCON"));
      #endif 
    delay(1000);  
    }


  if (fona.enableGPRS()) {
  #ifdef DEBUG  
    DEBUGSERIAL.println(F("GPRS"));
  #endif 
  }
}


void PostData() {    

  int8_t GPRSPDP=fona.GPRSPDP();  //check PDP
  int8_t GPRSstate=fona.GPRSstate();  //check GPRS

  if (GPRSstate !=1 or GPRSPDP !=1) {    // if no connection with network
       #ifdef DEBUG
        DEBUGSERIAL.println(F("err PDP"));
        DEBUGSERIAL.print(F("PDP "));
        DEBUGSERIAL.println(GPRSPDP);
        DEBUGSERIAL.print(F("GPRS "));
        DEBUGSERIAL.println(GPRSstate);      
       #endif
       connectGPRS();
   } 

//  sig=fona.getRSSI(); 
//  #ifdef DEBUG  
//    DEBUGSERIAL.println(F("NET"));
//    DEBUGSERIAL.println(sig);
//  #endif 
//
//  if (sig < 5 or sig >50) {    // if no connection with network
//       #ifdef DEBUG
//        DEBUGSERIAL.println(F("err sig"));     
//       #endif
//   } 
   

  unsigned long startTime=millis();      
  byte isConnected = fona.UDPconnected();  // UDP connection to server
       #ifdef DEBUG
        DEBUGSERIAL.print(F("UDP "));
        DEBUGSERIAL.println(isConnected);
       #endif

    if (isConnected ==10 ) { // if  PDP deact
       connectGPRS();
    } 
     #ifdef DEBUG
        DEBUGSERIAL.println(F("vetercek"));
     #endif
      while (fona.UDPconnect("vetercek.com",6789)==false and millis() - startTime <= 120000) {
        fona.UDPconnect("vetercek.com",6789);
          #ifdef DEBUG
            DEBUGSERIAL.println(F("err no_vet"));
         #endif
        delay(30*1000);
      } 
      if (millis() - startTime >= 120000 )  {
        #ifdef DEBUG
        DEBUGSERIAL.println(F("err vet_err"));
        #endif
        GSMerror();
      }        
     // }     

    //else  {
     //GSMerror();
     //} 


     
  //if (millis() - updateBattery >= 130000 or updateBattery == 0) {  // send data about battery and signal every 8+ minutes
    //updateBattery=millis();
    sig=fona.getRSSI(); 
    //delay(300);
    //#ifdef OLDPCB // old pcb
      battLevel = readVcc(); // Get voltage %
    //#else         // new
     // battLevel = analogRead(A1)*0.1999;      
    //#endif    
    //sendBatTemp=0;


  if (enableSolar==1){
    int curr = 0;  // measure solar cell current
    volatile unsigned currCount = 0;
    while (currCount < 10) {
          curr += ((analogRead(A0)*3.98)/1000/1.15)*940; //2.2k resistor
          currCount++;
          delay(20);
      }
    SolarCurrent=(curr/currCount)/5;  // calculate average solar current // divide with 5 so it can be send as byte

    }    
  //}
    //else {
    //  sendBatTemp=sendBatTemp+1;
  //}
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


  if (temp > 0) { // if positive or negative air temperature
    data[14]=1;
  } 
  else {
    data[14]=0;
  } 

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

  #ifdef BMP
    if (enableBmp==1) { // if send pressure value
      data[25]=pressure/100;
      data[26]=pressure%100;
    } 
  #else
  data[25]=sonicError;    
  #endif 


  byte response[10];  
  if ( fona.UDPsend(data,sizeof(data),response,9)) {

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
  else if (response[8] == 102 ) { GSMstate=2; moduleSetup(); } // temporarry change network - auto
  else if (response[8] == 113 ) { GSMstate=13; moduleSetup(); } // temporarry change network - 2G
  else if (response[8] == 138 ) { GSMstate=38; moduleSetup(); } // temporarry change network - nb-iot
  else if (response[8] == 2 or response[8]==13 or response[8]==38) { // if new settings for network prefference
    EEPROM.write(9, response[8]);   // write new data to EEPROM
    reset(3); 
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
      DEBUGSERIAL.println(F("SEND"));
     #endif
  
  AfterPost(); 
   } 

   else {  //if cannot send data to vetercek.com
     fona.UDPclose();
     failedSend=failedSend+1;
     delay(5000);

      if (failedSend > 1 and failedSend < 2) {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("err RstC3"));
      #endif
      moduleSetup(); // Establishes first-time serial comm and prints IMEI
      }
      
      else if (failedSend > 1 ) {
        GSMerror();
      }      
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
    memset(windGust, 0, sizeof(windGust)); // empty direction array
}



// send data to server
void SendData() {
  BeforePostCalculations();
  PostData();
}

void checkIMEI() {
  char IMEI[15]; // Use this for device ID
   if (EEPROM.read(0)==1) {  // read from EEPROM if data in it 
      for (int i = 0; i < 8; i++){
       data[i]=EEPROM.read(i+1);
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
