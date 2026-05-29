#ifndef __PWM_H
#define __PWM_H

void PWM_Init(void);
void TIM4_Init(void);
void PWM_SetCompare1(uint16_t Compare);
void PWM_SetCompare2(uint16_t Compare);
uint8_t TIM4_GetUpdateFlag(void);
void TIM4_ClearUpdateFlag(void);

#endif
