/*
 */

int res_init(void)
{
	return 1;
}

int res_query(const char *dname, int class, int type, unsigned char *answer, int anslen)
{
	return 1;
}

int res_search(const char *dname, int class, int type, unsigned char *answer, int anslen)
{
	return 1;
}

int res_querydomain(const char *name, const char *domain, int class, int type, unsigned char *answer, int anslen)
{
	return 1;
}

int res_mkquery(int op, const char *dname, int class, int type, char *data, int datalen, struct rrec *newrr, char *buf, int buflen)
{
	return 1;
}

int res_send(const char *msg, int msglen, char *answer, int anslen)
{
	return 1;
}

