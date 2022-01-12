bool netStatus() {
  int n = fona.getNetworkStatus();
  #ifdef DEBUG
  if (n == 0) Serial.println(F("NR"));
  if (n == 1) Serial.println(F("Reg"));
  if (n == 2) Serial.println(F("Src"));
  if (n == 3) Serial.println(F("NO"));
  if (n == 5) Serial.println(F("ROK"));
  #endif

if (n == 0){
  fona.setFunctionality(0); // AT+CFUN=0
  delay(3000);
  fona.setFunctionality(1); // AT+CFUN=1
  delay(200);
  }
  
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
      #ifdef DEBUG
        Serial.println(F("No F"));
      #endif
    while(1); // Don't proceed if it couldn't find the device
  }


  fonaSS.println("AT+CIPMUX=0"); // single ip
  fona.setFunctionality(0); // AT+CFUN=0
  delay(3000);
  fona.setFunctionality(1); // AT+CFUN=1
  fona.setNetworkSettings(F(APN)); // APN
  delay(200);

  fona.setPreferredMode(GSMstate); 
  delay(500);
  if (GSMstate == 38) {
    fona.setPreferredLTEMode(2);   
    fona.setOperatingBand("NB-IOT",20); 

  }
  
  fona.setNetLED(true,3,64,5000);
  delay(500);
  
  //fona.setOperatingBand("NB-IOT",20); // AT&T uses band 12
  fona.enableSleepMode(true);
  delay(200);
  fona.set_eDRX(1, 5, "1001");
  delay(200);
  fona.enablePSM(false);
  //fona.enablePSM(true, "00100001", "00100011");
  delay(200);


}  



void connectGPRS() {
unsigned long startTime=millis();  
    while (!netStatus()) {
      if (millis() - startTime >= 40000)
      {
       #ifdef DEBUG
        Serial.println(F("RstC"));
       #endif         
        powerOn(); // Power on the module
        delay(4000);
        moduleSetup(); // Establishes first-time serial comm and prints IMEI
       startTime=millis(); 
      }
      #ifdef DEBUG
        Serial.println(F("RetCON"));
      #endif 
      delay(2000); // Retry every 2s
    }
  #ifdef DEBUG  
    Serial.println(F("CON"));
  #endif 

  if (fona.enableGPRS(true)) {
  #ifdef DEBUG  
    Serial.println(F("GPRS"));
  #endif 
  }



}


void PostData() {    
if (fona.checkAT()) {  // wait untill modem is active
     #ifdef DEBUG
      Serial.println("Modem");
     #endif  
}
      
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

//  delay(500);

  if (millis() - updateBattery >= 130000 or updateBattery == 0) {  // send data about battery and signal every 8+ minutes
    updateBattery=millis();
    sig=fona.getRSSI(); 
    delay(300);
    //#ifdef OLDPCB // old pcb
      battLevel = readVcc(); // Get voltage %
    //#else         // new
     // battLevel = analogRead(A1)*0.1999;      
    //#endif    
    sendBatTemp=0;


  if (enableSolar==1){
    int curr = 0;  // measure solar cell current
    volatile unsigned currCount = 0;
    while (currCount < 10) {
      #ifdef OLDPCB // old pcb
          curr += analogRead(A0)*3.8;
      #else         // new
          curr += ((analogRead(A0)*3.98)/1000/1.15)*940;
      #endif

          currCount++;
          delay(50);
      }
    SolarCurrent=(curr/currCount)/5;  // calculate average solar current // divide with 5 so it can be send as byte

  }    
}
  else {
    sendBatTemp=sendBatTemp+1;
}

#ifdef UZ_NMEA
   if ( UltrasonicAnemo==1 ) { 
    measureCount=measureCount/10;
   }
#endif 

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
  
  
bool isConnected = fona.UDPconnected();
     #ifdef DEBUG
      Serial.print("UDP");
      Serial.println(isConnected);
     #endif
    if (isConnected ==0) {
    int8_t GPRSstate=fona.GPRSstate();

      if (GPRSstate !=1) {
          connectGPRS();
       } 
     fona.UDPconnect("vetercek.com",6789);
     }     

    else if (isConnected > 1) {
     fona.enableGPRS(false);
     connectGPRS();
     fona.UDPconnect("vetercek.com",6789);
     } 

  byte response[10];  
  if ( fona.UDPsend(data,sizeof(data),response,9)) {

  if (response[0] >0) { whenSend=response[0];}
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
  else if (response[8] == 2 or response[8]==13 or response[8]==38) { // if new settings for network prefference
    EEPROM.write(9, response[8]);   // write new data to EEPROM
    reset(3); 
    }
   
  onOffTmp=response[5];
  cutoffWind=response[6];

#ifndef UZ_NMEA  // if not old anemometer without sleep mode
 if ( UltrasonicAnemo==1 ) { 
  if ( response[7] < 4 and battLevel < 180 and battLevel > 170) { // if low battery < 3.6V
     response[7]=4;
  }
  else if ( response[7] < 8 and battLevel < 170 and battLevel > 17) { // if low battery < 3.4V
     response[7]=8;
  }
  if ( response[7]!= sleepBetween and UltrasonicAnemo==1 and response[7] > -1 and response[7] < 9 ) { //change of sleep time
      #ifdef DEBUG
      Serial.println("change sleep");
     #endif
     UZsleep(byte(response[7]));
  }

 }
#endif  

  if ( response[7]!= 0 or response[7]!= 1 or response[7]!= 2 or response[7]!= 4 or response[7]!= 8) { 
    sleepBetween=response[7];
  }  

     #ifdef DEBUG
      Serial.println("SEND");
     #endif
  AfterPost(); 
   } 

   else {
     fona.UDPclose();
     failedSend=failedSend+1;

      if (failedSend > 3) {
      #ifdef DEBUG
        Serial.println("softR");
      #endif
      powerOn(); // Power on the module
      delay(4000);
      wakeUp();
      delay(3000);
      moduleSetup(); // Establishes first-time serial comm and prints IMEI
      }
   } 
  
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
    PDPcount=0;
    failedSend=0;
    sonicError=0;
    rainCount=0;
    pressure=0;
    memset(windGust, 0, sizeof(windGust)); // empty direction array
}



// send data to server
void SendData() {
//    noInterrupts();
//    timergprs = 0;
//    interrupts();
  BeforePostCalculations();
  PostData();
//  noInterrupts();
//  timergprs = 0;
//  interrupts();
}

void checkIMEI() {
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
