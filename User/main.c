#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "LED.h"
//打开时应调整编码格式
#include "MPU6050.h"
#include "PWM.h"
#include "Motortwo.h"
#include "Blance.h"
#include "Speed.h"
#include <math.h>
volatile uint8_t RunFlag = 0;

float AngleAcc = 0;
float AngleGyro = 0;
float Angle = 0;
uint8_t ID;								//定义用于存放ID号的变量
int16_t AX, AY, AZ, GX, GY, GZ;			//定义用于存放各个数据的变量
int i=0,m=0,n=0;
int16_t j = 0;
// volatile int stay = 0; // Cleaned up unused variable

int main(void)
{
    OLED_Init();
	LED_Init();
	MPU6050_Init();
	ID = MPU6050_GetID();
	TIM4_Init();
	Motortwo_Init();
	PID_Init();
	Speed_Init();
	Delay_ms(500);
	RunFlag = 1;
	while(1)
	{
		if (RunFlag) {LED1_ON();} else {LED1_OFF();}
//		OLED_ShowSignedNum(0, 40, (AX*90)/2000, 5,6);
//		OLED_ShowSignedNum(0, 46, (int16_t)((AX * 90 + ((AX >= 0) ? 1000 : -1000)) / 2000), 5,6);
//		OLED_Update();
	}
}

void TIM4_IRQHandler(void)//中断服务函数
{
	static uint8_t Count0 = 0;
	
	if (TIM4_GetUpdateFlag())
	{
		TIM4_ClearUpdateFlag();

		if (RunFlag == 0)
		{
			Motorleft_SetSpeed(0);
			Motorright_SetSpeed(0);
			PID_Init();
			Speed_Reset();
			return;
		}

		Count0++;
		if (Count0 >= 10)
		{
			Count0 = 0;
			MPU6050_GetData(&AX, &AY, &AZ, &GX, &GY, &GZ);        //获取MPU6050的数据
			GY -= 16;
			AngleAcc = -atan2(AX, AZ) / 3.14159f * 180.0f;
			AngleAcc += 0.5f;
			AngleGyro = Angle + (float)GY / 32768.0f * 2000.0f * 0.01f;
			Angle = 0.01f * AngleAcc + 0.99f * AngleGyro;

			if (Angle > 50 || Angle < -50)
			{
				RunFlag = 0;
				Motorleft_SetSpeed(0);
				Motorright_SetSpeed(0);
				PID_Init();
				Speed_Reset();
				return;
			}

			PID_SetTargetOffset(Speed_Calc());
			j = PID_Calc((int16_t)(Angle + ((Angle >= 0) ? 0.5f : -0.5f)));
			Motorleft_SetSpeed(j);
			Motorright_SetSpeed(j);
		}
	}
}
