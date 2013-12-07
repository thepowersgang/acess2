#ifndef _TEST_COMMON_H
#define _TEST_COMMON_H

typedef struct
{
	const char	*Name;
	 int	(*Setup)(void);
	void	(*Run)(void);
	void	(*Teardown)(void);
} tTEST;

extern tTEST	gaTests[];
extern int	giNumTests;

extern void	TestFailure(const char *ReasonFmt, ...) __attribute__((noreturn));

#define TEST_ASSERT(expr) do{ \
	if( !(expr) )	TestFailure("Assertion %s", #expr );\
} while(0)

#define TEST_ASSERT_R(a,r,b) do{ \
	if( !((a) r (b)) )	TestFailure("Assertion %s(%i) %s %s(%i)", #a, (a), #r, #b, (b) );\
} while(0)
#define TEST_ASSERT_SR(a,r,b) do{ \
	if( !(strcmp((a),(b)) r 0) )	TestFailure("Assertion %s(\"%s\") %s %s(\"%s\")", #a, (a), #r, #b, (b) );\
} while(0)

#endif
