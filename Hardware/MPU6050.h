#ifndef __MPU6050_H
#define __MPU6050_H

void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data);
uint8_t MPU6050_ReadReg(uint8_t RegAddress);

void MPU6050_Init(void);
uint8_t MPU6050_GetID(void);
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, 
						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ);

#endif

//		MPU6050_GetData(&AX, &AY, &AZ, &GX, &GY, &GZ);		//获取MPU6050的数据
//		OLED_ShowSignedNum(0, 8, AX, 5,6);			//OLED显示数据
//		OLED_ShowSignedNum(0, 16, AY, 5,6);
//		OLED_ShowSignedNum(0, 24, AZ, 5,6);
//		OLED_ShowSignedNum(0, 32, GX, 5,6);
//		OLED_ShowSignedNum(0, 40, GY, 5,6);
//		OLED_ShowSignedNum(0, 48, GZ, 5,6);
//		OLED_Update();