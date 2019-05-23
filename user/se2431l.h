#ifndef __SE2431L_H
#define __SE2431L_H


#include "main.h"

#define SE2431L_CPS_PIN				(9999)
#define SE2431L_CSD_PIN				(29)
#define SE2431L_CTX_PIN				(28)


void SE2431L_GpioInit(void);
void SE2431L_TxMode(void);
void SE2431L_SleepMode(void);

#endif
