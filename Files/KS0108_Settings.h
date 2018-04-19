#ifndef KS0108_SETTINGS_H_INCLUDED
#define KS0108_SETTINGS_H_INCLUDED
/*
||
||  Filename:	 		KS0108_Settings.h
||  Title: 			    KS0108 Driver Settings
||  Author: 			Efthymios Koktsidis
||	Email:				efthymios.ks@gmail.com
||  Compiler:		 	AVR-GCC
||	Description:
||	Settings for the KS0108 driver. 
||
*/

//----- Configuration -------------//
//Chip Enable Pin
#define GLCD_Active_Low		0

//GLCD pins					PORT, PIN
#define GLCD_D0				A, 0
#define GLCD_D1				A, 1
#define GLCD_D2				A, 2
#define GLCD_D3				A, 3
#define GLCD_D4				A, 4
#define GLCD_D5				A, 5
#define GLCD_D6				A, 6
#define GLCD_D7				A, 7

#define GLCD_DI				B, 0
#define GLCD_RW				B, 1
#define GLCD_EN				B, 2
#define GLCD_CS1			B, 3
#define GLCD_CS2			B, 4
#define GLCD_RST			B, 5
//---------------------------------//
#endif