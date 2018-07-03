/*******************************************************************************
* @file      	usart.c
* @author    	Chen
* @version	 	1.0.0
* @date      	2018/03/24
* @brief     	串口
*********************************************************************************
* @attention	
* 			 
*********************************************************************************
* @license
*		All contributions by Chen:
*		Copyright (c) 2018, Chen.
*		All rights reserved.	 
********************************************************************************/

#include "c8051f310.h"

#define SYSCLK          24500000       	// SYSCLK frequency in Hz			频率

//----------------------------------------------------------
sfr16 TMR3RL   = 0x92;                 // Timer3 reload value     Timer3重载值
sfr16 TMR3     = 0x94;                 // Timer3 counter					Timer3计数器

//----------------------------------------------------------
sbit Re = P3^1;		//接收使能 低电平有效
sbit De = P3^2;		//发送使能 高电平有效

//----------------------------------------------------------
#define Length_Send 6															//发送缓存区长度
#define Length_Receive 6													//接收缓存区长度
unsigned char BUF_Send[Length_Send] = {0};				//发送缓存区
unsigned char BUF_Receive[Length_Receive] = {0};	//接收缓存区
unsigned char Position_Send = 0;	   							//接收数据当前位置
unsigned char Position_Receive = 0;								//发送数据当前位置

unsigned char DATA_Receive[Length_Receive] = {0x00};	//数据存储区
unsigned char Flag_Receive = 0;												//接收完成标志

unsigned int Step_record = 0;				//步数记录

//----------------------------------------------------------
//Receive
#define Frame_Start_Receive 0x43		//帧头
#define Frame_End_Receive   0x68		//帧尾
//Send	
#define Frame_Start_Send 		0x65		//帧头
#define Frame_End_Send   		0x6E		//帧尾
//Public
#define Fine_Positive		0x82		//正向微调
#define Fine_Reverse		0x83		//反向微调
#define Stop						0x26		//紧急停止
#define OK							0x01		//执行完成

//----------------------------------------------------------
/*
extern void UART0_Init (unsigned long int BAUDRATE);  //串口初始化
extern void Send(void); 												 			//发送
extern signed char Receive(void);											//接收
*/

//----------------------------------------------------------
extern void Motor_Stop(void);																	//刹车
extern void Motor_Positive(unsigned long int Step);						//正向转动
extern void Motor_Reverse(unsigned long int Step);						//反向转动

//----------------------------- 初始化 -----------------------------
/**
* @name      UART0_Init
* @brief     串口初始化
* @param    	BAUDRATE 波特率
* @retval	 		无
* @attention 8位数据，1位停止位，无奇偶校验，无硬件流控制 
*            波特率需要由定时器1工作在8位自动重装载模式（方式2）产生			
*/
void UART0_Init (unsigned long int BAUDRATE)
{
   SCON0 = 0x10;                       // SCON0: 8-bit variable bit rate
                                       //        level of STOP bit is ignored
                                       //        RX enabled
                                       //        ninth bits are zeros
                                       //        clear RI0 and TI0 bits
   if (SYSCLK/BAUDRATE/2/256 < 1) {
      TH1 = -(SYSCLK/BAUDRATE/2);
      CKCON &= ~0x0B;                  // T1M = 1; SCA1:0 = xx
      CKCON |=  0x08;
   } else if (SYSCLK/BAUDRATE/2/256 < 4) {
      TH1 = -(SYSCLK/BAUDRATE/2/4);
      CKCON &= ~0x0B;                  // T1M = 0; SCA1:0 = 01
      CKCON |=  0x01;
   } else if (SYSCLK/BAUDRATE/2/256 < 12) {
      TH1 = -(SYSCLK/BAUDRATE/2/12);
      CKCON &= ~0x0B;                  // T1M = 0; SCA1:0 = 00
   } else {
      TH1 = -(SYSCLK/BAUDRATE/2/48);
      CKCON &= ~0x0B;                  // T1M = 0; SCA1:0 = 10
      CKCON |=  0x02;
   }

   TL1 = TH1;                          // init Timer1
   TMOD &= ~0xf0;                      // TMOD: timer 1 in 8-bit autoreload
   TMOD |=  0x20;
   TR1 = 1;                            // START Timer1
   IP |= 0x10;                         // Make UART high priority
   Re = 0;
   De = 0;
   ES0 = 1;                            // Enable UART0 interrupts
}

//----------------------------- 串口发送 -----------------------------
/**
* @name      Send
* @brief     串口发送
* @param    
* @retval	 		null
* @attention 		
*/
void Send(void)
{
	unsigned char CheckSum = 0;
	unsigned char i = 1;

	while(De);												//忙则等待
	BUF_Send[0] = Frame_Start_Send;		//帧头
	BUF_Send[1] = OK;		//状态位
	
	//上一次执行步数
	BUF_Send[2] = Step_record >> 8;	
	BUF_Send[3] = Step_record;
	
	for(;i < Length_Send - 2;i++)
		CheckSum += BUF_Send[i];
	BUF_Send[Length_Send - 2] = CheckSum & 0xFF;		//校验和	
	BUF_Send[Length_Send - 1] = Frame_End_Send;			//帧尾
	
	Position_Send = 0;								//数据位置清零
	De = 1;	
	TI0 = 1;
}

//----------------------------- 串口接收 -----------------------------
/**
* @name      Receive
* @brief     串口接收
* @param    
* @retval	 		-1：未接收到数据
*							0 ：数据帧错误
*							1 ：数据帧正确
* @attention 		
*/
signed char Receive(void)
{
	unsigned char CheckSum_Receive = 0,Cycle_Receive = 0;
	unsigned long int Step_Usart = 0;		//步数暂存
	if(!Flag_Receive) return -1;		//未接收到数据，退出
	Flag_Receive = 0;								//清除接收完成标志
	for(Cycle_Receive = 0;Cycle_Receive < Length_Receive;Cycle_Receive++){		//拷贝数据
		DATA_Receive[Cycle_Receive] = BUF_Receive[Cycle_Receive];
		BUF_Receive[Cycle_Receive] = 0;
	}
	
//************ 处理数据 *****************	
	for(Cycle_Receive = 1;Cycle_Receive < Length_Receive - 2;Cycle_Receive++)		//计算校验和
		CheckSum_Receive += DATA_Receive[Cycle_Receive];
	if(DATA_Receive[0] != Frame_Start_Receive 									//帧头错误
		|| DATA_Receive[Length_Receive - 1] != Frame_End_Receive	//帧尾错误
		|| DATA_Receive[Length_Receive - 2] != CheckSum_Receive)	//校验和错误
		return 0;																									//均退出
	
	Step_Usart = ((DATA_Receive[2] & 0x00ff) << 8) | DATA_Receive[3];	//读取步数
	Step_record = Step_Usart;
	
	
	//执行对应指令
	switch(DATA_Receive[1]){
		case Stop:					//紧急停止
			Motor_Stop();
			break;		

		case Fine_Positive:	//正向微调
			Motor_Positive(Step_Usart);
			break;
		case Fine_Reverse:	//反向微调
			Motor_Reverse(Step_Usart);
			break;

		default:	//其他无效指令
			return 2;
			break;
	}
	return 1;
}

//----------------------------- 中断 -----------------------------
/**
* @name      UATR0_ISR
* @brief     串口中断
* @param    
* @retval	 
* @attention 需要软件清除标志位 			
*/
void UATR0_ISR(void)  interrupt 4
{		
	if(RI0){
		BUF_Receive[Position_Receive++] = SBUF0;
		if(Position_Receive >= Length_Receive){
			Position_Receive = 0;
			Flag_Receive = 1;
		}
		RI0 = 0;
	}
	if(TI0){
		if(Position_Send < Length_Send){
			SBUF0 = BUF_Send[Position_Send++];
		}
		else{
			Position_Send = 0;
			De = 0;
		}
		TI0 = 0; 
	}
}

///************************  COPYRIGHT (C) 2018 Chen ************************///
///******************************* END OF FILE ******************************///
