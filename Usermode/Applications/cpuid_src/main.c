/*
 * Acess2 - CPUID Parser
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
void	cpuid(uint32_t Num, uint32_t *EAX, uint32_t *EBX, uint32_t *EDX, uint32_t *ECX);

// === CODE ===
int main(int argc, char *argv[])
{
	char	sVendorId[13];
	 int	iMaxBasic = 0;
	uint32_t	eax, ebx, edx, ecx;
	
	// -- Get Vendor ID and maximum ID
	cpuid(0,
		(uint32_t*)&iMaxBasic, (uint32_t*)&sVendorId[0],
		(uint32_t*)&sVendorId[4], (uint32_t*)&sVendorId[8]);
	sVendorId[12] = '\0';
	printf("Vendor:        %s\n", sVendorId);
	printf("Maximum CPUID: %i\n", iMaxBasic);
	
	// -- Get Processor Information
	if( iMaxBasic == 0 )	return 0;
	/*
# 3:0 - Stepping
# 7:4 - Model
# 11:8 - Family
# 13:12 - Processor Type
# 19:16 - Extended Model
# 27:20 - Extended Famil
	*/
	cpuid(1, &eax, &ebx, &edx, &ecx);
	printf("Model:         Family %i, Model %i, Stepping %i\n", (eax>>8)&7, (eax>>4)&7, eax&7);
	printf("Type:          %i\n", (eax>>12)&7);
	printf("EDX Features:  ");
	if(edx & 1 << 0)	printf("FPU ");
	if(edx & 1 << 1)	printf("VME ");
	if(edx & 1 << 2)	printf("DE ");
	if(edx & 1 << 3)	printf("PSE ");
	if(edx & 1 << 4)	printf("TSC ");
	if(edx & 1 << 5)	printf("MSR ");
	if(edx & 1 << 6)	printf("PAE ");
	if(edx & 1 << 7)	printf("MCE ");
	if(edx & 1 << 8)	printf("CX8 ");
	if(edx & 1 << 9)	printf("APIC ");
	// -- 1 << 10
	if(edx & 1 << 11)	printf("SEP ");
	if(edx & 1 << 12)	printf("MTRR ");
	if(edx & 1 << 13)	printf("PGE ");
	if(edx & 1 << 14)	printf("MCA ");
	if(edx & 1 << 15)	printf("CMOV ");
	if(edx & 1 << 16)	printf("PAT ");
	if(edx & 1 << 17)	printf("PSE36 ");
	if(edx & 1 << 18)	printf("PSN ");
	if(edx & 1 << 19)	printf("CLF ");
	// -- 1 << 20
	if(edx & 1 << 21)	printf("DTES ");
	if(edx & 1 << 22)	printf("ACPI ");
	if(edx & 1 << 23)	printf("MMX ");
	if(edx & 1 << 25)	printf("SSE ");
	if(edx & 1 << 26)	printf("SSE2 ");
	if(edx & 1 << 27)	printf("SS ");
	if(edx & 1 << 28)	printf("HTT ");
	if(edx & 1 << 29)	printf("TM1 ");
	if(edx & 1 << 30)	printf("IA64 ");
	if(edx & 1 << 31)	printf("PBE ");
	printf("\n");

	printf("ECX Features:  ");
	if(ecx & 1 << 0)	printf("SSE3 ");
	if(ecx & 1 << 1)	printf("PCLMUL ");
	if(ecx & 1 << 4)	printf("DTES64 ");
	if(ecx & 1 << 5)	printf("VMX ");
	if(ecx & 1 << 6)	printf("SMX ");
	if(ecx & 1 << 7)	printf("EST ");
	if(ecx & 1 << 8)	printf("TM2 ");
	if(ecx & 1 << 9)	printf("SSSE3 ");
	if(ecx & 1 << 10)	printf("CID ");
	// -- 1 << 11
	if(ecx & 1 << 12)	printf("FMA ");
	if(ecx & 1 << 13)	printf("CX16 ");
	if(ecx & 1 << 14)	printf("ETPRD ");
	if(ecx & 1 << 15)	printf("PDCM ");
	// -- 1 << 16
	// -- 1 << 17
	if(ecx & 1 << 18)	printf("DCA ");
	if(ecx & 1 << 19)	printf("SSE4.1");
	if(ecx & 1 << 20)	printf("SSE4.2");
	if(ecx & 1 << 21)	printf("x2APIC ");
	if(ecx & 1 << 22)	printf("MOVBE ");
	if(ecx & 1 << 23)	printf("POPCNT ");
	// -- 1 << 24
	// -- 1 << 25
	if(ecx & 1 << 26)	printf("XSAVE ");
	if(ecx & 1 << 27)	printf("OSXSAVE ");
	if(ecx & 1 << 28)	printf("AVX ");
	printf("\n");
	
	return 0;
}

/**
 * \brief Call the CPUID opcode
 */
void cpuid(uint32_t Num, uint32_t *EAX, uint32_t *EBX, uint32_t *EDX, uint32_t *ECX)
{
	uint32_t	eax, ebx, edx, ecx;
	
	__asm__ __volatile__ (
		"cpuid"
		: "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
		: "a"(Num)
		);
	
	if(EAX)	*EAX = eax;
	if(EBX)	*EBX = ebx;
	if(EDX)	*EDX = edx;
	if(ECX)	*ECX = ecx;
}
