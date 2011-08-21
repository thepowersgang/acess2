/*
 * Write test
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// === CONSTANTS ===
int main(int argc, char *argv[])
{
	FILE	*fp;
	char	*file;
	char	*text;
	
	// Check arguments
	if(argc < 3) {
		fprintf(stderr, "Usage: %s <file> \"<text>\"\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	//file = argv[1];
	//text = argv[2];
	file = "/Mount/writetest.txt";
	text = "Written!\n";
	
	printf("Testing File Output...\n");
	printf("file = '%s'\n", file);
	printf("text = \"%s\"\n", text);
	
	// Open file
	fp = fopen(file, "w");
	if(!fp) {
		fprintf(stderr, "Unable to open '%s' for writing\n", file);
		return EXIT_FAILURE;
	}
	
	//printf("fwrite('%s', %i, 1, %p)\n", argv[2], strlen(argv[2]), fp);
	if( fwrite(text, strlen(text), 1, fp) == -1 )
	{
		fprintf(stderr, "Unable to write to '%s'\n", file);
		return EXIT_FAILURE;
	}
	fclose(fp);
	
	printf("Wow, It worked!\n");
	
	return EXIT_SUCCESS;
}
