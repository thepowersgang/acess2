/*
 */
#include <stddef.h>
#include <libintl.h>

// === CODE ===
char *gettext(const char *msg)
{
	return dcgettext(NULL, msg, 0);
}
char *dgettext(const char *domain, const char *msg)
{
	return dcgettext(domain, msg, 0);
}
char *dcgettext(const char *domain, const char *msg, int category)
{
	return (char*)msg;
}

char *ngettext(const char *msg, const char *msgp, unsigned long int n)
{
	return dcngettext(NULL, msg, msgp, n, 0);
}
char *dngettext(const char *domain, const char *msg, const char *msgp, unsigned long int n)
{
	return dcngettext(domain, msg, msgp, n, 0);
}
char *dcngettext(const char *domain, const char *msg, const char *msgp, unsigned long int n, int category)
{
	if( n == 1 )
		return (char*)msg;
	else
		return (char*)msgp;
}

