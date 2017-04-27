
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "oi.h"
#include "serial.h"

enum bool {false, true};
typedef unsigned char byte;

Serial* serial;

void send_byte(byte b) {

	serialSend(serial, b);

}

byte get_byte() {

	byte c;
	usleep(15000); // wait 15ms to read
	               // From Roomba Open Interface (OI) Specification

	// if there is no waiting byte, wait
	while ( serialNumBytesWaiting(serial) == 0 )
		usleep(15000);

	serialGetChar(serial, &c);
	return c;

}

void start(byte state) {

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

}

unsigned char get_bump() {

	send_byte( CmdSensors );
	send_byte( SenBumpDrop ); //bumps and wheel drops

	return get_byte() & BmpBoth; //discard wheel drops

}

unsigned char get_button() {

	send_byte( CmdSensors );
	send_byte( SenButton );
	return get_byte();

}

void set_led(byte ledBits, byte pwrLedColor) {

	send_byte( CmdLeds );
	send_byte( ledBits );
	send_byte( pwrLedColor );
	send_byte( 255 ); // set intensity high

}

/*
The distance that Roomba has traveled in millimeters since the distance it was last requested is sent as a
signed 16-bit value, high byte first. This is the same as the sum of the distance traveled by both wheels
divided by two. Positive values indicate travel in the forward direction; negative values indicate travel in
the reverse direction. If the value is not polled frequently enough, it is capped at its minimum or
maximum.
Range: -32768 – 32767
*/
int get_distance() {

	int distance = 0;

	send_byte( CmdSensors );
	send_byte( 19 );

	distance = get_byte();
	distance = distance << 8;
	distance += get_byte();

	return distance;

}

/*
The angle in degrees that Roomba has turned since the angle was last requested is sent as a signed 16-
bit value, high byte first. Counter-clockwise angles are positive and clockwise angles are negative. If the
value is not polled frequently enough, it is capped at its minimum or maximum.
Range: -32768 – 32767
*/
int get_angle() {

	// Potential problem [The value returned must be divided by 0.324056 to get degrees]

	send_byte( CmdSensors );
	send_byte( 20 );

	int value = get_byte();
	value = value << 8;
	value += get_byte();

	return value;

}

unsigned int get_cliff_front_left() {

	send_byte( CmdSensors );
	send_byte( 29 );

	unsigned int signalValue = get_byte();
	signalValue = signalValue << 8;
	signalValue += get_byte();

	return signalValue;

}

void playSong() {

	// song opcode
	send_byte( 140 );

	// set song track to 0
	send_byte( 0 );

	//send length in notes
	send_byte( 15 );

	send_byte( 43 ); // n1
	send_byte( 35 );

	send_byte( 43 ); // n2
	send_byte( 35 );

	send_byte( 43 ); // n3
	send_byte( 35 );

	send_byte( 39 ); // n4
	send_byte( 25 );

	send_byte( 46 ); // n5
	send_byte( 15 );

	send_byte( 43 ); // n6
	send_byte( 35 );

	send_byte( 39 ); // n7
	send_byte( 25 );

	send_byte( 46 ); // n8
	send_byte( 15 );

	send_byte( 43 ); // n9
	send_byte( 75 );

	send_byte( 50 ); // n10
	send_byte( 35 );

	send_byte( 50 ); // n11
	send_byte( 35 );

	send_byte( 50 ); // n12
	send_byte( 35 );

	send_byte( 39 ); // n13
	send_byte( 155 );

	send_byte( 46 ); // n14
	send_byte( 15 );

	send_byte( 43 ); // n15
	send_byte( 75 );

	usleep(100000);

	// play song opcode
	send_byte( 141 );

	// select song track to play
	send_byte( 0 );

	usleep(5000000);

}

/*
Enables robot to drive directly non linearly
using the sign and value of both velocity values
by moving the wheels at different velocities.
*/
void drive(short leftWheelVelocity, short rightWheelVelocity) {

	byte left_low = leftWheelVelocity; // cast short to byte (discard high byte)
	byte left_high = leftWheelVelocity >> 8; // bitwise shift high to low to save high byte

	byte right_low = rightWheelVelocity;
	byte right_high = rightWheelVelocity >> 8;

	send_byte( CmdDriveWheels );
	send_byte( right_high );
	send_byte( right_low );
	send_byte( left_high );
	send_byte( left_low );

}

// convert feet to mm
double get_mm(double feet) {
	return feet / 0.00328084;
}

/*
takes in distance(feet)
drives that distance
*/
void drive_distance(double distance_in_feet, byte *b) {
	
	unsigned int i = get_cliff_front_left();
	int prev_i = 0;
	int threshold = 150;
	int i_diff = 0;

	if (! (*b)) {

		usleep(100000);

		int distance_traveled = 0;
		int distance_in_mm = (int) get_mm(distance_in_feet);

		// clear garbage value
		get_distance();

		// begin driving
		drive(100, 100);

		// until distance is reached or btn is pressed, drive
		while ( (distance_traveled < distance_in_mm) && (*b == 0) ) {

			// update dist
			distance_traveled += get_distance();
			if (distance_traveled >= distance_in_mm) {
				drive(0, 0);
				break;
			}

			// check for card
			prev_i = i;
			i = get_cliff_front_left();
			i_diff = i - prev_i;

			// update dist
			distance_traveled += get_distance();
			if (distance_traveled >= distance_in_mm) {
				drive(0, 0);
				break;
			}

			// if found toggle light
			if (i_diff > threshold) {
				set_led(0, 0);
			} else {
				set_led(0, 255);
			}

			// update dist
			distance_traveled += get_distance();
			if (distance_traveled >= distance_in_mm) {
				drive(0, 0);
				break;
			}


			*b = get_button();
			// update dist
			distance_traveled += get_distance();
			if (distance_traveled >= distance_in_mm) {
				drive(0, 0);
				break;
			}

		}

		// stop
		drive(0, 0);

		// kill inertia
		usleep(100000);

	}

}

//1845685.68398
void rotateLeft(byte *b) {

	if (!(*b)) {
		drive(-100, 100);
		usleep(1585000);
		drive(0, 0);
	}

}

//1845685.68398
void rotateRight(byte *b) {

	if (!(*b)) {
		drive(100, -100);
		usleep(1585000);
		drive(0, 0);
	}

}

int main(int args, char** argv) {

	byte btn = 0;
	int h_dist = 0;

	start(CmdFull); //full mode
	set_led(0, 255); // init clean led to red

	// SETUP
	drive_distance(2, &btn);
	rotateLeft(&btn);
	drive_distance(2, &btn);
	rotateLeft(&btn);
	// END SETUP

	// BEGIN SEARCH (STRAFE METHOD)
	while (!btn && h_dist < 50) {
		drive_distance(4, &btn);
		rotateLeft(&btn);
		drive_distance(.5, &btn);
		rotateLeft(&btn);
		drive_distance(4, &btn);
		rotateRight(&btn);
		drive_distance(.5, &btn);
		rotateRight(&btn);
		h_dist++;
	}
	// END SEARCH

	//playSong();

	send_byte(CmdPwrDwn);
	return 0;

}
