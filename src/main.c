//------------------------------------------------------------------------------------------------------
// Software to interact with an accelerometer (via SPI) and display data to user
// 1)Enable the interrupt for UART - interrupt when character received
// 2)Go to sleep mode
// 3)On interruption if the character is 'Return' enable rest of program to continue otherwise echo back to user
// 4)Display X, Y and Z accelerometer data
// 5)Loop forever doing the above.
//
// Version 1 - November 2015
//------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include "DES_M0_SoC.h"

#define BUF_SIZE						100
#define ASCII_CR						'\r'		// ASCII for new line
#define ASCII_X							'x'			// ASCII for 'x'
#define ASCII_Y							'y'			// ASCII for 'y'
#define ASCII_Z							'z'			// ASCII for 'z'
#define ASCII_A							'a'			// ASCII for 'a'
#define ASCII_DIF						32			// Difference between capital and lower case letters in ASCII
#define CHANGEMODE					'*'			// Changes the mode of operation of the device to and from continuous measurement mode. 
#define nLOOPS_per_DELAY		6000000	
#define THRESHOLD				10				// Threshold to show LEDS
#define INITIALPATTERN			0x180			//// 0000000110000000 LED PATTERN

#define XDATA	8			// Register addresses of X, Y and Z readings from Accelerometer (8 bit values only)
#define YDATA	9
#define ZDATA	10

#define READ_INSTRUCTION	0x0B	// Accelerometer Read Instruction
#define WRITE_INSTRUCTION	0x0A	// Accelerometer Write Instruction
#define GARBAGE_DATA		0x00	// Garbage data for use when reading data

#define FILTER_CTL_REG		0x2C	// Address of Filter Control Register
#define FILTER_CTL_VAL		0x13	// Set to 100HZ ODR, Half BW and +/- 2g accelerometer readings

#define POWER_CTL_REG		0x2D	// Address of Power Control Register
#define POWER_CTL_VAL		0x02	// Set the accelerometer to Measurement mode disabling autosleep and wakeup modes - use internal clock of accelerometer

#define ACCEL_RANGE 2					// the range of the accelerometer is +/- 2g

#define ISPI_LOW	!(pt2SPI->SPICON & ISPI_MASK)		// checking if ISPI is low or not

volatile uint8  GetData = 0; 	// Flag to indicate if there is a sentence worth of data in RxBuf
volatile char enteredChar;		// Variable to hold character entered by the user (Determines the mode of the measurement device - Used to pass user selection into main)

//////////////////////////////////////////////////////////////////
// Interrupt service routine, runs when UART interrupt occurs - see cm0dsasm.s
//////////////////////////////////////////////////////////////////
void UART_ISR()		
{
	enteredChar = pt2UART->RxData;	 					// read a character from UART - interrupt only occurs when character waiting
	pt2UART->TxData = enteredChar;   					// write (echo) the character to UART (assuming transmit queue not full!)
	GetData = 1;	    												// Indicate to main that a user wants some data
}

//////////////////////////////////////////////////////////////////
// Function to send byte to accelerometer via SPI
//////////////////////////////////////////////////////////////////
void sendByte(uint8 byte) {	
	pt2SPI->SPIDAT = byte;
	while(ISPI_LOW)	//Wait until ISPI is set high indicating transmission is complete.
	{			
	};
	pt2SPI->SPICON = pt2SPI->SPICON & ZERO_ISPI;		// Reset ISPI to zero
}

//////////////////////////////////////////////////////////////////
// Function to send sequence of three bytes to accelerometer via SPI
//////////////////////////////////////////////////////////////////
void sendSequence(uint8 instruction, uint8 address, uint8 data) {
	pt2SPI->SPICON = (1 << BIT_POS_CS);					//Set CS to one
	sendByte(instruction);
	sendByte(address);
	sendByte(data);
	pt2SPI->SPICON = 0;			//Reset CS (and ISPI) to zero
}

//////////////////////////////////////////////////////////////////
// Function to read data from accelerometer via SPI
// Sends three bytes to SPI then reads from SPIDAT
//////////////////////////////////////////////////////////////////
uint8 readData(uint8 address) {
	sendSequence(READ_INSTRUCTION, address, GARBAGE_DATA);
	return pt2SPI->SPIDAT;
}

//////////////////////////////////////////////////////////////////
// Function to perform initial set-up of Accelerometer
//////////////////////////////////////////////////////////////////
void initialSetupAccelerometerData() {
	sendSequence(WRITE_INSTRUCTION, FILTER_CTL_REG, FILTER_CTL_VAL);		// Write to Filter Control Reg first
	sendSequence(WRITE_INSTRUCTION, POWER_CTL_REG, POWER_CTL_VAL);			// Write to Power Control Reg -> Measurement mode
}

//////////////////////////////////////////////////////////////////
// Function to convert 8 bit accelerometer data into a number of 'g'
//////////////////////////////////////////////////////////////////
float convertToG(int8 data) {
	float dataFloat = 0;
	dataFloat = (float) (data);				// Cast 'data' to float
	return (dataFloat/128)*2;
}

//////////////////////////////////////////////////////////////////
// Software delay function
//////////////////////////////////////////////////////////////////
void wait_n_loops(uint32 n) {
	volatile uint32 i;
		for(i=0;i<n;i++){
			;
		}
}

//////////////////////////////////////////////////////////////////
// Function to Display pattern on LEDs based on 
//////////////////////////////////////////////////////////////////
uint16 rollingLED(uint16 pattern, int8 x) {
	if (x < -THRESHOLD)
	{
		pattern = (pattern << 1);
	}
	else if (x > THRESHOLD)
	{
		pattern = (pattern >> 1);
	}
	else if (x < THRESHOLD && x > -THRESHOLD)
	{
		if(pattern > INITIALPATTERN)
		{
			pattern = (pattern >> 1);
		}
		else if (pattern < INITIALPATTERN)
		{
			pattern = (pattern << 1);
		}
	}
	pt2GPIO->LED = pattern;
	return pattern;
}

//////////////////////////////////////////////////////////////////
// Print accelerometer data based on user input
//////////////////////////////////////////////////////////////////
void printCommand(int8 x, int8 y, int8 z) {	
	printf("\r\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");			//	Clear screen
	if(enteredChar == ASCII_X || enteredChar == ASCII_X - ASCII_DIF)			// User wants X measurement
	{
		printf("\r\n\n\t\tX : |%0.4fg| ", convertToG(x));
	}
	else if(enteredChar == ASCII_Y || enteredChar == ASCII_Y - ASCII_DIF)	//User wants Y measurement
	{
		printf("\r\n\n\t\tY : |%0.4fg| ", convertToG(y));
	}
	else if(enteredChar == ASCII_Z || enteredChar == ASCII_Z - ASCII_DIF)	//User wants Z measurement
	{
		printf("\r\n\n\t\tZ : |%0.4fg| ", convertToG(z));
	}
	else if(enteredChar == ASCII_A || enteredChar == ASCII_A - ASCII_DIF)	//User wants X,Y and Z measurements
	{	
		printf("\r\n\n\t\tX : |%0.4fg| ", convertToG(x));
		printf("\r\n\n\t\tY : |%0.4fg| ", convertToG(y));
		printf("\r\n\n\t\tZ : |%0.4fg| ", convertToG(z));
	}
	else
	{
		printf("\r\n\nUnrecognised Command |%c|", enteredChar);					// User entered char that was not [x,y,z,X,Y,Z,*]
	}
}

//////////////////////////////////////////////////////////////////
// Main Function
//////////////////////////////////////////////////////////////////
int main(void) {
	int8 x_data, y_data, z_data;					// variables to hold x,y,z data
	uint16 ledPattern = INITIALPATTERN;						// 0000000110000000 LED PATTERN
	int i = 0;														// counter to be used in continuous measurement mode
	
	initialSetupAccelerometerData();			// Set up the accelerometer
	
	pt2UART->Control = (1 << UART_RX_FIFO_EMPTY_BIT_INT_POS);	// Enable rx data available interrupt, and no others.
	pt2NVIC->Enable	 = (1 << NVIC_UART_BIT_POS);							// Enable interrupts for UART in the NVIC
	wait_n_loops(nLOOPS_per_DELAY);														// wait a little
	
	printf("\r\n\nWelcome to Console - Accelerometer On Demand \r\nBrought to you by Ger&Ian%c", 169);			// output welcome message
	
	while(1){			// loop forever
		
			while (GetData == 0 && enteredChar != CHANGEMODE)				// User is in normal mode of measurement (requesting individual measurements)
			{
				printf("\r\n\nPress 'x', 'y', 'z' or 'a' for acceleration data\r\n(Press '*' to switch to continuous measurement mode)\n");
				__wfi();  														// Wait For Interrupt: enter Sleep Mode - wake on character received
				pt2GPIO->LED = enteredChar;  					// display code for character received
			}
			GetData = 0; 														// Reset 'GetData' for next interrupt
			
			x_data = readData(XDATA);								// Get x acceleration data from accelerometer
			y_data = readData(YDATA);								// Get y acceleration data from accelerometer
			z_data = readData(ZDATA);								// Get z acceleration data from accelerometer		
			if (enteredChar == CHANGEMODE)					// If we are in Continuous Measurement Mode (i.e. user entered a '*')
			{
				wait_n_loops(nLOOPS_per_DELAY);
				ledPattern = rollingLED(ledPattern, x_data);
				if(i%10 == 0)													// Print the reminder and the Table heading every 10 measurements
				{
					printf("\r\n\nTo Change Mode Back Enter a character that is not '*' ");
					printf("\r\n\n    X\t\t\t    Y\t\t\t    Z");
				}
				printf("\r\n\n|%0.4fg|\t\t|%0.4fg|\t\t|%0.4fg| ", convertToG(x_data), convertToG(y_data), convertToG(z_data));	// Print Data			
				i++;				// increment continuous measurement mode counter
			}
			else																						// we are in the normal mode (printing requested data)
			{
				printCommand(x_data, y_data, z_data);					// Print requested data
				i = 0;																				// Reset continuous measurement mode counter
			}
			
		} // end of infinite loop

}  // end of main
