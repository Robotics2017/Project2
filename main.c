
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

/*
Enables robot to drive directly non linearly 
using the sign and value of both velocity values
by moving the wheels at different velocities.
*/
void drive(short leftWheelVelocity, short rightWheelVelocity)
{
	byte left_low = leftWheelVelocity; // cast short to byte (discard high byte)
	byte left_high = leftWheelVelocity >> 8; // bitwise shift high to low to save high byte
	
	byte right_low = leftWheelVelocity;
	byte right_high = leftWheelVelocity >> 8;
	
	send_byte( CmdDriveWheels );
	send_byte( right_high );
	send_byte( right_low );
	send_byte( left_high );
	send_byte( left_low );
};

/*
Enables robot to drive directly forward/backwards 
based on the sign and value of the velocity value
by moving the wheels at the same velocity.
*/
void linear_drive(short wheelVelocity)
{
	byte low = wheelVelocity; // cast short to byte (discard high byte)
	byte high = wheelVelocity >> 8; // bitwise shift high to low to save high byte
	
	send_byte( CmdDriveWheels );
	send_byte( high );
	send_byte( low );
	send_byte( high );
	send_byte( low );
};

int main(int args, char** argv)
{
	int i, j; // for indices
	byte bmp = 0, btn = 0, pwrLed = 255, bmpLed = 0;
	byte signalValue = 0;
	
	bool drive_enabled = false; //remove after testing

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
					if (drive_enabled) {
						linear_drive(-500);
					}
				} else {
					if (bmp == 1 || bmp == 2) {
						//drive backwards in a circle away from the activated bump with an ICC of 1.0m until the sensor is deactivated
					}
				}
			} else {
				if (drive_enabled) {
					linear_drive(0);
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
