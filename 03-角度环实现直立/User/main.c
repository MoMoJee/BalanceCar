#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "LED.h"
#include "Timer.h"
#include "Key.h"
#include "MPU6050.h"
#include "Motor.h"
#include "Encoder.h"
#include "Serial.h"
#include "BlueSerial.h"
#include "PID.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

/*定义全局变量*/
int16_t AX, AY, AZ, GX, GY, GZ;		//读取MPU6050的原始数据
uint8_t TimerErrorFlag;	//定时器错误标志位，如果定时中断函数执行时间超过了定时时间，则此标志位置1
uint16_t TimerCount;	//定时器计数值，此值可用于计算定时中断函数具体的执行时间

float AngleAcc;			//由加速度计得到的角度值
float AngleGyro;		//由陀螺仪得到的角度值，执行互补滤波后，此值基本与Angle相等
float Angle;			//互补滤波后的角度值，准确且无漂移

uint8_t KeyNum, RunFlag;			//按键键码，运行标志位

int16_t LeftPWM, RightPWM;			//左PWM，右PWM
int16_t AvePWM, DifPWM;				//平均PWM，差分PWM

/*定义PID结构体变量*/
PID_t AnglePID = {					//角度环PID结构体变量，定义的时候同时给部分成员赋初值
	.Kp = 3,						//比例项权重
	.Ki = 0.1,						//积分项权重
	.Kd = 3,						//微分项权重
	
	.OutMax = 100,					//输出限幅的最大值
	.OutMin = -100,					//输出限幅的最小值
};

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	MPU6050_Init();		//MPU6050初始化
	BlueSerial_Init();	//蓝牙串口初始化
	LED_Init();			//LED初始化
	Key_Init();			//按键初始化
	Motor_Init();		//电机初始化
	Encoder_Init();		//编码器初始化
	Serial_Init();		//串口初始化
	
	Timer_Init();		//定时器初始化，定时中断时间1ms
	
	while (1)
	{
		/*LED指示程序运行状态*/
		if (RunFlag) {LED_ON();} else {LED_OFF();}		//RunFlag非0时LED点亮
		
		/*按键控制*/
		KeyNum = Key_GetNum();		//获取键码
		if (KeyNum == 1)			//如果K1按下
		{
			/*RunFlag变量取非*/
			if (RunFlag == 0)		//如果RunFlag为0
			{
				PID_Init(&AnglePID);//启动程序时给PID初始化，清除之前可能遗留的参数
				RunFlag = 1;		//则RunFlag置1
			}
			else					//否则，RunFlag非0
			{
				RunFlag = 0;		//则RunFlag置0
			}
		}
		
		/*OLED显示*/
		OLED_Clear();
		OLED_Printf(0, 0, OLED_6X8, "  Angle");						//OLED左侧显示角度环参数
		OLED_Printf(0, 8, OLED_6X8, "P:%05.2f", AnglePID.Kp);		//角度环Kp
		OLED_Printf(0, 16, OLED_6X8, "I:%05.2f", AnglePID.Ki);		//角度环Ki
		OLED_Printf(0, 24, OLED_6X8, "D:%05.2f", AnglePID.Kd);		//角度环Kd
		OLED_Printf(0, 32, OLED_6X8, "T:%+05.1f", AnglePID.Target);	//角度环目标值
		OLED_Printf(0, 40, OLED_6X8, "A:%+05.1f", Angle);			//角度环实际值
		OLED_Printf(0, 48, OLED_6X8, "O:%+05.0f", AnglePID.Out);	//角度环输出值
		OLED_Printf(0, 56, OLED_6X8, "GY:%+05d", GY);				//显示GY，便于校准陀螺仪零漂
		OLED_Update();				//OLED更新
		
		/*蓝牙串口接收数据包处理*/
		/*规定的数据包格式为：[数据1,数据2,数据3,...]*/
		if (BlueSerial_RxFlag == 1)		//如果收到数据包
		{
			char *Tag = strtok(BlueSerial_RxPacket, ",");	//提取数据1，定义为标签Tag
			if (strcmp(Tag, "key") == 0)					//Tag为key，收到按键数据包
			{
				char *Name = strtok(NULL, ",");				//提取数据2，定义为按键名称
				char *Action = strtok(NULL, ",");			//提取数据3，定义为按键动作
				
				/*此处可执行按键操作，目前程序暂时没用到按键*/
			}
			else if (strcmp(Tag, "slider") == 0)			//Tag为slider，收到滑杆数据包
			{
				char *Name = strtok(NULL, ",");				//提取数据2，定义为滑杆名称
				char *Value = strtok(NULL, ",");			//提取数据3，定义为滑杆值
				
				/*执行滑杆操作*/
				if (strcmp(Name, "AngleKp") == 0)			//如果滑杆名称是AngleKp
				{
					AnglePID.Kp = atof(Value);				//则把滑杆值赋值给角度环Kp
				}
				else if (strcmp(Name, "AngleKi") == 0)		//如果滑杆名称是AngleKi
				{
					AnglePID.Ki = atof(Value);				//则把滑杆值赋值给角度环Ki
				}
				else if (strcmp(Name, "AngleKd") == 0)		//如果滑杆名称是AngleKd
				{
					AnglePID.Kd = atof(Value);				//则把滑杆值赋值给角度环Kd
				}
			}
			else if (strcmp(Tag, "joystick") == 0)			//Tag为joystick，收到摇杆数据包
			{
				int8_t LH = atoi(strtok(NULL, ","));		//提取数据2，定义为摇杆值LH
				int8_t LV = atoi(strtok(NULL, ","));		//提取数据3，定义为摇杆值LV
				int8_t RH = atoi(strtok(NULL, ","));		//提取数据4，定义为摇杆值RH
				int8_t RV = atoi(strtok(NULL, ","));		//提取数据5，定义为摇杆值RV
				
				/*执行摇杆操作*/
				AnglePID.Target = LV / 10;		//摇杆值LV缩放后，控制角度环目标值，前后行进控制
				DifPWM = RH / 2;				//摇杆值RH缩放后，控制差分PWM，左右转弯控制
			}
			
			BlueSerial_RxFlag = 0;				//处理完成后，标志位置0，允许接收下一个数据包
		}
		
		/*蓝牙串口打印波形，需配合蓝牙串口小程序实现波形绘制*/
		BlueSerial_Printf("[plot,%f,%f]", AnglePID.Target, Angle);	//绘制AnglePID.Target和Angle的波形
	}
}

void TIM1_UP_IRQHandler(void)
{
	static uint16_t Count0;
	
	if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET)
	{
		/*定时中断函数1ms自动执行一次*/
		
		/*进入中断函数后，立刻清标志位*/
		/*如果中断函数退出前，标志位又置1了，说明中断函数执行时间超过了定时时间（1ms）*/
		TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
		
		Key_Tick();				//调用按键的Tick函数
		
		/*计次分频*/
		Count0 ++;				//计次自增
		if (Count0 >= 10)		//如果计次10次，则if成立，即if每隔10ms进一次
		{
			Count0 = 0;			//计次清零，便于下次计次
			
			/*在中断里读取MPU6050，可以保证读取间隔严格为1ms*/
			/*但要保证MPU6050_GetData执行时间不超过1ms*/
			MPU6050_GetData(&AX, &AY, &AZ, &GX, &GY, &GZ);
			
			/*校准陀螺仪Y轴零漂*/
			/*此值需实测确定，不同的设备零漂一般不同*/
			/*实测方法是，在完全静止时，观察OLED显示的GY值，即为零漂值*/
			/*然后在此处将零漂值减去，使得完全静止时，GY值为0*/
			GY -= 16;
			
			/*由加速度计计算得到角度值*/
			/*atan2计算反正切，得到角度（弧度制）， / 3.14159 * 180可将弧度制转为角度值*/
			AngleAcc = -atan2(AX, AZ) / 3.14159 * 180;
			
			/*校准中心角度*/
			/*此值需实测确定，不同的设备中心角度一般不同*/
			/*实测方法是，使平衡车绝对竖直，观察OLED显示的角度环实际值，即为中心角度偏移值*/
			/*然后在此处将偏移值减去，使得绝对竖直时，角度环实际值为0*/
			AngleAcc += 0.5;
			
			/*由陀螺仪积分得到角度值*/
			/*互补滤波下，角度积分要在上次滤波后的Angle上进行*/
			/*公式中32768是int16_t变量的最大值，2000是陀螺仪配置的满量程2000度每秒，0.01是间隔时间10ms*/
			AngleGyro = Angle + GY / 32768.0 * 2000 * 0.01;
			
			/*执行互补滤波*/
			float Alpha = 0.01;		//互补滤波系数，值越大，越偏向于加速度计，值越小，越偏向于陀螺仪
			Angle = Alpha * AngleAcc + (1 - Alpha) * AngleGyro;		//互补滤波计算，得到稳定的角度值
			
			/*平衡车倒地后自动停止PID程序*/
			if (Angle > 50 || Angle < -50)	//角度超过-50度~50度的范围，认为平衡车倒地了
			{
				RunFlag = 0;				//RunFlag置0，停止PID程序
			}
			
			/*执行PID调控程序*/
			if (RunFlag)					//RunFlag非0时，启动PID程序
			{
				/*角度环PID控制*/
				AnglePID.Actual = Angle;	//角度环实际值为Angle
				PID_Update(&AnglePID);		//调用封装好的函数，一步完成PID计算和更新
				AvePWM = -AnglePID.Out;		//角度环的输出值给到电机平均PWM，用于控制前后行进
				
				/*控制量转换*/
				LeftPWM = AvePWM + DifPWM / 2;		//由平均PWM和差分PWM计算得到左轮PWM
				RightPWM = AvePWM - DifPWM / 2;		//由平均PWM和差分PWM计算得到右轮PWM
				
				/*PWM限幅*/
				/*上式计算后，LeftPWM和RightPWM可能会超出电机允许的PWM范围，此处将PWM值范围限制在-100~100之内*/
				if (LeftPWM > 100) {LeftPWM = 100;} else if (LeftPWM < -100) {LeftPWM = -100;}
				if (RightPWM > 100) {RightPWM = 100;} else if (RightPWM < -100) {RightPWM = -100;}
				
				/*PWM输出给电机*/
				Motor_SetPWM(1, LeftPWM);		//LeftPWM输出给左轮电机
				Motor_SetPWM(2, RightPWM);		//RightPWM输出给右轮电机
			}
			else		//RunFlag为0时，停止PID程序
			{
				/*左右电机PWM均设置为0*/
				Motor_SetPWM(1, 0);
				Motor_SetPWM(2, 0);
			}
		}
		
		/*中断函数退出前，再次检查标志位*/
		if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET)
		{
			/*标志位又置1了，说明中断函数执行时间超过了定时时间（1ms）*/
			/*置TimerErrorFlag为1，表示定时中断错误*/
			TimerErrorFlag = 1;
			
			/*清标志位，避免中断连续触发，导致主函数完全无法执行*/
			TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
		}
		
		/*中断函数退出前，读取计数器的值，此值可用于测量中断函数的具体执行时间*/
		TimerCount = TIM_GetCounter(TIM1);
	}
}
