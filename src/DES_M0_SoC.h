// DES_M0_SoC.h
#ifndef DES_M0_SoC_ALREADY_INCLUDED
#define DES_M0_SoC_ALREADY_INCLUDED

typedef unsigned       char uint8;
typedef   signed       char  int8;
typedef unsigned short int  uint16;
typedef   signed short int   int16;
typedef unsigned       int  uint32;
typedef   signed       int   int32;

#pragma anon_unions
typedef struct {
	union {
		volatile uint8   RxData;
		volatile uint32  reserved0;
	};
	union {
		volatile uint8   TxData;
		volatile uint32  reserved1;
	};
	union {
		volatile uint8   Status;
		volatile uint32  reserved2;
	};
	union {
		volatile uint8   Control;
		volatile uint32  reserved3;
	};
} UART_t;
// bit position defs for the UART status register
#define UART_TX_FIFO_FULL_BIT_POS		0			// Tx FIFO full
#define UART_TX_FIFO_EMPTY_BIT_POS	1			// Rx FIFO empty
#define UART_RX_FIFO_FULL_BIT_POS		2			// Rx FIFO full
#define UART_RX_FIFO_EMPTY_BIT_POS	3			// Rx FIFO not empty (data available)
// corresponding bit position defs for the UART interrupts enables in the control register`
#define UART_TX_FIFO_FULL_INT_BIT_POS		(UART_TX_FIFO_FULL_BIT_POS )	// Tx FIFO full
#define UART_TX_FIFO_EMPTY_INT_BIT_POS	(UART_TX_FIFO_EMPTY_BIT_POS)	// Rx FIFO empty
#define UART_RX_FIFO_FULL_BIT_INT_POS		(UART_RX_FIFO_FULL_BIT_POS )	// Rx FIFO full
#define UART_RX_FIFO_EMPTY_BIT_INT_POS	(UART_RX_FIFO_EMPTY_BIT_POS)	// Rx FIFO data available


typedef struct {
	union {
		volatile uint32	Enable;
		volatile uint32	reserved0;
	};
	volatile uint32		reserved1[0x20-1];  // force next guy to to 0x80 bytes = 0x20 words away from start of this one
	union {
		volatile uint32	Disable;
		volatile uint32	reserved2;
	};
} NVIC_t;
#define NVIC_UART_BIT_POS		1      // bit position of UART in ARM's interrupt control register



typedef struct {
	union {
		volatile uint16  LED;
		volatile uint32  reserved0;
	};
	union {
		volatile uint16  NotConnected;
		volatile uint32  reserved1;
	};
	union {
		volatile uint16  Switches;
		volatile uint32  reserved2;
	};
	union {
		volatile uint16  Buttons;
		volatile uint32  reserved3;
	};
} GPIO_t;

typedef struct {
	union {
		volatile uint8 SPIDAT;				// Data Register
		volatile uint32 reserved0;
	};
	union {
		volatile uint8 SPICON;				// Configuration/Status Register
		volatile uint32 reserved1;
	};	
} SPI_t;
#define BIT_POS_ISPI  1			// Bit position of ISPI in the SPICON memory location
#define ISPI_MASK  0x02			// Mask ISPI bit
#define BIT_POS_CS  0				// Bit position of ISPI in the SPICON memory location
#define ZERO_ISPI  0xfd			// AND SPICON with this to set ISPI to zero (11111101)
#define ZERO_CS  	0xfe			// AND SPICON with this to set CS to zero (11111110)


// use above typedefs to define the memory map.
#define pt2NVIC ((NVIC_t *)0xE000E100)
#define pt2SPI	((SPI_t *)0x52000000)
#define pt2UART ((UART_t *)0x51000000)
#define pt2GPIO ((GPIO_t *)0x50000000)


#endif
