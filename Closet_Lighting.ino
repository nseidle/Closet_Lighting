/*
  Turns on a strip of LEDs when the closet door is open
  By: Nathan Seidle
  SparkFun Electronics
  Date: May 21st, 2016
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  The strip of LEDs fade on and off. If the user leaves the door adjar the LEDs will turn off
  after some amount of time.

  When fading the LEDs from zero there's a weird bouncing effect to the illumination (bright briefly then
  low then fades up again). Starting from 30 to 255 instead of 0 works well.
*/

#include <SimpleTimer.h> //https://github.com/jfturcot/SimpleTimer
#include <avr/wdt.h> //We need watch dog for this program

SimpleTimer timer;
//long secondTimerID;

//Pin definitions
#define STAT_LED 13 //Onboard LED
#define DOOR_SWITCH 11
#define DOOR_SWITCH_GND 10
#define LIGHT_MOSFET 9 //Must be PWM

#define DOOR_ADJAR_TIMEOUT (5 * 60 * 1000L) //Max seconds before lights turn off

#define CLOSED 'c'
#define OPEN 'o'
#define TIMEOUT 't'
char doorState = CLOSED;

unsigned long timeDoorWasOpened = 0; //Amount of time since the door was opened

void setup()
{
  wdt_reset(); //Pet the dog
  wdt_disable(); //We don't want the watchdog during init

  Serial.begin(115200);

  pinMode(STAT_LED, OUTPUT);
  pinMode(DOOR_SWITCH, INPUT_PULLUP);
  pinMode(DOOR_SWITCH_GND, OUTPUT);
  digitalWrite(DOOR_SWITCH_GND, LOW);
  pinMode(LIGHT_MOSFET, OUTPUT);

  timer.setInterval(1000L, secondPassed); //Call secondPassed every Xms

  Serial.println("Closet Light Controller Online");

  wdt_reset(); //Pet the dog
  wdt_enable(WDTO_1S); //Unleash the beast
}

void loop()
{
  wdt_reset(); //Pet the dog
  timer.run(); //Update any timers we are running

  if (doorState == CLOSED && digitalRead(DOOR_SWITCH) == HIGH) //User opened the door
  {
    delay(50); //Debounce
    if (digitalRead(DOOR_SWITCH) == HIGH)
    {
      Serial.println("Door adjar");
      doorState = OPEN;

      delay(250); //Delay until user gets the door open
      
      setBrightness(20, 255, 2000); //Turn on the LED strip. Starting at 20 still has the pulse on artifact
      //setBrightness(0, 255, 3000); //Turn on the LED strip
      //digitalWrite(LIGHT_MOSFET, HIGH);
      timeDoorWasOpened = millis();
    }
  }
  else if (doorState == OPEN && digitalRead(DOOR_SWITCH) == LOW) //User closed the door
  {
    delay(50); //Debounce
    if (digitalRead(DOOR_SWITCH) == LOW)
    {
      Serial.println("Door closed");
      doorState = CLOSED;
      digitalWrite(LIGHT_MOSFET, LOW);
      //analogWrite(LIGHT_MOSFET, 0); //Turn off the LED strip immediately

      //On a few reed switch installations it switch can trigger twice when closing the door
      //Hand here for a second while door closes
      delay(1000);
      
    }
  }
  else if (doorState == TIMEOUT && digitalRead(DOOR_SWITCH) == LOW) //User has finally closed the door after a timeout
  {
    Serial.println("Door returned to closed");
    doorState = CLOSED;
    digitalWrite(LIGHT_MOSFET, LOW); //Turn off LED
    //analogWrite(LIGHT_MOSFET, 0); //Turn off the LED strip immediately
  }

  if (doorState == OPEN)
  {
    if (millis() - timeDoorWasOpened > DOOR_ADJAR_TIMEOUT) //If door has been open for too long, turn off LED
    {
      Serial.println("Door timeout, LED off");
      doorState = TIMEOUT;
      digitalWrite(LIGHT_MOSFET, LOW); //Turn off LED
      //setBrightness(255, 0, 4000); //Turn off the LED strip slowly
    }
  }

  delay(1);
}

//Blink the status LED to show we are alive
void secondPassed()
{
  digitalWrite(STAT_LED, !digitalRead(STAT_LED));
}

//Given an old and new brightness, slowly go to the new brightness
void setBrightness(float oldBrightness, float newBrightness, long overallTimeToGetThere)
{
  //Low step numbers (30) mean we get there slowly, high numbers (100) mean we get there quickly
  //Low step numbers make it look stepped, high numbers make it more gradual
  int STEPS = 100; 

  int brightDiff = abs(oldBrightness - newBrightness);

  int brightStep = (float)brightDiff / STEPS; //50 / 75 = 0.66;

  if (brightStep == 0) brightStep = 1;

  //Move oldBrightness towards newBrightness
  for (int fadeSteps = 0 ; fadeSteps <= STEPS ; fadeSteps++)
  {
    wdt_reset(); //Pet the dog
    timer.run(); //Update any timers we are running

    //if less, then add
    //if more, then subtract
    //if equal, do nothing

    if (abs(oldBrightness - newBrightness) < 0.9  ) ; //Do nothing
    else if (oldBrightness < newBrightness) oldBrightness += brightStep;
    else if (oldBrightness > newBrightness)
    {
      if (oldBrightness > brightStep) oldBrightness -= brightStep;
      else if (oldBrightness > 0) oldBrightness--;
    }

    //Some interesting case corrections
    if (oldBrightness > 255) oldBrightness = 255;
    if (oldBrightness < 0) oldBrightness = 0;

    analogWrite(LIGHT_MOSFET, (byte)oldBrightness);

    /*Serial.print("brightDiff:");
    Serial.print(brightDiff, DEC);
    Serial.print(" brightStep:");
    Serial.print(brightStep, 2);
    Serial.print(" oldBrightness:");
    Serial.print(oldBrightness, 2);
    Serial.print(" newBrightness:");
    Serial.println(newBrightness, 2);*/

    delay(overallTimeToGetThere / STEPS); //Some amount of time between steps
  }
}

