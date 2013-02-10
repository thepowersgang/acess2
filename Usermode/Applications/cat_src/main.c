/*
 * Acess2 CAT command
 */
#include <stdlib.h>
#include <stdio.h>

#define	BUF_SIZE	1024

/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	size_t	num;
	char	buf[BUF_SIZE];

	if(argc < 2) {
		printf("Usage: cat <file>\n");
		return -1;
	}

	FILE *fp = fopen(argv[1], "r");
	if(!fp) {
		printf("Unable to open '%s' for reading\n", argv[1]);
		return -1;
	}

	do {
		num = fread(buf, 1, BUF_SIZE, fp);
		if(num <= 0)	break;
		fwrite(buf, 1, num, stdout);
	} while(num == BUF_SIZE);

	fclose(fp);
	printf("\n");

	return 0;
}
