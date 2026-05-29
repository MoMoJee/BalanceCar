#include "Turn.h"

/* ========== Turn angular velocity loop parameter config ========== */
#define TURN_KP                 35
#define TURN_KI                 1
#define TURN_SCALE              100
#define TURN_FILTER_SHIFT       2
#define TURN_MAX_TARGET_DPS     180
#define TURN_MAX_OUTPUT         30
#define TURN_GYRO_ZERO_OFFSET   0
#define TURN_OUTPUT_DIR         1

typedef struct {
    int16_t targetDps;
    int32_t actualDps;
    int32_t error;
    int32_t integral;
    int16_t output;
} TurnPID_t;

static TurnPID_t g_turnPid = {0};

static int16_t turn_clamp_i32(int32_t val, int32_t min, int32_t max)
{
    if (val > max) return (int16_t)max;
    if (val < min) return (int16_t)min;
    return (int16_t)val;
}

static int32_t turn_integral_limit(void)
{
#if TURN_KI == 0
    return 0;
#else
    return ((int32_t)TURN_MAX_OUTPUT * TURN_SCALE) / ((TURN_KI > 0) ? TURN_KI : -TURN_KI);
#endif
}

static int16_t turn_raw_to_dps(int16_t gyroZRaw)
{
    int32_t gyro = (int32_t)gyroZRaw - TURN_GYRO_ZERO_OFFSET;

    if (gyro >= 0) {
        return (int16_t)((gyro * 2000 + 16384) / 32768);
    }
    return (int16_t)(-(((-gyro) * 2000 + 16384) / 32768));
}

void Turn_Init(void)
{
    Turn_Reset();
}

void Turn_Reset(void)
{
    g_turnPid.targetDps = 0;
    g_turnPid.actualDps = 0;
    g_turnPid.error = 0;
    g_turnPid.integral = 0;
    g_turnPid.output = 0;
}

void Turn_SetTarget(int16_t targetDps)
{
    g_turnPid.targetDps = turn_clamp_i32(targetDps, -TURN_MAX_TARGET_DPS, TURN_MAX_TARGET_DPS);
}

int16_t Turn_Calc(int16_t gyroZRaw)
{
    int32_t actualDps = turn_raw_to_dps(gyroZRaw);
    int32_t integralLimit;
    int32_t outputRaw;
    int32_t output;

    g_turnPid.actualDps += (actualDps - g_turnPid.actualDps) >> TURN_FILTER_SHIFT;
    g_turnPid.error = (int32_t)g_turnPid.targetDps - g_turnPid.actualDps;

    if (TURN_KI != 0) {
        integralLimit = turn_integral_limit();
        g_turnPid.integral += g_turnPid.error;
        g_turnPid.integral = turn_clamp_i32(g_turnPid.integral, -integralLimit, integralLimit);
    } else {
        g_turnPid.integral = 0;
    }

    outputRaw = TURN_KP * g_turnPid.error + TURN_KI * g_turnPid.integral;
    if (outputRaw >= 0) {
        output = (outputRaw + TURN_SCALE / 2) / TURN_SCALE;
    } else {
        output = -(((-outputRaw) + TURN_SCALE / 2) / TURN_SCALE);
    }

    output = (int32_t)TURN_OUTPUT_DIR * output;
    g_turnPid.output = turn_clamp_i32(output, -TURN_MAX_OUTPUT, TURN_MAX_OUTPUT);
    return g_turnPid.output;
}

int16_t Turn_GetActual(void)
{
    return (int16_t)g_turnPid.actualDps;
}

int16_t Turn_GetTarget(void)
{
    return g_turnPid.targetDps;
}

int16_t Turn_GetOutput(void)
{
    return g_turnPid.output;
}