/*
 */

#include "test_common.h"
#include <string.h>
#include <display.h>

// === GLOBALS ===
void	(*gpTest_VT100_AddText)(size_t Length, const char *UTF8Text);
void	(*gpTest_VT100_SetTitle)(const char *Title);

// === CODE ===
int Test_VT100_Setup(void)
{
	gpTest_VT100_AddText = NULL;
	gpTest_VT100_SetTitle = NULL;
	return 0;
}

int Test_VT100_int_Command(int MaxCalls, const char *Command)
{
	const char *buf = Command;
	 int	len = strlen(Command);
	 int	nCalls = 0;
	while( len > 0 )
	{
		TEST_ASSERT_R(nCalls, <, MaxCalls);
		int rv = Term_HandleVT100(NULL, len, buf);
		if( rv < 0 )
			return buf - Command;
		TEST_ASSERT_R(rv, !=, 0);
		TEST_ASSERT_R(rv, <=, len);
		len -= rv;
		buf += rv;
		nCalls ++;
	}
	
	return buf - Command;
}

void Test_VT100_TitleSet(void)
{
	#define TITLE1	"Testing Title, quite long and complex ;;"
	const char cmd_long_long[] = "\033]0;"TITLE1"\033\\";
	#define TITLE2	"Another completely[]different title--"
	const char cmd_long_st[] = "\033]0;"TITLE2"\x9c";
	#define TITLE3	"12345678b sbuog egu nz vlj=+~`][]\033  "
	const char cmd_long_bel[] = "\033]0;"TITLE3"\007";
	const char *expected_title;

	 int	title_was_set=0, rv;
	
	void _settitle(const char *Title) {
		TEST_ASSERT_SR( Title, ==, expected_title );
		title_was_set = 1;
	}
	gpTest_VT100_SetTitle = _settitle;
	
	expected_title = TITLE1;
	title_was_set = 0;
	rv = Test_VT100_int_Command(10, cmd_long_long);
	TEST_ASSERT_R(rv, ==, sizeof(cmd_long_long)-1);
	TEST_ASSERT(title_was_set);
	
	expected_title = TITLE2;
	title_was_set = 0;
	rv = Test_VT100_int_Command(10, cmd_long_st);
	TEST_ASSERT_R(rv, ==, sizeof(cmd_long_st)-1);
	TEST_ASSERT(title_was_set);
	
	expected_title = TITLE3;
	title_was_set = 0;
	rv = Test_VT100_int_Command(10, cmd_long_bel);
	TEST_ASSERT_R(rv, ==, sizeof(cmd_long_bel)-1);
	TEST_ASSERT(title_was_set);
}

// === TESTS ===
tTEST	gaTests[] = {
	{"VT100 TitleSet", NULL, Test_VT100_TitleSet, NULL}
};
int giNumTests = sizeof(gaTests)/sizeof(gaTests[0]);

// === Hooks ===
void	*gpTermStatePtr;
void *Display_GetTermState(tTerminal *Term) {
	return gpTermStatePtr;
}
void Display_SetTermState(tTerminal *Term, void *State) {
	gpTermStatePtr = State;
}

void Display_AddText(tTerminal *Term, size_t Length, const char *UTF8Text) {
	TEST_ASSERT(gpTest_VT100_AddText);
	gpTest_VT100_AddText(Length, UTF8Text);
}
void Display_Newline(tTerminal *Term, bool bCarriageReturn) {
	//if( gpTest_VT100_Newline )
	//	gpTest_VT100_Newline(Term, bCarriageReturn);
}
void Display_SetScrollArea(tTerminal *Term, int Start, int Count) {
	//if( gpTest_VT100_SetScrollArea )
	//	gpTest_VT100_SetScrollArea(Term, Start, Count);
}
void Display_ScrollDown(tTerminal *Term, int Count) {
}
void Display_SetCursor(tTerminal *Term, int Row, int Col) {
}
void Display_MoveCursor(tTerminal *Term, int RelRow, int RelCol) {
}
void Display_SaveCursor(tTerminal *Term) {
}
void Display_RestoreCursor(tTerminal *Term) {
}
void Display_ClearLine(tTerminal *Term, int Dir) {	// 0: All, 1: Forward, -1: Reverse
}
void Display_ClearLines(tTerminal *Term, int Dir) {	// 0: All, 1: Forward, -1: Reverse
}
void Display_ResetAttributes(tTerminal *Term) {
}
void Display_SetForeground(tTerminal *Term, uint32_t RGB) {
}
void Display_SetBackground(tTerminal *Term, uint32_t RGB) {
}
void Display_Flush(tTerminal *Term) {
}
void Display_ShowAltBuffer(tTerminal *Term, bool AltBufEnabled) {
}
void Display_SetTitle(tTerminal *Term, const char *Title) {
	TEST_ASSERT(gpTest_VT100_SetTitle);
	gpTest_VT100_SetTitle(Title);
}
