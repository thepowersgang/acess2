
#ifndef _WM_H_
#define _WM_H_

typedef struct sElement
{
	struct sElement	*NextSibling;
	
	short	CachedX;
	short	CachedY;
	short	CachedW;
	short	CachedH;
	
	struct sElement	*FirstChild;
}	tElement;

typedef struct sTab
{
	char	*Name;
	
	tElement	*RootElement;
}	tTab;

typedef struct sApplication
{
	pid_t	PID;
	
	 int	nTabs;
	tTab	*Tabs;
	
	char	Name[];
}	tApplication;

#endif
