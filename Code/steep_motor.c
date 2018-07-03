/*******************************************************************************
* @file      	steep_motor.c
* @author    	Chen
* @version	 	1.0.0
* @date      	2018/03/24
* @brief     	�����������		   
*********************************************************************************
* @attention  
*							Ĭ��24.5MHzʱ�ӣ�12��Ƶ
* 			 			����������ƾ��мӼ��٣��Ӽ����Զ����ƣ���������Ϊ��Ԥ��
*							�Ӽ��ٽ����������&���Ƶ�ʼ���Ƶ��������
*********************************************************************************
* @license
*		All contributions by Chen:
*		Copyright (c) 2018, Chen.
*		All rights reserved.	 
********************************************************************************/

#include "c8051f310.h"
#include <math.h>

//----------------------------------------------------------
sfr16 TMR2RL   = 0xca;                 // Timer2 reload value     Timer2����ֵ
sfr16 TMR2     = 0xcc;                 // Timer2 counter					Timer2������

sbit PULL = P0^0;		//����
sbit EN =  P0^2;		//ʹ�ܣ�Ӳ�����죩
sbit DIR = P0^1;		//����Ӳ�����죩
sbit MS1 = P3^4;		//ϸ��
sbit MS2 = P3^3;		//ϸ��

sbit  Zero = P1^5;		//Zero2 ��������  0 Read

//----------------------------------------------------------
#define Hertz_min 32											//����ٶ�32hz
#define Hertz_max 3200										//����ٶ�1600hz
#define Steps_Low_speed 32									//���ٲ���
#define Frequency_conversion_Step_Default 500			//Ĭ�ϱ�Ƶ���� 200��
#define Frequency_conversion (Hertz_max - Hertz_min - 2 * Steps_Low_speed) / Frequency_conversion_Step			

#define Dir_Default 1					//Ĭ�ϳ�ʼ������
//#define Step_Default 3200			//Ĭ��һȦ3200��
#define Difference 32				//Ĭ�ϲ�ֵ 100 ��

//----------------------------------------------------------
unsigned long int STEP = 0,STEP_Executed = 0;
signed int STEP_Zero = 100;		//�����㲽��
unsigned long int Frequency_conversion_Step = Frequency_conversion_Step_Default;		//ʵ�ʱ�Ƶ����

//----------------------------------------------------------
/*
extern void Motor_Init (void);  //���������ʼ��
extern void Motor_Stop(void);		//ɲ��
extern void Motor_Positive(unsigned long int Step);		//����ת��
extern void Motor_Reverse(unsigned long int Step);		//����ת��
extern unsigned long int Inquire_STEP(void);					//��ѯʣ�ಽ��
*/

//----------------------------------------------------------
extern void Delay500ms(void);								//��ʱ500ms
extern void Timer3_Init (void);							//��ʱ��3��ʼ��
extern unsigned char Inquire_trigger(void);	//��ѯ�������ص�ǰ����״̬
extern unsigned char Inquire_Hall(void);		//��ѯ�������ش���״̬	  
extern void Restart(void);	 								//����
extern signed int Flash_Read(void);					//��ȡFlash
extern unsigned int Read_Step_Default_Flash(void);						//��ȡflash��ÿȦ����
extern unsigned char Flash_Write(unsigned int Step_Default);	//д��Flash
extern unsigned int Read_Step_Default_Flash(void);						//��ȡflash��ÿȦ����

//----------------------------------------------------------
void Motor_Init (void);			//���������ʼ��
void Timer2_Init (void);		//��ʱ��2��ʼ��
void Drive_Init(void);			//�������������ʼ��

void Motor(unsigned char Dir,unsigned long int Step);		//ת���������
void Motor_Positive(unsigned long int Step);						//����ת��
void Motor_Reverse(unsigned long int Step);							//����ת��
void Motor_Stop(void);																	//ɲ��

//----------------------------- �����ʼ�� -----------------------------
/**
* @name      Motor_Init
* @brief     ���������ʼ��
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
* @brief     ��ʱ��2��ʼ��
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
* @brief     �������������ʼ��
* @param    	
* @retval	 
* @attention 			
*/
void Drive_Init(void)
{
	EN = 1;		//ʧ��
	MS1 = 1;	//Ĭ�����ϸ�� 8ϸ��=16ϸ��
	MS2 = 1;
	DIR = Dir_Default;	//����Ĭ��
	EN = 0;
}

//----------------------------- ��� -----------------------------
/**
* @name       Motor
* @brief     �����������
* @param     ����&����
* @retval	 		null
* @attention 			
*/
void Motor(unsigned char Dir,unsigned long int Step)
{
	DIR = Dir;	//�趨����
	EN = 0;		//ʹ��
	if(!Step) return;
	//�жϲ����Ƿ���ɼӼ���ȫ����
	if(Step < 2 * Frequency_conversion_Step_Default) Frequency_conversion_Step = Step / 2;		//����ȫ����
	else Frequency_conversion_Step = Frequency_conversion_Step_Default;												//��ȫ����
	STEP = Step;					//�趨����
	STEP_Executed = 0;		//�����ִ�в���
	TMR2 = TMR2RL = -(2058672 / Hertz_min); 	//�趨��ʱ��2�Զ���װ��ֵ
	TR2 = 1;			//ʹ�ܶ�ʱ��2
	EA = 1;				//ʹ��ȫ���ж�
}

/**
* @name       Motor_Positive
* @brief     �����������
* @param     ����&����
* @retval	 		null
* @attention 			
*/
void Motor_Positive(unsigned long int Step)
{
	Motor(Dir_Default,Step);
}

/**
* @name       Motor_Reverse
* @brief     �����������
* @param     ����&����
* @retval	 		null
* @attention 			
*/
void Motor_Reverse(unsigned long int Step)
{
	Motor(!Dir_Default,Step);
}

/**
* @name       Motor_Stop
* @brief     �������ɲ��
* @param     null
* @retval	 		null
* @attention 			
*/
void Motor_Stop(void)
{	
//	if(STEP <= Steps_Low_speed) {
//		EN = 0;		//ʹ��
//		TR2 = 0;			//ʹ�ܶ�ʱ��2
//		STEP = 0;
//	}
//	else if(STEP >= Frequency_conversion_Step && STEP_Executed >= Frequency_conversion_Step )//ȫ����ֹͣ
//					STEP = Frequency_conversion_Step;	
//	else if(STEP < Frequency_conversion_Step);	//������
//	else if(STEP_Executed < Frequency_conversion_Step)	//������
//					STEP = STEP_Executed;
//	else
	{
//----------------------------------------	
		EN = 0;		//ʹ��
		TR2 = 0;			//ʹ�ܶ�ʱ��2
		STEP = 0;
	}
}

//----------------------------- ��ѯ -----------------------------
/**
* @name      Inquire_STEP
* @brief     ��ѯʣ�ಽ��
* @param    	
* @retval	 		ʣ�ಽ��
* @attention 			
*/
unsigned long int Inquire_STEP(void)
{
	return STEP;
}

//----------------------------- �ж� -----------------------------
/**
* @name      Timer2_ISR
* @brief     ��ʱ��2�ж�
* @param    
* @retval	 
* @attention 			
*/
void Timer2_ISR(void) interrupt 5
{
	PULL = 0;
	STEP--;
	STEP_Executed++;	
	if(!STEP) TR2 = 0;  //�������꣬�رն�ʱ��2
	
	if(DIR == Dir_Default) STEP_Zero++;		//�����㲽����Ĭ�Ϸ���ͬ�� +1
	else STEP_Zero--;											//�����㲽����Ĭ�Ϸ����� -1
	
	if(STEP <= Steps_Low_speed || STEP_Executed <= Steps_Low_speed) //����������
		TMR2 = TMR2RL = -(2058672 / Hertz_min);
	if(STEP < Frequency_conversion_Step) 			//����
		TMR2 = TMR2RL = -(2058672 / (Hertz_min + STEP * Frequency_conversion)); // Reload value to be used in Timer2
	else if(STEP_Executed < Frequency_conversion_Step)		//����
					TMR2 = TMR2RL = -(2058672 / (Hertz_min + STEP_Executed * Frequency_conversion)); // Reload value to be used in Timer2
	
	PULL = 1;
	TF2H = TF2L = 0;			//���������
}

///************************  COPYRIGHT (C) 2018 Chen ************************///
///******************************* END OF FILE ******************************///
