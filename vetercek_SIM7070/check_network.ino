bool checkNetwork() {  
  byte GSMstatus=99;
  unsigned long startTime=millis();  
  byte zero_network=0;
    do {
      GSMstatus=netStatus();
      if ((GSMstatus==0 or GSMstatus==3) and (millis() - startTime) > 20000) {
            fona.setCOPS(2); //de-register
            delay(500);
            fona.setCOPS(0); //auto            
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
    const int maxAttempts = 10;
    bool success = false;

    #ifdef DEBUG
      DEBUGSERIAL.println(F("try gprs"));
    #endif 
    if (checkGPRS() ) { return;} //break function
    while (attempts < maxAttempts && !success) {
        dropConnection(0);
        delay(500);
        success = checkGPRS();
        attempts++;
        fona.enableGPRS(true);        
        delay(1000);
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
  conn=fona.UDPconnect(broker,6789);
  
  if (conn== false){ // cant connect
    int count = 0;
    delay(1000);
    while (count < 2) {
      conn = fona.UDPconnect(broker, 6789);
      if (conn) {
        return true; // Connection successful
      }
      count++;
      delay(2000); // Optional: Wait 2 second before retrying
    }
  }

  if (conn== false){ // cant connect
        checkServernum=checkServernum+1;
     #ifdef DEBUG
        DEBUGSERIAL.print(F("vet_con_fail"));
        DEBUGSERIAL.println(checkServernum);
     #endif 
             
        if (checkServernum==3 )  { 
           simReset();     
           resetReason=33;       
        } 
              
        else if (checkServernum==4 )  {
          reset(12);
        } 
        else if (checkServernum==2 )  {
          connectGPRS();
           resetReason=32;       
        } 
     
    return false;  
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
  failedSend=failedSend+1;

#ifdef DEBUG
  DEBUGSERIAL.print(F("Fail_Send"));
  DEBUGSERIAL.println(failedSend);
#endif 


  if (failedSend ==4) {    
     reset(13);
  }  

  else if (failedSend ==3) {    
     simReset();
     resetReason=36;
  }  

  else if (failedSend ==2) {    
     connectGPRS();
     resetReason=35;
  } 

  else  {       
    delay(500);
    resetReason=34;       

  }  
  
 
}
