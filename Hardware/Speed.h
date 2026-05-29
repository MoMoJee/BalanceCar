#ifndef __SPEED_H
#define __SPEED_H

#include <stdint.h>

void Speed_Init(void);
void Speed_Reset(void);
int16_t Speed_Calc(void);
int16_t Speed_CalcByEncoder(int16_t leftEncoder, int16_t rightEncoder);
int16_t Speed_GetLeftEncoder(void);
int16_t Speed_GetRightEncoder(void);
int16_t Speed_GetActual(void);
int16_t Speed_GetTargetAngle(void);

#endif