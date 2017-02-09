
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

	start(CmdFull); //full mode
	do
	{
		for (i=15; i>-1; --i)
		{
			set_led(bmpLed, pwrLed);

			for ( j=0; j<10; ++j )
			{
				bmp = get_bump(); // halts 15ms
				// map left bump (1 bit) to left LED (3 bit)
				bmpLed = (bmp & BmpLeft) << 2;
				// map right bump (0 bit) to right LED (0 bit)
				bmpLed |= bmp & BmpRight;

				set_led(bmpLed, pwrLed);

				btn = get_button(); // halts 15ms
				if ( btn )
					break;
				
				// Wait 0.1s = 100ms = 100,000us = 2(15,000)us + 70,000us,
				// where us is a microsecond
				usleep(70000);
			}
			if ( btn )
				break;

			pwrLed = 17*i;
		}
		pwrLed = 0;
	}
	while (!btn);

	send_byte(CmdPwrDwn);
	return 0;
}

