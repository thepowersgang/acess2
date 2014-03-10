/*
 * AcessNative Kernel
 *
 * syscall_getpath.c
 * - Implementation of the SYS_AN_GETPATH system call
 */

#include <acess.h>
#include <vfs_int.h>

extern char	*getcwd(char *buf, size_t size);

extern tVFS_NodeType	gNativeFS_FileNodeType;
extern tVFS_NodeType	gNativeFS_DirNodeType;

int Syscall_AN_GetPath_Real(char *Dst, size_t DstLen, const char *Path)
{
	tVFS_Node	*node = VFS_ParsePath(Path, NULL, NULL);
	if(!node)	return -1;

	const char *relpath = NULL;

	if( node->Type == &gNativeFS_FileNodeType || node->Type == &gNativeFS_DirNodeType )
	{
		relpath = node->Data;
	}
	else
	{
		relpath = NULL;
	}
	
	size_t	ret;
	if( relpath )
	{
		if( relpath[0] == '/' ) {
			ret = snprintf(Dst, DstLen, "%s", relpath);
		}
		else {
			getcwd(Dst, DstLen);
			ret = strlen(Dst);
			ret += snprintf(Dst+ret, DstLen-ret, "/%s", relpath);
		}
	}
	else
	{
		ret = 0;
	}
	
	_CloseNode(node);
	return ret;
}
