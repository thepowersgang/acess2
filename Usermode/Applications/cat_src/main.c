/*
 * Acess2 CAT command
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>

#define	BUF_SIZE	1024

/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	 int	fd;
	 int	num;
	char	buf[BUF_SIZE+1];

	if(argc < 2) {
		printf("Usage: cat <file>\n");
		return -1;
	}

	fd = open(argv[1], OPENFLAG_READ);
	if(fd == -1) {
		printf("Unable to open '%s' for reading\n", argv[1]);
		return -1;
	}

	do {
		num = read(fd, BUF_SIZE, buf);
		buf[num] = '\0';
		printf("%s", buf);
	} while(num == BUF_SIZE);

	close(fd);
	printf("\n");

	return 0;
}
