// Do serial text reading for Serial Motor Controller
// J. Walthour 2011 john.walthour@gmail.com

// Commands:
// S <channel>,<direction>,<speed>
//  Set a motor speed and direction.
//  channel: the motor number, 0-5
//  direction: the motor direction, either R or F
//  speed: the motor speed, 000-255
//
// G <channel>
//  Get a motor speed and direction.
//  Results will be of the form <direction>,<speed>.
//  channel: the motor number, 0-5
//  direction: the motor direction, either R or F
//  speed: the motor speed, 0-255
//
// (unimplemented)
// R <channel>
//  Read an analog sensor pin.
//  channel: the sensor pin, 0-4.
// Motor pins.  Index is motor number.
// Each motor driver has a digital pin
// and a PWM pin - see control ftn for more info.
// 
// W <millis>
//  millis: delay time in ms, 000-999
//
//  Set the watchdog timer to cut off motors
//  after <millis> ms without a command.  Set to
//  0 to disable watchdog behavior.  Defaults
//  to disabled.

#define NUM_MOTORS 6
int PWM_PIN_MOTOR[] = {11, 10, 5, 3, 9, 6};
int DIR_PIN_MOTOR[] = {13, 12, 4, 2, 8, 7};

char current_command;
#define CMD_NONE '\0'
#define CMD_SET 'S'
#define CMD_GET 'G'
#define CMD_ANALOG_READ 'R'
#define CMD_SET_WATCHDOG 'W'

#define PARAMLEN_SET 7
#define PARAMLEN_GET 1
#define PARAMLEN_ANALOG_READ 1
#define PARAMLEN_SET_WATCHDOG 3

unsigned long watchdog_delay; // ms after last contact to cutoff motors
unsigned long watchdog_time; // system clock value at which to cutoff motors

void setup()
{
  current_command = CMD_NONE;
  for(int i = 0; i < NUM_MOTORS; ++i)
  {
    pinMode(PWM_PIN_MOTOR[i], OUTPUT);
    pinMode(DIR_PIN_MOTOR[i], OUTPUT);
    analogWrite(PWM_PIN_MOTOR[i], 0);
    digitalWrite(DIR_PIN_MOTOR[i], LOW);
  }
  
  watchdog_delay = 0;
  watchdog_time = 0;
  
  Serial.begin(9600);
  Serial.println("Welcome!");
  Serial.println("$");
}

void loop()
{
  int chars_left_to_read = 0;

  // Parameters to read from various commands
  int channel;
  boolean fwd;
  int speed;
  int value;

  // Step 1 - wait for a command (1 char + a space)
  while (Serial.available() < 2)
  {
    if(watchdog_time != 0)
    {
      if(millis() > watchdog_time)
      {
        DeactivateAll();
        watchdog_time = 0;
      }
    }
  }
  char current_command = Serial.read();
  switch(current_command)
  {
    case CMD_SET: chars_left_to_read = PARAMLEN_SET; break;
    case CMD_GET: chars_left_to_read = PARAMLEN_GET; break;
    case CMD_ANALOG_READ: chars_left_to_read = PARAMLEN_ANALOG_READ; break;
    case CMD_SET_WATCHDOG: chars_left_to_read = PARAMLEN_SET_WATCHDOG; break;
    default: 
      Serial.println("X$");
      break;
  }
  
  if(chars_left_to_read > 0)
  {
    Serial.read();// throw away space
    // Step 2 - wait for appropriate number of characters
    while (Serial.available() < chars_left_to_read)
      if(Serial.peek() == '\n')
        break;
    if(Serial.available() >= chars_left_to_read)
    {
      switch(current_command)
      {
        case CMD_SET:
        {
          int channel = Char2Int(Serial.read());
          if(Serial.read() != ',') break;
          boolean fwd = (Serial.read() == 'F');
          if(Serial.read() != ',') break;
          int speed = 0;
          speed += 100 * Char2Int(Serial.read());
          speed += 10 * Char2Int(Serial.read());
          speed += 1 * Char2Int(Serial.read());
/*          Serial.print("Received set cmd, ch");
          Serial.print(channel, DEC);
          Serial.print(", ");
          Serial.print(fwd? "fwd" : "rev");
          Serial.print(", speed ");
          Serial.print(speed, DEC);
          Serial.println();*/
          SetMotor(channel, fwd, speed);
        }
        break;
        case CMD_GET:
          GetMotor(Char2Int(Serial.read()));
        break;
        case CMD_ANALOG_READ:
          GetAnalogPin(Char2Int(Serial.read()));
        break;
        case CMD_SET_WATCHDOG:
          watchdog_delay = 0;
          watchdog_delay += 100 * Char2Int(Serial.read());
          watchdog_delay += 10 * Char2Int(Serial.read());
          watchdog_delay += 1 * Char2Int(Serial.read());
          /*Serial.print("Received watchdog cmd, delay is ");
          Serial.print(watchdog_delay);
          Serial.println("ms");*/
      }
    }
    
    // Reset watchdog timer if active
    if(watchdog_delay > 0)
    { watchdog_time = millis() + watchdog_delay; }
    else
    { watchdog_time = 0; }

    Serial.println("$");
  }
}

int Char2Int(char in)
{
  return (in - '0');
}

char Int2Char(int in)
{
  return (in + '0');
}

void SetMotor(int channel, boolean fwd, int speed)
{
  // The L298N motor drivers have three inputs - PWM magnitude,
  // and two direction inputs mapped to direction.  
  // In our board, one "direction" pin is mapped to an inverter,
  // so one pin creates two output values that are the inverse
  // of each other.
  
  if(speed > 255) speed = 255;
  else if(speed < 0) speed = 0;
  if(channel >= NUM_MOTORS) channel = NUM_MOTORS - 1;
  
  /*Serial.print(DIR_PIN_MOTOR[channel], DEC);
  Serial.print(fwd? "H" : "L");
  Serial.print(" ");
  Serial.print(PWM_PIN_MOTOR[channel], DEC);
  Serial.print(" ");
  Serial.print(speed, DEC);
  Serial.println(".");*/

  digitalWrite(DIR_PIN_MOTOR[channel], fwd? HIGH : LOW);
  analogWrite(PWM_PIN_MOTOR[channel], speed);
}

void DeactivateAll()
{
  Serial.println("Watchdog expired");

  for(int i = 0; i < NUM_MOTORS; ++i)
  {
    SetMotor(i, true, 0);
  }
}

void GetMotor(int channel)
{
  int dir = digitalRead(DIR_PIN_MOTOR[channel]);
  Serial.print("G ");
  Serial.print(Int2Char(channel));
  Serial.print(",");
  Serial.print(dir == HIGH? "F" : "R");
  Serial.print(",");
  Serial.print(analogRead(PWM_PIN_MOTOR[channel]), DEC);
  Serial.println();
}

void GetAnalogPin(int pin)
{
}


