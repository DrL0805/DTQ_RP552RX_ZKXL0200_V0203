#ifndef _MY_SPI_H
#define _MY_SPI_H

#include "main.h"

#define TX_BUF_SIZE   (255)           	  /**< SPI TX buffer size. */      
#define RX_BUF_SIZE   TX_BUF_SIZE       /**< SPI RX buffer size. */  

#define SPI_MAX_NUMBER_OF_RETRANSMITS	(3)		// 最大重发次数，包括第一次



#define SPIS_IRQ_PIN   0     
#define SPIS_CE_PIN    1
#define SPIS_MOSI_PIN  2    // SPI MOSI signal. 
#define SPIS_MISO_PIN  3    // SPI MISO signal. 
#define SPIS_SCK_PIN   4    // SPI SCK signal.
#define SPIS_CSN_PIN   5    // SPI CSN signal. 


#define SPI_DATA_HEAD			(0x86)
#define SPI_DATA_END			(0x76)


typedef enum
{
	SPI_CMD_NONE = 0x00,
	SPI_CMD_SET_CHANNAL = 0x20,
	SPI_CMD_GET_STATE = 0x21,
	SPI_CMD_GET_24G_DATA = 0x22,
	SPI_CMD_SEND_24G_DATA = 0x23
}SPI_CMD_TYPE;

typedef struct
{
	uint8_t			Head;
	uint8_t 		DevId;
	uint8_t			CmdType;
	uint8_t			CmdLen;
	uint8_t			CmdData[255];
	uint8_t			Xor;
	uint8_t			End;
}SPI_CMD_FORMAT_T;


// 仅用于收到的2.4G数据的重发和确认
typedef struct
{
	bool					BusyFlg;		
	
	uint8_t					RetransmitCnt;		
	
	uint8_t 				Len;					
	uint8_t					Data[255];	
}SPI_TX_FORMAT_T;

typedef struct
{
	SPI_CMD_FORMAT_T		RX;
	SPI_TX_FORMAT_T			TX;
	bool					SpiTriggerIrqFlg;
}SPI_PARAMETERS_T;



extern uint8_t tx_data_len;
extern uint8_t m_tx_buf[TX_BUF_SIZE];        
extern uint8_t m_rx_buf[RX_BUF_SIZE];   
extern uint8_t spi_channal_set_flg;

extern SPI_PARAMETERS_T		SPI;

void my_spi_slave_init(void);
void spi_gpio_init(void);
void spi_gpio_init(void);
void spi_trigger_irq(void);
	
#endif


