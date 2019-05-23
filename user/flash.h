#ifndef __FLASH_H
#define __FLASH_H	 

// Includes -----------------------------------------------------------
//#include "nrf_gpio.h"
#include "main.h"

// Defines ------------------------------------------------------------

/*
	Flash存储地址分配，为读写方便，每种信息至少占1字（4Byte）：
	0x3FC00~0x3FC03，基地址+0：接收器UID			
	0x3FC04~0x3FC07，基地址+4：答题器UID			
	0x3FC08~0x3FC0B，基地址+8：答题器配对序号		
	0x3FC0C~0x3FC0F，基地址+12：答题器发送频点
	0x3FC10~0x3FC13，基地址+16：答题器接收频点
	
	
*/

// 地址偏移，单位为word（4Byte）
#define		OFFSET_ADDR_JSQ_UID				(0)
#define		OFFSET_ADDR_DTQ_UID				(OFFSET_ADDR_JSQ_UID+1)
#define		OFFSET_ADDR_DTQ_SERIAL			(OFFSET_ADDR_DTQ_UID+1)
#define		OFFSET_ADDR_TX_CHANNAL			(OFFSET_ADDR_DTQ_SERIAL+1)
#define		OFFSET_ADDR_RX_CHANNAL			(OFFSET_ADDR_TX_CHANNAL+1)




// Typedefs -----------------------------------------------------------
typedef struct 
{
//	uint32_t 		PageSize;			// == 2014
//	uint32_t		PageNum;			// == 256
	uint32_t*		UserAddr;			// 存储用户信息的地址，为51822最后一页flash，地址为0x3FC00，共1K
}FLASH_PARAMETERS_T;

// Functions ----------------------------------------------------------
extern void FLASH_Init(void);
extern void FLASH_WriteAppData(void);
extern void FLASH_ReadAppData(void);
extern void flash_page_erase(uint32_t * page_address);
extern void flash_word_write(uint32_t * address, uint32_t value);
// Globals ------------------------------------------------------------
extern FLASH_PARAMETERS_T 		FLASH;

		 				    
#endif














