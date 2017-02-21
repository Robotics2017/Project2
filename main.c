
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "oi.h"
#include "serial.h"

enum bool {false, true};
typedef unsigned char byte;
Serial* serial;

void send_byte(byte b)
{
	serialSend(serial, b);
};

byte get_byte()
{
	byte c;
	
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
	
	byte right_low = rightWheelVelocity;
	byte right_high = rightWheelVelocity >> 8;
	
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
positve wheelVelocity moves robot forward
negative wheelVelocity moves robot in reverse
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

/*
Enables robot to drive directly angularly 
using the sign and value of both the velocity values and the radius
positve wheelVelocity moves robot forward
negative wheelVelocity moves robot in reverse
positve radius turns robot left
negative radius turns robot right
*/
void angular_drive(short wheelVelocity, short radius)
{
	byte wheel_low = wheelVelocity; // cast short to byte (discard high byte)
	byte wheel_high = wheelVelocity >> 8; // bitwise shift high to low to save high byte
	
	byte radius_low = radius;
	byte radius_high = radius >> 8;
	
	send_byte( CmdDrive );
	send_byte( wheel_high );
	send_byte( wheel_low );
	send_byte( radius_high );
	send_byte( radius_low );
};

int main(int args, char** argv)
{
	int j; // for index
	byte bmp = 0, btn = 0, pwrLed = 255, bmpLed = 0;
	
	start(CmdFull); //full mode
	
	// initialize pwrLed to red
	set_led(bmpLed, pwrLed);
	
	int drive_enabled = true; //remove after testing

	do
	{
		for ( j=0; j<10; ++j )
		{

			/*
			Note: mapping_ratio may need to flipped (mapping_ratio = 1 - mapping_ratio)
			to correct colors. Green(0) is away from wall & Red(255) is hitting the wall.
			*/
			int wall = get_wall();
			float mapping_ratio = wall / 1023.0;
			pwrLed = mapping_ratio * 255;
			set_led(bmpLed, pwrLed);

			bmp = get_bump(); // halts 15ms
			if (bmp != 0 && drive_enabled) {
				/*
				radius measured in mm
				1.0m = 1000mm???
				*/
				short wheelVelocity = -500; // max velocity in reverse
				short turnRadius = 1000; // in mm
				if (bmp == BmpBoth) {
					//drive straight backwards until the sensors are deactivated
					linear_drive(-500);
				} else {
					if (bmp == BmpRight) {
						//drive backwards in a circle away from the activated bump with an ICC of 1.0m until the sensor is deactivated
						angular_drive(wheelVelocity, turnRadius); // back away curving to the left
					} else {
						//drive backwards in a circle away from the activated bump with an ICC of 1.0m until the sensor is deactivated
						angular_drive(wheelVelocity, (-1 * turnRadius)); // back away curving to the right
					}
				}
			} else {
				if (drive_enabled) {
					linear_drive(0); //stop
				}
			}

			//if button is pushed end program
			btn = get_button(); // halts 15ms
			if ( btn ) {
				break;
			}

			usleep(100000);
		}
		
		//if button is pushed end program
		if ( btn ) {
			break;
		}
		
	} while (!btn); //if button is pushed end program

	send_byte(CmdPwrDwn);
	return 0;
}
