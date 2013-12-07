/*
 */
#include "test_common.h"
#include <stdio.h>
#include <setjmp.h>
#include <stdarg.h>

// === GLOBALS ===
int	gbDebugEnabled = 0;
jmp_buf	gTest_FailJump;

// === CODE ===
int main(int argc, char *argv[])
{
	 int	nFail = 0;
	for( int i = 0; i < giNumTests; i ++ )
	{
		tTEST	*test = &gaTests[i];
		fprintf(stderr, "Test [%i] \"%s\": ", i, test->Name);
		if( test->Setup && test->Setup() != 0 ) {
			fprintf(stderr, "ERROR\n");
		}
		else {
			int res = setjmp(gTest_FailJump);
			if( res == 0 )
				test->Run();
			
			if(test->Teardown)
				test->Teardown();
			if( res == 0 )
				fprintf(stderr, "PASS\n");
			else
				nFail ++;
		}
	}
	return nFail;
}

void TestFailure(const char *ReasonFmt, ...)
{
	va_list	args;
	va_start(args, ReasonFmt);
	fprintf(stderr, "FAILURE: ");
	vfprintf(stderr, ReasonFmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	longjmp(gTest_FailJump, 1);
}

void _SysDebug(const char *Format, ...)
{
	if( gbDebugEnabled )
	{
		va_list	args;
		va_start(args, Format);
		printf("_SysDebug: ");
		vprintf(Format, args);
		printf("\n");
		va_end(args);
	}
}
