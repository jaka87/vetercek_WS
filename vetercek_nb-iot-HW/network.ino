void moduleSetup() {
Serial.begin(57600);
while (!Serial) {
  ; // wait for serial port to connect. Needed for native USB
}

if (!fona.begin(Serial)) {
  #ifdef DEBUG
    DEBUGSERIAL.println(F("NoF"));
  #endif 
  while (1);
}


  fona.println("AT+CIPMUX=0"); // single ip
  fona.setNetLED(true,3,64,5000);
  delay(100);
  fona.setNetworkSettings(F(APN)); // APN
  delay(100);

  fona.setPreferredMode(GSMstate); 
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
}  

byte netStatus() {
  byte n = fona.getNetworkStatus();
  return n;
}


void GSMerror(byte what) {      
  if (what==1) {
    #ifdef DEBUG    
      DEBUGSERIAL.println(F("gerr1"));   
    #endif 
      simReset();
      moduleSetup();
  }
  else {
    #ifdef DEBUG    
      DEBUGSERIAL.println(F("gerr0"));   
    #endif     
    fona.setFunctionality(0); // AT+CFUN=0
    delay(3000);
    fona.setFunctionality(1); // AT+CFUN=1        
  }

  delay(5000);     
  if (what<3) {
    moduleSetup();          
    connectGPRS();  
  }      
}


bool checkNetwork() {  
byte GSMstatus=99;
unsigned long startTime=millis();  

  do {
    GSMstatus=netStatus();
    if (GSMstatus==0 or GSMstatus==3) {
        #ifdef DEBUG
          DEBUGSERIAL.println(F("nogps"));
        #endif
        GSMerror(0);
        delay(10*1000);        
    }
    else {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("Src"));
      #endif     
      if (millis() - startTime >= 100 )  {
       delay(500);             
      }
     }
  } 
  while (GSMstatus !=5 and GSMstatus !=1 );
  return true;    
}

bool checkGPRS() {
  unsigned long startTime=millis();  
  byte GPRS=99;
  byte PDP=99;
  byte count=0;
  do {
    if (millis() - startTime >= 119000 )  {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("GPRSerr"));
      #endif
      reset(6);
    } 
    else if (millis() - startTime < 120000  and count==1)  {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("rGPRS"));
      #endif    
      delay(3000);
      fona.enableGPRS();
    } 
    else if (millis() - startTime < 120000 and count > 1)  {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("rGPRS2"));
      #endif    
      GSMerror(0);
      connectGPRS();
    } 
      GPRS=fona.GPRSstate();
      PDP=fona.GPRSPDP();
      count++;
      #ifdef DEBUG
        DEBUGSERIAL.println(GPRS);
        DEBUGSERIAL.println(PDP);
      #endif 
  }
  while (GPRS!=1 and PDP!=1 and millis() - startTime <= 120000);
  count=0;
  #ifdef DEBUG
    DEBUGSERIAL.println("GPRS");
  #endif  
}


bool checkServer() {
  unsigned long startTime=millis();    
  bool conn=false;
  do {
        if (millis() - startTime > 500 and millis() - startTime < 30*1000) {
            delay(5*1000);
         }
         else if (millis() - startTime > 30*1000) {
            delay(15*1000);        
         }
      conn=fona.UDPconnect("vetercek.com",6789);
     #ifdef DEBUG
        DEBUGSERIAL.print(F("vet "));
        DEBUGSERIAL.println(conn);
     #endif   
   
  } 
  while (conn == false and millis() - startTime < 120*1000);
  
  if (millis() - startTime >= 120*1000 )  {
    #ifdef DEBUG
    DEBUGSERIAL.println(F("veterr"));
    #endif
    reset(5);
  }    
}



void connectGPRS() {
  bool GPRS=false;
  int8_t info=fona.getNetworkInfo();  // check if connected to network, else try 2G
  if ((info <1 or info > 10) and GSMstate==2)  {
   #ifdef DEBUG
      DEBUGSERIAL.println(F("n2g"));
   #endif 
    fona.setPreferredMode(13);   
  }
  
  checkNetwork();
  unsigned long startTime=millis();    

  do {
    if (millis() - startTime < 60000 and millis() - startTime > 500)  {
      #ifdef DEBUG
      DEBUGSERIAL.println(F("gprserr"));
      #endif
      delay(10000);
      GSMerror(1);
    }      
    GPRS=fona.enableGPRS();
    #ifdef DEBUG
      DEBUGSERIAL.print(F("GPRS "));
      DEBUGSERIAL.println(GPRS);
    #endif 
  }
  while (GPRS==false and millis() - startTime >= 60000);
}



void PostData() {        
  //if (millis() - updateBattery >= 40000 or updateBattery == 0) {  // send data about battery and signal every x
   // updateBattery=millis();
    sig=fona.getRSSI(); 
    battLevel = readVcc(); // Get voltage %   

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
     delay(3000);
      
      if (failedSend > 2 ) {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("errRC4"));
      #endif        
        GSMerror(1);
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
  checkNetwork();
  checkGPRS();
  checkServer();
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
