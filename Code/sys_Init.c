/*******************************************************************************
* @file    		sys_Init.c
* @author    	Chen
* @version	 	1.0.0
* @date      	2018/03/24
* @brief     	系统初始化		   
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
#include "intrins.h"

//----------------------------------------------------------
/*
extern void sys_Init (void); 	//系统初始化
extern void Restart(void);	 	//重启
extern void Delay500ms(void);	//延时500ms
extern void Delay10ms(void);		//延时10ms
*/

//----------------------------------------------------------
void SYSCLK_Init (void); 	//系统时钟初始化
void PORT_Init (void);	 	//交叉开关初始化			 	
void Watchdog(void);     	//关闭看门狗
void Restart(void);			//重启
void Delay500ms(void);		//延时500ms

//----------------------------- xxx -----------------------------
/**
* @name      Restart
* @brief     重启
* @param    
* @retval	 
* @attention 			
*/
void Restart(void)
{
	Delay500ms();
	Delay500ms();
	Delay500ms();
	Delay500ms();
	RSTSRC |= 0x10;
}

/**
* @name      sys_Init
* @brief     系统初始化
* @param    
* @retval	 
* @attention 			
*/
void sys_Init (void)
{
	Watchdog();			//禁止看门狗
	PORT_Init();		//配置GPIO
	SYSCLK_Init();	//配置时钟
}

/**
* @name      SYSCLK_Init
* @brief     系统时钟初始化
* @param    
* @retval	 
* @attention 此程序初始化系统时钟使用内部24.5MHz振荡器作为其时钟源。
*            且使时钟检测器复位失效。			
*/
void SYSCLK_Init (void)
{
   OSCICN |= 0x03;                     // Configure internal oscillator for
                                       // its maximum frequency
   RSTSRC  = 0x04;                     // Enable missing clock detector
}

/**
* @name      PORT_Init
* @brief     交叉开关初始化
* @param    
* @retval	 
* @attention P0.4 - UART TX；P0.5 - UART RX			
*/
void PORT_Init (void)
{
/*   P0MDIN  	= 0xFF;                    //
   P0MDOUT 	= 0x37;                    // P0.0-P0.2推挽
   P1MDIN  	= 0xFF;                    //
   P1MDOUT 	= 0x3C;
   P2MDIN  	= 0x0F;                    // 
   P3MDIN  	= 0x18;                    //
   P3MDOUT |= 0x06;                    // 
   P1 	  	= 0x00;                    // P1口下拉默认2V，初始化使P1低电平
   XBR0    	= 0x01;                    // 使能交叉开关 UART on P0.4(TX) and P0.5                    
   XBR1    	= 0x40;   
*/
	P0MDOUT   = 0xD7;
    P2MDOUT   = 0x03;
    P3MDOUT   = 0x1E;
    P0SKIP    = 0xC7;
    P1SKIP    = 0x3C;
    P2SKIP    = 0x03;

	P1MDIN  |= 0x3C; 
	P1 |= 0x3C;
	P0 &= 0x3F;	
	P2  = 0xFC;   
//	P3 &= 0X19;
//	P3 |= 0x18;
	  	
    XBR0      = 0x01;
    XBR1      = 0x40;
}

/**
* @name      Watchdog
* @brief     关闭看门狗
* @param    
* @retval	 
* @attention 		
*/
void Watchdog(void){
   PCA0MD &= ~0x40;                    // WDTE = 0 (clear watchdog timer
                                       // enable)
}

/**
* @name      Delay500ms
* @brief     延时500ms
* @param    
* @retval	 
* @attention 24.5000MHz			
*/
void Delay500ms(void)		
{
	unsigned char i, j, k;

	i = 48;
	j = 128;
	k = 84;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

/**
* @name      Delay10ms
* @brief     延时10ms
* @param    
* @retval	 
* @attention 24.5000MHz			
*/
void Delay10ms(void)		//@24.5MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 1;
	j = 239;
	k = 78;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

///************************  COPYRIGHT (C) 2018 Chen ************************///
///******************************* END OF FILE ******************************///
