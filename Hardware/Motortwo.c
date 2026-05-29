#include "stm32f10x.h"                  // Device header
#include "PWM.h"

void Motortwo_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	PWM_Init();													//初始化直流电机的底层PWM
}

void Motorleft_SetSpeed(int8_t leftSpeed)
{
	if (leftSpeed >= 0)							//如果设置正转的速度值
	{
		GPIO_SetBits(GPIOB, GPIO_Pin_15);	//PA4置高电平
		GPIO_ResetBits(GPIOB, GPIO_Pin_14);	//PA5置低电平，设置方向为正转
		PWM_SetCompare1(leftSpeed);				//PWM设置为速度值
	}
	else									//否则，即设置反转的速度值
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_15);	
		GPIO_SetBits(GPIOB, GPIO_Pin_14);	
		PWM_SetCompare1(-leftSpeed);			
	}
}
void Motorright_SetSpeed(int8_t rightSpeed)
{
	if (rightSpeed >= 0)							
	{
		GPIO_SetBits(GPIOB, GPIO_Pin_13);	
		GPIO_ResetBits(GPIOB, GPIO_Pin_12);	
		PWM_SetCompare2(rightSpeed);				
	}
	else									
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_13);	
		GPIO_SetBits(GPIOB, GPIO_Pin_12);	
		PWM_SetCompare2(-rightSpeed);			
	}
}
