/*
 */
#ifndef _LIBRESOLV__RESOLV_H_
#define _LIBRESOLV__RESOLV_H_

extern int res_init(void);

extern int res_query(const char *dname, int class, int type, unsigned char *answer, int anslen);

extern int res_search(const char *dname, int class, int type, unsigned char *answer, int anslen);

extern int res_querydomain(const char *name, const char *domain, int class, int type, unsigned char *answer, int anslen);

extern int res_mkquery(int op, const char *dname, int class, int type, char *data, int datalen, struct rrec *newrr, char *buf, int buflen);

extern int res_send(const char *msg, int msglen, char *answer, int anslen);


#endif

