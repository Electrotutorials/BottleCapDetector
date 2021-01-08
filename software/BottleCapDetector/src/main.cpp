#include "Arduino.h"
#include "OneButton.h"

//Define the used pins
#define BOTTLE_DETECTION_LED_PIN 2
#define CAP_DETECTION_LED_PIN 3
#define ALARM_LED_PIN 4
#define RESET_BUTTON_PIN 5
#define INDUCTION_SENSOR_PIN 6
#define LIGHT_SENSOR_PIN 7
#define ALARM_RELAY_PIN 8

//The number of bottlkes without a cap within the error window that trigger a alamr
#define MAX_ERRORS 3
//The number of bottles in the error window. The error window is opened when the first faulty bottle is detected. If the maximum number of faulty bottles is detected within this window the aram is triggered.
#define ERROR_WINDOW 20
//Set the parameter below to true to get debug messages on the serial port
//Set the parameter below to false to disable the serial port and not get any debug messages
#define DEBUG false

//Instaniate the varables
int NrOfErrors;
int NrOfBottlesSinceFirstError;
bool bottleDetected;
bool capDetected;
bool alarm;
bool buttonPressed;

//Instantiate the button object and hook it up to the button pin/
OneButton button(RESET_BUTTON_PIN, true);

//Create an array of LED statusses. This makes the code easier to read.
enum ledStates
{
  OFF,
  ON
};

//Checks if a bottle is detected. 
//Returns false when no bottle is detected.
//Returns true when a bottle is detected
bool detectBottle()
{
  if (digitalRead(LIGHT_SENSOR_PIN) == 1)
  {
    return true;
  }
  else
  {
    return false;
  }
}

//checks if a bottle cap is detected
//Returns true of a bottle cap is detected
//Returns false if no bottle cap is detected
bool detectCap()
{
  if (digitalRead(INDUCTION_SENSOR_PIN) == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

//Switches the alarm relay on if the alarm state is ON
void setAlarmRelay(ledStates state)
{
  if (state == OFF)
  {
    digitalWrite(ALARM_RELAY_PIN, LOW);
  }
  else
  {
    digitalWrite(ALARM_RELAY_PIN, HIGH);
  }
}

//Switches on the LED that indicates if a bottle is detected when the bottle detected state is ON
void setBottleLED(ledStates state)
{
  if (state == OFF)
  {
    digitalWrite(BOTTLE_DETECTION_LED_PIN, LOW);
  }
  else
  {
    digitalWrite(BOTTLE_DETECTION_LED_PIN, HIGH);
  }
}

//Switches on the LED that indicates if a bottle cap is detected when the bottle cap detected state is ON
void setCapLED(ledStates state)
{
  if (state == OFF)
  {
    digitalWrite(CAP_DETECTION_LED_PIN, LOW);
  }
  else
  {
    digitalWrite(CAP_DETECTION_LED_PIN, HIGH);
  }
}

//Switches the alamr LED on indicating that an alarm is triggered
void setAlarmLED(ledStates state)
{
  if (state == OFF)
  {
    digitalWrite(ALARM_LED_PIN, LOW);
  }
  else
  {
    digitalWrite(ALARM_LED_PIN, HIGH);
  }
}

//This method is called when the button is pressed for at least 1 second
void buttonLongPress()
{
  //Set the flag indicating that the button has been pressed
  buttonPressed = true;
}

//Initialise the parameters used in the application
void initParameters()
{
  NrOfErrors = 0;
  NrOfBottlesSinceFirstError = 0;
  bottleDetected = false;
  capDetected = false;
  alarm = false;
  buttonPressed = false;

  //Initialise the indicator LEDs
  setBottleLED(OFF);
  setCapLED(OFF);
  setAlarmLED(OFF);
  setAlarmRelay(OFF);
}

//Set the hardware pin modes
void setPinModes()
{
  //Set the input pins
  pinMode(INDUCTION_SENSOR_PIN, INPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  //Set the output pins
  pinMode(CAP_DETECTION_LED_PIN, OUTPUT);
  pinMode(BOTTLE_DETECTION_LED_PIN, OUTPUT);
  pinMode(ALARM_RELAY_PIN, OUTPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);
}

//The set up routine is only called once when starting up the code. It's used to set everything up.
void setup()
{
  //Only set up the serial port when debug messages are used.
  if (DEBUG)
    Serial.begin(9600);

  //Connect the "long press" event to the buttnoLongPress method
  button.attachLongPressStart(buttonLongPress);

  //Set the hardware pin modes
  setPinModes();

  //Initialise all parameters
  initParameters();
}

//The loop method is called over and over again, thus providing the mail application functionality
void loop()
{
  //monitor de reset button
  button.tick();
  //We can only start when there is no alarm
  if (!alarm)
  {
    //We wait for a bottle to be detected
    if (detectBottle())
    {
      if (DEBUG)
        Serial.println("Bottle detected");
      //Keep track of the fact a bottle is detected
      bottleDetected = true;
      //Suppose no bottle cap has been detected at this point
      capDetected = false;
      //Switch the "bottle detected" indicator LED on
      setBottleLED(ON);
      //We start the loop that runs as long as the bottle is detected and the reset button is not pressed
      while (detectBottle() && !buttonPressed)
      {
        //monitor the reset knop. We need this command here too, otherwise we can't get out of this loop
        button.tick();
        //Check if a bottle cap is detected. Only take action the first time the cap is detecxted
        if (detectCap() && !capDetected)
        {
          if (DEBUG)
            Serial.println("Cap detected");
          //Keep track that the cap is detected
          capDetected = true;
          //Swith the "bottle cap" indicator LED on
          setCapLED(ON);
        }
      }
      //The bottle has passed, switch the indicator LEDS off
      setCapLED(OFF);
      setBottleLED(OFF);
      //Take some action if no cap has been detected
      if (!capDetected)
      {
        //No cap detected, keep track of the count
        NrOfErrors++;
        if (DEBUG)
        {
          Serial.print("Number of faulty bottles = ");
          Serial.println(NrOfErrors);
        }
      }
      //If this is not the first error, increase the number of faulty bottles in the error window
      if (NrOfErrors > 0)
      {
        NrOfBottlesSinceFirstError++;
        if (DEBUG)
        {
          Serial.print("Number of bottles detected in the error window = ");
          Serial.println(NrOfBottlesSinceFirstError);
        }
      }
      //We check if the number of bottles is greater or equal to the maximum number of errors in the error window
      //If so, reset the error counts
      if (NrOfBottlesSinceFirstError >= ERROR_WINDOW)
      {
        NrOfErrors = 0;
        NrOfBottlesSinceFirstError = 0;
      }
      //If the maximum number of errors is reached at this point, this can only be within the fault window, so raise the alarm
      if (NrOfErrors >= MAX_ERRORS)
      {
        if (DEBUG)
          Serial.println("Maximum number of faulty bottles reached. ALARM!!");
        //Keep track of the alarm
        alarm = true;
        //Switch the alarm indicator LED on
        setAlarmLED(ON);
        //Switch the alarm relay on
        setAlarmRelay(ON);
      }
    }
  }
  else
  {
    //Whe we get here, the alarm state is raised, so we wait for the reset button to be pressed.
    if (buttonPressed)
    {
      if (DEBUG)
        Serial.println("Reset button is pressed. Initialise parameters and start again.");
      //Reset is pressed, so we reset all parameters and start all over again.
      initParameters();
    }
  }
}
