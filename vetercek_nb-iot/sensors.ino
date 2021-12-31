void UltrasonicAnemometer() { //measure wind speed
    unsigned long startedWaiting = millis();
    int successcount=0;
    int windav=0;
      while (successcount < 1 && millis() - startedWaiting <= 10000) {
            serialResponse = ultrasonic.readStringUntil('\r\n');
            int commaIndex = serialResponse.indexOf(',');
            int secondCommaIndex = serialResponse.indexOf(',', commaIndex + 1);
            int thrdCommaIndex = serialResponse.indexOf(',', secondCommaIndex + 1);
            int fourthCommaIndex = serialResponse.indexOf(',', thrdCommaIndex + 1);
            int fiftCommaIndex = serialResponse.indexOf(',', fourthCommaIndex + 2);

#ifdef UZ_NMEA
        int dir = serialResponse.substring(commaIndex + 1, secondCommaIndex).toInt();;
        int wind = serialResponse.substring(thrdCommaIndex + 1, fourthCommaIndex).toFloat()*19.4384449 ;
        String check = serialResponse.substring(fiftCommaIndex + 1, fiftCommaIndex+2) ;
        if (check=="A") {  // calculate wind direction and speed
#else
        int dir = serialResponse.substring(commaIndex + 1, secondCommaIndex).toInt();
        int wind = serialResponse.substring(secondCommaIndex + 1, thrdCommaIndex).toFloat()*19.4384449 ;
        String check = serialResponse.substring(0, commaIndex) ;
        if (check==":1") {  // calculate wind direction and speed
#endif            

              successcount++;
              windav=windav+wind;
              calDirection = dir + vaneOffset;
              CalculateWindDirection();  // calculate wind direction from data
              CalculateWindGust(wind);
             }
        else if ( sonicError >= 500)  { // if more than 500 US errors
                reset(4);
        }  
        else { // if more than 500 US errors
                sonicError++;               
        } 
                           
        }
      windSpeed=windav/successcount;
      CalculateWind();


}


void UZsleep(byte sleepTime) { //ultrasonic anemometer sleep mode
  unsigned long startedWaiting = millis();
  while ( millis() - startedWaiting <= 12000) {
    while ( ultrasonic.readStringUntil('\r\n')!="ok" ) {    
    delay(500);
    if (sleepTime==1) { ultrasonic.write(">PwrIdleCfg:1,1\r\n"); }
    else if (sleepTime==2) { ultrasonic.write(">PwrIdleCfg:1,2\r\n"); }
    else if (sleepTime==4) { ultrasonic.write(">PwrIdleCfg:1,4\r\n"); }
    else if (sleepTime==8) { ultrasonic.write(">PwrIdleCfg:1,8\r\n"); }
    else if (sleepTime==0) { ultrasonic.write(">PwrIdleCfg:0,1\r\n"); }
    }
    ultrasonic.write(">SaveConfig\r\n");
  }
}

    
void Anemometer() { //measure wind speed
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

void DominantDirection() { // get dominant wind direction
  windDir =  atan2(windAvgY, windAvgX) / PI * 180;
  if (windDir < 0) windDir += 360;
}

// Get Wind direction, and split it in 16 parts and save it to array
void GetWindDirection() {
  vaneValue = analogRead(windVanePin);
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
  digitalWrite(pwrAir, HIGH);   // turn on power
  delay(100);
  sensor_air.requestTemperatures(); // Send the command to get temperatures
  delay (750) ;
  temp = sensor_air.getTempCByIndex(0);
  digitalWrite(pwrAir, LOW);   // turn off power
}


void GetWater() {
  digitalWrite(pwrWater, HIGH);   // turn on power
  delay(500);
  sensor_water.requestTemperatures(); // Send the command to get temperatures
  delay (750) ;
  water = sensor_water.getTempCByIndex(0);
  digitalWrite(pwrWater, LOW);   // turn off power
}


#ifdef BMP
void GetPressure() {
  pressure=bmx280.readPressure() / 100.0F;
}
#endif


void BeforePostCalculations() {
  DominantDirection();                          // wind direction
  GetAvgWInd();                                 // avg wind
  if (onOffTmp == 1) {
    GetAir();                               // air
    data[18]=99;
    delay(100);
  }
  else if (onOffTmp == 2) {    
    if (enableRain==0){  
      GetWater();                             // water
    }          
    data[19]=99;
    delay(100);
  }
  else if (onOffTmp > 2) {
    GetAir();                               // air
    if (enableRain==0){  
      GetWater();                             // water
    }          
    delay(100);
  }

  #ifdef BMP
    if (enableBmp == 1) {
      GetPressure();
      delay(100);
    }
  #endif 

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
