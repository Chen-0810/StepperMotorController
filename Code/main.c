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
sfr16 DP       = 0x82;                 	// data pointer						����ָ��
sfr16 PCA0CP0  = 0xfb;                 	// PCA0 Module 0 Capture/Compare		
sfr16 PCA0CP1  = 0xe9;                 	// PCA0 Module 1 Capture/Compare		
sfr16 PCA0CP2  = 0xeb;                 	// PCA0 Module 2 Capture/Compare		
sfr16 PCA0CP3  = 0xed;                 	// PCA0 Module 3 Capture/Compare		
sfr16 PCA0CP4  = 0xfd;                 	// PCA0 Module 4 Capture/Compare			
sfr16 PCA0     = 0xf9;                 	// PCA0 counter						PCA0������

//----------------------------------------------------------
#define Failed_passbacks	5							//ʧ�ܻش�����

//----------------------------------------------------------
extern void sys_Init (void); 												//ϵͳ��ʼ��
extern void UART0_Init (unsigned long int BAUDRATE);//���ڳ�ʼ��
extern void Motor_Init (void);  										//���������ʼ��
extern unsigned char Init(void);										//�ϵ��ʼ��
extern void Send(void); 												 		//����
extern signed char Receive(void);										//����
extern unsigned long int Inquire_STEP(void);				//��ѯʣ�ಽ��
extern void Motor_Stop(void);												//ɲ��

//----------------------------- ������ -----------------------------
/**
* @name      main
* @brief     ������
* @param    
* @retval	 
* @attention 			
*/
void main(void){
	unsigned char Return = 0;
	unsigned char Count = 0;
//-------------------- ��ʼ�� ----------------------------	
	sys_Init();					//ϵͳ��ʼ��
	Motor_Init();				//�����ʼ��
	UART0_Init(115200);	//��ʼ�����ڣ�������115200bps
 	EA = 1;							//ʹ��ȫ���ж�
	
//------------------- �������� ----------------------------
	while(1){
		if(Receive() == 1)					//���յ���ȷ����
			Return = 1;
		if(!Inquire_STEP() && (Return == 1)){		//���յ���ȷ������ִ�����
			Motor_Stop();							//ɲ��
			Send();										//ִ����ɣ��ش�
			Return  = 0;
		}
	}
}

///************************  COPYRIGHT (C) 2018 Chen ************************///
///******************************* END OF FILE ******************************///
