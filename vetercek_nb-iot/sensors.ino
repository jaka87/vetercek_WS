
void UltrasonicAnemometer() { //measure wind speed
    unsigned long startedWaiting = millis();
    int successcount=0;
    int windav=0;
      while (successcount <= windDelay/1000 && millis() - startedWaiting <= 10000) {
            serialResponse = ultrasonic.readStringUntil('\r\n');
            int commaIndex = serialResponse.indexOf(',');
            int secondCommaIndex = serialResponse.indexOf(',', commaIndex + 1);
            int thrdCommaIndex = serialResponse.indexOf(',', secondCommaIndex + 1);
            int fourthCommaIndex = serialResponse.indexOf(',', thrdCommaIndex + 1);
            int fiftCommaIndex = serialResponse.indexOf(',', fourthCommaIndex + 2);
        int dir = serialResponse.substring(commaIndex + 1, secondCommaIndex).toInt();;
        int wind = serialResponse.substring(thrdCommaIndex + 1, fourthCommaIndex).toFloat()*19.4384449 ;
        String check = serialResponse.substring(fiftCommaIndex + 1, fiftCommaIndex+2) ;
        
        if (check=="A") {
              //Serial.println(dir);
              //Serial.println(wind);
              successcount++;
              windav=windav+wind;

              calDirection = dir + vaneOffset;
              if (calDirection > 360)
                calDirection = calDirection - 360;
            
              if (calDirection < 0)
                calDirection = calDirection + 360;
            
              // convert reading to radians
              float theta = calDirection / 180.0 * PI;
              // running average
              windAvgX = windAvgX * .75 + cos(theta) * .25;
              windAvgY = windAvgY * .75 + sin(theta) * .25;
              
             }
        }
      windSpeed=windav/successcount;

      ++measureCount; // add +1 to counter
      windAvr += windSpeed; // add to sum of average wind values
    
      if (windSpeed >= windGust[2]) { // check if > than old gust3 of wind
        windGust[0] = windGust[1];
        windGust[1] = windGust[2];
        windGust[2] = windSpeed;
      }
    
      else if (windSpeed >= windGust[1]) { // check if > than old gust2 of wind
        windGust[0] = windGust[1];
        windGust[1] = windSpeed;
      }
    
      else if (windSpeed > windGust[0]) { // check if > than old gust1 of wind
        windGust[0] = windSpeed;
      }
      windGustAvg = (windGust[0] + windGust[1] + windGust[2]) / 3;
    


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
    ++measureCount; // add +1 to counter
    windAvr += windSpeed; // add to sum of average wind values
  
    if (windSpeed >= windGust[2]) { // check if > than old gust3 of wind
      windGust[0] = windGust[1];
      windGust[1] = windGust[2];
      windGust[2] = windSpeed;
    }
  
    else if (windSpeed >= windGust[1]) { // check if > than old gust2 of wind
      windGust[0] = windGust[1];
      windGust[1] = windSpeed;
    }
  
    else if (windSpeed > windGust[0]) { // check if > than old gust1 of wind
      windGust[0] = windSpeed;
    }
    windGustAvg = (windGust[0] + windGust[1] + windGust[2]) / 3;
  }

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

void GetAvgWInd() {
  wind_speed = windAvr / measureCount; // calculate average wind
}


// Get Wind direction, and split it in 16 parts and save it to array
void GetWindDirection() {
  vaneValue = analogRead(windVanePin);
  direction = map(vaneValue, 0, 1023, 0, 360);
  calDirection = direction + vaneOffset;

  if (calDirection > 360)
    calDirection = calDirection - 360;

  if (calDirection < 0)
    calDirection = calDirection + 360;

  // convert reading to radians
  float theta = calDirection / 180.0 * PI;
  // running average
  windAvgX = windAvgX * .75 + cos(theta) * .25;
  windAvgY = windAvgY * .75 + sin(theta) * .25;

}


void BeforePostCalculations() {
  DominantDirection();                          // wind direction
  GetAvgWInd();                                 // avg wind
  if (onOffTmp == 1) {
    GetAir();                               // air
    data[18]=99;
    delay(100);
  }
  else if (onOffTmp == 2) {                 
    GetWater();                             // water
    data[19]=99;
    delay(100);
  }
  else if (onOffTmp > 2) {
    GetAir();                               // air
    GetWater();                             // water
    delay(100);
  }

}

// Read the module's power supply voltage
float readVcc() {
  // Read battery voltage
if (fona.getBattVoltage(&battVoltage)) {

  //battLevel=(battVoltage-3600)/5; //percentage
  battLevel=battVoltage/20; // voltage
  
}
else {
  battLevel=0;
}

return battLevel;
}
