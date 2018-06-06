/*******************************************************************************
* @file    		sys_Init.c
* @author    	Chen
* @version	 	1.0.0
* @date      	2018/03/24
* @brief     	ϵͳ��ʼ��		   
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
extern void sys_Init (void); 	//ϵͳ��ʼ��
extern void Restart(void);	 	//����
extern void Delay500ms(void);	//��ʱ500ms
extern void Delay10ms(void);		//��ʱ10ms
*/

//----------------------------------------------------------
void SYSCLK_Init (void); 	//ϵͳʱ�ӳ�ʼ��
void PORT_Init (void);	 	//���濪�س�ʼ��			 	
void Watchdog(void);     	//�رտ��Ź�
void Restart(void);			//����
void Delay500ms(void);		//��ʱ500ms

//----------------------------- xxx -----------------------------
/**
* @name      Restart
* @brief     ����
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
* @brief     ϵͳ��ʼ��
* @param    
* @retval	 
* @attention 			
*/
void sys_Init (void)
{
	Watchdog();			//��ֹ���Ź�
	PORT_Init();		//����GPIO
	SYSCLK_Init();	//����ʱ��
}

/**
* @name      SYSCLK_Init
* @brief     ϵͳʱ�ӳ�ʼ��
* @param    
* @retval	 
* @attention �˳����ʼ��ϵͳʱ��ʹ���ڲ�24.5MHz������Ϊ��ʱ��Դ��
*            ��ʹʱ�Ӽ������λʧЧ��			
*/
void SYSCLK_Init (void)
{
   OSCICN |= 0x03;                     // Configure internal oscillator for
                                       // its maximum frequency
   RSTSRC  = 0x04;                     // Enable missing clock detector
}

/**
* @name      PORT_Init
* @brief     ���濪�س�ʼ��
* @param    
* @retval	 
* @attention P0.4 - UART TX��P0.5 - UART RX			
*/
void PORT_Init (void)
{
/*   P0MDIN  	= 0xFF;                    //
   P0MDOUT 	= 0x37;                    // P0.0-P0.2����
   P1MDIN  	= 0xFF;                    //
   P1MDOUT 	= 0x3C;
   P2MDIN  	= 0x0F;                    // 
   P3MDIN  	= 0x18;                    //
   P3MDOUT |= 0x06;                    // 
   P1 	  	= 0x00;                    // P1������Ĭ��2V����ʼ��ʹP1�͵�ƽ
   XBR0    	= 0x01;                    // ʹ�ܽ��濪�� UART on P0.4(TX) and P0.5                    
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
* @brief     �رտ��Ź�
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
* @brief     ��ʱ500ms
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
* @brief     ��ʱ10ms
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
