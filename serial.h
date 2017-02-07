/*
 ** This software is may be freely distributed and modified for
 ** noncommercial purposes.  It is provided without warranty of
 ** any kind.  Copyright 2006, Jason O'Kane and the University
 ** of Illinois.
 **
 ** Maintainer: Jeremy S Lewis
 ** Last updated: 20 Dec 2016
 **/

// This file declares some basic functions for serial communications.

#ifndef INCLUDE_SERIAL_H
#define INCLUDE_SERIAL_H

#include <termios.h>
#include <sys/ioctl.h>

typedef struct
{
	int fd; // file descriptor from ioctl
	int verbose; // should bytes sent be printed to stdout
}
Serial;

/* 
 * Function: serialOpen
 *  Opens a port/device for serial access.
 *
 *  s: represents a connection to a serial device through file descriptor
 *  device: full path to serial device (a Create)
 *  baudCode: baud rate. Current Create 2 default is B115200
 *  verbose: talkie-talkie?
 */
void serialOpen(Serial* s, char* device, int baudCode, int verbose);

/*
 * Function serialClose
 *
 * Helper function closes s
 */
void serialClose(Serial* s);

/*
 * Function serialSetBaud
 *
 * Helper function sets the baud rate of s
 */
void serialSetBaud(Serial* s, int baudCode);

/*
 * Function serialClose
 *
 * Helper function sends c to dev at s.
 */
int serialSend(Serial *s, unsigned char c);
int serialNumBytesWaiting(Serial *s);
int serialGetChar(Serial *s, unsigned char *c);
int serialGetSignal(Serial *s, int sig);
void serialSetSignal(Serial *s, int sig);
void serialClearSignal(Serial *s, int sig);

#endif

