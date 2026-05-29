#include "stm32f10x.h"
#include "Speed.h"

/* ========== Speed loop parameter config ========== */
#define SPEED_KP                 30
#define SPEED_KI                 2
#define SPEED_TARGET             0
#define SPEED_SCALE              100
#define SPEED_FILTER_SHIFT       2
#define SPEED_MAX_TARGET_ANGLE   8
#define SPEED_LEFT_DIR           1
#define SPEED_RIGHT_DIR          (-1)

typedef struct {
    int32_t speed;
    int32_t error;
    int32_t integral;
    int16_t targetAngle;
} SpeedPID_t;

static SpeedPID_t g_speedPid = {0};

static int16_t speed_clamp_i32(int32_t val, int32_t min, int32_t max)
{
    if (val > max) return (int16_t)max;
    if (val < min) return (int16_t)min;
    return (int16_t)val;
}

static int32_t speed_integral_limit(void)
{
#if SPEED_KI == 0
    return 0;
#else
    return ((int32_t)SPEED_MAX_TARGET_ANGLE * SPEED_SCALE) / ((SPEED_KI > 0) ? SPEED_KI : -SPEED_KI);
#endif
}

static void speed_encoder_timer_init(TIM_TypeDef *timer)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;

    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 65536 - 1;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 1 - 1;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(timer, &TIM_TimeBaseInitStructure);

    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
    TIM_ICInitStructure.TIM_ICFilter = 0xF;
    TIM_ICInit(timer, &TIM_ICInitStructure);

    TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
    TIM_ICInitStructure.TIM_ICFilter = 0xF;
    TIM_ICInit(timer, &TIM_ICInitStructure);

    TIM_EncoderInterfaceConfig(timer, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_SetCounter(timer, 0);
    TIM_Cmd(timer, ENABLE);
}

static void speed_encoder_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    speed_encoder_timer_init(TIM2);
    speed_encoder_timer_init(TIM1);
}

void Speed_Init(void)
{
    speed_encoder_init();
    Speed_Reset();
}

void Speed_Reset(void)
{
    g_speedPid.speed = 0;
    g_speedPid.error = 0;
    g_speedPid.integral = 0;
    g_speedPid.targetAngle = 0;

    TIM_SetCounter(TIM2, 0);
    TIM_SetCounter(TIM1, 0);
}

int16_t Speed_GetLeftEncoder(void)
{
    int16_t encoder = (int16_t)TIM_GetCounter(TIM2);

    TIM_SetCounter(TIM2, 0);
    return (int16_t)(SPEED_LEFT_DIR * encoder);
}

int16_t Speed_GetRightEncoder(void)
{
    int16_t encoder = (int16_t)TIM_GetCounter(TIM1);

    TIM_SetCounter(TIM1, 0);
    return (int16_t)(SPEED_RIGHT_DIR * encoder);
}

int16_t Speed_CalcByEncoder(int16_t leftEncoder, int16_t rightEncoder)
{
    int32_t actualSpeed;
    int32_t integralLimit;
    int32_t outputRaw;
    int32_t output;

    actualSpeed = ((int32_t)leftEncoder + (int32_t)rightEncoder) / 2;
    g_speedPid.speed += (actualSpeed - g_speedPid.speed) >> SPEED_FILTER_SHIFT;
    g_speedPid.error = (int32_t)SPEED_TARGET - g_speedPid.speed;

    if (SPEED_KI != 0) {
        integralLimit = speed_integral_limit();
        g_speedPid.integral += g_speedPid.error;
        g_speedPid.integral = speed_clamp_i32(g_speedPid.integral, -integralLimit, integralLimit);
    } else {
        g_speedPid.integral = 0;
    }

    outputRaw = SPEED_KP * g_speedPid.error + SPEED_KI * g_speedPid.integral;
    if (outputRaw >= 0) {
        output = (outputRaw + SPEED_SCALE / 2) / SPEED_SCALE;
    } else {
        output = -(((-outputRaw) + SPEED_SCALE / 2) / SPEED_SCALE);
    }

    g_speedPid.targetAngle = speed_clamp_i32(output, -SPEED_MAX_TARGET_ANGLE, SPEED_MAX_TARGET_ANGLE);
    return g_speedPid.targetAngle;
}

int16_t Speed_Calc(void)
{
    int16_t leftEncoder = Speed_GetLeftEncoder();
    int16_t rightEncoder = Speed_GetRightEncoder();

    return Speed_CalcByEncoder(leftEncoder, rightEncoder);
}

int16_t Speed_GetActual(void)
{
    return (int16_t)g_speedPid.speed;
}

int16_t Speed_GetTargetAngle(void)
{
    return g_speedPid.targetAngle;
}