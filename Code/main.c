/*******************************************************************************
* @file      	main.c
* @author    	Chen
* @version   	1.0.0
* @date      	2018/03/24
* @brief     		   
*********************************************************************************
* @attention
*
*********************************************************************************
* @license
* 		All contributions by Chen:
*			Copyright (c) 2018, Chen.
*			All rights reserved.	 
********************************************************************************/

#include "c8051f310.h"
#include "intrins.h"

//----------------------------------------------------------
sfr16 DP       = 0x82;                 	// data pointer						数据指针
sfr16 PCA0CP0  = 0xfb;                 	// PCA0 Module 0 Capture/Compare		
sfr16 PCA0CP1  = 0xe9;                 	// PCA0 Module 1 Capture/Compare		
sfr16 PCA0CP2  = 0xeb;                 	// PCA0 Module 2 Capture/Compare		
sfr16 PCA0CP3  = 0xed;                 	// PCA0 Module 3 Capture/Compare		
sfr16 PCA0CP4  = 0xfd;                 	// PCA0 Module 4 Capture/Compare			
sfr16 PCA0     = 0xf9;                 	// PCA0 counter						PCA0计数器

//----------------------------------------------------------
#define Failed_passbacks	5							//失败回传次数

//----------------------------------------------------------
extern void sys_Init (void); 												//系统初始化
extern void UART0_Init (unsigned long int BAUDRATE);//串口初始化
extern void Motor_Init (void);  										//步进电机初始化
extern unsigned char Init(void);										//上电初始化
extern void Send(void); 												 		//发送
extern signed char Receive(void);										//接收
extern unsigned long int Inquire_STEP(void);				//查询剩余步数
extern void Motor_Stop(void);												//刹车

//----------------------------- 主函数 -----------------------------
/**
* @name      main
* @brief     主函数
* @param    
* @retval	 
* @attention 			
*/
void main(void){
	unsigned char Return = 0;
	unsigned char Count = 0;
//-------------------- 初始化 ----------------------------	
	sys_Init();					//系统初始化
	Motor_Init();				//电机初始化
	UART0_Init(115200);	//初始化串口，波特率115200bps
 	EA = 1;							//使能全局中断
	
//------------------- 正常工作 ----------------------------
	while(1){
		if(Receive() == 1)					//接收到正确数据
			Return = 1;
		if(!Inquire_STEP() && (Return == 1)){		//接收到正确数据且执行完成
			Motor_Stop();							//刹车
			Send();										//执行完成，回传
			Return  = 0;
		}
	}
}

///************************  COPYRIGHT (C) 2018 Chen ************************///
///******************************* END OF FILE ******************************///
