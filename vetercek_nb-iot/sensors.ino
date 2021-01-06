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
  temp = sensor_air.getTempC();

  if (temp > -100 && temp < 85  && fabs(temp + 0.0625) >= 0.01 * fabs(temp)) {
    dtostrf(temp, 4, 1, tmp);  //float Tmp to char
  }
  digitalWrite(pwrAir, LOW);   // turn off power
}

void GetWater() {
  digitalWrite(pwrWater, HIGH);   // turn on power
  delay(100);
  sensor_water.requestTemperatures(); // Send the command to get temperatures
  delay (750) ;
  water = sensor_water.getTempC();
  if (water > -100 && water < 85 && fabs(water + 0.0625) >= 0.01 * fabs(water)) {
    dtostrf(water, 4, 1, wat);  //float Tmp to char
  }
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
  if (onOffTmp > 0) {
    GetAir();                               // air
    GetWater();                             // water
    delay(100);
  }

}

// Read the module's power supply voltage
float readVcc() {
  // Read battery voltage
  if (!fona.getBattPercent(&battLevel)) Serial.println(F("Failed to read batt"));
  else Serial.print(F("battery = ")); Serial.print(battLevel); Serial.println(F("%"));
  return battLevel;
}
