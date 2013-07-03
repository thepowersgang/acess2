/*
 * Acess2 Automounter
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Program core
 */
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <acess/sys.h>
#include <assert.h>

#define	LVMBASE	"/Devices/LVM"
#define AMOUNTROOT	"/Mount/auto"

void	TryMount(const char *Volume, const char *Part);

// === CODE ===
int main(int argc, char *argv[])
{
	mkdir(AMOUNTROOT, 0755);
	// TODO: Handle variants like 'by-label' and 'by-id'
	
	// Iterate '/Devices/LVM/*/*'
	DIR	*dp = opendir(LVMBASE);
	assert(dp);
	struct dirent	*ent;
	while( (ent = readdir(dp)) )
	{
		if( ent->d_name[0] == '.' )
			continue ;
		if( strncmp(ent->d_name, "by-", 3) == 0 )
			continue ;
		
		char	path[sizeof(LVMBASE)+1+strlen(ent->d_name)+1];
		sprintf(path, LVMBASE"/%s", ent->d_name);
		DIR	*ddp = opendir(path);
		assert(ddp);

		// Attempt to mount each sub-volume
		struct dirent	*dent;
		 int	nParts = 0;
		while( (dent = readdir(ddp)) )
		{
			if(dent->d_name[0] == '.')
				continue ;
			if(strcmp(dent->d_name, "ROOT") == 0)
				continue ;

			TryMount(ent->d_name, dent->d_name);

			nParts ++;
		}
		closedir(ddp);
		
		// If only ROOT exists, attempt to mount that as '/Mount/auto/devname'
		if( nParts == 0 )
		{
			TryMount(ent->d_name, NULL);
		}
	}
	
	closedir(dp);
	
	return 0;
}

void TryMount(const char *Volume, const char *Part)
{
	// Create device path
	size_t	devpath_len = sizeof(LVMBASE)+1+strlen(Volume)+1 + (Part ? strlen(Part)+1 : 5);
	char devpath[devpath_len];
	sprintf(devpath, LVMBASE"/%s/%s", Volume, (Part ? Part : "ROOT"));

	// Create mount path
	size_t	mntpath_len = sizeof(AMOUNTROOT)+1+strlen(Volume)+1 + (Part ? strlen(Part)+1 : 0);
	char mntpath[mntpath_len];
	sprintf(mntpath, AMOUNTROOT"/%s", Volume);
	if( Part ) {
		strcat(mntpath, "-");
		strcat(mntpath, Part);
	}
	mkdir(mntpath, 0755);
	
	// Attempt mount
	if( _SysMount(devpath, mntpath, NULL, "") ) {
		fprintf(stderr, "Unable to mount '%s'\n", devpath);
	}
}

