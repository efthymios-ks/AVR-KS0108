#include "KS0108.h"

//----- Auxiliary data ------//
uint8_t __GLCD_Buffer[__GLCD_Screen_Width][__GLCD_Screen_Lines];
	
GLCD_t __GLCD;

#define __GLCD_XtoChip(X)		((X < (__GLCD_Screen_Width / __GLCD_Screen_Chips)) ? Chip_1 : Chip_2)
#define __GLCD_Min(X, Y)		((X < Y) ? X : Y)
#define __GLCD_AbsDiff(X, Y)	((X > Y) ? (X - Y) : (Y - X))
#define __GLCD_Swap(X, Y)		do { typeof(X) t = X; X = Y; Y = t; } while (0)
//---------------------------//

//----- Prototypes ----------------------------//
static void GLCD_Send(const uint8_t Data);
static void GLCD_WaitBusy(enum Chip_t Chip);
static void GLCD_BufferWrite(const uint8_t X, const uint8_t Y, const uint8_t Data);
static uint8_t GLCD_BufferRead(const uint8_t X, const uint8_t Y);
static void GLCD_SelectChip(enum Chip_t Chip);
static void __GLCD_GotoX(const uint8_t X);
static void __GLCD_GotoY(const uint8_t Y);
static void GLCD_DrawHLine(uint8_t X1, uint8_t X2, const uint8_t Y, enum Color_t Color);
static void GLCD_DrawVLine(uint8_t Y1, uint8_t Y2, const uint8_t X, enum Color_t Color);
static void Int2bcd(int32_t Value, char BCD[]);
static inline void Pulse_En(void);
//---------------------------------------------//

//----- Functions -------------//
void GLCD_SendCommand(const uint8_t Command, enum Chip_t Chip)
{
	//Check if busy
	if (Chip != Chip_All)
	{
		GLCD_WaitBusy(Chip);
	}
	else
	{
		GLCD_WaitBusy(Chip_1);
		GLCD_WaitBusy(Chip_2);
	}
	GLCD_SelectChip(Chip);
	
	DigitalWrite(GLCD_DI, Low);      //RS = 0
	DigitalWrite(GLCD_RW, Low);      //RW = 0
	
	//Send data
	GLCD_Send(Command);
}

void GLCD_SendData(const uint8_t Data, enum Chip_t Chip)
{
	//Check if busy
	if (Chip != Chip_All)
	{
		GLCD_WaitBusy(Chip);
	}
	else
	{
		GLCD_WaitBusy(Chip_1);
		GLCD_WaitBusy(Chip_2);
	}
	GLCD_SelectChip(Chip);

	DigitalWrite(GLCD_DI, High);     //RS = 1
	DigitalWrite(GLCD_RW, Low);      //RW = 0

	//Send data
	GLCD_Send(__GLCD.Mode == GLCD_Non_Inverted ? Data : ~Data);
	
	__GLCD.X++;
	if (__GLCD.X == (__GLCD_Screen_Width / __GLCD_Screen_Chips))
		__GLCD_GotoX(__GLCD.X);
	else if (__GLCD.X >= __GLCD_Screen_Width)
		__GLCD.X = __GLCD_Screen_Width - 1;
}

void GLCD_Setup(void)
{
	//Setup pins
	PinMode(GLCD_D0, Output);	//GLCD pins = Outputs
	PinMode(GLCD_D1, Output);
	PinMode(GLCD_D2, Output);
	PinMode(GLCD_D3, Output);
	PinMode(GLCD_D4, Output);
	PinMode(GLCD_D5, Output);
	PinMode(GLCD_D6, Output);
	PinMode(GLCD_D7, Output);

	PinMode(GLCD_CS1, Output);
	PinMode(GLCD_CS2, Output);
	PinMode(GLCD_DI, Output);
	PinMode(GLCD_EN, Output);
	PinMode(GLCD_RW, Output);
	PinMode(GLCD_RST, Output);

	DigitalWrite(GLCD_DI, Low);		//GLCD pins = 0
	DigitalWrite(GLCD_RW, Low);
	DigitalWrite(GLCD_EN, Low);
	
	DigitalWrite(GLCD_RST, Low);	//!RST
	_delay_ms(5);
	DigitalWrite(GLCD_RST, High);
	_delay_ms(50);

	//Initialize chips
	GLCD_SendCommand(__GLCD_Command_On, Chip_All);
	GLCD_SendCommand(__GLCD_Command_Display_Start, Chip_All);

	//Go to 0,0
	GLCD_GotoXY(0, 0);
	
	//Reset GLCD structure
	__GLCD.Mode = GLCD_Non_Inverted;
	__GLCD.X = __GLCD.Y = __GLCD.Font.Width = __GLCD.Font.Height = __GLCD.Font.Lines = 0;
}

void GLCD_Render(void)
{
	uint8_t i, j;
	
	for (j = 0 ; j < __GLCD_Screen_Height ; j += __GLCD_Screen_Line_Height)
	{
		__GLCD_GotoX(0);
		__GLCD_GotoY(j);
		for (i = 0 ; i < __GLCD_Screen_Width ; i++)
			GLCD_SendData(GLCD_BufferRead(i, __GLCD.Y), __GLCD_XtoChip(i));
	}
}

void GLCD_InvertMode(void)
{
	if (__GLCD.Mode == GLCD_Inverted)
		__GLCD.Mode = GLCD_Non_Inverted;
	else
		__GLCD.Mode = GLCD_Inverted;
}

void GLCD_Clear(void)
{
	GLCD_FillScreen(__GLCD.Mode == GLCD_Non_Inverted ? GLCD_White : GLCD_Black);
}

void GLCD_ClearLine(const uint8_t Line)
{
	if (Line < __GLCD_Screen_Lines)
	{
		uint8_t i, color;
		i = 0;
		color = __GLCD.Mode == GLCD_Non_Inverted ? GLCD_White : GLCD_Black;

		GLCD_GotoXY(0, Line * __GLCD_Screen_Line_Height);
		for (i = 0 ; i < __GLCD_Screen_Width ; i++)
			GLCD_BufferWrite(i, __GLCD.Y, color);	
	}
}

void GLCD_GotoX(const uint8_t X)
{
	if (X < __GLCD_Screen_Width)
		__GLCD.X = X;
}

void GLCD_GotoY(const uint8_t Y)
{
	if (__GLCD.Y < __GLCD_Screen_Height)
		__GLCD.Y = Y;
}

void GLCD_GotoXY(const uint8_t X, const uint8_t Y)
{
	GLCD_GotoX(X);
	GLCD_GotoY(Y);
}

void GLCD_GotoLine(const uint8_t Line)
{
	if (Line < __GLCD_Screen_Lines)
		__GLCD.Y = Line * __GLCD_Screen_Line_Height;
}

uint8_t GLCD_GetX(void)
{
	return __GLCD.X;
}

uint8_t GLCD_GetY(void)
{
	return __GLCD.Y;
}

uint8_t GLCD_GetLine(void)
{
	return (__GLCD.Y / __GLCD_Screen_Line_Height);
}

void GLCD_SetPixel(const uint8_t X, const uint8_t Y, enum Color_t Color)
{
	uint8_t data = 0;
	
	//Goto to point
	GLCD_GotoXY(X, Y);

	//Read data
	data = GLCD_BufferRead(__GLCD.X, __GLCD.Y);
	
	//Set pixel
	if (Color == GLCD_Black)
		BitSet(data, Y % 8);
	else
		BitClear(data, Y % 8);
	
	//Sent data
	GLCD_BufferWrite(__GLCD.X, __GLCD.Y, data);
}

void GLCD_SetPixels(const uint8_t X1, uint8_t Y1, const uint8_t X2, const uint8_t Y2, enum Color_t Color)
{
	if ((X1 < __GLCD_Screen_Width) && (X2 < __GLCD_Screen_Width) &&
	(Y1 < __GLCD_Screen_Height) && (Y2 < __GLCD_Screen_Height))
	{
		uint8_t height, width, offset, mask, h, i, data;
		height = Y2 - Y1 + 1;
		width = X2 - X1 + 1;
		offset = Y1 % __GLCD_Screen_Line_Height;
		Y1 -= offset;
		mask = 0xFF;
		data = 0;

		//Calculate mask for top fractioned region
		if (height <(__GLCD_Screen_Line_Height - offset))
		{
			mask >>=(__GLCD_Screen_Line_Height - height);
			h = height;
		}
		else
			h = __GLCD_Screen_Line_Height - offset;
		mask <<= offset;

		//Draw fractional rows at the top of the region
		GLCD_GotoXY(X1, Y1);
		for (i = 0 ; i < width ; i++)
		{
			//Read
			data = GLCD_BufferRead(__GLCD.X, __GLCD.Y);
			//Mask
			data = ((Color == GLCD_Black) ? (data | mask) : (data & ~mask));
			//Write
			GLCD_BufferWrite(__GLCD.X++, __GLCD.Y, data);
		}

		//Full rows
		while ((h + __GLCD_Screen_Line_Height) <= height)
		{
			h += __GLCD_Screen_Line_Height;
			Y1 += __GLCD_Screen_Line_Height;
			GLCD_GotoXY(X1, Y1);
			for (i = 0 ; i < width ; i++)
				GLCD_BufferWrite(__GLCD.X++, __GLCD.Y, Color);
		}

		//Fractional rows at the bottom of the region
		if (h < height)
		{
			mask = ~(0xFF << (height - h));
			GLCD_GotoXY(X1, Y1 + __GLCD_Screen_Line_Height);
			for (i = 0 ; i < width ; i++)
			{
				//Read
				data = GLCD_BufferRead(__GLCD.X, __GLCD.Y);
				//Mask
				data = ((Color == GLCD_Black) ? (data | mask) : (data & ~mask));
				//Write
				GLCD_BufferWrite(__GLCD.X++, __GLCD.Y, data);
			}
		}
	}
}

void GLCD_DrawBitmap(const uint8_t *Bitmap, uint8_t Width, const uint8_t Height, enum PrintMode_t Mode)
{
	uint16_t lines, bmpRead, bmpReadPrev;
	uint8_t x, y, y2, i, j, overflow, data, dataPrev;
	lines = bmpRead = bmpReadPrev = x = y = i = j = overflow = data = dataPrev = 0;
	
	//#1 - Save current position
	x = __GLCD.X;
	y = y2 = __GLCD.Y;
	
	//#2 - Read width - First two bytes
	data = __GLCD.X + Width;														//"data" is used temporarily
	//If character exceed screen bounds, reduce
	if (data >= __GLCD_Screen_Width)
		Width -= data - __GLCD_Screen_Width;
	
	//#3 - Read height - Second two bytes - Convert to lines
	lines = (Height + __GLCD_Screen_Line_Height - 1) / __GLCD_Screen_Line_Height;	//lines = Ceiling(A/B) = (A+B-1)/B
	data = __GLCD.Y / __GLCD_Screen_Line_Height + lines;							//"data" is used temporarily
	//If bitmap exceed screen bounds, reduce
	if (data > __GLCD_Screen_Lines)
		lines -= data - __GLCD_Screen_Lines;
	
	//#4 - Calculate overflowing bits
	overflow = __GLCD.Y % __GLCD_Screen_Line_Height;
	
	//#5 - Print the character
	//Scan the lines needed
	for (j = 0 ; j < lines ; j++)
	{
		//Go to the start of the line
		GLCD_GotoXY(x, y);
		
		//Update the indices for reading the line
		bmpRead = j * Width;
		bmpReadPrev = bmpRead - Width;		//Previous = 4 + (j - 1) * width = Current - width

		//Scan bytes of selected line
		for (i = 0 ; i < Width ; i++)
		{
			//Read byte
			data = pgm_read_byte(&(Bitmap[bmpRead++]));
			
			//Shift byte
			data <<= overflow;
			
			//Merge byte with previous one
			if (j > 0)
			{
				dataPrev = pgm_read_byte(&(Bitmap[bmpReadPrev++]));
				dataPrev >>= __GLCD_Screen_Line_Height - overflow;
				data |= dataPrev;
			}
			//Edit byte depending on the mode
			if (Mode == GLCD_Merge)
				data |= GLCD_BufferRead(__GLCD.X, __GLCD.Y);
			
			//Send byte
			GLCD_BufferWrite(__GLCD.X++, __GLCD.Y, data);
		}
		//Send an empty column of 1px in the end'
		if (__GLCD.Font.Mode == GLCD_Overwrite)
			data = GLCD_White;
		else
			data = GLCD_BufferRead(__GLCD.X, __GLCD.Y);
		GLCD_BufferWrite(__GLCD.X, __GLCD.Y, data);
						
		//Increase line counter
		y += __GLCD_Screen_Line_Height;
	}

	//#6 - Update last line, if needed
	if (lines > 1)
	{
		//Go to the start of the line
		GLCD_GotoXY(x, y);
		
		//Update the index for reading the last printed line
		bmpReadPrev = (j - 1) * Width;

		//Scan bytes of selected line
		for (i = 0 ; i < Width ; i++)
		{
			//Read byte
			data = GLCD_BufferRead(__GLCD.X, __GLCD.Y);
			
			//Merge byte with previous one
			dataPrev = pgm_read_byte(&(Bitmap[bmpReadPrev++]));
			dataPrev >>= __GLCD_Screen_Line_Height - overflow;
			data |= dataPrev;
			
			//Edit byte depending on the mode
			if (Mode == GLCD_Merge)
				data |= GLCD_BufferRead(__GLCD.X, __GLCD.Y);
			
			//Send byte
			GLCD_BufferWrite(__GLCD.X++, __GLCD.Y, data);
		}
		//Send an empty column of 1px in the end
		if (__GLCD.Font.Mode == GLCD_Overwrite)
			data = GLCD_White;
		else if (__GLCD.Font.Mode == GLCD_Merge)
			data = GLCD_BufferRead(__GLCD.X, __GLCD.Y);
		else
			data = ~GLCD_BufferRead(__GLCD.X, __GLCD.Y);
		GLCD_BufferWrite(__GLCD.X++, __GLCD.Y,data);
	}
	
	//Go to the upper-right corner of the printed bitmap
	GLCD_GotoXY(GLCD_GetX(), y2);
}

void GLCD_DrawLine(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2, enum Color_t Color)
{
	if ((X1 < __GLCD_Screen_Width) && (X2 < __GLCD_Screen_Width) &&
	(Y1 < __GLCD_Screen_Height) && (Y2 < __GLCD_Screen_Height))
	{
		if (X1 == X2)
			GLCD_DrawVLine(Y1, Y2, X1, Color);
		else if (Y1 == Y2)
			GLCD_DrawHLine(X1, X2, Y1, Color);
		else
		{
			uint8_t deltax, deltay, x, y, slope;
			int8_t error, ystep;
			slope = ((__GLCD_AbsDiff(Y1, Y2) > __GLCD_AbsDiff(X1,X2)) ? 1 : 0);
			if (slope)
			{
				//Swap x1, y1
				__GLCD_Swap(X1, Y1);
				//Swap x2, y2
				__GLCD_Swap(X2, Y2);
			}
			if (X1 > X2)
			{
				//Swap x1, x2
				__GLCD_Swap(X1, X2);
				//Swap y1,y2
				__GLCD_Swap(Y1, Y2);
			}
			
			deltax = X2 - X1;
			deltay = __GLCD_AbsDiff(Y2, Y1);
			error = deltax / 2;
			y = Y1;
			ystep = ((Y1 < Y2) ? 1 : -1);
			
			for (x=X1 ; x<=X2 ; x++)
			{
				if (slope)
					GLCD_SetPixel(y, x, Color);
				else
					GLCD_SetPixel(x, y, Color);
				
				error -= deltay;
				if (error < 0)
				{
					y = y + ystep;
					error = error + deltax;
				}
			}
		}
	}
}

void GLCD_DrawRectangle(const uint8_t X1, const uint8_t Y1, const uint8_t X2, const uint8_t Y2, enum Color_t Color)
{
	if ((X1 < __GLCD_Screen_Width) && (X2 < __GLCD_Screen_Width) &&
	(Y1 < __GLCD_Screen_Height) && (Y2 < __GLCD_Screen_Height))
	{
		GLCD_DrawHLine(X1, X2, Y1, Color);
		GLCD_DrawHLine(X1, X2, Y2, Color);
		GLCD_DrawVLine(Y1, Y2, X1, Color);
		GLCD_DrawVLine(Y1, Y2, X2, Color);
	}
}

void GLCD_DrawRoundRectangle(const uint8_t X1, const uint8_t Y1, const uint8_t X2, const uint8_t Y2, const uint8_t Radius, enum Color_t Color)
{
	if ((X1<__GLCD_Screen_Width) && (X2<__GLCD_Screen_Width) &&
	(Y1<__GLCD_Screen_Height) && (Y2<__GLCD_Screen_Height))
	{

		int16_t tSwitch = 3 - 2 * Radius;
		uint8_t width, height, x, y;
		width = X2-X1;
		height = Y2-Y1;
		x = 0;
		y = Radius;

		//Draw perimeter
		GLCD_DrawHLine(X1+Radius, X2-Radius, Y1, Color);	//Top
		GLCD_DrawHLine(X1+Radius, X2-Radius, Y2, Color);	//Bottom
		GLCD_DrawVLine(Y1+Radius, Y2-Radius, X1, Color);	//Left
		GLCD_DrawVLine(Y1+Radius, Y2-Radius, X2, Color);	//Right
		
		while (x <= y)
		{
			//Upper left corner
			GLCD_SetPixel(X1+Radius-x, Y1+Radius-y, Color);
			GLCD_SetPixel(X1+Radius-y, Y1+Radius-x, Color);

			//Upper right corner
			GLCD_SetPixel(X1+width-Radius+x, Y1+Radius-y, Color);
			GLCD_SetPixel(X1+width-Radius+y, Y1+Radius-x, Color);

			//Lower left corner
			GLCD_SetPixel(X1+Radius-x, Y1+height-Radius+y, Color);
			GLCD_SetPixel(X1+Radius-y, Y1+height-Radius+x, Color);

			//Lower right corner
			GLCD_SetPixel(X1+width-Radius+x, Y1+height-Radius+y, Color);
			GLCD_SetPixel(X1+width-Radius+y, Y1+height-Radius+x, Color);

			if (tSwitch < 0)
				tSwitch += 4 * x + 6;
			else
			{
				tSwitch += 4 * (x - y) + 10;
				y--;
			}
			x++;
		}
	}
}

void GLCD_DrawTriangle(const uint8_t X1, const uint8_t Y1, const uint8_t X2, const uint8_t Y2, const uint8_t X3, const uint8_t Y3, enum Color_t Color)
{
	if (((X1 < __GLCD_Screen_Width) && (X2 < __GLCD_Screen_Width) && (X3 < __GLCD_Screen_Width) &&
		(Y1 < __GLCD_Screen_Height) && (Y2 < __GLCD_Screen_Height) && (Y3 < __GLCD_Screen_Height)))
	{
		GLCD_DrawLine(X1, Y1, X2, Y2, Color);
		GLCD_DrawLine(X2, Y2, X3, Y3, Color);
		GLCD_DrawLine(X3, Y3, X1, Y1, Color);
	}
};

void GLCD_DrawCircle(const uint8_t CenterX, const uint8_t CenterY, const uint8_t Radius, enum Color_t Color)
{
	if (((CenterX + Radius) < __GLCD_Screen_Width) && ((CenterY + Radius) < __GLCD_Screen_Height))
	{
		uint8_t x, y;
		int16_t xChange, radiusError;
		uint16_t yChange;
		x = Radius;
		y = 0;
		xChange = 1 - 2 * Radius;
		yChange = 1;
		radiusError = 0;
		
		while (x >= y)
		{
			GLCD_SetPixel(CenterX+x, CenterY+y, Color);
			GLCD_SetPixel(CenterX-x, CenterY+y, Color);
			GLCD_SetPixel(CenterX-x, CenterY-y, Color);
			GLCD_SetPixel(CenterX+x, CenterY-y, Color);
			GLCD_SetPixel(CenterX+y, CenterY+x, Color);
			GLCD_SetPixel(CenterX-y, CenterY+x, Color);
			GLCD_SetPixel(CenterX-y, CenterY-x, Color);
			GLCD_SetPixel(CenterX+y, CenterY-x, Color);
			y++;
			radiusError += yChange;
			yChange += 2;
			if ((2 * radiusError + xChange) > 0)
			{
				x--;
				radiusError += xChange;
				xChange += 2;
			}
		}
	}
}

void GLCD_FillScreen(enum Color_t Color)
{
	uint8_t i, j;

	for (j = 0 ; j < __GLCD_Screen_Height ; j += __GLCD_Screen_Line_Height)
		for (i = 0 ; i < __GLCD_Screen_Width ; i++)
			GLCD_BufferWrite(i, j, Color);
}

void GLCD_FillRectangle(const uint8_t X1, const uint8_t Y1, const uint8_t X2, const uint8_t Y2, enum Color_t Color)
{
	GLCD_SetPixels(X1, Y1, X2, Y2, Color);
}

void GLCD_FillRoundRectangle(const uint8_t X1, const uint8_t Y1, const uint8_t X2, const uint8_t Y2, const uint8_t Radius, enum Color_t Color)
{
	if ((X1 < __GLCD_Screen_Width) && (X2 < __GLCD_Screen_Width) &&
	(Y1 < __GLCD_Screen_Height) && (Y2 < __GLCD_Screen_Height))
	{

		int16_t tSwitch = 3 - 2 * Radius;
		uint8_t width, height, x, y;
		width = X2 - X1;
		height = Y2 - Y1;
		x = 0;
		y = Radius;
		
		//Fill center block
		GLCD_FillRectangle(X1+Radius, Y1, X2 - Radius, Y2, Color);
		
		while (x <= y)
		{
			//Left side
			GLCD_DrawLine(
			X1 + Radius - x, Y1 + Radius - y,				//Upper left corner upper half
			X1 + Radius - x, Y1 + height - Radius + y,		//Lower left corner lower half
			Color);
			GLCD_DrawLine(
			X1 + Radius - y, Y1 + Radius - x,					//Upper left corner lower half
			X1 + Radius - y, Y1 + height - Radius + x,			//Lower left corner upper half
			Color);

			//Right side
			GLCD_DrawLine(
			X1 + width - Radius	+ x, Y1 + Radius - y,			//Upper right corner upper half
			X1 + width - Radius + x, Y1 + height - Radius + y,	//Lower right corner lower half
			Color);
			GLCD_DrawLine(
			X1 + width - Radius + y, Y1 + Radius - x,			//Upper right corner lower half
			X1 + width - Radius + y, Y1 + height - Radius + x,	//Lower right corner upper half
			Color);

			if (tSwitch < 0)
			{
				tSwitch += 4 * x +6;
			}
			else
			{
				tSwitch += 4 * (x - y) + 10;
				y--;
			}
			x++;
		}
	}
}

void GLCD_FillTriangle(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t y2, uint8_t X3, uint8_t Y3, enum Color_t Color)
{
	if (((X1 < __GLCD_Screen_Width) && (X2 < __GLCD_Screen_Width) && (X3 < __GLCD_Screen_Width) &&
	(Y1 < __GLCD_Screen_Height) && (y2 < __GLCD_Screen_Height) && (Y3 < __GLCD_Screen_Height)))
	{
		uint8_t sl, sx1, sx2;
		double m1, m2, m3;
		sl = sx1 = sx2 = m1 = m2 = m3 = 0;
		
		if (y2 > Y3)
		{
			__GLCD_Swap(X2, X3);
			__GLCD_Swap(y2, Y3);
		}
		if (Y1 > y2)
		{
			__GLCD_Swap(X1, X2);
			__GLCD_Swap(Y1, y2);
		}
		m1 = (double)(X1 - X2) / (Y1 - y2);
		m2 = (double)(X2 - X3) / (y2 - Y3);
		m3 = (double)(X3 - X1) / (Y3 - Y1);
		for(sl = Y1 ; sl <= y2 ; sl++)
		{
			sx1= m1 * (sl - Y1) + X1;
			sx2= m3 * (sl - Y1) + X1;
			if (sx1> sx2)
			__GLCD_Swap(sx1, sx2);
			GLCD_DrawLine(sx1, sl, sx2, sl, Color);
		}
		for (sl = y2 ; sl <= Y3 ; sl++)
		{
			sx1= m2 * (sl - Y3) + X3;
			sx2= m3 * (sl - Y1) + X1;
			if (sx1 > sx2)
				__GLCD_Swap(sx1, sx2);
			GLCD_DrawLine(sx1, sl, sx2, sl, Color);
		}
	}
}

void GLCD_FillCircle(const uint8_t CenterX, const uint8_t CenterY, const uint8_t Radius, enum Color_t Color)
{
	if (((CenterX + Radius) < __GLCD_Screen_Width) && 
	((CenterY + Radius) < __GLCD_Screen_Height))
	{
		int8_t f, ddF_x, ddF_y;
		uint8_t  x, y;
		f = 1 - Radius;
		ddF_x = 1;
		ddF_y = -2 * Radius;
		x = 0;
		y = Radius;
		
		
		//Fill in the center between the two halves
		GLCD_DrawLine(CenterX, CenterY - Radius, CenterX, CenterY + Radius, Color);

		while(x < y)
		{
			//ddF_x = 2 * x + 1;
			//ddF_y = -2 * y;
			//f = x*x + y*y - radius*radius + 2*x - y + 1;
			if (f >= 0)
			{
				y--;
				ddF_y += 2;
				f += ddF_y;
			}
			x++;
			ddF_x += 2;
			f += ddF_x;

			//Now draw vertical lines between the points on the circle rather than
			//draw the points of the circle. This draws lines between the
			//perimeter points on the upper and lower quadrants of the 2 halves of the circle.
			GLCD_DrawVLine(CenterY + y, CenterY - y, CenterX + x, Color);
			GLCD_DrawVLine(CenterY + y, CenterY - y, CenterX - x, Color);
			GLCD_DrawVLine(CenterY + x, CenterY - x, CenterX + y, Color);
			GLCD_DrawVLine(CenterY + x, CenterY - x, CenterX - y, Color);
		}
	}
}

void GLCD_InvertRect(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2)
{
	uint8_t width, height, offset, mask, h, i, data;

	width = X2 - X1 + 1;
	height = Y2 - Y1 + 1;
	offset = Y1 % __GLCD_Screen_Line_Height;
	Y1 -= offset;
	mask = 0xFF;
	data = 0;

	//Calculate mask for top fractioned region
	if (height < (__GLCD_Screen_Line_Height - offset))
	{
		mask >>= (__GLCD_Screen_Line_Height - height);
		h = height;
	}
	else
		h = __GLCD_Screen_Line_Height - offset;
	mask <<= offset;
	
	//Draw fractional rows at the top of the region
	GLCD_GotoXY(X1, Y1);
	for (i = 0 ; i < width ; i++)
	{
		data = GLCD_BufferRead(__GLCD.X, __GLCD.Y);
		data = ((~data) & mask) | (data & (~mask));
		GLCD_BufferWrite(__GLCD.X++, __GLCD.Y, data);
	}

	//Full rows
	while ((h + __GLCD_Screen_Line_Height) <= height)
	{
		h += __GLCD_Screen_Line_Height;
		Y1 += __GLCD_Screen_Line_Height;
		GLCD_GotoXY(X1, Y1);
		
		for (i=0 ; i < width ; i++)
		{
			data = ~GLCD_BufferRead(__GLCD.X, __GLCD.Y);
			GLCD_BufferWrite(__GLCD.X++, __GLCD.Y, data);
		}
	}

	//Fractional rows at the bottom of the region
	if (h < height)
	{
		mask = ~(0xFF<<(height - h));
		GLCD_GotoXY(X1, (Y1 + __GLCD_Screen_Line_Height));

		for (i = 0 ; i < width ; i++)
		{
			data = GLCD_BufferRead(__GLCD.X, __GLCD.Y);
			data = ((~data) & mask) | (data & (~mask));
			GLCD_BufferWrite(__GLCD.X++, __GLCD.Y, data);
		}
	}
}

void GLCD_SetFont(const uint8_t *Name, const uint8_t Width, const uint8_t Height, enum PrintMode_t Mode)
{
	if ((Width < __GLCD_Screen_Width) && (Height < __GLCD_Screen_Height) && ((Mode == GLCD_Overwrite) || (Mode == GLCD_Merge)))
	{
		//Change font pointer to new font
		__GLCD.Font.Name = (uint8_t *)(Name);
		
		//Update font's size
		__GLCD.Font.Width = Width;
		__GLCD.Font.Height = Height;
		
		//Update lines required for a character to be fully displayed
		__GLCD.Font.Lines = (Height - 1) / __GLCD_Screen_Line_Height + 1;
		
		//Update blending mode
		__GLCD.Font.Mode = Mode;
	}
}

uint8_t GLCD_GetWidthChar(const char Character)
{
	return (pgm_read_byte(&(__GLCD.Font.Name[(Character - 32) * (__GLCD.Font.Width * __GLCD.Font.Lines + 1)])));
}

uint16_t GLCD_GetWidthString(const char *Text)
{
	uint16_t width = 0;

	while (*Text)
	width += GLCD_GetWidthChar(*Text++);
	
	return width;
}

uint16_t GLCD_GetWidthString_P(const char *Text)
{
	uint16_t width = 0;
	char r = pgm_read_byte(Text++);
	while (r)
	{
		width += GLCD_GetWidthChar(r);
		r = pgm_read_byte(Text++);
	}
	
	return width;
}

void GLCD_PrintChar(char Character)
{
	uint16_t fontStart, fontRead, fontReadPrev;
	uint8_t x, y, y2, i, j, width, lines, overflow, data, dataPrev;
	fontStart = fontRead = fontReadPrev = x = y = y2 = i = j = width = lines = overflow = data = dataPrev = 0;
	
	//#1 - Save current position
	x = __GLCD.X;
	y = y2 = __GLCD.Y;
	
	//#2 - Remove leading empty characters
	Character -= 32;														//32 is the ASCII of the first printable character
	
	//#3 - Find the start of the character in the font array
	fontStart = Character * (__GLCD.Font.Width * __GLCD.Font.Lines + 1);		//+1 due to first byte of each array line being the width
	
	//#4 - Update width - First byte of each line is the width of the character
	width = pgm_read_byte(&(__GLCD.Font.Name[fontStart++]));
	data = __GLCD.X + width;											//"data" is used temporarily
	//If character exceed screen bounds, reduce
	if (data >= __GLCD_Screen_Width)
		width -= data-__GLCD_Screen_Width;
	
	//#5 - Update lines
	lines = __GLCD.Font.Lines;
	data = __GLCD.Y / __GLCD_Screen_Line_Height + lines;				//"data" is used temporarily
	//If character exceed screen bounds, reduce
	if (data > __GLCD_Screen_Lines)
		lines -= data - __GLCD_Screen_Lines;
	
	//#6 - Calculate overflowing bits
	overflow = __GLCD.Y % __GLCD_Screen_Line_Height;
		
	//#7 - Print the character
	//Scan the lines needed
	for (j = 0 ; j < lines ; j++)
	{
		//Go to the start of the line
		GLCD_GotoXY(x, y);
		
		//Update the indices for reading the line
		fontRead = fontStart + j;
		fontReadPrev = fontRead - 1;

		//Scan bytes of selected line
		for (i = 0 ; i < width ; i++)
		{
			//Read byte
			data = pgm_read_byte(&(__GLCD.Font.Name[fontRead]));
			
			//Shift byte
			data <<= overflow;
			
			//Merge byte with previous one
			if (j > 0)
			{
				dataPrev = pgm_read_byte(&(__GLCD.Font.Name[fontReadPrev]));
				dataPrev >>= __GLCD_Screen_Line_Height - overflow;
				data |= dataPrev;
				fontReadPrev += __GLCD.Font.Lines;
			}
			//Edit byte depending on the mode
			if (__GLCD.Font.Mode == GLCD_Merge)
				data |= GLCD_BufferRead(__GLCD.X, __GLCD.Y);
			
			//Send byte
			GLCD_BufferWrite(__GLCD.X++, __GLCD.Y, data);
			
			//Increase index
			fontRead += __GLCD.Font.Lines;
		}
		//Send an empty column of 1px in the end
		if (__GLCD.Font.Mode == GLCD_Overwrite)
			data = GLCD_White;
		else
			data = GLCD_BufferRead(__GLCD.X, __GLCD.Y);
		GLCD_BufferWrite(__GLCD.X, __GLCD.Y, data);
		
		//Increase line counter
		y += __GLCD_Screen_Line_Height;
	}

	//#8 - Update last line, if needed
	if (lines > 1)
	{
		//Go to the start of the line
		GLCD_GotoXY(x, y);
		
		//Update the index for reading the last printed line
		fontReadPrev = fontStart + j - 1;

		//Scan bytes of selected line
		for (i = 0 ; i < width ; i++)
		{
			//Read byte
			data = GLCD_BufferRead(__GLCD.X, __GLCD.Y);
			
			//Merge byte with previous one
			dataPrev = pgm_read_byte(&(__GLCD.Font.Name[fontReadPrev]));
			dataPrev >>= __GLCD_Screen_Line_Height - overflow;
			data |= dataPrev;
			
			//Edit byte depending on the mode
			if (__GLCD.Font.Mode == GLCD_Merge)
				data |= GLCD_BufferRead(__GLCD.X, __GLCD.Y);
			
			//Send byte
			GLCD_BufferWrite(__GLCD.X, __GLCD.Y, data);

			//Increase index
			fontReadPrev += __GLCD.Font.Lines;
		}
		//Send an empty column of 1px in the end
		if (__GLCD.Font.Mode == GLCD_Overwrite)
			data = GLCD_White;
		else if (__GLCD.Font.Mode == GLCD_Merge)
			data = GLCD_BufferRead(__GLCD.X, __GLCD.Y);
		else
			data = ~GLCD_BufferRead(__GLCD.X, __GLCD.Y);
		GLCD_BufferWrite(__GLCD.X++, __GLCD.Y,data);
	}

	
	//Move cursor to the end of the printed character
	GLCD_GotoXY(x + width + 1, y2);
}

void GLCD_PrintString(const char *Text)
{
	while(*Text)
	{
		if ((__GLCD.X + __GLCD.Font.Width) >= __GLCD_Screen_Width)
			break;
		
		GLCD_PrintChar(*Text++);
	}
}

void GLCD_PrintString_P(const char *Text)
{
	char r = pgm_read_byte(Text++);
	while(r)
	{
		if ((__GLCD.X + __GLCD.Font.Width) >= __GLCD_Screen_Width)
			break;

		GLCD_PrintChar(r);
		r = pgm_read_byte(Text++);
	}
}

void GLCD_PrintInteger(const int32_t Value)
{
	if (Value == 0)
	{
		GLCD_PrintChar('0');
	}
	else if ((Value > INT32_MIN) && (Value <= INT32_MAX))
	{
		//int32_max + sign + null = 12 bytes
		char bcd[12] = { '\0' };
		
		//Convert integer to array
		Int2bcd(Value, bcd);
		
		//Print from first non-zero digit
		GLCD_PrintString(bcd);
	}
}

void GLCD_PrintDouble(double Value, const uint32_t Tens)
{
	if (Value == 0)
	{
		//Print characters individually so no string is stored in RAM
		GLCD_PrintChar('0');
		GLCD_PrintChar('.');
		GLCD_PrintChar('0');
	}
	else if ((Value >= (-2147483647)) && (Value < 2147483648))
	{
		//Print sign
		if (Value<0)
		{
			Value = -Value;
			GLCD_PrintChar('-');
		}
		
		//Print integer part
		GLCD_PrintInteger(Value);
		
		//Print dot
		GLCD_PrintChar('.');
		
		//Print decimal part
		GLCD_PrintInteger((Value - (uint32_t)(Value)) * Tens);
	}
}

static void GLCD_Send(const uint8_t Data)
{
	//Send nibble
	DigitalWrite(GLCD_D0, BitCheck(Data, 0));
	DigitalWrite(GLCD_D1, BitCheck(Data, 1));
	DigitalWrite(GLCD_D2, BitCheck(Data, 2));
	DigitalWrite(GLCD_D3, BitCheck(Data, 3));
	DigitalWrite(GLCD_D4, BitCheck(Data, 4));
	DigitalWrite(GLCD_D5, BitCheck(Data, 5));
	DigitalWrite(GLCD_D6, BitCheck(Data, 6));
	DigitalWrite(GLCD_D7, BitCheck(Data, 7));
	Pulse_En();
}

static void GLCD_WaitBusy(enum Chip_t Chip)
{
	uint8_t status = 0;
	
	GLCD_SelectChip(Chip);
	
	//Busy pin = Input
	PinMode(GLCD_D7, Input);

	DigitalWrite(GLCD_DI, Low);
	DigitalWrite(GLCD_RW, High);
	DigitalWrite(GLCD_EN, Low);
	_delay_us(__GLCD_Pulse_En);
	
	//Send Enable pulse and wait till busy flag goes Low
	do
	{
		DigitalWrite(GLCD_EN, High);
		_delay_us(__GLCD_Pulse_En);
		
		status = DigitalRead(GLCD_D7) << 7;
		
		DigitalWrite(GLCD_EN, Low);
		_delay_us(__GLCD_Pulse_En<<3);
	}
	while(BitCheck(status, __GLCD_BUSY_FLAG));

	DigitalWrite(GLCD_RW, Low);
	
	//Busy pin = Output
	PinMode(GLCD_D7, Output);
}

static void GLCD_BufferWrite(const uint8_t X, const uint8_t Y, const uint8_t Data)
{
	//a>>3 = a/8
	__GLCD_Buffer[X][Y>>3] = Data;
}

static uint8_t GLCD_BufferRead(const uint8_t X, const uint8_t Y)
{
	//a>>3 = a/8
	return (__GLCD_Buffer[X][Y>>3]);
}

static void GLCD_SelectChip(enum Chip_t Chip)
{
	uint8_t on, off;

	#if (GLCD_ACTIVE_LOW != 0)
		on = 0;
		off = 1;
	#else
		on = 1;
		off = 0;
	#endif

	if(Chip == Chip_1)
	{
		DigitalWrite(GLCD_CS2, off);
		DigitalWrite(GLCD_CS1, on);
	}
	else if (Chip == Chip_2)
	{
		DigitalWrite(GLCD_CS1, off);
		DigitalWrite(GLCD_CS2, on);
	}
	else
	{
		DigitalWrite(GLCD_CS1, on);
		DigitalWrite(GLCD_CS2, on);
	}
}

static void __GLCD_GotoX(const uint8_t X)
{
	if (X < __GLCD_Screen_Width)
	{
		uint8_t chip, cmd;
		
		//Update command
		chip = __GLCD_XtoChip(X);
		cmd = ((chip == Chip_1) ? (__GLCD_Command_Set_Address | X) : (__GLCD_Command_Set_Address | (X - __GLCD_Screen_Width / __GLCD_Screen_Chips)));
		
		//Update tracker
		__GLCD.X = X;
		
		//Send command
		GLCD_SendCommand(cmd, chip);
	}
}

static void __GLCD_GotoY(const uint8_t Y)
{
	if (Y < __GLCD_Screen_Height)
	{
		uint8_t cmd;
		
		//Update command
		cmd = __GLCD_Command_Set_Page | (Y / __GLCD_Screen_Line_Height);
				
		//Update tracker
		__GLCD.Y = Y;
		
		//Send command
		GLCD_SendCommand(cmd, Chip_All);
	}
}

static inline void GLCD_DrawHLine(uint8_t X1, uint8_t X2, const uint8_t Y, enum Color_t Color)
{
	if (X1 > X2)
		__GLCD_Swap(X1, X2);
	
	while (X1 <= X2)
	{
		GLCD_SetPixel(X1, Y, Color);
		X1++;
	}
}

static inline void GLCD_DrawVLine(uint8_t Y1, uint8_t Y2, const uint8_t X, enum Color_t Color)
{
	if (Y1 > Y2)
		__GLCD_Swap(Y1, Y2);

	GLCD_SetPixels(X, Y1, X, Y2, Color);
}

static inline void Pulse_En(void)
{
	DigitalWrite(GLCD_EN, High);
	_delay_us(__GLCD_Pulse_En);
	DigitalWrite(GLCD_EN, Low);
	_delay_us(__GLCD_Pulse_En);
}

static void Int2bcd(int32_t Value, char BCD[])
{
	uint8_t isNegative = 0;
	
	BCD[0] = BCD[1] = BCD[2] =
	BCD[3] = BCD[4] = BCD[5] =
	BCD[6] = BCD[7] = BCD[8] =
	BCD[9] = BCD[10] = '0';
	
	if (Value < 0)
	{
		isNegative = 1;
		Value = -Value;
	}
	
	while (Value > 1000000000)
	{
		Value -= 1000000000;
		BCD[1]++;
	}
	
	while (Value >= 100000000)
	{
		Value -= 100000000;
		BCD[2]++;
	}
	
	while (Value >= 10000000)
	{
		Value -= 10000000;
		BCD[3]++;
	}
	
	while (Value >= 1000000)
	{
		Value -= 1000000;
		BCD[4]++;
	}
	
	while (Value >= 100000)
	{
		Value -= 100000;
		BCD[5]++;
	}

	while (Value >= 10000)
	{
		Value -= 10000;
		BCD[6]++;
	}

	while (Value >= 1000)
	{
		Value -= 1000;
		BCD[7]++;
	}
	
	while (Value >= 100)
	{
		Value -= 100;
		BCD[8]++;
	}
	
	while (Value >= 10)
	{
		Value -= 10;
		BCD[9]++;
	}

	while (Value >= 1)
	{
		Value -= 1;
		BCD[10]++;
	}

	uint8_t i = 0;
	//Find first non zero digit
	while (BCD[i] == '0')
	i++;

	//Add sign
	if (isNegative)
	{
		i--;
		BCD[i] = '-';
	}

	//Shift array
	uint8_t end = 10 - i;
	uint8_t offset = i;
	i = 0;
	while (i <= end)
	{
		BCD[i] = BCD[i + offset];
		i++;
	}
	BCD[i] = '\0';
}
//-----------------------------//
