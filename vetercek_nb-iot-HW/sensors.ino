#ifdef UZ_Anemometer
void UltrasonicAnemometer() { //measure wind speed
    char buffer[70];
    char hexbuffer[5];
    int sum;
    unsigned long startedWaiting = millis();
             
      while(ultrasonic.available() < 63 ) {
        delay(10);
      }  
      int size = ultrasonic.readBytesUntil('\r\n', buffer, 70);
      buffer[size]='\0'; 

      delay(20); // important in case of error

       #ifdef DEBUG 
       delay(20);
        DEBUGSERIAL.print(size); 
        DEBUGSERIAL.print(F(" buff ")); 
        DEBUGSERIAL.println(buffer); 
       delay(20);
       #endif 
          
      char *dir = strtok(buffer, ",/");
      char *wind = strtok(NULL, ",/");
      char* check = strtok(NULL, ",/");

//       #ifdef DEBUG 
//       delay(20);
//        DEBUGSERIAL.println(dir); 
//        DEBUGSERIAL.println(wind); 
//        DEBUGSERIAL.println(check); 
//       delay(70);
//       #endif 

      if( check!=NULL and wind!=NULL and dir!=NULL)  {    
        sum+=countBytes(dir);
        sum+=countBytes(wind);
        sum+=2605;
        sum=-(sum % 256);    
        sprintf(hexbuffer,"%02X", sum);
    
        if( check[0] ==hexbuffer[2] and check[1] ==hexbuffer[3] )  {  
              calDirection = atoi(dir) + vaneOffset;
              CalculateWindDirection();  // calculate wind direction from data
              windSpeed=atof(wind)*19.4384449;
              CalculateWindGust(windSpeed);
              CalculateWind();
              timergprs = 0;                                            
        }
        else { 
          UZerror(3); 
          }        
      }
      else { 
        UZerror(2); 
        }      

ultrasonicFlush();
}

int countBytes( const char * data )
{
  int total = 0;
  const char * p = data;
  while ( *p != '\0' )
  {
    total += *p++;
  }
  return total;
}

void UZerror(byte where) { //ultrasonic error
  if ( sonicError>0){
      ultrasonic.end();
      delay(2000);
      unsigned long startedWaiting = millis();
      UZ_wake(startedWaiting);
  }
  sonicError++;
  #ifdef DEBUG
      DEBUGSERIAL.print(F("err UZ "));
      DEBUGSERIAL.println(where);
  #endif
if ( sonicError >=5)  { reset(4);  }   // if more than x US errors 
}

void UZsleep(byte sleepT) { //ultrasonic anemometer sleep mode
  unsigned long startedWaiting = millis();   
  char buffer[70];

#ifdef DEBUG
    DEBUGSERIAL.println(F("UZzzz"));
    delay(20);
#endif

  byte trow_away;
  trow_away=(ultrasonic.read());
  //while (ultrasonic.read() != ':') {  }
  int size = ultrasonic.readBytesUntil('\n', buffer, 70);
  buffer[size]='\0'; 

  while (strstr (buffer,"IdleSec") == NULL && millis() - startedWaiting <= 20000) {
    trow_away=(ultrasonic.read());
    //while (ultrasonic.read() != ':') {  }
    size = ultrasonic.readBytesUntil('\n', buffer, 70); 
    buffer[size]='\0';   
    if (sleepT==1) { ultrasonic.write(">PwrIdleCfg:1,1\r\n"); }
    else if (sleepT==2) { ultrasonic.write(">PwrIdleCfg:1,2\r\n"); }
    else if (sleepT==3) { ultrasonic.write(">PwrIdleCfg:1,3\r\n"); }
    else if (sleepT==4) { ultrasonic.write(">PwrIdleCfg:1,4\r\n"); }
    else if (sleepT==5) { ultrasonic.write(">PwrIdleCfg:1,5\r\n"); }
    else if (sleepT==6) { ultrasonic.write(">PwrIdleCfg:1,6\r\n"); }
    else if (sleepT==7) { ultrasonic.write(">PwrIdleCfg:1,7\r\n"); }
    else if (sleepT==8) { ultrasonic.write(">PwrIdleCfg:1,8\r\n"); }  
    else if (sleepT==0) { ultrasonic.write(">PwrIdleCfg:0,1\r\n"); }
    delay(300);
     
    }

    if(millis() - startedWaiting < 19900){ ultrasonic.write(">SaveConfig\r\n"); }
    if(millis() - startedWaiting < 19900){ 
      sleepBetween=sleepT;
      changeSleep=0;
      stopSleepChange=0;
     #ifdef DEBUG 
      DEBUGSERIAL.print(F("sleepcok ")); 
      DEBUGSERIAL.println(sleepT); 
      delay(10);
     #endif 
      }
    else { 
     stopSleepChange++;
     #ifdef DEBUG 
      DEBUGSERIAL.println(F("err sleepc")); 
     #endif       
      }
}
#endif 

#ifndef UZ_Anemometer
  void Anemometer() { //measure wind speed
    float actualWindDelay; //time between first and last measured anemometer rotation
    firstWindPulse = 1; // dont count first rotation
    contactBounceTime = millis();
    rotations = 0; // Set rotations count to 0 ready for calculations
    EIFR = (1 << INTF0); // clear interrupt flag
    attachInterrupt(digitalPinToInterrupt(windSensorPin), ISRrotation, FALLING); //setup interrupt on anemometer input pin, interrupt will occur whenever falling edge is detected
    delay (windDelay); // Wait x second to average
    detachInterrupt(digitalPinToInterrupt(windSensorPin));
  
  if (rotations == 0)  {
    windSpeed = 0;
  }
  else  {
    actualWindDelay = (lastPulseMillis - firstPulseMillis);
    windSpeed = rotations * (2250 / actualWindDelay) * 0.868976242 * 10 ; // convert to mp/h using the formula V=P(2.25/Time);
    // 2250 instead of 2.25 because formula is in seconds not millis   & * 0.868976242 to convert in knots   & *10 so we can calculate decimals later
  }

  if (windSpeed < 600) { // delete data larger than 60KT
      CalculateWind();
      CalculateWindGust(windSpeed);
  }

}
#endif

void CalculateWind() {  
    ++measureCount; // add +1 to counter
    windAvr += windSpeed; // add to sum of average wind values
}


void CalculateWindGust( int w ) {
      if (w >= windGust[2]) { // check if > than old gust3 of wind
        windGust[0] = windGust[1];
        windGust[1] = windGust[2];
        windGust[2] = w;
      }
    
      else if (w >= windGust[1]) { // check if > than old gust2 of wind
        windGust[0] = windGust[1];
        windGust[1] = w;
      }
    
      else if (w > windGust[0]) { // check if > than old gust1 of wind
        windGust[0] = w;
      }
      windGustAvg = (windGust[0] + windGust[1] + windGust[2]) / 3;
}

void GetAvgWInd() {
  wind_speed = windAvr / measureCount; // calculate average wind
}


#ifndef UZ_Anemometer
  void ISRrotation () {  // This is the function that the interrupt calls to increment the rotation count
  currentMillis = millis(); //we have to read millis at the same position in ISR each time to get the most accurate readings
  if (firstWindPulse == 1) { //discard first pulse as we don't know exactly when it happened
    contactBounceTime = currentMillis;
    firstWindPulse = 0;
    firstPulseMillis = currentMillis;
  }
  else if ((currentMillis - contactBounceTime) > 15 ) { // debounce the switch contact.
    rotations++;
    contactBounceTime = currentMillis;
    lastPulseMillis = currentMillis;
  }
}
#endif

void DominantDirection() { // get dominant wind direction
  windDir =  atan2(windAvgY, windAvgX) / PI * 180;
  if (windDir < 0) windDir += 360;
}

// Get Wind direction, and split it in 16 parts and save it to array
void GetWindDirection() {
  vaneValue = analogRead(PIN_A3);
  direction = map(vaneValue, 0, 1023, 0, 360);
  calDirection = direction + vaneOffset;
  CalculateWindDirection();
}

void CalculateWindDirection() {
   // convert reading to radians
  float theta = calDirection / 180.0 * PI;
  // running average
  windAvgX = windAvgX * .75 + cos(theta) * .25;
  windAvgY = windAvgY * .75 + sin(theta) * .25;
}

void rain_count() {
  currentMillis2 = millis(); //we have to read millis at the same position in ISR each time to get the most accurate readings
if ((currentMillis2 - contactBounceTime2) > 500 ) { // debounce the switch contact.
    contactBounceTime2 = currentMillis2;
    rainCount++;
  }
}

void GetAir() {
  unsigned long startedWaiting = millis();    
#ifdef TMP_POWER_ONOFF
  digitalWrite(pwrAir, HIGH);   // turn on power
  delay(100);
#endif  
  sensor_air.requestTemperatures(); // Send the command to get temperatures
  while (!sensor_air.isConversionComplete() and millis()-startedWaiting<900);
  temp = sensor_air.getTempCByIndex(0);
#ifdef TMP_POWER_ONOFF
  digitalWrite(pwrAir, LOW);   // turn off power
#endif

#ifdef DEBUG
  DEBUGSERIAL.print(F("tmp: "));
  DEBUGSERIAL.println(temp);
#endif
}


void GetWater() {
  unsigned long startedWaiting = millis();    
#ifdef TMP_POWER_ONOFF
  digitalWrite(pwrWater, HIGH);   // turn on power
  delay(100);
#endif  
  sensor_water.requestTemperatures(); // Send the command to get temperatures
  while (!sensor_water.isConversionComplete() and millis()-startedWaiting<1500);  
  water = sensor_water.getTempCByIndex(0);
#ifdef TMP_POWER_ONOFF
  digitalWrite(pwrWater, LOW);   // turn off power
#endif

#ifdef DEBUG
    DEBUGSERIAL.print(F("water: "));
    DEBUGSERIAL.println(water);
#endif
}


#ifdef BMP
void GetPressure() {
  float abs_pressure;  
  lps.requestOneShot();  // important to request new data before reading
  delay(100);
  abs_pressure = lps.readPressure();  // hPa
  if (temp> -30) { 
    pressure=(abs_pressure / pow(1.0 - 0.0065 * sea_level_m / (temp  + 273.15), 5.255))*10;  // ICAO formula
    }
  else { 
    temp=lps.readTemp();
    pressure=(abs_pressure / pow(1.0 - 0.0065 * sea_level_m / (temp  + 273.15), 5.255))*10;  // ICAO formula
    }
}
#endif

void GetTmpNow() {
  if (onOffTmp == 1) {
    GetAir();                               // air
    data[18]=99;
    delay(20);
  }
  else if (onOffTmp == 2) {    
    if (enableRain==0){  
      GetWater();                             // water
    }          
    data[19]=99;
    delay(20);
  }
  else if (onOffTmp > 2) {
    GetAir();                               // air
    if (enableRain==0){  
      GetWater();                             // water
    }          
    delay(20);
  }

  #ifdef BMP
    if (enableBmp == 1) {
      GetPressure();
      delay(20);
    }
  #endif   
}


void BeforePostCalculations() {
  DominantDirection();                          // wind direction
  GetAvgWInd();                                 // avg wind
}

// Read the module's power supply voltage
float readVcc() {
  // Read battery voltage
if (fona.getBattVoltage(&battVoltage)) {
  battLevel=battVoltage/20; // voltage  
}
else {
  battLevel=0;
}

return battLevel;
}

#ifdef UZ_Anemometer
void ultrasonicFlush(){
  while(ultrasonic.available() > 0 ) {
    char t = ultrasonic.read();
  }
}

  
#endif 
