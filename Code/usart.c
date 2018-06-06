/*******************************************************************************
* @file      	usart.c
* @author    	Chen
* @version	 	1.0.0
* @date      	2018/03/24
* @brief     	����
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

#define SYSCLK          24500000       	// SYSCLK frequency in Hz			Ƶ��

//----------------------------------------------------------
sfr16 TMR3RL   = 0x92;                 // Timer3 reload value     Timer3����ֵ
sfr16 TMR3     = 0x94;                 // Timer3 counter					Timer3������

//----------------------------------------------------------
sbit Re = P3^1;		//����ʹ�� �͵�ƽ��Ч
sbit De = P3^2;		//����ʹ�� �ߵ�ƽ��Ч

//----------------------------------------------------------
#define Length_Send 6															//���ͻ���������
#define Length_Receive 6													//���ջ���������
unsigned char BUF_Send[Length_Send] = {0};				//���ͻ�����
unsigned char BUF_Receive[Length_Receive] = {0};	//���ջ�����
unsigned char Position_Send = 0;	   							//�������ݵ�ǰλ��
unsigned char Position_Receive = 0;								//�������ݵ�ǰλ��

unsigned char DATA_Receive[Length_Receive] = {0x00};	//���ݴ洢��
unsigned char Flag_Receive = 0;												//������ɱ�־

unsigned int Step_record = 0;				//������¼

//----------------------------------------------------------
//Receive
#define Frame_Start_Receive 0x43		//֡ͷ
#define Frame_End_Receive   0x68		//֡β
//Send	
#define Frame_Start_Send 		0x65		//֡ͷ
#define Frame_End_Send   		0x6E		//֡β
//Public
#define Fine_Positive		0x82		//����΢��
#define Fine_Reverse		0x83		//����΢��
#define Stop						0x26		//����ֹͣ
#define OK							0x01		//ִ�����

//----------------------------------------------------------
/*
extern void UART0_Init (unsigned long int BAUDRATE);  //���ڳ�ʼ��
extern void Send(void); 												 			//����
extern signed char Receive(void);											//����
*/

//----------------------------------------------------------
extern void Motor_Stop(void);																	//ɲ��
extern void Motor_Positive(unsigned long int Step);						//����ת��
extern void Motor_Reverse(unsigned long int Step);						//����ת��

//----------------------------- ��ʼ�� -----------------------------
/**
* @name      UART0_Init
* @brief     ���ڳ�ʼ��
* @param    	BAUDRATE ������
* @retval	 		��
* @attention 8λ���ݣ�1λֹͣλ������żУ�飬��Ӳ�������� 
*            ��������Ҫ�ɶ�ʱ��1������8λ�Զ���װ��ģʽ����ʽ2������			
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

//----------------------------- ���ڷ��� -----------------------------
/**
* @name      Send
* @brief     ���ڷ���
* @param    
* @retval	 		null
* @attention 		
*/
void Send(void)
{
	unsigned char CheckSum = 0;
	unsigned char i = 1;

	while(De);												//æ��ȴ�
	BUF_Send[0] = Frame_Start_Send;		//֡ͷ
	BUF_Send[1] = OK;		//״̬λ
	
	//��һ��ִ�в���
	BUF_Send[2] = Step_record >> 8;	
	BUF_Send[3] = Step_record;
	
	for(;i < Length_Send - 2;i++)
		CheckSum += BUF_Send[i];
	BUF_Send[Length_Send - 2] = CheckSum & 0xFF;		//У���	
	BUF_Send[Length_Send - 1] = Frame_End_Send;			//֡β
	
	Position_Send = 0;								//����λ������
	De = 1;	
	TI0 = 1;
}

//----------------------------- ���ڽ��� -----------------------------
/**
* @name      Receive
* @brief     ���ڽ���
* @param    
* @retval	 		-1��δ���յ�����
*							0 ������֡����
*							1 ������֡��ȷ
* @attention 		
*/
signed char Receive(void)
{
	unsigned char CheckSum_Receive = 0,Cycle_Receive = 0;
	unsigned long int Step_Usart = 0;		//�����ݴ�
	if(!Flag_Receive) return -1;		//δ���յ����ݣ��˳�
	Flag_Receive = 0;								//���������ɱ�־
	for(Cycle_Receive = 0;Cycle_Receive < Length_Receive;Cycle_Receive++){		//��������
		DATA_Receive[Cycle_Receive] = BUF_Receive[Cycle_Receive];
		BUF_Receive[Cycle_Receive] = 0;
	}
	
//************ �������� *****************	
	for(Cycle_Receive = 1;Cycle_Receive < Length_Receive - 2;Cycle_Receive++)		//����У���
		CheckSum_Receive += DATA_Receive[Cycle_Receive];
	if(DATA_Receive[0] != Frame_Start_Receive 									//֡ͷ����
		|| DATA_Receive[Length_Receive - 1] != Frame_End_Receive	//֡β����
		|| DATA_Receive[Length_Receive - 2] != CheckSum_Receive)	//У��ʹ���
		return 0;																									//���˳�
	
	Step_Usart = ((DATA_Receive[2] & 0x00ff) << 8) | DATA_Receive[3];	//��ȡ����
	Step_record = Step_Usart;
	
	
	//ִ�ж�Ӧָ��
	switch(DATA_Receive[1]){
		case Stop:					//����ֹͣ
			Motor_Stop();
			break;		

		case Fine_Positive:	//����΢��
			Motor_Positive(Step_Usart);
			break;
		case Fine_Reverse:	//����΢��
			Motor_Reverse(Step_Usart);
			break;

		default:	//������Чָ��
			return 2;
			break;
	}
	return 1;
}

//----------------------------- �ж� -----------------------------
/**
* @name      UATR0_ISR
* @brief     �����ж�
* @param    
* @retval	 
* @attention ��Ҫ��������־λ 			
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
