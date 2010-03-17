/*
 * Acess2 InitRD
 * InitRD Data
 * Generated <?php echo date("Y-m-d H:i"),"\n"; ?>
 */
#include "initrd.h"
<?php
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
}

function hd($fp)
{
	return "0x".str_pad( dechex(ord(fgetc($fp))), 2, "0", STR_PAD_LEFT );
}

function ProcessFolder($prefix, $items)
{
	foreach($items as $i=>$item)
	{
		if(is_array($item[1])) {
			
			ProcessFolder("{$prefix}_{$i}", $item[1]);
			
			echo "tInitRD_File {$prefix}_{$i}_entries[] = {\n";
			foreach($item[1] as $j=>$child)
			{
				if($j)	echo ",\n";
				echo "\t{\"".addslashes($child[0])."\",&{$prefix}_{$i}_{$j}}";
			}
			echo "\n};\n";
			
			$size = count($item[1]);
			echo <<<EOF
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
			$size = filesize($item[1]);
			
			echo "Uint8 {$prefix}_{$i}_data[] = {\n";
			$fp = fopen($item[1], "rb");
			for( $j = 0; $j + 16 < $size; $j += 16 )
			{
				echo "\t";
				echo hd($fp),",",hd($fp),",";
				echo hd($fp),",",hd($fp),",";
				echo hd($fp),",",hd($fp),",";
				echo hd($fp),",",hd($fp),",";
				echo hd($fp),",",hd($fp),",";
				echo hd($fp),",",hd($fp),",";
				echo hd($fp),",",hd($fp),",";
				echo hd($fp),",",hd($fp),",\n";
			}
			echo "\t";
			for( ; $j < $size; $j ++ )
			{
				if( $j & 15 )	echo ",";
				echo hd($fp);
			}
			fclose($fp);
			echo "\n};\n";
			echo <<<EOF
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

ProcessFolder("gInitRD_Files", $lStack[0][1]);

echo "tInitRD_File gInitRD_Root_Files[] = {\n";
foreach($lStack[0][1] as $j=>$child)
{
	if($j)	echo ",\n";
	echo "\t{\"".addslashes($child[0])."\",&gInitRD_Files_{$j}}";
}
echo "\n};\n";
?>
tVFS_Node gInitRD_RootNode = {
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = VFS_FFLAG_DIRECTORY,
	.Size = <?php echo count($lStack[0][1]);?>,
	.ImplPtr = gInitRD_Root_Files,
	.ReadDir = InitRD_ReadDir,
	.FindDir = InitRD_FindDir
};
