//Headers
#include <avr/pgmspace.h>
#include "KS0108.h"
#include "Tahoma11x13.h"
#include "Tekton_Pro_Ext27x28.h"

const char txt1[] PROGMEM = "Big";
const char txt2[] PROGMEM = "Small";

int main(void)
{		
	//Setup
	GLCD_Setup();
	GLCD_Clear();

	//Inverted mode
	GLCD_InvertMode();
		
	//Print text #1
	GLCD_SetFont(Tekton_Pro_Ext27x28, 27, 28, GLCD_Merge);
	GLCD_GotoXY(42, 1);
	GLCD_PrintString_P(txt1);

	//Print text #2
	GLCD_SetFont(Tahoma11x13, 11, 13, GLCD_Merge);
	GLCD_GotoXY(48, 30);
	GLCD_PrintString_P(txt2);
	
	//Draw outline
	GLCD_DrawRoundRectangle(1, 1, 126, 62, 5, GLCD_Black);
	
	//Render screen
	GLCD_Render();
	
	//Loop
	while (1 == 1);

	return 0;
}
