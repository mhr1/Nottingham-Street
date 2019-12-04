/*
	Compile g++ notty.cpp -o ntty -lwiringPi
*/

#include <unistd.h>			//Needed for I2C port
#include <fcntl.h>			//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c-dev.h>		//Needed for I2C port
#include <iostream>
#include <fstream>
#include <wiringPi.h>

#define SECOND 1000
#define MINUTE 60000

#define RESET_LINE	0	// Pin 11 - GPIO 17 - Grey
#define PULSE 		1  	// Pin 12 - GPIO 18 - Purple
#define RESET_LATCH 	2 	// Pin 13 - GPIO 27 - Red
#define DETECT 		3	// Pin 15 - GPIO 22 - Orange

#define TESTS 50
#define TEST_TIME 500

using namespace std;

static int detect = 0;

int reset(void){
	digitalWrite(RESET_LINE, LOW);
	delayMicroseconds(10);
	//digitalWrite(RESET_LATCH, LOW);
	//digitalWrite(RESET_LATCH, HIGH);
	digitalWrite(RESET_LINE, HIGH);
	return 0;}

bool pulse(int x){	
	digitalWrite(x, HIGH);
	digitalWrite(x, LOW);
	return 1;}

void switchInterrupt(void)
{
	//cout << "Event" << endl;
	detect = 1;
}

int main(void)
{
	int file_i2c;
	int length;
	char buffer[60];
	int i, r = 0, event = 0;
	int vca = 0;
	int sum = 0, pre_detect_sum = 0, post_detect_sum = 0;
	bool result, final;
	bool detect1, detect2, did_pulse;

	ofstream fout("test6_results.txt");

	srand(time(NULL));
	
  	wiringPiSetup();

  	pinMode(RESET_LINE, OUTPUT);
	pullUpDnControl(RESET_LINE, PUD_UP);
	digitalWrite(RESET_LINE, HIGH);

	pinMode(PULSE, OUTPUT);
	pullUpDnControl(PULSE, PUD_DOWN);
	digitalWrite(PULSE, LOW);

	//pinMode(RESET_LATCH, OUTPUT);
	//pullUpDnControl(RESET_LATCH, PUD_UP);
	//digitalWrite(RESET_LATCH, HIGH);

	pinMode(DETECT, INPUT);

// OPEN THE I2C BUS

	if((file_i2c = open("/dev/i2c-1", O_RDWR)) < 0)
	{			
		printf("Failed to open the i2c bus");
		exit;
	}

	int addr = 0x48;          // I2C address for PCF8591 module

	if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
	{
		printf("Failed to acquire bus access and/or talk to slave.\n"); 
		return(0);
	}

	// Cause an interrupt when switch is pressed (0V)
	wiringPiISR(DETECT, INT_EDGE_FALLING, switchInterrupt);
  
	for(vca = 130; vca < 156; vca+=3)
	{
		buffer[0] = 0x41;
		buffer[1] = vca; // A0 (160) good for 2V
		length = 2;	
 
		if (write(file_i2c, buffer, length) != length)
			printf("Failed to write to the i2c bus.\n");
		sum = 0;
		pre_detect_sum = 0;
		post_detect_sum = 0;

		for (i = 0; i < TESTS; i++)
  		{   	
					
			r = rand() % 2;
			reset();

// Enable ISR
			system("/usr/bin/gpio edge DETECT falling");
// Allow time to detect event
			delayMicroseconds(1000); 
// Disable ISR
			system("/usr/bin/gpio edge DETECT none");

			if(detect) pre_detect_sum++;

			//detect1 = digitalRead(DETECT); // Use this instead of ISR
			if(r)
				result = detect;
			else
				result  = !detect;
			
			

			cout << detect << ": " << flush;
			

			delay(TEST_TIME); 

			event = rand() % 2;
			did_pulse = 0;

			if(event)
			{
				if(r)did_pulse = pulse(PULSE);
				final = 1;
			}
			else
			{
				if(!r)did_pulse = pulse(PULSE);
				final = 0;
			}

			//detect2 = digitalRead(DETECT);
			delay(100);
			detect = 0;

// Enable ISR
			system("/usr/bin/gpio edge DETECT falling");
// Allow time to detect event
			delayMicroseconds(1000); 
// Disable ISR
			system("/usr/bin/gpio edge DETECT none");

			cout << "\t" << detect << ": " << did_pulse << endl;
			
			if(result == final)
				sum++;

			if(detect == did_pulse)
				post_detect_sum++;
			
			detect = 0;
  		}
		cout << vca << " Positives = " << sum*100/TESTS << "%" ;
		cout << ", detection = " << post_detect_sum*100/TESTS << "%";
		cout << " Pre-events = " << pre_detect_sum << endl;

		fout << vca << " Positives = " << sum*100/TESTS << "%" ;
		fout << ", detection = " << post_detect_sum*100/TESTS << "%"; 
		fout << " Pre-events = " << pre_detect_sum << endl;
	}
	fout.close();
	return(0);
}


 
