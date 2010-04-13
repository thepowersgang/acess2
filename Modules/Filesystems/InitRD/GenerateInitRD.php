<?php
$lGenDate = date("Y-m-d H:i");
$gOutput = <<<EOF
/*
 * Acess2 InitRD
 * InitRD Data
 * Generated $lGenDate
 */
#include "initrd.h"

EOF;

$lines = file($argv[1]);

$lDepth = 0;
$lTree = array();
$lStack = array( array("",array()) );
foreach($lines as $line)
{
	$line = trim($line);
	if(preg_match('/^Dir\s+"([^"]+)"\s+{$/', $line, $matches))
	{
		$new = array($matches[1], array());
		array_push($lStack, $new);
		$lDepth ++;
		continue;
	}
	if($line == "}")
	{
		$lDepth --;
		$lStack[$lDepth][1][] = array_pop($lStack);
		continue;
	}
	if(preg_match('/^File\s+"([^"]+)"\s+"([^"]+)"$/', $line, $matches))
	{
		$lStack[$lDepth][1][] = array($matches[1], $matches[2]);
		continue;
	}
	echo "ERROR: $line\n";
	exit(0);
}

function hd($fp)
{
	return "0x".str_pad( dechex(ord(fgetc($fp))), 2, "0", STR_PAD_LEFT );
}

function ProcessFolder($prefix, $items)
{
	global	$gOutput;
	foreach($items as $i=>$item)
	{
		if(is_array($item[1]))
		{
			ProcessFolder("{$prefix}_{$i}", $item[1]);
			
			$gOutput .= "tInitRD_File {$prefix}_{$i}_entries[] = {\n";
			foreach($item[1] as $j=>$child)
			{
				if($j)	$gOutput .= ",\n";
				$gOutput .= "\t{\"".addslashes($child[0])."\",&{$prefix}_{$i}_{$j}}";
			}
			$gOutput .= "\n};\n";
			
			$size = count($item[1]);
			$gOutput .= <<<EOF
tVFS_Node {$prefix}_{$i} = {
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = VFS_FFLAG_DIRECTORY,
	.Size = $size,
	.ImplPtr = {$prefix}_{$i}_entries,
	.ReadDir = InitRD_ReadDir,
	.FindDir = InitRD_FindDir
};

EOF;
		}
		else {
			if(!file_exists($item[1])) {
				echo "ERROR: '{$item[1]}' does not exist\n", 
				exit(1);
			}
			$size = filesize($item[1]);
			
			$gOutput .= "Uint8 {$prefix}_{$i}_data[] = {\n";
			$fp = fopen($item[1], "rb");
			for( $j = 0; $j + 16 < $size; $j += 16 )
			{
				$gOutput .= "\t";
				$gOutput .= hd($fp).",".hd($fp).",";
				$gOutput .= hd($fp).",".hd($fp).",";
				$gOutput .= hd($fp).",".hd($fp).",";
				$gOutput .= hd($fp).",".hd($fp).",";
				$gOutput .= hd($fp).",".hd($fp).",";
				$gOutput .= hd($fp).",".hd($fp).",";
				$gOutput .= hd($fp).",".hd($fp).",";
				$gOutput .= hd($fp).",".hd($fp).",\n";
			}
			$gOutput .= "\t";
			for( ; $j < $size; $j ++ )
			{
				if( $j & 15 )	$gOutput .= ",";
				$gOutput .= hd($fp);
			}
			fclose($fp);
			$gOutput .= "\n};\n";
			$gOutput .= <<<EOF
tVFS_Node {$prefix}_{$i} = {
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = 0,
	.Size = $size,
	.ImplPtr = {$prefix}_{$i}_data,
	.Read = InitRD_ReadFile
};

EOF;
		}
	}
}

//print_r($lStack);
//exit(1);

ProcessFolder("gInitRD_Files", $lStack[0][1]);

$gOutput .= "tInitRD_File gInitRD_Root_Files[] = {\n";
foreach($lStack[0][1] as $j=>$child)
{
	if($j)	$gOutput .= ",\n";
	$gOutput .= "\t{\"".addslashes($child[0])."\",&gInitRD_Files_{$j}}";
}
$gOutput .= "\n};\n";
$nRootFiles = count($lStack[0][1]);
$gOutput .= <<<EOF
tVFS_Node gInitRD_RootNode = {
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = VFS_FFLAG_DIRECTORY,
	.Size = $nRootFiles,
	.ImplPtr = gInitRD_Root_Files,
	.ReadDir = InitRD_ReadDir,
	.FindDir = InitRD_FindDir
};
EOF;

$fp = fopen($argv[2], "w");
fputs($fp, $gOutput);
fclose($fp);
?>
