bool checkNetwork() {  
  byte GSMstatus = 99;
  unsigned long startTime = millis();  
  byte zero_network = 0;
  
  // Maximum timer of 60 seconds
  unsigned long maxTime = 60000;

  do {
    GSMstatus = netStatus();
    
    // Check if more than 20 seconds have passed with no network or forbidden status
    if ((GSMstatus == 0 or GSMstatus == 3) and (millis() - startTime) > 20000) {
      fona.setCOPS(2); // de-register
      delay(500);
      fona.setCOPS(0); // auto-register
      #ifdef DEBUG
        DEBUGSERIAL.println(F("dereg"));
      #endif
      startTime = millis(); 
    }
    
    // If no GSM signal
    else if (GSMstatus == 0) {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("nogsm"));
      #endif
      fona.setFunctionality(0); // AT+CFUN=0
      delay(3000);
      fona.setFunctionality(1); // AT+CFUN=1
      delay(10 * 1000);        
    }
    
    // If forbidden GSM status
    else if (GSMstatus == 3) {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("forbidden"));
      #endif
      fona.setPreferredMode(2);   
      startTime = millis(); 
    }
    
    // If no connection within 60 seconds
    else if (GSMstatus == 2 and millis() - startTime >= 60000) {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("NTWRK fail"));
        DEBUGSERIAL.println(GSMstatus);
      #endif
      fona.setFunctionality(0); // AT+CFUN=0
      delay(3000);
      fona.setFunctionality(1); // AT+CFUN=1     
      startTime = millis(); 
    }
    
    // General status messages
    else {
      #ifdef DEBUG
        DEBUGSERIAL.print(F("Src "));
        DEBUGSERIAL.println(GSMstatus);
      #endif     
      delay(1000);            
    }

    // Break if 60 seconds have passed
    if (millis() - startTime > maxTime) {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("Max time exceeded"));
      #endif
    reset(5);
    }
    
  } while (GSMstatus != 5 and GSMstatus != 1);

  return true; // Successfully connected to the network
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

        if (checkServernum==4 )  {
          reset(12);
        } 
             
//        else if (checkServernum==4 )  { 
//           simReset();     
//            #ifdef DEBUG_ERROR
//              resetReason=34;  
//            #endif
//        } 


       else if (checkServernum==3 ){ 
          connectGPRS(1); //check network, restart gprs
            #ifdef DEBUG_ERROR
              resetReason=33;  
            #endif
            }
              
       else if (checkServernum==2 )  {
           if (checkGPRS()==false){ 
              connectGPRS(1); //check network, restart gprs
           }
            #ifdef DEBUG_ERROR
              resetReason=32;  
            #endif 
                
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


  if (failedSend ==4) {    reset(13);}  

//  else if (failedSend ==4) {    simReset();  
//            #ifdef DEBUG_ERROR
//              resetReason=38;  
//            #endif
//  }  

  else if (failedSend ==3) {    
     connectGPRS(2); //check network, restart gprs
            #ifdef DEBUG_ERROR
              resetReason=37;  
            #endif
  } 

  else {    
     if (checkGPRS()==false){ 
        connectGPRS(1); //check network, restart gprs
     }
     delay(1000);
            #ifdef DEBUG_ERROR
              resetReason=36;  
            #endif
  } 

 
}
