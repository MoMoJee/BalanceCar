#ifndef __BALANCE_H
#define __BALANCE_H

#include <stdint.h>

/* 限幅函数 */
int16_t clamp_i32(int32_t val, int32_t min, int32_t max);

/* PID 初始化 */
void PID_Init(void);

/* 设置速度环给角度环的目标角度修正量 */
void PID_SetTargetOffset(int16_t targetOffset);

/* PID 计算：输入角度(-90~90)，输出电机PWM(-100~100) */
int16_t PID_Calc(int16_t angle);

/* 动态调参：参数为放大1000倍后的整数值 */
void PID_SetParam(int32_t kp, int32_t ki, int32_t kd);

#endif