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
#define ASCII_CR						'\r'
#define CASE_BIT						('A' ^ 'a')
#define nLOOPS_per_DELAY		3000000

#define INVERT_LEDS					(pt2GPIO->LED ^= 0xff)

#define ARRAY_SIZE(__x__)       (sizeof(__x__)/sizeof(__x__[0]))

#define XDATA	0x08			// Register addresses of X, Y and Z readings from Accelerometer (8 bit values only)
#define YDATA	0x09
#define ZDATA	0x0A
#define TEMP_L  0x14
#define TEMP_H  0x15

#define READ_INSTRUCTION	0x0B	// Accelerometer Read Instruction
#define WRITE_INSTRUCTION	0x0A	// Accelerometer Write Instruction
#define GARBAGE_DATA		0x00	// Garbage data for use when reading data

#define FILTER_CTL_REG		0x2C	// Address of Filter Control Register
#define FILTER_CTL_VAL		0x00	// Set to lowest ODR and +/- 2g accelerometer readings

#define POWER_CTL_REG		0x2D	// Address of Power Control Register
#define POWER_CTL_VAL		0x02	// Set the accelerometer to Measurement mode disabling autosleep and wakeup modes - use internal clock of accelerometer

#define ISPI_LOW	!(pt2SPI->SPICON & ISPI_MASK)		// checking if ISPI is low or not

volatile uint8  counter  = 0; // current number of char received on UART currently in RxBuf[]
volatile uint8  GetData = 0; // Flag to indicate if there is a sentence worth of data in RxBuf
volatile uint8  RxBuf[BUF_SIZE];


//////////////////////////////////////////////////////////////////
// Interrupt service routine, runs when UART interrupt occurs - see cm0dsasm.s
//////////////////////////////////////////////////////////////////
void UART_ISR()		
{
	char c;
	c = pt2UART->RxData;	 					// read a character from UART - interrupt only occurs when character waiting
	RxBuf[counter]  = c;   						// Store in buffer
	counter++;             						// Increment counter to indicate that there is now 1 more character in buffer
	pt2UART->TxData = c;   						// write (echo) the character to UART (assuming transmit queue not full!)
	if (counter == BUF_SIZE-1 || c == ASCII_CR)  {
		counter--;								// decrement counter (CR will be over-written)
		RxBuf[counter] = NULL;  				// Null terminate
		GetData = 1;	    					// Indicate to code that a user wants some data
	}
}

//////////////////////////////////////////////////////////////////
// Function to send byte to accelerometer via SPI
//////////////////////////////////////////////////////////////////
void sendByte(uint8 byte) {	
	pt2SPI->SPIDAT = byte;
	while(ISPI_LOW)	//Wait until ISPI is set high indicating transmission is complete.
	{
		printf("\n SPICON = %d", pt2SPI->SPICON);
		printf("\n SPIDAT = %d", pt2SPI->SPIDAT);		
	};
		printf("\n OUT! -> SPICON = %d", pt2SPI->SPICON);
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
// Function to read temperature data from accelerometer via SPI
//////////////////////////////////////////////////////////////////
uint16 readTemperature() {
	uint8 t_hi, t_lo;
	t_lo = readData(TEMP_L);
	t_hi = readData(TEMP_H);
	
	return (t_hi << 8) + t_lo;
}

//////////////////////////////////////////////////////////////////
// Function to perform initial set-up of Accelerometer
//////////////////////////////////////////////////////////////////
void initialSetupAccelerometerData() {
	sendSequence(WRITE_INSTRUCTION, FILTER_CTL_REG, FILTER_CTL_VAL);
	sendSequence(WRITE_INSTRUCTION, POWER_CTL_REG, POWER_CTL_VAL);
}

//////////////////////////////////////////////////////////////////
// Function to convert 8 bit accelerometer data into a number of 'g'
//////////////////////////////////////////////////////////////////
float convertToG(uint8 data) {	
		return (data - 0x7F)*4/(0xFF);	//Converting to G for the +/- 2g Range in accelerometer
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
	uint8 x_data, y_data, z_data;
	uint8 TxBuf[ARRAY_SIZE(RxBuf)];
	uint16 t_data;
	
	printf("\r\nWelcome to Cortex-M0 SoC - Accelerometer On Demand\r\n");			// output welcome message

	
	initialSetupAccelerometerData();			// Set up the accelerometer
	
	pt2UART->Control = (1 << UART_RX_FIFO_EMPTY_BIT_INT_POS);			// Enable rx data available interrupt, and no others.
	pt2NVIC->Enable	 = (1 << NVIC_UART_BIT_POS);						// Enable interrupts for UART in the NVIC
	wait_n_loops(nLOOPS_per_DELAY);										// wait a little
	

	
	while(1){			// loop forever
			//Sequence to indicate device is alive during testing
			
			pt2GPIO->LED = readData(pt2GPIO->Switches); 		// Read from acc using address on switches and show result on LEDs
			/* wait_n_loops(nLOOPS_per_DELAY);			// delay a little
			INVERT_LEDS;							// invert the 8 rightmost LEDs
			wait_n_loops(nLOOPS_per_DELAY);
			INVERT_LEDS;
			wait_n_loops(nLOOPS_per_DELAY); 

			printf("\r\nPress 'Return' for data: ");
			while (GetData == 0)
			{			
				__wfi();  							// Wait For Interrupt: enter Sleep Mode - wake on character received
				pt2GPIO->LED = RxBuf[counter-1];  	// display code for character received
			}
			GetData = 0; */							// Reset 'GetData'
			
			wait_n_loops(nLOOPS_per_DELAY);
			x_data = readData(XDATA);
			y_data = readData(YDATA);
			z_data = readData(ZDATA);
			//t_data = readTemperature();
			
			printf("x : |%d|\n", x_data);			
			printf("y : |%d|\n", y_data);
			printf("z : |%d|\n", z_data);
			//printf("temp : |%d|\n", t_data);	
			// printf("x : |%.2f|\n", convertToG(x_data));			
			// printf("y : |%.2f|\n", convertToG(y_data));
			// printf("z : |%.2f|\n", convertToG(z_data));				
			
		} // end of infinite loop

}  // end of main
