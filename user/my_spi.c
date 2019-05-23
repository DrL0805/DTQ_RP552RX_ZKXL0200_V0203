#include "my_spi.h"
	
	
uint8_t tx_data_len;
uint8_t m_tx_buf[TX_BUF_SIZE];   			/**< SPI TX buffer. */      
uint8_t m_rx_buf[RX_BUF_SIZE];   			/**< SPI RX buffer. */ 

uint8_t spi_channal_set_flg = false;		//收到频点设置指令后，spi发送缓冲区存入频点配置信息，等待stm32读取	
											//这段时间内不把收到的2.4G数据存入SPI缓冲区
											
SPI_PARAMETERS_T		SPI;											
											
static const nrf_drv_spis_t spis = NRF_DRV_SPIS_INSTANCE(1);


static __INLINE void spi_slave_buffers_init(uint8_t * const p_tx_buf, uint8_t * const p_rx_buf, const uint16_t len)
{
    uint16_t i;
    for (i = 0; i < len; i++)
    {
        p_tx_buf[i] = 0x00;
        p_rx_buf[i] = 0x00;
    }
}


static __INLINE void spi_slave_tx_buffers_init(SPI_CMD_TYPE spi_buf_type)
{
	switch(spi_buf_type)
	{
		case SPI_CMD_NONE:
			memset(m_tx_buf,0x66,TX_BUF_SIZE);		// 0x66是为了万一出错了容易发现问题在哪儿
			break;
		case SPI_CMD_SEND_24G_DATA:
			break;
		case SPI_CMD_SET_CHANNAL:
			m_tx_buf[0] = 0x86;							// 包头
			m_tx_buf[1] = NRF_RX_DEV_ID;				// 设备ID
			m_tx_buf[2] = SPI_CMD_SET_CHANNAL;			// CmdType
			m_tx_buf[3] = 0x01;							// CmdLen
			m_tx_buf[4] = RADIO.RxChannal;
			m_tx_buf[5] = XOR_Cal(m_tx_buf+1,4);		// XOR校验
			m_tx_buf[6] = 0x76;							// 包尾
			memset(m_tx_buf+8,0x00,TX_BUF_SIZE - 8);	// 其他缓冲区值默认为0
			break;
		case SPI_CMD_GET_STATE:
			
			break;
		default:
			break;
	}
}

// SPI相关引脚初始化
// SPI中断脚在使用的时候在初始化，否则在初始化时会给STM32产生一个软中断
void spi_gpio_init(void)
{
//	nrf_gpio_cfg_output(SPIS_IRQ_PIN);
	nrf_gpio_cfg_input(SPIS_CE_PIN, NRF_GPIO_PIN_PULLUP);  
//	nrf_gpio_pin_set(SPIS_IRQ_PIN);
}


/* 产生低电平脉冲，通知stm32中断读取SPI数据 */
void spi_trigger_irq(void)
{	
	static uint8_t SpiPinInitFlg = false;
	
	if(false == SpiPinInitFlg)
	{
		SpiPinInitFlg = true;
		nrf_gpio_cfg_output(SPIS_IRQ_PIN);
	}

//	printf("I \r\n");
	
	nrf_gpio_pin_clear(SPIS_IRQ_PIN);	
	nrf_delay_us(1);
	nrf_gpio_pin_set(SPIS_IRQ_PIN);
}




void spis_event_handler(nrf_drv_spis_event_t event)
{
	switch(event.evt_type)
	{
		case NRF_DRV_SPIS_BUFFERS_SET_DONE:
			break;
		case NRF_DRV_SPIS_XFER_DONE:
			
//			printf("R \r\n");
			
//			if( (0x86              == m_rx_buf[0])       &&
//				(0x76              == m_rx_buf[event.rx_amount - 1]) &&
//				m_rx_buf[event.rx_amount - 2] == XOR_Cal(m_rx_buf+1, event.rx_amount - 3) )
			if(0x86              == m_rx_buf[0] && 
				NRF_RX_DEV_ID    == m_rx_buf[1] && 
				event.rx_amount  >= 7			)
			{
				SPI.RX.Head 		= m_rx_buf[0];
				SPI.RX.DevId 		= m_rx_buf[1];
				SPI.RX.CmdType 		= m_rx_buf[2];
				SPI.RX.CmdLen 		= m_rx_buf[3];
				memcpy(SPI.RX.CmdData, m_rx_buf+4, SPI.RX.CmdLen);
				SPI.RX.Xor 			= m_rx_buf[4+SPI.RX.CmdLen];
				SPI.RX.End 			= m_rx_buf[5+SPI.RX.CmdLen];
				
				switch(SPI.RX.CmdType)
				{
					case SPI_CMD_SET_CHANNAL:	
//						printf("SPI_CMD_SET_CHANNAL \r\n");
//						printf("Ch:%02X \r\n",SPI.RX.CmdData[0]);
						if((SPI.RX.CmdData[0]&0x7F) <= 125)
//						if( 2 == (SPI.RX.CmdData[0]&0x7F) ||
//							1 == (SPI.RX.CmdData[0]&0x7F) ||
//							5 == (SPI.RX.CmdData[0]&0x7F) ||
//							6 == (SPI.RX.CmdData[0]&0x7F) ||
//							9 == (SPI.RX.CmdData[0]&0x7F) ||
//							10 == (SPI.RX.CmdData[0]&0x7F) )
						{
							RADIO.RxChannal = SPI.RX.CmdData[0];
//							
							FLASH_WriteAppData();		// 接收频点写入Flash
							
							//设置接收信道时，RADIO必须处于空闲状态
							nrf_esb_stop_rx();
							nrf_esb_set_rf_channel(RADIO.RxChannal);		
							nrf_esb_start_rx();												
							spi_slave_tx_buffers_init(SPI_CMD_SET_CHANNAL);	
							SPI.SpiTriggerIrqFlg = true;								
						}					
						break;
					case SPI_CMD_GET_STATE:		
//						printf("SPI_CMD_GET_STATE \r\n");
						spi_slave_tx_buffers_init(SPI_CMD_GET_STATE);	
						break;
					case SPI_CMD_GET_24G_DATA:
//						printf("SPI_CMD_GET_24G_DATA \r\n");
						spi_overtime_timer_stop();
						SPI.TX.BusyFlg = false;
						spi_slave_tx_buffers_init(SPI_CMD_NONE);
						break;
					default:
						spi_slave_tx_buffers_init(SPI_CMD_NONE);
						break;
				}
				SPI.RX.CmdType = SPI_CMD_NONE;		// 清空SPI命令类型
			}
			else
			{
//				UART_DEBUG(m_rx_buf, 6);
//				printf("Check Err \r\n");
			}
			
			// SPIS每次收发完数据后需要重新SET下
			nrf_drv_spis_buffers_set(&spis,m_tx_buf,TX_BUF_SIZE,m_rx_buf,RX_BUF_SIZE);
			
			if(SPI.SpiTriggerIrqFlg)
			{
				SPI.SpiTriggerIrqFlg = false;
				spi_trigger_irq();	
			}
			break;
		case NRF_DRV_SPIS_EVT_TYPE_MAX:
			
			break;
		default:
			break;
	}	
}


void my_spi_slave_init(void)
{
    nrf_drv_spis_config_t spis_config = NRF_DRV_SPIS_DEFAULT_CONFIG(1);              
    spis_config.miso_pin = SPIS_MISO_PIN;                                         
    spis_config.mosi_pin = SPIS_MOSI_PIN;                                         
    spis_config.sck_pin = SPIS_SCK_PIN;
    spis_config.csn_pin	= SPIS_CSN_PIN;
    spis_config.mode = NRF_DRV_SPIS_MODE_0;
    spis_config.bit_order = NRF_DRV_SPIS_BIT_ORDER_MSB_FIRST;
    spis_config.def = NRF_DRV_SPIS_DEFAULT_DEF;
    spis_config.orc = NRF_DRV_SPIS_DEFAULT_ORC;
	
    APP_ERROR_CHECK(nrf_drv_spis_init(&spis, &spis_config, spis_event_handler));
	
	spi_slave_buffers_init(m_tx_buf, m_rx_buf, (uint16_t)TX_BUF_SIZE);
	APP_ERROR_CHECK(nrf_drv_spis_buffers_set(&spis,m_tx_buf,TX_BUF_SIZE,m_rx_buf,RX_BUF_SIZE));
}





