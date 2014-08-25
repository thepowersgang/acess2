/*
 */
#ifndef _LIBINTL_H_
#define _LIBINTL_H_

extern char	*gettext(const char *msg);
extern char	*dgettext(const char *domain, const char *msg);
extern char	*dcgettext(const char *domain, const char *msg, int category);
extern char	*ngettext(const char *msg, const char *msgp, unsigned long int n);
extern char	*dngettext(const char *domain, const char *msg, const char *msgp, unsigned long int n);
extern char	*dcngettext(const char *domain, const char *msg, const char *msgp, unsigned long int n, int category);

#endif
