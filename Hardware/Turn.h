#ifndef __TURN_H
#define __TURN_H

#include <stdint.h>

void Turn_Init(void);
void Turn_Reset(void);
void Turn_SetTarget(int16_t targetDps);
void Turn_TestTick(void);
int16_t Turn_Calc(int16_t gyroZRaw);
int16_t Turn_GetActual(void);
int16_t Turn_GetTarget(void);
int16_t Turn_GetOutput(void);

#endif