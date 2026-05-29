#include "stm32f10x.h"
#include <stdint.h>

/* ========== PID 参数配置 ========== */
#define PID_KP      3000    /* 比例系数 (放大1000倍) */
#define PID_KI      150       /* 积分系数 (放大1000倍) */
#define PID_KD      4500    /* 微分系数 (放大1000倍) */
#define PID_ZERO_OFFSET  (-6)   /* 零点偏置：实测平衡点对应的角度 */

#define PID_DT_MS   5       /* 控制周期 ms */
#define PID_SCALE   1000    /* 定点缩放倍数 */
#define PID_MIN_OUTPUT 12   /* 最小起转补偿：克服静摩擦 */

/* 输出限幅 */
#define OUT_MAX     100
#define OUT_MIN     (-100)

/* ========== PID 状态结构体 ========== */
typedef struct {
    int32_t error;      /* 当前误差 e(k) */
    int32_t error_last; /* 上次误差 e(k-1) */
    int32_t integral;   /* 积分累计 */
    int32_t output;     /* 本次输出 */
} PID_t;

static PID_t g_pid = {0};

/* ========== 限幅函数 ========== */
int16_t clamp_i32(int32_t val, int32_t min, int32_t max)
{
    if (val > max) return (int16_t)max;
    if (val < min) return (int16_t)min;
    return (int16_t)val;
}

static inline int32_t pid_integral_limit(void)
{
#if PID_KI == 0
    return 0;
#else
    return (100 * PID_SCALE) / ((PID_KI > 0) ? PID_KI : -PID_KI);
#endif
}

/* ========== PID 初始化 ========== */
void PID_Init(void)
{
    g_pid.error = 0;
    g_pid.error_last = 0;
    g_pid.integral = 0;
    g_pid.output = 0;
}

/* ========== PID 计算 ========== */
/* 输入 angle: -90 ~ 90 (0度直立), 返回 speed: -100 ~ 100 (0停转) */
int16_t PID_Calc(int16_t angle)
{
    int32_t p_term, i_term, d_term;
    int32_t sum;
    int32_t output_raw;

    /* 计算误差: 目标零点 - 当前角度 */
    g_pid.error = (int32_t)PID_ZERO_OFFSET - (int32_t)angle;

    /* 比例项 */
    p_term = PID_KP * g_pid.error;

    /* 积分项 (带抗饱和) */
    if (PID_KI != 0) {
        int32_t integral_limit = pid_integral_limit();

        g_pid.integral += g_pid.error;
        g_pid.integral = clamp_i32(g_pid.integral, -integral_limit, integral_limit);
        i_term = PID_KI * g_pid.integral;
    } else {
        g_pid.integral = 0;
        i_term = 0;
    }

    /* 微分项 */
    d_term = PID_KD * (g_pid.error - g_pid.error_last);

    /* 保存本次误差 */
    g_pid.error_last = g_pid.error;

    /* 合并输出并缩放，使用四舍五入减少整数截断带来的台阶感 */
    sum = p_term + i_term + d_term;
    if (sum >= 0) {
        output_raw = (sum + PID_SCALE / 2) / PID_SCALE;
    } else {
        output_raw = -(((-sum) + PID_SCALE / 2) / PID_SCALE);
    }

    /* 输出限幅 */
    g_pid.output = clamp_i32(output_raw, OUT_MIN, OUT_MAX);

    /* 最小起转补偿：非零输出至少给 6 */
    if (g_pid.output > 0 && g_pid.output < PID_MIN_OUTPUT) {
        g_pid.output = PID_MIN_OUTPUT;
    } else if (g_pid.output < 0 && g_pid.output > -PID_MIN_OUTPUT) {
        g_pid.output = -PID_MIN_OUTPUT;
    }

    return (int16_t)g_pid.output;
}

/* ========== 调参辅助：修改参数 ========== */
void PID_SetParam(int32_t kp, int32_t ki, int32_t kd)
{
    /* 参数同样为放大1000倍后的值 */
    /* 可在调试时通过串口/按键动态调整 */
    (void)kp; (void)ki; (void)kd; /* 示例占位 */
}
