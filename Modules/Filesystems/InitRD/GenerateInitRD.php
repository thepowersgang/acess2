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

$ACESSDIR = getenv("ACESSDIR");
$ARCH = getenv("ARCH");

$gInputFile = $argv[1];
$gOutputFile = $argv[2];
$gDepFile = ($argc > 3 ? $argv[3] : false);

$gDependencies = array();

$lines = file($argv[1]);

$lDepth = 0;
$lTree = array();
$lStack = array( array("",array()) );
foreach($lines as $line)
{
	$line = trim($line);
	// Directory
	if(preg_match('/^Dir\s+"([^"]+)"\s+{$/', $line, $matches))
	{
		$new = array($matches[1], array());
		array_push($lStack, $new);
		$lDepth ++;
		continue;
	}
	// End of a block
	if($line == "}")
	{
		$lDepth --;
		$lStack[$lDepth][1][] = array_pop($lStack);
		continue;
	}
	// File
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
	//return "0x".str_pad( dechex(ord(fgetc($fp))), 8, "0", STR_PAD_LEFT );
	$val = unpack("I", fread($fp, 4));
	//print_r($val);	exit -1;
	return "0x".dechex($val[1]);
}

function hd8($fp)
{
	return "0x".str_pad( dechex(ord(fgetc($fp))), 2, "0", STR_PAD_LEFT );
}

$inode = 0;
function ProcessFolder($prefix, $items)
{
	global	$gOutput, $gDependencies;
	global	$ACESSDIR, $ARCH;
	global	$inode;
	foreach($items as $i=>$item)
	{
		$inode ++;
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
	.Inode = {$inode},
	.ImplPtr = {$prefix}_{$i}_entries,
	.ReadDir = InitRD_ReadDir,
	.FindDir = InitRD_FindDir
};

EOF;
		}
		else
		{
			$path = $item[1];
			
			// Parse path components
			$path = str_replace("__BIN__", "$ACESSDIR/Usermode/Output/$ARCH", $path);
			$path = str_replace("__FS__", "$ACESSDIR/Usermode/Filesystem", $path);
			echo $path,"\n";
			// ---
			
			$gDependencies[] = $path;

			if(!file_exists($path)) {
				echo "ERROR: '{$path}' does not exist\n", 
				exit(1);
			}
			$size = filesize($path);
			
			$fp = fopen($path, "rb");
			
			$gOutput .= "Uint8 {$prefix}_{$i}_data[] = {\n";
			for( $j = 0; $j + 16 < $size; $j += 16 ) {
				$gOutput .= "\t";
				$gOutput .= hd8($fp).",".hd8($fp).",";
				$gOutput .= hd8($fp).",".hd8($fp).",";
				$gOutput .= hd8($fp).",".hd8($fp).",";
				$gOutput .= hd8($fp).",".hd8($fp).",";
				$gOutput .= hd8($fp).",".hd8($fp).",";
				$gOutput .= hd8($fp).",".hd8($fp).",";
				$gOutput .= hd8($fp).",".hd8($fp).",";
				$gOutput .= hd8($fp).",".hd8($fp).",\n";
			}
			$gOutput .= "\t";
			for( ; $j < $size; $j ++ ) {
				if( $j & 15 )	$gOutput .= ",";
				$gOutput .= hd8($fp);
			}
			fclose($fp);
			$gOutput .= "\n};\n";
			$gOutput .= <<<EOF
tVFS_Node {$prefix}_{$i} = {
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = 0,
	.Size = $size,
	.Inode = {$inode},
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

$gOutput .= <<<EOF

tVFS_Node * const gInitRD_FileList[] = {
&gInitRD_RootNode
EOF;

function PutNodePointers($prefix, $items)
{
	global $gOutput;
	foreach($items as $i=>$item)
	{
		$gOutput .= ",&{$prefix}_{$i}";
		if(is_array($item[1]))
		{
			PutNodePointers("{$prefix}_{$i}", $item[1]);
		}
	}
}

PutNodePointers("gInitRD_Files", $lStack[0][1]);

$gOutput .= <<<EOF
};
const int giInitRD_NumFiles = sizeof(gInitRD_FileList)/sizeof(gInitRD_FileList[0]);

EOF;


$fp = fopen($gOutputFile, "w");
fputs($fp, $gOutput);
fclose($fp);


if($gDepFile !== false)
{
	$fp = fopen($gDepFile, "w");
	$line = $gOutputFile.":\t".implode(" ", $gDependencies);
	fputs($fp, $line);
	fclose($fp);
}

?>
