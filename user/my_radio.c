#include "my_radio.h"

 

nrf_esb_payload_t        tx_payload;
nrf_esb_payload_t        rx_payload;
RADIO_PARAMETERS_T		 RADIO;

void nrf_esb_event_handler(nrf_esb_evt_t const * p_event)
{
    switch (p_event->evt_id)
    {
        case NRF_ESB_EVENT_TX_SUCCESS:
//            radio_debug("TX SUCCESS EVENT\r\n");
            break;
        case NRF_ESB_EVENT_TX_FAILED:
//            radio_debug("TX FAILED EVENT\r\n");
            break;
        case NRF_ESB_EVENT_RX_RECEIVED:
            break;
    }
}

// 帧号过滤逻辑， true不过滤（发送给接收器），false过滤 
bool RADIO_SeqFilter(uint8_t* UID, uint8_t Seq)
{
	uint8_t PublicUid[4] = {0,0,0,0};
	
	// UID全为0，不过滤
	if(ArrayCmp(PublicUid, UID, 4))
		return true;
	
	// 帧号过滤条件：答题器UID名单中有此答题器，并且帧号相同
	for(uint8_t i=0;i<DTQ_MAX_NUM;i++)
	{ 
		if(ArrayCmp(RADIO.DTQ[i].UID, UID, 4))
		{
			// 帧号相同，过滤；帧号不同，更新帧号并发送给接收器
			if(Seq == RADIO.DTQ[i].LastSeqNum)
			{
				return false;
			}	
			else
			{
				RADIO.DTQ[i].LastSeqNum = Seq;
				return true;
			}
		}
	}
	
	// 新的答题器，存入缓冲
	memcpy(RADIO.DTQ[RADIO.pLastDtq].UID, UID, 4);
	RADIO.DTQ[RADIO.pLastDtq].LastSeqNum = Seq;
	
	if(RADIO.pLastDtq < DTQ_MAX_NUM)
		RADIO.pLastDtq++;
	else
		RADIO.pLastDtq = 0;
	
	return true;
}

void nrf_rx_data_handler(void)
{
	 if(nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS)
	 {
		 
//		 UART_DEBUG(rx_payload.data, rx_payload.length);
//		 radio_debug("%02X \r\n",rx_payload.data[12]);
		 
		/* 包头、包尾、异或校验正确则开启SPI传输 */
		if(rx_payload.data[0]				  == NRF_DATA_HEAD &&
		   rx_payload.data[rx_payload.length-1] == NRF_DATA_END  &&
		   rx_payload.data[rx_payload.length-2] == XOR_Cal(rx_payload.data+1,rx_payload.length-3))
		{
//			radio_debug("Rx \r\n");
			
//			UART_DEBUG(rx_payload.data, rx_payload.length);
			
			//过滤相同帧号
//			if(RADIO_SeqFilter(rx_payload.data+1,rx_payload.data[11]))
			{
				/* SPI传输层协议封装 */
				tx_data_len 			  = rx_payload.length + 7;						// SPI传输数据总长度
				m_tx_buf[0] 			  = 0x86;										// 包头
				m_tx_buf[1]				  = 0x01;										// 设备ID
				m_tx_buf[2]				  = 0x22;										// CmdType
				m_tx_buf[3]				  = rx_payload.length+1;						// CmdLen
				m_tx_buf[4]				  = 0x7F & rx_payload.rssi;                     // CmdData
				memcpy(m_tx_buf+5, rx_payload.data, rx_payload.length);
				m_tx_buf[tx_data_len - 2] = XOR_Cal(m_tx_buf+1, tx_data_len-3);			//异或校验
				m_tx_buf[tx_data_len - 1] = 0x76;										//包尾
				
				// 保存发送的SPI数据，第一次若未发送成功，将用于重发
				SPI.TX.Len = tx_data_len;
				memcpy(SPI.TX.Data, m_tx_buf, SPI.TX.Len);
				
				/* 产生低电平脉冲，通知stm32中断读取SPI数据 */
				spi_trigger_irq();
				
				//SPI传输超时定时器
				SPI.TX.BusyFlg = true;
				SPI.TX.RetransmitCnt = 0;
				spi_overtime_timer_stop();
				spi_overtime_timer_start();
			}
		}
		else
		{
//			radio_debug("ringbuf read data check err  \r\n");
		}
	}
}

uint32_t my_tx_esb_init(void)
{
    uint32_t err_code;
    uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8 };

    nrf_esb_config_t nrf_esb_config         = NRF_ESB_DEFAULT_CONFIG;
    nrf_esb_config.protocol                 = NRF_ESB_PROTOCOL_ESB_DPL;
    nrf_esb_config.retransmit_delay         = 600;
    nrf_esb_config.bitrate                  = NRF_ESB_BITRATE_1MBPS;
    nrf_esb_config.event_handler            = nrf_esb_event_handler;
    nrf_esb_config.mode                     = NRF_ESB_MODE_PTX;
	nrf_esb_config.selective_auto_ack       = true;	//lj 理解，等于true时，每次发送的时候可选择回不回复ACK
	nrf_esb_config.payload_length           = 250;
	
	err_code = nrf_esb_set_rf_channel(12);		//注意：答题器发送频点61接收频点21，接收器相反
	VERIFY_SUCCESS(err_code);
	
    err_code = nrf_esb_init(&nrf_esb_config);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_0(base_addr_0);
    VERIFY_SUCCESS(err_code);
	
    err_code = nrf_esb_set_prefixes(addr_prefix, 8);
    VERIFY_SUCCESS(err_code);

    return err_code;
}

uint32_t my_rx_esb_init(void)
{
	uint32_t err_code;
	uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
	uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8 };

	nrf_esb_config_t nrf_esb_config         = NRF_ESB_DEFAULT_CONFIG;
	nrf_esb_config.protocol                 = NRF_ESB_PROTOCOL_ESB_DPL;
	nrf_esb_config.retransmit_delay         = 600;
	nrf_esb_config.bitrate                  = NRF_ESB_BITRATE_2MBPS;
	nrf_esb_config.event_handler            = nrf_esb_event_handler;
	nrf_esb_config.mode                     = NRF_ESB_MODE_PRX;
	nrf_esb_config.selective_auto_ack       = true;	
	nrf_esb_config.payload_length           = 250;

	err_code = nrf_esb_set_rf_channel(RADIO.RxChannal);		//注意：答题器与接收器收发频点相反
	VERIFY_SUCCESS(err_code);

	err_code = nrf_esb_set_base_address_0(base_addr_0);
	VERIFY_SUCCESS(err_code);

	err_code = nrf_esb_set_prefixes(addr_prefix, 8);
	VERIFY_SUCCESS(err_code);
	
	err_code = nrf_esb_init(&nrf_esb_config);
	VERIFY_SUCCESS(err_code);	

	return err_code;
}















