/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * udi_ma.h
 * - Management Agent
 */
#ifndef _UDI_MA_H_
#define _UDI_MA_H_

extern void	UDI_MA_BindParents(tUDI_DriverModule *Module);
extern tUDI_DriverInstance	*UDI_MA_CreateInstance(tUDI_DriverModule *DriverModule);
extern tUDI_DriverRegion	*UDI_MA_InitRegion(tUDI_DriverInstance *Inst, udi_ubit16_t Index, udi_ubit16_t Type, size_t RDataSize);
extern void	UDI_MA_BeginEnumeration(tUDI_DriverInstance *Inst);

extern void	UDI_MA_AddChild(udi_enumerate_cb_t *cb, udi_index_t ops_idx);


extern const udi_cb_init_t cUDI_MgmtCbInitList[];

extern tUDI_DriverModule	*gpUDI_LoadedModules;


#endif

