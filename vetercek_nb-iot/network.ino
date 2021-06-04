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

fonaSS.begin(9600);
  
  //Serial.println(F("Configuring to 9600 baud"));
  fonaSS.println("AT+IPR=9600"); // Set baud rate
  delay(100); // Short pause to let the command run
  fonaSS.begin(9600);
  if (! fona.begin(fonaSS)) {
      #ifdef DEBUG
        Serial.println(F("Couldn't find FONA"));
      #endif
    while(1); // Don't proceed if it couldn't find the device
  }


  fonaSS.println("AT+CIPMUX=0"); // Set baud rate
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
        Serial.println(F("Restart connection..."));
       #endif         
        powerOn(); // Power on the module
        delay(4000);
        moduleSetup(); // Establishes first-time serial comm and prints IMEI
       startTime=millis(); 
      }
      #ifdef DEBUG
        Serial.println(F("Failed to connect to cell network, retrying..."));
      #endif 
      delay(2000); // Retry every 2s
    }
  #ifdef DEBUG  
    Serial.println(F("Connected to cell network!"));
  #endif 

  if (fona.enableGPRS(true)) {
  #ifdef DEBUG  
    Serial.println(F("Enabled data"));
  #endif 
  }



}


void PostData() {    
if (fona.checkAT()) {  // wait untill modem is active
     #ifdef DEBUG
      Serial.println("Modem active");
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

  if (sendBatTemp >= 10) {  // send data about battery and signal every 10 measurements
    sig=fona.getRSSI(); 
    delay(300);
    battLevel = readVcc(); // Get voltage %
    sendBatTemp=0;
}
  else {
    sendBatTemp=sendBatTemp+1;
}

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
     #ifdef DEBUG
      Serial.print("UDP STATUS ");
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
     failedSend=failedSend+1;

      if (failedSend > 3) {
      #ifdef DEBUG
        Serial.println("soft reset");
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
    memset(windGust, 0, sizeof(windGust)); // empty direction array
}



// send data to server
void SendData() {
#ifdef DEBUG
  Serial.print("TIMER ");
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
