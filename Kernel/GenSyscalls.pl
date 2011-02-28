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
 * Acess2
 * syscalls.h
 * - System Call List
 *
 * NOTE: Generated from Kernel/syscalls.lst
 */
#ifndef _SYSCALLS_H
#define _SYSCALLS_H

enum eSyscalls {
";

$lastid = -1;
foreach my $call (@calls)
{
	if( $lastid + 1 != $call->[0] ) {
		print HEADER "\n";
	}
	print HEADER "\t", $call->[1];
	if( $lastid + 1 != $call->[0] ) {
		print HEADER " = ", $call->[0];
	}
	print HEADER ",\t// ", $call->[2], "\n";
	$lastid = $call->[0];
}
print HEADER "
\tNUM_SYSCALLS,
\tSYS_DEBUG = 0x100
};

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
