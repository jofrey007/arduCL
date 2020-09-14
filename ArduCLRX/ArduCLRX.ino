/*This program puts the servo values into an array,
 reagrdless of channel number, polarity, ppm frame length, etc...
 You can even change these while scanning!*/

#include <Servo.h> 

// kanal 1 a 2 su nepouzite (Pin 0 a Pin 1)
#define PPM_Pin 2  //na doske je to kanal 3 (1,2,3....)
#define multiplier (F_CPU/8000000)  //leave this alone
#define numservos 5 // number of servos
int ppm[16];  //array for storing up to 16 servo signals
// kanaly su cislovane od 1, ale serva od 0
// takze servo[3] je kanal 4 atd.
byte servo[] = {3,4,5,6,7};  //pin number of servo output
byte led = 8; //  LED pin
byte ledPause = 120; // LED delay


#define servoOut  //comment this if you don't want servo output
//#define DEBUG

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;
Servo servo5;

void setup()
{
//	Serial.begin(115200);
// Serial.println("ready");

	servo1.attach(3,1050,1950);
	servo2.attach(4,1050,1950);
	servo3.attach(5,1050,1950);
	servo4.attach(6,1050,1950);
	servo5.attach(7,1050,1950);

	pinMode(PPM_Pin, INPUT);
	attachInterrupt(PPM_Pin - 2, read_ppm, CHANGE);
	pinMode(led,OUTPUT);
	digitalWrite(led,LOW);
	TCCR1A = 0;  //reset timer1
	TCCR1B = 0;
	TCCR1B |= (1 << CS11);  //set timer1 to increment every 0,5 us

}

void loop()
{
// nothing
}

void read_ppm(){  //leave this alone
  static unsigned int pulse;
  static unsigned long counter;
  static byte channel;
  static unsigned long last_micros;
  static byte i = 0;
  static byte j = 0;
  counter = TCNT1;
  TCNT1 = 0;

  if(counter < 510*multiplier){  //must be a pulse if less than 510us
    pulse = counter;
    #if defined(servoOut)
    if(sizeof(servo) > channel) digitalWrite(servo[channel], HIGH);
    if(sizeof(servo) >= channel && channel != 0) digitalWrite(servo[channel-1], LOW);
    #endif
  }
  else if(counter > 1910*multiplier){  //sync pulses over 1910us
    channel = 0;
    #if defined(DEBUG)
		Serial.print("PPM Frame Len: ");
		Serial.println(micros() - last_micros);
		last_micros = micros();
    #endif
	if ((micros()-last_micros>25000))
	{
		digitalWrite(led,LOW);	
		i++;
	}
	last_micros = micros();
  }
  else{  //servo values between 710us and 2420us will end up here
    ppm[channel] = (counter + pulse)/multiplier;
	if(i>ledPause && j==0)
	{
		digitalWrite(led,HIGH);	
		i=0;
		j++;
	}
	if(i>ledPause && j==1)
	{
		digitalWrite(led,LOW);	
		i=0;
		j=0;
	}	
    #if defined(DEBUG)
    Serial.print(ppm[channel]);
    Serial.print("  ");
    #endif
    
    channel++;
	i++;
  }
}


