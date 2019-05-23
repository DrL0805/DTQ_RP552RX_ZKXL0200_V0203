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

// ֡�Ź����߼��� true�����ˣ����͸�����������false���� 
bool RADIO_SeqFilter(uint8_t* UID, uint8_t Seq)
{
	uint8_t PublicUid[4] = {0,0,0,0};
	
	// UIDȫΪ0��������
	if(ArrayCmp(PublicUid, UID, 4))
		return true;
	
	// ֡�Ź���������������UID�������д˴�����������֡����ͬ
	for(uint8_t i=0;i<DTQ_MAX_NUM;i++)
	{ 
		if(ArrayCmp(RADIO.DTQ[i].UID, UID, 4))
		{
			// ֡����ͬ�����ˣ�֡�Ų�ͬ������֡�Ų����͸�������
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
	
	// �µĴ����������뻺��
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
		 
		/* ��ͷ����β�����У����ȷ����SPI���� */
		if(rx_payload.data[0]				  == NRF_DATA_HEAD &&
		   rx_payload.data[rx_payload.length-1] == NRF_DATA_END  &&
		   rx_payload.data[rx_payload.length-2] == XOR_Cal(rx_payload.data+1,rx_payload.length-3))
		{
//			radio_debug("Rx \r\n");
			
//			UART_DEBUG(rx_payload.data, rx_payload.length);
			
			//������ͬ֡��
//			if(RADIO_SeqFilter(rx_payload.data+1,rx_payload.data[11]))
			{
				/* SPI�����Э���װ */
				tx_data_len 			  = rx_payload.length + 7;						// SPI���������ܳ���
				m_tx_buf[0] 			  = 0x86;										// ��ͷ
				m_tx_buf[1]				  = 0x01;										// �豸ID
				m_tx_buf[2]				  = 0x22;										// CmdType
				m_tx_buf[3]				  = rx_payload.length+1;						// CmdLen
				m_tx_buf[4]				  = 0x7F & rx_payload.rssi;                     // CmdData
				memcpy(m_tx_buf+5, rx_payload.data, rx_payload.length);
				m_tx_buf[tx_data_len - 2] = XOR_Cal(m_tx_buf+1, tx_data_len-3);			//���У��
				m_tx_buf[tx_data_len - 1] = 0x76;										//��β
				
				// ���淢�͵�SPI���ݣ���һ����δ���ͳɹ����������ط�
				SPI.TX.Len = tx_data_len;
				memcpy(SPI.TX.Data, m_tx_buf, SPI.TX.Len);
				
				/* �����͵�ƽ���壬֪ͨstm32�ж϶�ȡSPI���� */
				spi_trigger_irq();
				
				//SPI���䳬ʱ��ʱ��
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
	nrf_esb_config.selective_auto_ack       = true;	//lj ��⣬����trueʱ��ÿ�η��͵�ʱ���ѡ��ز��ظ�ACK
	nrf_esb_config.payload_length           = 250;
	
	err_code = nrf_esb_set_rf_channel(12);		//ע�⣺����������Ƶ��61����Ƶ��21���������෴
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

	err_code = nrf_esb_set_rf_channel(RADIO.RxChannal);		//ע�⣺��������������շ�Ƶ���෴
	VERIFY_SUCCESS(err_code);

	err_code = nrf_esb_set_base_address_0(base_addr_0);
	VERIFY_SUCCESS(err_code);

	err_code = nrf_esb_set_prefixes(addr_prefix, 8);
	VERIFY_SUCCESS(err_code);
	
	err_code = nrf_esb_init(&nrf_esb_config);
	VERIFY_SUCCESS(err_code);	

	return err_code;
}















