/*
 */
#include <stdarg.h>
#include <stdio.h>

#ifdef __WINDOWS__
int DllMain(void)
{
	return 0;
}

#endif

#ifdef __linux__
int main(int argc, char *argv[], char **envp)
{
	return 0;
}
#endif


void Debug(const char *format, ...)
{
	va_list	args;
	printf("Debug: ");
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	printf("\n");
}

void Warning(const char *format, ...)
{
	va_list	args;
	printf("Warning: ");
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	printf("\n");
}

