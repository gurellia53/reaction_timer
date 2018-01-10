/*
 reaction_timer.ino
 By: Andrew Gurik
 Date: 1/10/2018
 
 Created based on "Controlling large 7-segment displays" by Nathan Seidle of SparkFun Electronics

 Here's how to hook up the Arduino pins to the Large Digit Driver IN

 Arduino pin 6 -> CLK (Green on the 6-pin cable)
 5 -> LAT (Blue)
 7 -> SER on the IN side (Yellow)
 5V -> 5V (Orange)
 Power Arduino with 12V and connect to Vin -> 12V (Red)
 GND -> GND (Black)

 There are two connectors on the Large Digit Driver. 'IN' is the input side that should be connected to
 your microcontroller (the Arduino). 'OUT' is the output side that should be connected to the 'IN' of addtional
 digits.

 Each display will use about 150mA with all segments and decimal point on.
*/

#define DISPLAY_DIGITS 3

//GPIO declarations
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
byte segmentClock = 6;
byte segmentLatch = 5;
byte segmentData = 7;

const byte sw1Pin= 2;
const byte sw2Pin= 3;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Globals
unsigned long sw1_time = 0;
unsigned long sw2_time = 0;
unsigned long response_time = 0;
float response_time_sec = 0;
float response_time_centisec = 0;
float display_time = 0;

byte waiting_for_response = 0;
byte waiting_for_response_prev = 0;
byte triggered= 0;

void setup()
{
  Serial.begin(9600);
  Serial.println("Large Digit Driver Example");

  pinMode(segmentClock, OUTPUT);
  pinMode(segmentData, OUTPUT);
  pinMode(segmentLatch, OUTPUT);

  // switch pins
  pinMode(sw1Pin, INPUT_PULLUP);
  pinMode(sw2Pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(sw1Pin), isr_sw1, FALLING);
  attachInterrupt(digitalPinToInterrupt(sw2Pin), isr_sw2, FALLING);

  // output
  pinMode(LED_BUILTIN, OUTPUT);
  
  digitalWrite(segmentClock, LOW);
  digitalWrite(segmentData, LOW);
  digitalWrite(segmentLatch, LOW);

  // initialize to 0
  showNumber(response_time, 4, 0);
  digitalWrite(segmentLatch, LOW);
  digitalWrite(segmentLatch, HIGH);
}

int number = 0;

void loop()
{
//  if((0 == waiting_for_response) && (0xFF == waiting_for_response_prev))
  if(0xFF == triggered)
  {
    Serial.print("Switch 1 time: ");
    Serial.println(sw1_time);
    Serial.print("Switch 2 time: ");
    Serial.println(sw2_time);
    Serial.print("Response time: ");
    response_time = (sw2_time - sw1_time);
    response_time_sec = (float)response_time/1000;
    response_time_centisec = (float)response_time/10;
    Serial.println(response_time);
    Serial.println(response_time_centisec);

    // time to display on 7-seg displays
    if(9.9999 < response_time_sec)
    {
      // too long - display "1.1.1." as error
      postNumber(1,true);
      postNumber(1,true);
      postNumber(1,true);
    }
    else
    {
      showNumber(response_time_centisec, DISPLAY_DIGITS, DISPLAY_DIGITS-1);
    }

    //Latch the current segment data
    digitalWrite(segmentLatch, LOW);
    digitalWrite(segmentLatch, HIGH); //Register moves storage register on the rising edge of RCK
    triggered = 0;
  }


  //delay(500);
  
  waiting_for_response_prev = waiting_for_response;
}

void isr_sw1() {
  if(0 == waiting_for_response)
  {
    sw1_time = millis();
    waiting_for_response = 0xFF;
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void isr_sw2() {
  if(0xFF == waiting_for_response)
  {
    sw2_time = millis();
    waiting_for_response = 0;
    triggered = 0xFF;
    digitalWrite(LED_BUILTIN, LOW);
  }
}

//Takes a number and displays 2 numbers. Displays absolute value (no negatives)
void showNumber(float value, char len, byte point)
{
  int number = abs(value); //Remove negative signs and any decimals

  //Serial.print("number: ");
  //Serial.println(number);

  for (byte x = 0 ; x < len ; x++)
  {
    int remainder = number % 10;

    postNumber(remainder, (point == x));

    number /= 10;
  }

  //Latch the current segment data
  digitalWrite(segmentLatch, LOW);
  digitalWrite(segmentLatch, HIGH); //Register moves storage register on the rising edge of RCK
}

//Given a number, or '-', shifts it out to the display
void postNumber(byte number, boolean decimal)
{
  //    -  A
  //   / / F/B
  //    -  G
  //   / / E/C
  //    -. D/DP

#define a  1<<0
#define b  1<<6
#define c  1<<5
#define d  1<<4
#define e  1<<3
#define f  1<<1
#define g  1<<2
#define dp 1<<7

  byte segments;

  switch (number)
  {
    case 1: segments = b | c; break;
    case 2: segments = a | b | d | e | g; break;
    case 3: segments = a | b | c | d | g; break;
    case 4: segments = f | g | b | c; break;
    case 5: segments = a | f | g | c | d; break;
    case 6: segments = a | f | g | e | c | d; break;
    case 7: segments = a | b | c; break;
    case 8: segments = a | b | c | d | e | f | g; break;
    case 9: segments = a | b | c | d | f | g; break;
    case 0: segments = a | b | c | d | e | f; break;
    case ' ': segments = 0; break;
    case 'c': segments = g | e | d; break;
    case '-': segments = g; break;
  }

  if (decimal) segments |= dp;

  //Clock these bits out to the drivers
  for (byte x = 0 ; x < 8 ; x++)
  {
    digitalWrite(segmentClock, LOW);
    digitalWrite(segmentData, segments & 1 << (7 - x));
    digitalWrite(segmentClock, HIGH); //Data transfers to the register on the rising edge of SRCK
  }
}



