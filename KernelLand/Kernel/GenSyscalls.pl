#!/usr/bin/perl
#
#

open(FILE, "syscalls.lst");

$num = 0;
@calls = ();
while($_ = <FILE>)
{
	if(/(\d+)/)
	{
		$num = $1;
	}
	elsif(/([A-Z_]+)\s+(.+)/)
	{
		push @calls, [$num, $1, $2];
		$num ++;
	}
}

close(FILE);

# C header
open(HEADER, ">include/syscalls.h");
print HEADER "/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * syscalls.h
 * - System Call List
 *
 * NOTE: Generated from Kernel/syscalls.lst
 */
#ifndef _SYSCALLS_H
#define _SYSCALLS_H

";

$lastid = -1;
$i = 0;
foreach my $call (@calls)
{
	print HEADER "#define ", $call->[1], "\t", $call->[0], "\t// ", $call->[2], "\n";
	$i = $call->[0] + 1;
}
print HEADER "
#define NUM_SYSCALLS	",$i,"
#define SYS_DEBUG	0x100
#define SYS_DEBUGHEX	0x101

#if !defined(__ASSEMBLER__) && !defined(NO_SYSCALL_STRS)
static const char *cSYSCALL_NAMES[] = {
";

$lastid = -1;
foreach $call (@calls)
{
	while( $lastid + 1 < $call->[0] )
	{
		print HEADER "\t\"\",\n";
		$lastid = $lastid + 1;
	}
	print HEADER "\t\"", $call->[1], "\",\n";
	$lastid = $lastid + 1;
}
print HEADER  "
\t\"\"
};
#endif

#endif
";

close(HEADER);

# Assembly Header
open(ASM, ">include/syscalls.inc.asm");
print ASM "; Acess2
; System Calls List
; 

";
foreach $call (@calls)
{
	print ASM "%define ", $call->[1], "\t", $call->[0], "\t ;", $call->[2], "\n";
}
close(ASM);
