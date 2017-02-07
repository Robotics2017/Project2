#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "oi.h"
#include "serial.h"

enum bool {false, true};
typedef unsigned char byte;
unsigned char color = 255;

void init(Serial* serial, byte state)
{
    serialClose(serial);
    serialOpen(serial, "/dev/ttyUSB0", B115200, true);
    serialSend(serial, 128);    // Send Start
    serialSend(serial, state);    // Send state

    byte b;
    while( serialNumBytesWaiting(serial) > 0 )
   	 serialGetChar(serial, &b);
};

unsigned char bump(Serial* serial)
{
    unsigned char c = 0;
    serialSend(serial, 142);
    serialSend(serial, 7); //bumps and wheel drops
    serialGetChar(serial, &c);
    c &= 3; //example, c&=3 discards wheel drops
    return c;
};

void changeColor(Serial* serial)
{
    serialSend(serial, 139);
    serialSend(serial, UserButton);
    serialSend(serial, color);
    serialSend(serial, 255); //was half intensity (128), now full intensity
};

void activateLED(Serial* serial, unsigned char selectLED)
{
    serialSend(serial, 139);
    serialSend(serial, selectLED);
    serialSend(serial, color);
    serialSend(serial, 255);
};

int main(void)
{
    unsigned char stepCount = 16;
    
    const unsigned char BOTH_BUMPERS = 3;
    const unsigned char LEFT_BUMPER = 2;
    const unsigned char RIGHT_BUMPER = 1;
    unsigned char returnSignal = 0;
    
    Serial serial;
    init(&serial, 132); //full mode
    
    // display initial color
    changeColor(&serial);

    while (true)
    {
   	 
	int loopCount = 0;
	
	while ((time(NULL) - start) < 1) {
	
	returnSignal = bump(&serial);
	
   	 if (returnSignal != 0) {
   		 if (returnSignal == BOTH_BUMPERS) {
   			 activateLED(&serial, 9); // check robot led
   		 }
   		 else {
   			 if (returnSignal == LEFT_BUMPER) {
   				 activateLED(&serial, 8); // check robot led
   			 }
   			 if (returnSignal == RIGHT_BUMPER) {
   				 activateLED(&serial, 1); // debris led
   			 }
   		 }

   	 }
   	 usleep(100000);
   	 }// end while(loopCount)

   	 if ((color - stepCount) > 0) { // this works up to stepCount - 1
   		 color -= stepCount;
   		 changeColor(&serial);
   	 } else { // finishes off remaining iteration
   		 if (color > 0) {
   			 color = 0;
   			 changeColor(&serial);
   		 } else { // color = 0
   			 color = 255;
			 changeColor(&serial);
   		 }
   	 }
   	 
    }
    
    return 0;
}
