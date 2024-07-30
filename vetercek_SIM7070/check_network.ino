bool checkNetwork() {  
  byte GSMstatus=99;
  unsigned long startTime=millis();  
  byte zero_network=0;
    do {
      GSMstatus=netStatus();
      if ((GSMstatus==0 or GSMstatus==3) and (millis() - startTime) > 20000) {
            dropConnection(1); //drop connection  
            #ifdef DEBUG
              DEBUGSERIAL.println(F("dereg"));
            #endif
            startTime=millis(); 
      }
      
      else if (GSMstatus==0) {
          #ifdef DEBUG
            DEBUGSERIAL.println(F("nogsm"));
          #endif
          fona.setFunctionality(0); // AT+CFUN=0
          delay(3000);
          fona.setFunctionality(1); // AT+CFUN=1
          delay(10*1000);        
      }
      else if (GSMstatus==3) {
          #ifdef DEBUG
            DEBUGSERIAL.println(F("forbidden"));
          #endif
          fona.setPreferredMode(2);   
          startTime=millis(); 
      }
      else if (GSMstatus==2 and millis() - startTime >= 60000) {
          #ifdef DEBUG
            DEBUGSERIAL.println(F("NTWRK fail"));
            DEBUGSERIAL.println(GSMstatus);
          #endif
          fona.setFunctionality(0); // AT+CFUN=0
          delay(3000);
          fona.setFunctionality(1); // AT+CFUN=1     
          startTime=millis(); 
      }
      else {
        #ifdef DEBUG
          DEBUGSERIAL.print(F("Src "));
          DEBUGSERIAL.println(GSMstatus);
        #endif     
         delay(1000);            
       }
    } 
    while (GSMstatus !=5 and GSMstatus !=1 );
    return true;
}


void tryGPRS() {
    int attempts = 0;
    const int maxAttempts = 3;
    bool success = false;

    if (checkGPRS() ) { return;} //break function
    while (attempts < maxAttempts && !success) {
        success = checkGPRS();
        attempts++;
        dropConnection(0);
        fona.enableGPRS(true);        
        delay(2000);
    }

    if (!success) {
        reset(11);
    }
}

bool checkGPRS() {
  if (fona.GPRSstate()!=1)  { 
     #ifdef DEBUG
        DEBUGSERIAL.println(F("GPRS_NI1"));
     #endif     
    return false; 
    }
  if (!fona.checkPDP())  { 
         #ifdef DEBUG
        DEBUGSERIAL.println(F("GPRS_NI2"));
     #endif     
    return false; 
    }
     #ifdef DEBUG
        DEBUGSERIAL.println(F("GPRS OK"));
     #endif   
  return true;
}

bool checkServer() {
  unsigned long startTime=millis();    
  bool conn=false;
  byte checkServernum=0;
  conn=fona.UDPconnect(broker,6789);
  
  if (conn== false){ // cant connect
    delay(200);
    do {
        checkServernum=checkServernum+1;
        conn=fona.UDPconnect(broker,6789);
  
        if (checkServernum>0 and conn== false)  { 
           delay( 2000*checkServernum); 
           checkNetwork();
           tryGPRS();
           #ifdef DEBUG
              DEBUGSERIAL.println(F("vet_con_fail"));
           #endif 
        } 
      
        if (checkServernum==4 )  {
          #ifdef DEBUG
            DEBUGSERIAL.println(F("srvrF 3"));
          #endif
          reset(12);
        } 
  
    } 
    while (conn == false and checkServernum < 6);
  } 
  
//must wait couple seconds before sending data - otherwise UDPsend3 error
sig=fona.getRSSI(); 
battLevel = readVcc(); // Get voltage %
#ifdef DEBUG
  DEBUGSERIAL.println(F("SRVR_OK"));
#endif     
}

void fail_to_send() {     //if cannot send data to vetercek.com
  fona.UDPclose();
  
  if (failedSend ==4) {       
      //GSMerror();
      reset(6);
  }   
  if (failedSend ==3) {       
    dropConnection(1); 
    checkNetwork();
    tryGPRS();  
  }  
  
  else if (failedSend == 2) {          
    dropConnection(0);
    fona.enableGPRS(true);
  } 

  else { tryGPRS();  } 

#ifdef DEBUG
  DEBUGSERIAL.print(F("Fail_Send"));
  DEBUGSERIAL.println(failedSend);
#endif  
failedSend=failedSend+1;
}
