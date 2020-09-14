//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//
//  CL-RC V2.1
//  Control Line Remote Control
//  Attila Csontos, (c) 2015,2016,2020
//
// LCD Nokia PCD8544
//
// A0 - Throtle
// A1 - Flaps
// A2,A3 - Normal Switch
// A4 - Dual Switch
// A7 - Battery monitor
// D2-4 - rotary encoder
// D5 - PWM output
// D8 - Normal Switch - Throtle Cut (TC)
// D9-D13 - Display

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// Software SPI (slower updates, more flexible pin options):
// pin 9 - Serial clock out (SCLK)
// pin 10 - Serial data out (DIN)
// pin 11 - Data/Command select (D/C)
// pin 12 - LCD chip select (CS)
// pin 13 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(9, 10, 11, 12,13);

#include <EEPROM.h>

#define VERSION_MAJOR 2
#define VERSION_MINOR 1
char svnRevision[] = "$Revision: 70 $";

#define pinA 2		// KY-040 rotary encoder pin CLK
#define pinB 3		// KY-040 rotary encoder pin DT
#define pinC 4		// KY-040 rotary encoder pin SW

// config PPM
#define servoCentro 1500       //servo center value (us)
#define ppmLong 22500          //ppm frame
// pre Corona TX modul je sirka impulzu 400
// pre drotove pripojenie je 300
// pre FlySky TX modul je sirka impulzu 300
#define ppmPulso 300           //ppm pulse width
// pre Corona TX modul je signal pozitivny - 1
// pre pripojenie cez izolovane lanka je signal negativny - 0
// pre FlySky modul je signal negativny - 0
#define onState 0              //Polarity: 0 neg / 1 pos
#define sigPin 5               //ppm output pin
#define ppmMin 1050
#define ppmMax 1950
#define contrastDefault 40   // vychodzia hodnota kontrastu

// config TX
// pre Corona TX modul musi byt 6 kanalov
#define CHANNELS 5              //number of channels
#define MODELS 5              //total model memory
#define canalTh 0              //Throtle channel ID
#define potMin 0               //Pot min value
#define potMax 1023            //Por max value
#define swTC 8					// Throttle Cut
#define numBytePerModel 31  //pocet byte na 1 model
// servo revers - 1
// calMin - 2
// calMax - 2
// travelmin - 1
// travelmax - 1
// channels - 5 
// total = 1+(2+2+1+1)*5 = 31 bytes

char msgs[6][14] = {
	"Disp. Contr.",
	"Srv. Direct.",
	"Model Select.",
	"Stick Calibr.",
	"Srv. Trav.",
	"Exit"
};

int ppm[CHANNELS];              //ppm output array
int ppmDisplay[CHANNELS];		//ppm pre zobrazenie
unsigned char sw_status = 0x00; //0:TC 1:DR
int pots[CHANNELS];    			//ADC data array
unsigned char inPots[CHANNELS] = {A0, A1, A2, A3, A4}; //Input pins array
int calibration[CHANNELS][2];
byte travel[CHANNELS][2];
unsigned char servoRevers;
unsigned char modeloActual = 0;	//aktualny model
unsigned char contrast = 25; // vychodzi kontrast

//static uint8_t pinALast;
byte button = 0;
int pinALast;  

byte menuSubActual; //menu displaya
unsigned char menu;
char i;
unsigned char vBat; 
unsigned char batPin = A7;

// timer1 setup
void configTimer1() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A = 100;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11);  // prescaler 8: 0.5us - 16mhz
  TIMSK1 |= (1 << OCIE1A);
  sei();  
}

// Interrupcion del timer
ISR(TIMER1_COMPA_vect){  
  static boolean state = true;
  TCNT1 = 0;
  if(state) {   
    digitalWrite(sigPin, onState);
    OCR1A = ppmPulso * 2;
    state = false;
  }
  else{   
    static byte cur_chan_numb;
    static unsigned int calc_rest;
    digitalWrite(sigPin, !onState);
    state = true;
    if(cur_chan_numb >= CHANNELS){
      cur_chan_numb = 0;
      calc_rest = calc_rest + ppmPulso; 
      OCR1A = (ppmLong - calc_rest) * 2;
      calc_rest = 0;
    }
    else{
      OCR1A = (ppm[cur_chan_numb] - ppmPulso) * 2;
      calc_rest = calc_rest + ppm[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
}



void setup()
{
//    Serial.begin(38400);
	pinMode(pinC, INPUT); // Connected to SW on KY-040
	digitalWrite(pinC,HIGH);
	pinMode(sigPin, OUTPUT);

	menu = 0;
	menuSubActual = 1;
	pinALast = digitalRead(pinA);   

	// servo reverse: 0 normal, 1 reverse
	servoRevers = 0b00000000;

	// resetovanie EEPROM - len pri inicializacii Arduino Nano
	// resetEeprom();
	
	// load data from eeprom
	modeloActual = cargaDatosEeprom(255);

	// zero state values for ppm
	ppm[0] = servoCentro;
	ppm[1] = servoCentro;
	ppm[2] = ppmMin;
	ppm[3] = servoCentro;
	ppm[4] = servoCentro;

  // vygenerovanie cisla revizie zo subversion
  char svnRevisionNumber[4] = "";
  byte ij = 0;
  for(byte ii = 0; ii < sizeof(svnRevision)-1; ii++)
   {
      if(isDigit(svnRevision[ii]))
      {
        svnRevisionNumber[ij] = svnRevision[ii];
        ij++;
      }
  } 

	display.begin();
	display.setTextWrap(false);
//moje No.001 - setContrast(8);
//Kiko No.002 - setContrast(60);
// hodnoty 0-127
	if (contrast <1)
	{
		display.setContrast(contrastDefault);
	} else {
		display.setContrast(contrast);
	}
// hodnoty 0-5
	display.setBias(4); // normal mode  4 
	display.setTextColor(BLACK);
	for (int cnt = 4; cnt >= 0; cnt--)
	{
		display.clearDisplay();
		display.setTextSize(1);
		display.setCursor(1,1);
		display.print("ArduCL ");
		display.print(VERSION_MAJOR);
		display.print(".");
		display.print(VERSION_MINOR);
		display.print(".");
		display.print(svnRevisionNumber);
		display.setCursor(1,10);
		display.print("(c) 2015-20");
		display.setCursor(33,20);
		display.setTextSize(3);
		display.print(cnt+1);
		display.display();
		delay(1000);
	}
	// ppm timer config
	display.setTextSize(1);
	pinMode(sigPin, OUTPUT);
	digitalWrite(sigPin, !onState);
	configTimer1();
}

void loop()
{
	while (1)
	{
		readPots();
		readSwitches();
		button = readKeys();

		if (button == 2)
		{
			if (menu == 0)   //prechod z info do menu
			{
				menu = 1;
				menuSubActual = 0; // aktualne graficke submenu
				display.clearDisplay();
				display.setCursor(0,10);
				display.print("Entering");
				display.setCursor(0,20);
				display.print("to menu");
				display.display();
				delay(500);
				button = 0; 
			}
		}

		display.clearDisplay();
		
		if (menu == 0) // zakladna obrazovka info
		{
			// info screen
			unsigned char valBar;
			display.setCursor(0, 0);
			display.print("M: ");
			display.setCursor(10, 0);
			display.print(modeloActual + 1);
			// throtle cut switch value
			// throtle cut
			if (bitRead(sw_status, 0) == 0)
			{
				display.setCursor(20, 0);
				display.print("TC");
			}
			else
			{
				display.setCursor(20, 0);
				display.print("  ");
			}
			// batery Voltage
			display.setCursor(35, 0);
			display.print("Bat:");
			display.setCursor(60, 0);
			// min.napatie 3.25V pre LiPo - 665
			// max.mapatie 4.2V pre Lipo - 859
			vBat = map(analogRead(A7), 665, 859, 0, 100);
// debug	Serial.println(vBat);
			display.print(vBat);
			display.setCursor(79, 0);
			display.print("%");

			// draw the input values - CH0-CH1
			display.setCursor(16, 12);
			display.print("CH1");
			display.setCursor(16, 28);
			display.print("CH2");
			
			// channel 1
			display.drawRect(35,10,49,10,BLACK);
			display.fillRect(35,10,0.49*ppmDisplay[0],10,BLACK);
			// channel 2
			display.drawRect(35,26,49,10,BLACK);
			display.fillRect(35,26,0.49*ppmDisplay[1],10,BLACK);

			// vykreslenie vypinacov
			for (i = 2; i <= 4; i++)
			{
				valBar = ppmDisplay[i];
				display.drawCircle(30 + (i - 2) * 23,42,3,BLACK);
				display.fillCircle(30 + (i - 2) * 23,42,1,BLACK);
				if (valBar <= 33)
				{
					//	button  OFF
					display.fillRect(29 + (i - 2) * 23,43,3,5,BLACK);
				}
				else if (valBar > 34 && valBar <= 66 )
				{
					//	button  ---
					display.fillRect(29 + (i - 2) * 23,41,3,3,BLACK);
				}
				else
				{
					//	button  ON
					display.fillRect(29 + (i - 2) * 23,37,3,5,BLACK);
				}
			}


			for (i = 0; i <= 4; i++)
			{

				// servo reverse indicator
				// cisla kanalov 1-5
				display.setCursor(0, 8 + (i * 8));
				display.print(i + 1);
				if (bitRead(servoRevers, i))
				{
					display.setCursor(8, 8 + (i * 8));
					display.print("R");
				}
				else
				{
					display.setCursor(8, 8 + (i * 8));
					display.print("N");
				}
			}
		}

		// prechod z info obrazovky do menu
		if (menu == 1)  
		{
			while (button !=2 )
			{
				button = 0;
				display.clearDisplay();
				//display.setCursor(0, 0);
				//display.print(msgs[0]);
				for (i = 0; i <= 5; i++)
				{
					display.setCursor( 6, i * 8);
					display.print(msgs[i]);
				}
				display.setCursor( 0, menuSubActual * 8);
				display.write(175);
				display.display();
					
				while ( (button != 3) && (button !=1) && (button !=2) ) 
				{
					button = readKeys();
					readPots();
					readSwitches();

				}
				
				switch (button)
				{
					case 3:
					{
						if (menuSubActual >=6)
						{
							menuSubActual = 0;
						}
						menuSubActual++;
						break;
					}
					case 1:
					{
						if (menuSubActual <=0)
						{
							menuSubActual = 6;
						}
						menuSubActual--;
						break;
					}
					case 2:
					{
						switch (menuSubActual) // potvrdenie menu
						{
							case 0:
							{
								button = 0;
								delay(200);
								displayContrast();
								break;
							}
							case 1:
							{
								button = 0;
								delay(200);
								servoReversing();
								break;
							}
							case 2:
							{
								button = 0;
								delay(200);
								modelSelection();
								break;
							}
							case 3:
							{
								stickCalibration();
								break;
							}
							case 4:
							{
								button = 0;
								delay(200);
								servoTravel();
								break;
							}
							case 5:
							{
								delay(200);
								menuSubActual=1;
								menu = 0;
								break;
							}
						}
						break;
					}					
				}
			}
			
		}
	display.display();

	}
}

byte readKeys()
{
   byte btn = 0;
   int aVal;

   aVal = digitalRead(pinA);
   if (aVal != pinALast)
   { // Means the knob is rotating
     // if the knob is rotating, we need to determine direction
     // We do that by reading pin B.
		if (digitalRead(pinB) != aVal)
		{  // Means pin A Changed first - We're Rotating Clockwise
			btn = 3;
		} 
		else 
		{// Otherwise B changed first and we're moving CCW
			btn = 1;
		}
	}
	if (digitalRead(pinC)==0) btn = 2;
	pinALast = aVal;
	return btn;
}

void readPots() {
	unsigned char kk;
	int temporalPots[CHANNELS];
	for (kk = 0; kk <= (CHANNELS - 1); kk++) {
		// read the pots
		temporalPots[kk] = analogRead(inPots[kk]);
		// calibration mapping
		pots[kk] = map(temporalPots[kk], calibration[kk][0], calibration[kk][1], 0, 1023);
		ppmDisplay[kk] = map(temporalPots[kk], calibration[kk][0], calibration[kk][1], 0, 100);
		if (ppmDisplay[kk] < 0) ppmDisplay[kk] = 0;
		if (ppmDisplay[kk] > 100) ppmDisplay[kk] = 100;
		if (pots[kk] < 0) pots[kk] = 0;
		if (pots[kk] > 1023) pots[kk] = 1023;
		// servo reversing
		if (bitRead(servoRevers, kk) == 1) {
			pots[kk] = 1023 - pots[kk];
//			ppmDisplay[kk] = 100 - ppmDisplay[kk];
		}


		// Throtle cut
		if (kk == canalTh && bitRead(sw_status, 0) == 0) {
			// TC activado
			pots[kk] = 0;
			ppmDisplay[kk] = 0;
		}

		// ppmMin = travelMin = 0%
		// ppmMax = travelMax = 100%

		float tempTravelMin, tempTravelMax;
		int tmpMin, tmpMax;
		
		tempTravelMin = ppmMin+(ppmMax-ppmMin)/100*travel[kk][0];
		tempTravelMax = ppmMin+(ppmMax-ppmMin)/100*travel[kk][1];
		tmpMin=(int)tempTravelMin;
		tmpMax=(int)tempTravelMax;
		ppm[kk] = map(pots[kk], potMin, potMax, tmpMin, tmpMax);
	}
}

void readSwitches() {
  // read switches and set into the sw_status var
  if (digitalRead(swTC)==HIGH) {
    bitWrite(sw_status,0,1);
  } 
  else {
    bitWrite(sw_status,0,0);
  }
  bitWrite(sw_status,1,1);
}

unsigned char cargaDatosEeprom(unsigned char mod) {
	// load data from eeprom, for selected model
	int eepromBase;
	unsigned char i;
	if (mod = 255) {
		mod = EEPROM.read(511);
	}
	contrast = EEPROM.read(510);
	
	// priklad pre model 1 =>  mod = 0
	eepromBase = numBytePerModel * mod;  // eepromBase = 25 * 0
	unsigned char eepromPos = eepromBase;  // eepromPos = 0
	// servo reversing
	servoRevers = EEPROM.read(eepromPos);
	eepromPos++;    // eepromPos = 1
	int posEeprom;
	for (i = 0; i <= (CHANNELS - 1); i++)
	{
		posEeprom = eepromPos + (i * 6); // 1; 7; 13; 19; 25
		calibration[i][0] = EEPROMReadInt(posEeprom); //minimo
		travel[i][0] = EEPROM.read(posEeprom+4);
		posEeprom += 2;   // 3; 9; 15; 21; 27
		calibration[i][1] = EEPROMReadInt(posEeprom); //maximo
		travel[i][1] = EEPROM.read(posEeprom+3);
	}

	return mod;
}


void EEPROMWriteInt(int p_address, int p_value) {
	// write a 16bit value in eeprom
	byte Byte1 = ((p_value >> 0) & 0xFF);
	byte Byte2 = ((p_value >> 8) & 0xFF);
	EEPROM.write(p_address, Byte1);
	EEPROM.write(p_address + 1, Byte2);
}

int EEPROMReadInt(int p_address) {
	// read a 16 bit value in eeprom
	byte Byte1 = EEPROM.read(p_address);
	byte Byte2 = EEPROM.read(p_address + 1);
	long firstTwoBytes = ((Byte1 << 0) & 0xFF) + ((Byte2 << 8) & 0xFF00);
	return (firstTwoBytes);
}

void displayContrast()
{
	while (button != 2)
	{					
		display.clearDisplay();
		display.setTextSize(1);
		display.setCursor(  0, 0);
		display.print("Display");
		display.setCursor( 0, 8);
		display.print("Contrast: ");  
		display.setCursor( 28, 18);
		display.setTextSize(3);
		display.print(contrast);
		display.display();
		button=readKeys(); 

		while ( (button != 3) && (button !=1) && (button !=2) ) 
		{
			button = readKeys();
			readPots();
			readSwitches();

		}
//		Serial.print("Contrast init");
//		Serial.println(contrast);
		switch (button)
		{
			case 1:
			{
				contrast--;
				if (contrast <= 0) contrast = 255;
				display.setContrast(contrast);
				display.setCursor( 28, 18);
				display.print(contrast);
				display.display();
				break;
			}
		
			case 3:
			{
				contrast++;
				if (contrast >= 255 or contrast == 0) contrast = 1;
				display.setContrast(contrast);
				display.setCursor( 28, 18);
				display.print(contrast);
				display.display();
				break;
			}

			case 2:
			{
				// reversa de servos
				display.clearDisplay();
				display.setTextSize(1);
				display.setCursor(  0, 2);
				display.print("Save contrast");
				display.setCursor(   6, 40);
				display.print("...wait");
				display.display();
				display.setContrast(contrast); // debug mode
				EEPROM.write(510, contrast);
				delay(1500);

				// load data from eeprom
//				EEPROM.write(511, modeloActual);
//				cargaDatosEeprom(modeloActual);
				
				menu = 0;
				menuSubActual = 0;		
				return;
			}

		}
//		Serial.print("Contrast set");
//		Serial.println(contrast);
		display.setTextSize(1);
		display.display();
	}
}

void servoReversing()
{
	// servo reversing
	int ch = 1;
	while (button != 2)
	{				
		display.clearDisplay();
		display.setCursor( 0, 0);
		display.print(msgs[menuSubActual]);
		display.setCursor( 71, 0);
		display.print("M:");  
		display.setCursor( 79, 0);
		display.print(modeloActual+1);
		for (i=0; i<=(CHANNELS-1);i++)
		{
			display.setCursor(10, 8+i*8);
			display.print("CH: ");
			display.setCursor(30, 8+i*8);
			display.print(i+1);
			if (bitRead(servoRevers,i)) 
			{
				display.setCursor( 40, 8+i*8);
				display.print(" - R");
			} 
			else 
			{
				display.setCursor(  40, 8+i*8);
				display.print(" - N");
			}				
		}	
		display.setCursor(0,ch*8);
		display.write(175);				
		display.display();
		while ( (button != 3) && (button !=1) && (button !=2) ) 
		{
			button = readKeys();
			readPots();
			readSwitches();

		}

		switch (button)
		{
			case 1:
			{
				ch--;
				if (ch <= 0) ch = 5;
				break;
			}
		
			case 3:
			{
				ch++;
				if (ch >= 6) ch = 1;
				break;
			}

			case 2:
			{
				// reversa de servos
				display.clearDisplay();
				display.setCursor(0, 0);
				display.print("Saving revers");
				display.setCursor(0, 8);
				display.print("data");
				display.setCursor(20, 24);
				display.print("...wait");
				display.display();
				delay(1000);
				bitWrite(servoRevers, ch-1, !bitRead(servoRevers,ch-1));
				// data save
				int eepromBase;
				eepromBase=numBytePerModel*modeloActual;
				// actual model
				EEPROM.write(511,modeloActual);
				// servo reverse
				EEPROM.write(eepromBase, servoRevers);				
				menu = 0;
				menuSubActual = 1;		

				return;
			}

		}
		display.setCursor(0,ch*8);
		display.write(175);				
		display.display();
		button = 0;	
	}
}

void stickCalibration()
{
	display.clearDisplay();
	display.setCursor(  0, 0);
	display.print(msgs[menuSubActual]);
	display.display();
	//reseteaEeprom();
	delay(250);
	display.clearDisplay();
	for (i=0;i<=(CHANNELS-1);i++) 
	{
		calibration[i][0]=512;
		calibration[i][1]=512;
	}
	button=readKeys(); 

	while (button!=2) 
	{
		display.clearDisplay();
		display.setCursor(  0, 0);
		display.print("Calibration");
		display.setCursor(  0, 8);
		display.print("Move sticks");
		display.setCursor(  0, 16);
		display.print("1: ");
		display.setCursor(12, 16);
		display.print(map(analogRead(inPots[0]),0,1023,0,100));
		display.setCursor(  40, 16);
		display.print("2: ");
		display.setCursor(52, 16);
		display.print(map(analogRead(inPots[1]),0,1023,0,100));
		display.setCursor(  0, 24);
		display.print("3: ");
		display.setCursor(12, 24);
		display.print(map(analogRead(inPots[2]),0,1023,0,100));
		display.setCursor(  40, 24);
		display.print("4: ");
		display.setCursor(52, 24);
		display.print(map(analogRead(inPots[3]),0,1023,0,100));
		display.setCursor( 0, 32);
		display.print("5: ");
		display.setCursor(12, 32);
		display.print(map(analogRead(inPots[4]),0,1023,0,100));
		display.display();
		int lecturaTemporal;
		for (i=0;i<=(CHANNELS-1);i++) 
		{
			lecturaTemporal=analogRead(inPots[i]);
			// min values
			if (lecturaTemporal<=calibration[i][0]) 
			{
				calibration[i][0]=lecturaTemporal;
			}
			// max values
			if (lecturaTemporal>=calibration[i][1]) 
			{
			calibration[i][1]=lecturaTemporal;
			}
			delay(10);
		}
		button=readKeys();
		readPots();
		readSwitches();

	}
	// save calibration data
	int eepromBase, eepromPos, posEeprom;
	eepromBase=numBytePerModel*modeloActual;
	eepromPos = eepromBase + 1;
	for (i=0;i<=(CHANNELS-1);i++) 
	{
		posEeprom= eepromPos+(i*6);
		EEPROMWriteInt(posEeprom, calibration[i][0]); //min
		posEeprom+=2;
		EEPROMWriteInt(posEeprom, calibration[i][1]); //max
	}
	display.clearDisplay();
	display.setCursor(  0, 2);
	display.print("Saving");
	display.setCursor(  0, 10);
	display.print("calibration");
	display.setCursor(  0, 18);
	display.print("data");
	display.setCursor(   6, 40);
	display.print("...wait");
	display.display();
	delay(1500);
	menu = 0;
	menuSubActual = 1;		
	return;
}

void modelSelection()
{
	int ch = 1;
	while (button != 2)
	{					
		display.clearDisplay();
		display.setCursor(  0, 0);
		display.print(msgs[menuSubActual]);
		display.setCursor( 10, 52);
		display.print("Current model: ");  
		display.setCursor( 100, 52);
		display.print(modeloActual+1);
		for (i=0; i<=(MODELS-1);i++)
		{
			display.setCursor(10, 8+i*8);
			display.print("Model: ");
			display.setCursor(50, 8+i*8);
			display.print(i+1);
		}	
		display.setCursor(0,ch*8);
		display.write(175);					
		display.display();
		button=readKeys(); 

		while ( (button != 3) && (button !=1) && (button !=2) ) 
		{
			button = readKeys();
			readPots();
			readSwitches();

		}

		switch (button)
		{
			case 1:
			{
				ch--;
				if (ch <= 0) ch = 5;
				break;
			}
		
			case 3:
			{
				ch++;
				if (ch >= 6) ch = 1;
				break;
			}

			case 2:
			{
				// reversa de servos
				display.clearDisplay();
				display.setCursor(  0, 2);
				display.print("Load model");
				display.setCursor(  0, 10);
				display.print("parameters");
				display.setCursor(  0, 18);
				display.print("data");
				display.setCursor(   6, 40);
				display.print("...wait");
				display.display();
				delay(1500);

				// model selection
				modeloActual=ch-1;
				// load data from eeprom
				EEPROM.write(511, modeloActual);
				cargaDatosEeprom(modeloActual);
				
				menu = 0;
				menuSubActual = 1;		
				return;
			}

		}
		display.setCursor(0,ch*8);
		display.write(175);				
		display.display();
	}
}

void servoTravel()
{
	byte ch = 1;
	byte trvMin;
	while ( button !=2 ) 
	{
		display.clearDisplay();
		display.setCursor(  0, 0);
		display.print(msgs[menuSubActual]);
		display.setCursor( 66, 0);
		display.print("M:");  
		display.setCursor( 79, 0);
		display.print(modeloActual+1);
		for (i=0; i<=(CHANNELS-1);i++)
		{
			display.setCursor(7, 8+i*8);
			display.print("CH:");
			display.setCursor(24, 8+i*8);
			display.print(i+1);
		}	
		display.setCursor(0,ch*8);
		display.write(175);				
		display.display();

		while ( (button != 3) && (button !=1) && (button !=2) ) 
		{
			button = readKeys();
			readPots();
			readSwitches();
		}

		switch (button)
		{
			case 1:
			{
				ch--;
				if (ch <= 0) ch = 5;
				break;
			}
		
			case 3:
			{
				ch++;
				if (ch >= 6) ch = 1;
				break;
			}
			
			case 2:
			{
				button = 0;
				servoSubTravel(ch);
				// save to EEPROM
				
				display.clearDisplay();
				display.setCursor(  0, 2);
				display.print("Saving");
				display.setCursor(  0, 10);
				display.print("servo");
				display.setCursor(  0, 18);
				display.print("travels");
				display.setCursor(   6, 40);
				display.print("...wait");
				display.display();
				delay(1500);
				menu = 0;
				menuSubActual = 1;		
				return;
			}
		}
		display.setCursor(0,ch*8);
		display.write(175);				
		display.display();
		button = 0;	
	}

}


void servoSubTravel(byte ch)
{
	byte tmpTravelMin;
	byte tmpTravelMax;
	byte i = 0;
	int eepromPos;
	eepromPos = numBytePerModel*modeloActual;
	tmpTravelMin = EEPROM.read(eepromPos+(ch*6)-1); 		//  nacitanie tmpTravelMin pre kanal ch
	tmpTravelMax = EEPROM.read(eepromPos+(ch*6));		//  nacitanie tmpTravelMax pre kanal ch
	delay(250);
	while ((button !=2) || (i != 2))
	{
		display.clearDisplay();
		display.setCursor(  0, 0);
		display.print("Channel:");
		display.setCursor( 50, 0);
		display.print(ch);  
		display.setCursor( 66, 0);
		display.print("M:");  
		display.setCursor( 79, 0);
		display.print(modeloActual+1);		
		display.setCursor(10,16);
		display.print("Min:");
		display.setCursor(35,16);
		display.print(tmpTravelMin);
		display.setCursor(10,24);
		display.print("Max:");
		display.setCursor(35,24);
		display.print(tmpTravelMax);	
		display.setCursor(10,32);
		display.print("Save");
		display.setCursor(0,16+i*8);	
		display.write(175);
		//	vykreslenie baru
		display.drawRect(0,8,83,7,BLACK);
		display.fillRect(0.83*tmpTravelMin,8,0.83*(tmpTravelMax-tmpTravelMin),7,BLACK);
		
		display.display();
		
		while ( (button != 3) && (button !=1) && (button !=2) ) 
		{
			button = readKeys();
			readPots();
			readSwitches();
		}

		switch (button)
		{
			case 1:
			{
				i--;
				if (i == 255) i = 2;
				button = 0;
				break;
			}		
			case 3:
			{
				i++;
				if (i >= 3) i = 0;
				button = 0;
				break;
			}
			case 2:
			{
				button = 0;
				delay(250);
				switch (i)
				{
					case 0:
					{
						while (button != 2)
						{							
							display.clearDisplay();
							display.setCursor(  0, 0);
							display.print("Channel:");
							display.setCursor( 50, 0);
							display.print(ch);  
							display.setCursor( 66, 0);
							display.print("M:");  
							display.setCursor( 79, 0);
							display.print(modeloActual+1);							
							display.setCursor(10,16);
							display.print("Min:");
							display.setCursor(35,16);
							display.print(tmpTravelMin);
							display.setCursor(10,24);
							display.print("Max:");
							display.setCursor(35,24);
							display.print(tmpTravelMax);	
							display.setCursor(10,32);
							display.print("Save");
							display.setCursor(0,16+i*8);	
							display.print("*");
							//	vykreslenie baru
							display.drawRect(0,8,83,7,BLACK);
							display.fillRect(0.83*tmpTravelMin,8,0.83*(tmpTravelMax-tmpTravelMin),7,BLACK);
							
							display.display();

							while ( (button != 3) && (button !=1) && (button !=2) ) 
							{
								button = readKeys();
								readPots();
								readSwitches();
							}
							
							switch (button)
							{
								case 1:
								{
									tmpTravelMin--;
									if (tmpTravelMin == 255) tmpTravelMin = 0;
									travel[ch-1][0]=tmpTravelMin;
									button = 0;
									break;
								}		
								case 3:
								{
									tmpTravelMin++;
									if (tmpTravelMin >= tmpTravelMax) tmpTravelMin = tmpTravelMax;
									travel[ch-1][0]=tmpTravelMin;
									button = 0;
									break;
								}
							}
						}						
						button = 0;							
						delay(250);						
						break;
					}
					case 1:
					{
						while (button != 2)
						{							
							display.clearDisplay();
							display.setCursor(  0, 0);
							display.print("Channel:");
							display.setCursor( 50, 0);
							display.print(ch);  
							display.setCursor( 66, 0);
							display.print("M:");  
							display.setCursor( 79, 0);
							display.print(modeloActual+1);
							display.setCursor(10,16);
							display.print("Min:");
							display.setCursor(35,16);
							display.print(tmpTravelMin);
							display.setCursor(10,24);
							display.print("Max:");
							display.setCursor(35,24);
							display.print(tmpTravelMax);	
							display.setCursor(10,32);
							display.print("Save");
							display.setCursor(0,16+i*8);	
							display.print("*");
							//	vykreslenie baru
							display.drawRect(0,8,83,7,BLACK);
							display.fillRect(0.83*tmpTravelMin,8,0.83*(tmpTravelMax-tmpTravelMin),7,BLACK);

							display.display();

							while ( (button != 3) && (button !=1) && (button !=2) ) 
							{
								button = readKeys();
								readPots();
								readSwitches();
							}
							
							switch (button)
							{
								case 1:
								{
									tmpTravelMax--;
									if (tmpTravelMax==255) tmpTravelMax = 0;
									if (tmpTravelMax <= tmpTravelMin) tmpTravelMax = tmpTravelMin;									
									travel[ch-1][1]=tmpTravelMax;
									button = 0;
									break;
								}		
								case 3:
								{
									tmpTravelMax++;
									if (tmpTravelMax >= 100) tmpTravelMax = 100;
									travel[ch-1][1]=tmpTravelMax;
									button = 0;																
									break;
								}														
							}
						}
						button = 0;													
						delay(250);						
						break;
					}
					case 2:
					{
						button = 0;
						// save servo travel data
						//travel[ch-1][0]=tmpTravelMin;
						//travel[ch-1][1]=tmpTravelMax;
						EEPROM.write(eepromPos+(ch*6)-1,tmpTravelMin); // minimum servo travel
						EEPROM.write(eepromPos+(ch*6),tmpTravelMax);	// maximum servo travel
						return;
					}
				}
			}
		}
	}
}

unsigned char resetEeprom()
{
	// erases eeprom, just for checking values (unused now)
	int eepromBase, eepromPos;
	unsigned char j,k;
	EEPROM.write(511,0);
	for (k=0;k<=MODELS-1;k++)
	{
//		Serial.print("k: ");
//		Serial.println(k);
		
		eepromPos=numBytePerModel*k;

//		Serial.print("eepromPos: ");
//		Serial.println(eepromPos);
		
		EEPROM.write(eepromPos,0x00);
		for (j=1;j<=CHANNELS;j++)
		{
//			Serial.print("j: ");
//			Serial.println(j);
			EEPROM.write(eepromPos+(j*6)-1,0); // minimum servo travel
			EEPROM.write(eepromPos+(j*6),100);	// maximum servo travel
		}
	}      
}
