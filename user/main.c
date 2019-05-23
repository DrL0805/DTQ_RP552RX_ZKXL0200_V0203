#include "main.h"


int main (void)
{
    ret_code_t err_code;
//	nrf_delay_ms(50);
	clocks_start();
	debug_uart_init();			
	timers_init();
	FLASH_Init();
	
	//SPIS��ʼ��	
	spi_gpio_init();
	my_spi_slave_init();
	
	//RADIO�����ʼ��
    err_code = my_rx_esb_init();
    APP_ERROR_CHECK(err_code);
    err_code = nrf_esb_start_rx();
    APP_ERROR_CHECK(err_code);
	
	WDT_Init();
	
	temp_timeout_start();			//RTCУ׼��ʱ��
//	DEBUG_P("sys init ok \r\n");	
	
	while(true)
	{
		// ����ϵͳ���У��ſ�ʼ������һ���յ���2.4G����
		if(!SPI.TX.BusyFlg)
		{
			nrf_rx_data_handler();
		}
		
		WDT_FeedDog();		
//		printf("main running \r\n");
	}
}


/******************************************************************************
  @����:XOR_Cal
  @����:
  @����:* dat �������飬���ڼ�λ��ʼ
		length����������Ҫ���ĳ���
  @���:�������Ľ��
  @����:
******************************************************************************/
uint8_t XOR_Cal(uint8_t * dat,uint16_t length)
{
	uint8_t temp_xor;
	uint16_t i;

	temp_xor = *dat;
	for(i = 1;i < length; i++)
	{
		temp_xor = temp_xor ^ *(dat+i);
	}
	return temp_xor;
}

/******************************************************************************
  @����:gpio_default_init
  @����:51822��gpio����ΪĬ��(ʡ����)״̬
  @����:
  @���:
  @����:
******************************************************************************/
void gpio_default_init(void)
{
    uint32_t i = 0;
    for(i = 0; i< 32 ; ++i ) {
        NRF_GPIO->PIN_CNF[i] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
                               | (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)
                               | (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
                               | (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos)
                               | (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos);
    }
}

/******************************************************************************
  @����:clocks_start
  @����:�����ⲿ16Mʱ��
  @����:
  @���:
  @����:
******************************************************************************/
void clocks_start(void)
{
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;

    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
	
	
	 /* Start low frequency crystal oscillator for app_timer(used by bsp)*/
    NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_RC << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART    = 1;

    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
    {
        // Do nothing.
    }
	
    NRF_CLOCK->EVENTS_DONE = 0;					//RTCУ׼
    NRF_CLOCK->TASKS_CAL = 1;
}


/******************************************************************************
  @����:get_random_number
  @����:��ȡ0~255֮��������
  @����:
  @���:
  @����:
******************************************************************************/
uint8_t get_random_number(void)
{
    NRF_RNG->TASKS_START = 1; // start the RNG peripheral.

	// Wait until the value ready event is generated.
	while (NRF_RNG->EVENTS_VALRDY == 0)
	{
		// Do nothing.
	}
	NRF_RNG->EVENTS_VALRDY = 0;		 // Clear the VALRDY EVENT.
	
	return (uint8_t)NRF_RNG->VALUE;
}

// �Ƚ�������ֵ�Ƿ���ȫ��ͬ��true����ͬ��false����ͬ
bool ArrayCmp(uint8_t *str1, uint8_t *str2, uint8_t len)
{
	uint8_t i;
	for(i = 0; i < len ; i++)
	{
		if(str1[i] != str2[i])
			return false;
	}
	return true;
}


