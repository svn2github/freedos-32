/*
// Program:  Format
// Written By:  Brian E. Reifsnyder
// Module:  Save File System header file
// Version:  0.91r
// 0.91l Confirm_... changed, 0.91n more help, 0.91r Ask_Label (-ea 2004)
// Copyright:  1999 - 2004 under the terms of the GNU GPL, Version 2
*/


#ifdef USERINT
	#define UIEXTERN /* */
#else
	#define UIEXTERN extern
#endif

UIEXTERN void ASCII_CD_Number(unsigned long number);
UIEXTERN void Ask_User_To_Insert_Disk(void);
UIEXTERN int  Ask_Label(char * str);	/* 0.91r */
UIEXTERN void Confirm_Hard_Drive_Formatting(int format);
UIEXTERN void Critical_Error_Handler(int source, unsigned int error_code);
UIEXTERN void Display_Drive_Statistics(void);
UIEXTERN void Display_Invalid_Combination(void);
UIEXTERN void Display_Help_Screen(int); /* int argument new in 0.91n */
UIEXTERN void Display_Percentage_Formatted(unsigned long percentage);
UIEXTERN void IllegalArg(char *option, char *argptr);
