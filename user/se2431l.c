#include "se2431l.h"





void SE2431L_GpioInit(void)
{
	nrf_gpio_cfg_output(SE2431L_CSD_PIN);
	nrf_gpio_cfg_output(SE2431L_CTX_PIN);
}

void SE2431L_TxMode(void)
{
	nrf_gpio_pin_set(SE2431L_CSD_PIN);
	nrf_gpio_pin_set(SE2431L_CTX_PIN);
}


void SE2431L_SleepMode(void)
{
	nrf_gpio_pin_clear(SE2431L_CSD_PIN);
	nrf_gpio_pin_clear(SE2431L_CTX_PIN);
}

void SE2431L_RxMode(void)
{
	
}















