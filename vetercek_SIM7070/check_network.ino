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




bool checkGPRS() { //check if gprs connected
  bool checkAT = fona.checkAT(); // first thing to send to module after wake up
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

bool checkServer() { // try connecting to server
  unsigned long startTime=millis();    
  bool conn=false;
  if (checkGPRS()==false){ connectGPRS(1);  }  //check network, restart gprs
  conn=fona.UDPconnect(broker,6789);
  
  if (conn== false){ // cant connect
    int count = 0;
    delay(500);
    while (count < 5) {
      conn = fona.UDPconnect(broker, 6789);
      if (conn) {
        return true; // Connection successful
      }
      count++;
      delay(500); // Optional: Wait 0.5 second before retrying
    }
  }

  if (conn== false){ // cant connect
        checkServernum=checkServernum+1;
     #ifdef DEBUG
        DEBUGSERIAL.print(F("vet_con_fail"));
        DEBUGSERIAL.println(checkServernum);
     #endif 

        if (checkServernum==5 )  {
          reset(12);
        } 
             
        else if (checkServernum==4 )  { 
           simReset();     
           //resetReason=34;       
        } 


       else if (checkServernum==3 ){ 
          connectGPRS(1); //check network, restart gprs
           //resetReason=33;       
       }
              
       else if (checkServernum==2 )  {
           if (checkGPRS()==false){ 
              connectGPRS(1); //check network, restart gprs
           }
           //resetReason=32;       
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


  if (failedSend ==5) {    reset(13);}  

  else if (failedSend ==4) {    simReset();  
  //resetReason=38; 
  }  

  else if (failedSend ==3) {    
     connectGPRS(2); //check network, restart gprs
     //resetReason=37;
  } 

  else {    
     if (checkGPRS()==false){ 
        connectGPRS(1); //check network, restart gprs
     }
     delay(1000);
     //resetReason=36;
  } 

 
}
