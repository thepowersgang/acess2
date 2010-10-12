/*
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// === CONSTANTS ===
#define	SCAN_SPACE	8192
#define	MAGIC	0x1BADB002

// === TYPES ===
typedef struct {
	uint32_t	Magic;
	uint32_t	Flags;
	uint32_t	Checksum;
} __attribute__((packed)) tMBootImg;

// === PROTOTYPES ===
void	CheckMultiboot(char *file);

// === CODE ===
/**
 */
int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		fprintf(stderr, " <file>	Path of file to validate\n");
		fprintf(stderr, "\n");
	}
	
	CheckMultiboot(argv[1]);
	
	return 0;
}

/**
 */
void CheckMultiboot(char *file)
{
	FILE	*fp = fopen(file, "rb");
	 int	len, ofs;
	char	buf[SCAN_SPACE];
	tMBootImg	*img;
	
	// Error Check
	if(fp == NULL) {
		fprintf(stderr, "Unable to open '%s' for reading\n", file);
		exit(EXIT_FAILURE);
	}
	
	// Get file length
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	// Clip
	if(len > SCAN_SPACE)	len = SCAN_SPACE;
	
	// Read
	fread(buf, len, 1, fp);
	
	// Scan
	for(ofs = 0; ofs < len-sizeof(tMBootImg); ofs += 4)
	{
		img = (void*)&buf[ofs];
		// Check magic value
		if(img->Magic != MAGIC)	continue;
		// Validate checksum
		if(img->Magic + img->Flags + img->Checksum != 0) {
			printf("Checksum fail at 0x%x\n", ofs);
			continue;
		}
		// Check undefined feature flags
		if(img->Flags & 0xFFF8) {
			printf("Header at 0x%x uses undefined features (0x%lx)\n", ofs, img->Flags);
			return ;
		}
		// Print success
		printf("Found Multiboot header at 0x%x\n", ofs);
		return ;
	}
	
	printf("No multiboot header found\n");
}
