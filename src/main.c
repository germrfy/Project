//------------------------------------------------------------------------------------------------------
// Demonstration program for Cortex-M0 SoC design
// 1)Enable the interrupt for UART - interrupt when character received
// 2)Go to sleep mode
// 3)On interruption, echo character back to the UART and collect into buffer
// 4)When a whole sentence has been received, invert the case and send it back
// 5)Loop forever doing the above.
//
// Version 4 - October 2015
//------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include "DES_M0_SoC.h"

#define BUF_SIZE						100
#define ASCII_CR						'\r'
#define CASE_BIT						('A' ^ 'a')
#define nLOOPS_per_DELAY		1000000

#define INVERT_LEDS					(pt2GPIO->LED ^= 0xff)

#define ARRAY_SIZE(__x__)       (sizeof(__x__)/sizeof(__x__[0]))

#define XDATA	0x08			// Register addresses of X, Y and Z readings from Accelerometer (8 bit values only)
#define YDATA	0x09
#define ZDATA	0x0A

#define READ_INSTRUCTION	0x0B	// Accelerometer Read Instruction
#define WRITE_INSTRUCTION	0x0A	// Accelerometer Write Instruction
#define GARBAGE_DATA		0x00	// Garbage data for use when reading data

#define SOME_REGISTER		0x00	// **PLACEHOLDER** NOT IMPLEMENTED, DELETE
#define SOME_VALUE			0x00	// **PLACEHOLDER** NOT IMPLEMENTED, DELETE

volatile uint8  counter  = 0; // current number of char received on UART currently in RxBuf[]
volatile uint8  BufReady = 0; // Flag to indicate if there is a sentence worth of data in RxBuf
volatile uint8  RxBuf[BUF_SIZE];
uint8 x_data, y_data, z_data;


//////////////////////////////////////////////////////////////////
// Interrupt service routine, runs when UART interrupt occurs - see cm0dsasm.s
//////////////////////////////////////////////////////////////////
void UART_ISR()		
{
	char c;
	c = pt2UART->RxData;	 // read a character from UART - interrupt only occurs when character waiting
	RxBuf[counter]  = c;   // Store in buffer
	counter++;             // Increment counter to indicate that there is now 1 more character in buffer
	pt2UART->TxData = c;   // write (echo) the character to UART (assuming transmit queue not full!)
	// counter is now the position that the next character should go into
	// If this is the end of the buffer, i.e. if counter==BUF_SIZE-1, then null terminate
	// and indicate the a complete sentence has been received.
	// If the character just put in was a carriage return, do likewise.
	if (counter == BUF_SIZE-1 || c == ASCII_CR)  {
		counter--;							// decrement counter (CR will be over-written)
		RxBuf[counter] = NULL;  // Null terminate
		BufReady       = 1;	    // Indicate to rest of code that a full "sentence" has being received (and is in RxBuf)
	}
}

//////////////////////////////////////////////////////////////////
// Function to send byte to accelerometer via SPI
//////////////////////////////////////////////////////////////////
void sendByte(uint8 byte) {
	pt2SPI->SPIDAT = byte;
	pt2SPI->SPICON = (1 << BIT_POS_ISPI);
	while(pt2SPI->SPICON)
	{
		
	};
}

//////////////////////////////////////////////////////////////////
// Function to send sequence of three bytes to accelerometer via SPI
//////////////////////////////////////////////////////////////////
void sendSequence(uint8 instruction, uint8 address, uint8 data) {
	sendByte(instruction);
	sendByte(address);
	sendByte(data);
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
// Function to send sequence of three bytes to accelerometer via SPI
//////////////////////////////////////////////////////////////////
void getAccelerometerData(uint8 instruction, uint8 data) {
	x_data = readData(XDATA);
	y_data = readData(YDATA);
	z_data = readData(ZDATA);
}

//////////////////////////////////////////////////////////////////
// Function to perform initial set-up of Accelerometer
//////////////////////////////////////////////////////////////////
void initialSetupAccelerometerData(uint8 data) {
	sendSequence(WRITE_INSTRUCTION, SOME_REGISTER, SOME_VALUE);			// FINISH OFF AFTER RESEARCH IS DONE
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
// Main Function
//////////////////////////////////////////////////////////////////
int main(void) {
	uint8 i;
	uint8 TxBuf[ARRAY_SIZE(RxBuf)];

	
	pt2UART->Control = (1 << UART_RX_FIFO_EMPTY_BIT_INT_POS);		// Enable rx data available interrupt, and no others.
  pt2NVIC->Enable	 = (1 << NVIC_UART_BIT_POS);								// Enable interrupts for UART in the NVIC
	wait_n_loops(nLOOPS_per_DELAY);										// wait a little
	
	printf("\r\nWelcome to Cortex-M0 SoC - Edited By Ger & Ian\r\n");			// output welcome message

	
	while(1){			// loop forever
			
			// Do some processing before entering Sleep Mode
			
			pt2GPIO->LED = pt2GPIO->Switches; // Echo the switches onto the LEDs
			wait_n_loops(nLOOPS_per_DELAY);		// delay a little
			INVERT_LEDS;											// invert the 8 rightmost LEDs
			wait_n_loops(nLOOPS_per_DELAY);
			INVERT_LEDS;
			wait_n_loops(nLOOPS_per_DELAY);
			
			printf("\r\nType some characters: ");
			while (BufReady == 0)
			{			
				__wfi();  // Wait For Interrupt: enter Sleep Mode - wake on character received
				pt2GPIO->LED = RxBuf[counter-1];  // display code for character received
			}

			// get here when CR entered or buffer full - do some processing with interrupts disabled
			pt2NVIC->Disable	 = (1 << NVIC_UART_BIT_POS);	// Disable interrupts for UART in the NVIC

			// ---- start of critical section ----
			for (i=0; i<=counter; i++)
			{
				if (RxBuf[i] >= 'A') {							// if this character is a letter (roughly)
					TxBuf[i] = RxBuf[i] ^ CASE_BIT;  // copy to transmit buffer, changing case
				}
				else {
					TxBuf[i] = RxBuf[i];             // non-letter so don't change case
				}
			}
			
			BufReady = 0;	// clear the flag
			counter  = 0; // clear the counter for next sentence
					
			// ---- end of critical section ----		
			
			pt2NVIC->Enable	 = (1 << NVIC_UART_BIT_POS);		// Enable interrupts for UART in the NVIC


			printf("\r\n:--> |%s|\n Number of characters : |%d|\r\n", TxBuf, i);  // print the results between bars
			
			
		} // end of infinite loop

}  // end of main


