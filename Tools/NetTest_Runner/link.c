/*
 */
#include "common.h"
#include "net.h"
#include "link.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>

// === CODE ===
void Link_Send(int IfNum, const void *Src, const void *Dst, uint16_t Proto,
	int BufCount, size_t BufLens[], const void *Bufs[])
{
	size_t	total_len = 6+6+2;
	for( int i = 0; i < BufCount; i ++ )
		total_len += BufLens[i];
	uint8_t	*data = malloc(total_len);
	
	uint8_t	*pos = data;
	memcpy(pos, Dst, 6);	pos += 6;
	memcpy(pos, Src, 6);	pos += 6;
	*(uint16_t*)pos = htons(Proto);	pos += 2;
	
	for( int i = 0; i < BufCount; i ++ )
	{
		memcpy(pos, Bufs[i], BufLens[i]);
		pos += BufLens[i];
	}
	
	Net_Send(IfNum, total_len, data);
	
	free(data);
}

bool Link_Pkt_Check(size_t len, const void *data, size_t *ofs_out,
	const void *Src, const void *Dst, uint16_t Proto)
{
	const uint8_t	*data8 = data;
	TEST_ASSERT_REL(len, >=, 6+6+2);

	if(Src)	TEST_ASSERT( memcmp(data8+0, Src, 6) == 0 );
	if(Dst)	TEST_ASSERT( memcmp(data8+6, Dst, 6) == 0 );
	
	TEST_ASSERT_REL( ntohs(*(uint16_t*)(data8+12)), ==, Proto );

	*ofs_out = 6+6+2;
	return true;
}

