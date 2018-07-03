/*******************************************************************************
* @file      	steep_motor.c
* @author    	Chen
* @version	 	1.0.0
* @date      	2018/03/24
* @brief     	步进电机控制		   
*********************************************************************************
* @attention  
*							默认24.5MHz时钟，12分频
* 			 			步进电机控制具有加减速，加减速自动控制，无需认人为干预，
*							加减速仅需设置最低&最高频率及变频步长即可
*********************************************************************************
* @license
*		All contributions by Chen:
*		Copyright (c) 2018, Chen.
*		All rights reserved.	 
********************************************************************************/

#include "c8051f310.h"
#include <math.h>

//----------------------------------------------------------
sfr16 TMR2RL   = 0xca;                 // Timer2 reload value     Timer2重载值
sfr16 TMR2     = 0xcc;                 // Timer2 counter					Timer2计数器

sbit PULL = P0^0;		//脉冲
sbit EN =  P0^2;		//使能（硬件差异）
sbit DIR = P0^1;		//方向（硬件差异）
sbit MS1 = P3^4;		//细分
sbit MS2 = P3^3;		//细分

sbit  Zero = P1^5;		//Zero2 霍尔开关  0 Read

//----------------------------------------------------------
#define Hertz_min 32											//最低速度32hz
#define Hertz_max 3200										//最高速度1600hz
#define Steps_Low_speed 32									//低速步长
#define Frequency_conversion_Step_Default 500			//默认变频步长 200步
#define Frequency_conversion (Hertz_max - Hertz_min - 2 * Steps_Low_speed) / Frequency_conversion_Step			

#define Dir_Default 1					//默认初始化方向
//#define Step_Default 3200			//默认一圈3200步
#define Difference 32				//默认差值 100 步

//----------------------------------------------------------
unsigned long int STEP = 0,STEP_Executed = 0;
signed int STEP_Zero = 100;		//相对零点步数
unsigned long int Frequency_conversion_Step = Frequency_conversion_Step_Default;		//实际变频步长

//----------------------------------------------------------
/*
extern void Motor_Init (void);  //步进电机初始化
extern void Motor_Stop(void);		//刹车
extern void Motor_Positive(unsigned long int Step);		//正向转动
extern void Motor_Reverse(unsigned long int Step);		//反向转动
extern unsigned long int Inquire_STEP(void);					//查询剩余步数
*/

//----------------------------------------------------------
extern void Delay500ms(void);								//延时500ms
extern void Timer3_Init (void);							//定时器3初始化
extern unsigned char Inquire_trigger(void);	//查询霍尔开关当前触发状态
extern unsigned char Inquire_Hall(void);		//查询霍尔开关触发状态	  
extern void Restart(void);	 								//重启
extern signed int Flash_Read(void);					//读取Flash
extern unsigned int Read_Step_Default_Flash(void);						//读取flash中每圈步数
extern unsigned char Flash_Write(unsigned int Step_Default);	//写入Flash
extern unsigned int Read_Step_Default_Flash(void);						//读取flash中每圈步数

//----------------------------------------------------------
void Motor_Init (void);			//步进电机初始化
void Timer2_Init (void);		//定时器2初始化
void Drive_Init(void);			//步进电机驱动初始化

void Motor(unsigned char Dir,unsigned long int Step);		//转动步进电机
void Motor_Positive(unsigned long int Step);						//正向转动
void Motor_Reverse(unsigned long int Step);							//反向转动
void Motor_Stop(void);																	//刹车

//----------------------------- 电机初始化 -----------------------------
/**
* @name      Motor_Init
* @brief     步进电机初始化
* @param    	
* @retval	 
* @attention 			
*/
void Motor_Init (void)
{
	STEP = 1;
	Timer2_Init();
	Drive_Init();
}

/**
* @name      Timer2_Init
* @brief     定时器2初始化
* @param    	
* @retval	 
* @attention 			
*/
void Timer2_Init (void)
{
   CKCON &= ~0x60;                     // Timer2 uses SYSCLK/12
   TMR2CN &= ~0x01;

   TMR2RL = 810; 												// Reload value to be used in Timer2
   TMR2 = TMR2RL;                      // Init the Timer2 register

   TMR2CN = 0x04;                      // Enable Timer2 in auto-reload mode
   ET2 = 1;                            // Timer2 interrupt enabled   
	 TR2 = 0;
}

/**
* @name      Drive_Init
* @brief     步进电机驱动初始化
* @param    	
* @retval	 
* @attention 			
*/
void Drive_Init(void)
{
	EN = 1;		//失能
	MS1 = 1;	//默认最大细分 8细分=16细分
	MS2 = 1;
	DIR = Dir_Default;	//方向默认
	EN = 0;
}

//----------------------------- 电机 -----------------------------
/**
* @name       Motor
* @brief     步进电机控制
* @param     方向&步数
* @retval	 		null
* @attention 			
*/
void Motor(unsigned char Dir,unsigned long int Step)
{
	DIR = Dir;	//设定方向
	EN = 0;		//使能
	if(!Step) return;
	//判断步数是否够完成加减速全过程
	if(Step < 2 * Frequency_conversion_Step_Default) Frequency_conversion_Step = Step / 2;		//不够全过程
	else Frequency_conversion_Step = Frequency_conversion_Step_Default;												//够全过程
	STEP = Step;					//设定步数
	STEP_Executed = 0;		//清除已执行步数
	TMR2 = TMR2RL = -(2058672 / Hertz_min); 	//设定定时器2自动重装载值
	TR2 = 1;			//使能定时器2
	EA = 1;				//使能全局中断
}

/**
* @name       Motor_Positive
* @brief     步进电机控制
* @param     正向&步数
* @retval	 		null
* @attention 			
*/
void Motor_Positive(unsigned long int Step)
{
	Motor(Dir_Default,Step);
}

/**
* @name       Motor_Reverse
* @brief     步进电机控制
* @param     反向&步数
* @retval	 		null
* @attention 			
*/
void Motor_Reverse(unsigned long int Step)
{
	Motor(!Dir_Default,Step);
}

/**
* @name       Motor_Stop
* @brief     步进电机刹车
* @param     null
* @retval	 		null
* @attention 			
*/
void Motor_Stop(void)
{	
//	if(STEP <= Steps_Low_speed) {
//		EN = 0;		//使能
//		TR2 = 0;			//使能定时器2
//		STEP = 0;
//	}
//	else if(STEP >= Frequency_conversion_Step && STEP_Executed >= Frequency_conversion_Step )//全速中停止
//					STEP = Frequency_conversion_Step;	
//	else if(STEP < Frequency_conversion_Step);	//减速中
//	else if(STEP_Executed < Frequency_conversion_Step)	//加速中
//					STEP = STEP_Executed;
//	else
	{
//----------------------------------------	
		EN = 0;		//使能
		TR2 = 0;			//使能定时器2
		STEP = 0;
	}
}

//----------------------------- 查询 -----------------------------
/**
* @name      Inquire_STEP
* @brief     查询剩余步数
* @param    	
* @retval	 		剩余步数
* @attention 			
*/
unsigned long int Inquire_STEP(void)
{
	return STEP;
}

//----------------------------- 中断 -----------------------------
/**
* @name      Timer2_ISR
* @brief     定时器2中断
* @param    
* @retval	 
* @attention 			
*/
void Timer2_ISR(void) interrupt 5
{
	PULL = 0;
	STEP--;
	STEP_Executed++;	
	if(!STEP) TR2 = 0;  //步数走完，关闭定时器2
	
	if(DIR == Dir_Default) STEP_Zero++;		//相对零点步数与默认方向同向 +1
	else STEP_Zero--;											//相对零点步数与默认方向反向 -1
	
	if(STEP <= Steps_Low_speed || STEP_Executed <= Steps_Low_speed) //低速运行区
		TMR2 = TMR2RL = -(2058672 / Hertz_min);
	if(STEP < Frequency_conversion_Step) 			//减速
		TMR2 = TMR2RL = -(2058672 / (Hertz_min + STEP * Frequency_conversion)); // Reload value to be used in Timer2
	else if(STEP_Executed < Frequency_conversion_Step)		//加速
					TMR2 = TMR2RL = -(2058672 / (Hertz_min + STEP_Executed * Frequency_conversion)); // Reload value to be used in Timer2
	
	PULL = 1;
	TF2H = TF2L = 0;			//清除溢出标记
}

///************************  COPYRIGHT (C) 2018 Chen ************************///
///******************************* END OF FILE ******************************///
