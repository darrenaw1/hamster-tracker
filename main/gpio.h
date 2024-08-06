#ifndef GPIO_H
#define GPIO_H

#include "driver/gpio.h"

#define PIR_INPUT GPIO_NUM_5

void gpioInitialise();
int getPirLevel();
void startPirTask();
uint16_t getPirCounter();

#endif // GPIO_H