#ifndef KS0108_H_INCLUDED
#define KS0108_H_INCLUDED
/*
||
||  Filename:	 		KS0108.h
||  Title: 			    KS0108 Driver
||  Author: 			Efthymios Koktsidis
||	Email:				efthymios.ks@gmail.com
||  Compiler:		 	AVR-GCC
||	Description:
||	This library can drive KS0108 based GLCD.
||
*/

//----- Headers ------------//
#include <util/delay.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "IO_Macros.h"
#include "KS0108_Settings.h"
//--------------------------//

//----- Auxiliary data ---------------------------//
#define __GLCD_Pulse_En					1

#define __GLCD_Command_On				0x3F
#define __GLCD_Command_Off				0x3E
#define __GLCD_Command_Set_Address		0x40
#define __GLCD_Command_Set_Page	   		0xB8
#define __GLCD_Command_Display_Start	0xC0

#define __GLCD_Screen_Width          	128
#define __GLCD_Screen_Height         	64
#define	__GLCD_Screen_Line_Height		8
#define __GLCD_Screen_Lines				__GLCD_Screen_Height / __GLCD_Screen_Line_Height
#define __GLCD_Screen_Chips				2

#define __GLCD_BUSY_FLAG				7

enum Chip_t
{
	Chip_1,
	Chip_2,
	Chip_All
};
enum ReadMode_t
{
	GLCD_Increment,
	GLCD_No_Increment
};
enum OperatingMode_t
{
	GLCD_Inverted,
	GLCD_Non_Inverted
};
enum PrintMode_t
{
	GLCD_Overwrite,
	GLCD_Merge
};
enum Color_t
{
	GLCD_White = 0x00,
	GLCD_Black = 0xFF
};

typedef struct
{
	uint8_t *Name;
	uint8_t Width;
	uint8_t Height;
	uint8_t Lines;
	enum PrintMode_t Mode;
}Font_t;

typedef struct
{
	uint8_t X;
	uint8_t Y;
	enum OperatingMode_t Mode;
	Font_t Font;
}GLCD_t;
//------------------------------------------------//

//----- Prototypes ------------------------------------------------------------//
void GLCD_SendCommand(const uint8_t Command, enum Chip_t Chip);
void GLCD_SendData(const uint8_t Data, enum Chip_t Chip);
void GLCD_Setup(void);
void GLCD_Render(void);
void GLCD_InvertMode(void);

void GLCD_Clear(void);
void GLCD_ClearLine(const uint8_t Line);
void GLCD_GotoX(const uint8_t X);
void GLCD_GotoY(const uint8_t Y);
void GLCD_GotoXY(const uint8_t X, const uint8_t Y);
void GLCD_GotoLine(const uint8_t line);
uint8_t GLCD_GetX(void);
uint8_t GLCD_GetY(void);
uint8_t GLCD_GetLine(void);

void GLCD_SetPixel(const uint8_t X, const uint8_t Y, enum Color_t Color);
void GLCD_SetPixels(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2, enum Color_t Color);

void GLCD_DrawBitmap(const uint8_t *Bitmap, uint8_t Width, const uint8_t Height, enum PrintMode_t Mode);
void GLCD_DrawLine(const uint8_t X1, const uint8_t Y1, const uint8_t X2, const uint8_t Y2, enum Color_t Color);
void GLCD_DrawRectangle(const uint8_t X1, const uint8_t Y1, const uint8_t X2, const uint8_t Y2, enum Color_t Color);
void GLCD_DrawRoundRectangle(const uint8_t X1, const uint8_t Y1, const uint8_t X2, const uint8_t Y2, const uint8_t Radius, enum Color_t Color);
void GLCD_DrawTriangle(const uint8_t X1, const uint8_t Y1, const uint8_t X2, const uint8_t Y2, const uint8_t X3, const uint8_t Y3, enum Color_t Color);
void GLCD_DrawCircle(const uint8_t CenterX, const uint8_t CenterY, uint8_t Radius, enum Color_t Color);

void GLCD_FillScreen(enum Color_t Color);
void GLCD_FillRectangle(const uint8_t X1, const uint8_t Y1, const uint8_t X2, const uint8_t Y2, enum Color_t Color);
void GLCD_FillRoundRectangle(const uint8_t X1, const uint8_t Y1, const uint8_t X2, const uint8_t Y2, const uint8_t Radius, enum Color_t Color);
void GLCD_FillTriangle(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2, uint8_t X3, uint8_t Y3, enum Color_t Color);
void GLCD_FillCircle(const uint8_t CenterX, const uint8_t CenterY, const uint8_t Radius, enum Color_t Color);

void GLCD_InvertScreen(void);
void GLCD_InvertRect(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2);

void GLCD_SetFont(const uint8_t *Name, const uint8_t Width, const uint8_t Height, enum PrintMode_t Mode);
uint8_t GLCD_GetWidthChar(const char Character);
uint16_t GLCD_GetWidthString(const char *Text);
uint16_t GLCD_GetWidthString_P(const char *Text);
void GLCD_PrintChar(char Character);
void GLCD_PrintString(const char *Text);
void GLCD_PrintString_P(const char *Text);
void GLCD_PrintInteger(const int32_t Value);
void GLCD_PrintDouble(double Value, const uint32_t Tens);
//-----------------------------------------------------------------------------//
#endif