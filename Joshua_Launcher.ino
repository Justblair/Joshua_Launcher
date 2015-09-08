#include <toneAC.h>
#include <EEPROM.h>


const int greenLED	= 12;
const int redLed	= 13;
const int blueLed	= 11;
const int button	= 14;
const int relay		= 2;
const int voltSense	= A3;
const int continuity = A2;

int RangeCount = 0;
int optionNum		=	0;

#define BEEP 3136
#define ERROR 2489
#define LAUNCH  3520

const int continuityThresholdMin = 200;
const int continuityThresholdMax = 10;

int continuityVal;

unsigned long launchDelay = 400;
unsigned long testCounter;
unsigned long alarmTimer;

int countDelay = 250;

float voltage;
int voltPercent;

int del = 250;
int vol = 10;
int ign = 500;
bool launchState = 0;
bool testMode = 0;
bool testLED = true;
bool remoteLaunch = false;


char helloMSG[22] = "Shall we play a game?";

void setup(){
	Serial.begin(115200);
	for (int i = 0; i < 22; i++){
		Serial.print(helloMSG[i]);
		delay(50);
	}
	Serial.println("   ");
	pinMode(blueLed,OUTPUT);
	pinMode(greenLED,OUTPUT);
	pinMode(redLed,OUTPUT);

	pinMode(4,OUTPUT);
	pinMode(5,OUTPUT);
	pinMode(6,OUTPUT);
	pinMode(7,OUTPUT);
	pinMode(8,OUTPUT);
	pinMode(relay,OUTPUT);

	pinMode(button,INPUT_PULLUP);

	pinMode(relay, OUTPUT);

	vol = EEPROM.read(1);
	ign = EEPROM.read(2);

	continuityCheck(false);


	if (digitalRead (button) == LOW) {
		testMode = 1;
	}
	alarmTimer = millis();
}

void loop() {
	if (testMode == 1 && (millis() - testCounter > countDelay)){
		digitalWrite (blueLed, testLED);
		testLED = !testLED;
		testCounter = millis();
	}

	if (launchState == 0){
		digitalWrite (greenLED, testLED);
	}

	batteryCheck();

	if (digitalRead(button)==0){
		countdown();
	}

	serialVolumeCheck();
	if (millis() - alarmTimer > 60*1000){
		bool ledState; 
		while (1){

			toneAC(ERROR, 1, 500, false);
			delay(500);
			Serial.println("Danger Will Robinson");
			if (ledState == 0){
				redLedsOff();
				ledState = 1;
			} else {
				redLedsOn();
				ledState = 0;
			}

		}
	}
}


void batteryCheck(){
	int temp = analogRead(A3);
	voltage = temp/82.87+0.76;
	voltPercent = (voltage - 9.6)*33;	// divide by 3 and multiply by 100
	int ledlit = map(voltage, 9.6,12.6, 4,9);
	redLedsOff();
	for (int i = 4; i < ledlit; i++){
		digitalWrite(i, HIGH);
	}
}

void serialVolumeCheck(){
	if (Serial.available()){
		char rxBuffer = Serial.read();
		switch (rxBuffer)
		{
		case 'V':
		case 'v':
			vol = Serial.parseInt();
			Serial.print ("Volume: ");
			Serial.println (vol, DEC);
			EEPROM.write(1, vol);
			toneAC(BEEP , vol, 500, false);
			break;
		case 'I':
		case 'i':
			ign = Serial.parseInt();
			Serial.print("Ignition delay: ");
			Serial.println (ign, DEC);
			EEPROM.write (2, ign);
			break;

		case 'L':
		case 'l':
			if (continuityCheck(false)==true){
				Serial.println("Launch:");
			} else {
				continuityCheck(true);
				Serial.println("Launch Abort");
				return;
			}
			//remoteLaunch = true;
			//countdown();
			launch();
			break;

		case 'B':
		case 'b':
			batteryCheck();
			Serial.print ("Battery Voltage: ");
			Serial.println (voltage, 2);
			Serial.print ("Battery Percent: ");
			Serial.println (voltPercent, DEC);
			break;

		case 'C':
		case 'c':
			continuityCheck(false);
			Serial.print("Continuity: ");
			Serial.println(continuityVal,DEC);
			break;

		case 'T':
		case 't':
			Serial.print ("Test Mode: ");
			testMode = !testMode;
			if (testMode == 1){
				Serial.println("Enabled");
			} else {
				Serial.println("Disabled");
			}
			break;

		case 'R':
		case 'r':	
			// Range Test

			while (Serial.available()==0){
				Serial.println (RangeCount, DEC);
				RangeCount++;
				delay (500);
			}

			break;



		default:
			break;
		}
	}
}


void countdown(){
	if (continuityCheck(false)==true){
		Serial.println("Launch Commencing:");
	} else {
		continuityCheck(true);
		Serial.println("Launch Abort");
		return;
	}
	redLedsOff();
	statusLedsOff();
	if (testMode == 1){
		digitalWrite(blueLed, HIGH);
	} else {
		digitalWrite(greenLED, HIGH);
	}

	for (int led = 4; led < 9; led ++){
		if (digitalRead(button) == 0 || remoteLaunch == true){
			digitalWrite(led, HIGH);
			Serial.println(9-led, DEC);
			if (led == 8){
				launch();
			}
			toneAC(BEEP, vol, 500, false);
			delay(500);
		} else {
			redLedsOff();
			return;
		}
	}
	for (int led = 4; led < 9; led ++){
		digitalWrite(led, HIGH);

	}
	delay (1000);
	redLedsOff();
}

void redLedsOff (){
	for (int led = 4; led < 9; led ++){
		digitalWrite(led, LOW);
	}
}

void redLedsOn(){
	for (int led = 4; led < 9; led ++){
		digitalWrite(led, HIGH);
	}
}

void statusLedsOff(){
	digitalWrite(blueLed, LOW);
	digitalWrite(greenLED, LOW);
	digitalWrite(redLed, LOW);
}

void launch(){
	launchState == 1;
	toneAC(LAUNCH, vol, 0, true);
	digitalWrite(12, LOW);

	// Lets Launch!
	digitalWrite(relay, HIGH);
	digitalWrite(11, HIGH);
	delay (ign);

	noToneAC();
	digitalWrite(relay, LOW);
	digitalWrite(11, LOW);

	digitalWrite(13, HIGH);
	redLedsOff();
	randoms();
}

void randoms(){
	randomSeed(analogRead(A4));
	int oldRandNum;
	int randnum = random(4,8);
	bool pinState = 0;
	unsigned long lastTime = millis();

	while(1){
		randnum = random (4,10);
		if (oldRandNum != randnum){
			if (randnum == 9) randnum = 13;
			digitalWrite(randnum, HIGH);
			delay(22);
			digitalWrite(randnum, LOW);

			if (Serial.available()){
				if (Serial.read()=='R'){
					Serial.println("Reset");
					break;
				}
			}
		}

	}
}

void testRoutine(){
	Serial.println("Test");
	digitalWrite(blueLed, HIGH);
	while (true)

	{
		randoms();
	}

}

void adjustVolume(){
	unsigned long timer;
	if (digitalRead(button) == LOW){
		delay(100);
	}

}

bool continuityCheck(bool ledCheck){
	continuityVal = analogRead(A2);

	if (ledCheck  == true){
		redLedsOff();
		int ledlit = map(continuityVal, 1023,0, 4,9);

		statusLedsOff();
		digitalWrite(blueLed, HIGH);

		toneAC(ERROR,vol,500, true);

		for (int i = 4; i < ledlit; i++){
			digitalWrite(i, HIGH);
		}

		delay (2000);

		statusLedsOff();
	}

	if (continuityVal > 0 && continuityVal < 100 || testMode == true){
		return true;
	} else {
		return false;
	}
}
