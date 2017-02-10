
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "oi.h"
#include "serial.h"

enum bool {false, true};
typedef unsigned char byte;

Serial* serial;
inline void send_byte(byte b)
{
	serialSend(serial, b);
};
inline byte get_byte()
{
	byte c;

	               // From Roomba Open Interface (OI) Specification
	usleep(15000); // wait 15ms to read

	// if there is no waiting byte, wait
	while ( serialNumBytesWaiting(serial) == 0 )
		usleep(15000);

	serialGetChar(serial, &c);
	return c;
};

void start(byte state)
{
	// allocate memory for Serial struct
	serial = (Serial*)malloc(sizeof(Serial));

	// ensure serial is closed from any previous execution
	serialClose(serial);

	// constant B115200 comes from termios.h
	serialOpen(serial, "/dev/ttyUSB0", B115200, false);
	send_byte( CmdStart );	// Send Start
	send_byte( state );	// Send state

	// turn off any motors
	send_byte( CmdMotors );
	send_byte( 0 );
	send_byte( 0 );
	send_byte( 0 );

	// clear out any bites from previous runs
	while( serialNumBytesWaiting(serial) > 0 )
		get_byte();
};

/*
 * stops for 15ms
 */
unsigned char get_bump()
{
	send_byte( CmdSensors );
	send_byte( SenBumpDrop ); //bumps and wheel drops

	return get_byte() & BmpBoth; //discard wheel drops
};

int get_wall()
{
	send_byte( CmdSensors );
	send_byte( 27 );

	int value = get_byte();
	value += value << 8;
	value += get_byte();

	return value;
};

/*
 * stops for 15ms
 */
unsigned char get_button()
{
	send_byte( CmdSensors );
	send_byte( SenButton );

	return get_byte();
};

void set_led(byte ledBits, byte pwrLedColor)
{
	send_byte( CmdLeds );
	send_byte( ledBits );
	send_byte( pwrLedColor );
	send_byte( 255 ); // set intensity high
};

int main(int args, char** argv)
{
	int i, j; // for indices
	byte bmp = 0, btn = 0, pwrLed = 255, bmpLed = 0;
	byte signalValue = 0;

	start(CmdFull); //full mode
	do
	{
		set_led(bmpLed, pwrLed);

		for ( j=0; j<10; ++j )
		{

			wall = get_wall();
			float mapping_ratio = wall / 1023.0;
			pwrLed = mapping_ratio * 255;
			set_led(bmpLed, pwrLed);

			bmp = get_bump(); // halts 15ms
			if (bmp != 0) {
				if (bmp == 3) {
					//drive straight backwards until the sensors are deactivated
				} else {
					if (bmp == 1 || bmp == 2) {
						//drive backwards in a circle away from the activated bump with an ICC of 1.0m until the sensor is deactivated
					}
				}
			}

			set_led(bmpLed, pwrLed);

			btn = get_button(); // halts 15ms
			if ( btn ) {
				break;
			}

			usleep(100000);
		}
		if ( btn ) {
			break;
		}
		pwrLed = 0;
	}
	while (!btn);

	send_byte(CmdPwrDwn);
	return 0;
}
