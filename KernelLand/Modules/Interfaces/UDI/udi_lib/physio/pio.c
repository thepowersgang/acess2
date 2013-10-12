/**
 * \file udi_lib/physio/pio.c
 * \author John Hodge (thePowersGang)
 */
#define DEBUG	1
#include <udi.h>
#include <udi_physio.h>
#include <acess.h>
#include <udi_internal.h>

typedef void	_udi_pio_do_io_op_t(uint32_t child_ID, udi_ubit32_t regset_idx, udi_ubit32_t ofs, size_t len,
	void *data, bool isOutput);

// === STRUCTURES ===
struct udi_pio_handle_s
{
	tUDI_DriverInstance	*Inst;
	udi_index_t	RegionIdx;

	_udi_pio_do_io_op_t	*IOFunc;
	udi_ubit32_t	ChildID;
	udi_index_t	RegSet;
	
	udi_ubit32_t	Offset;
	udi_ubit32_t	Length;
	
	size_t	nTransOps;
	udi_pio_trans_t	*TransOps;
	
	udi_ubit16_t	PIOAttributes;
	udi_ubit32_t	Pace;
	udi_index_t	Domain;

	// TODO: Cached labels
//	size_t	Labels[];
};

// === IMPORTS ===
extern _udi_pio_do_io_op_t	pci_pio_do_io;

// === EXPORTS ===
EXPORT(udi_pio_map);
EXPORT(udi_pio_unmap);
EXPORT(udi_pio_atmic_sizes);
EXPORT(udi_pio_abort_sequence);
EXPORT(udi_pio_trans);
EXPORT(udi_pio_probe);

// === CODE ===
void udi_pio_map(udi_pio_map_call_t *callback, udi_cb_t *gcb,
	udi_ubit32_t regset_idx, udi_ubit32_t base_offset, udi_ubit32_t length,
	udi_pio_trans_t *trans_list, udi_ubit16_t list_length,
	udi_ubit16_t pio_attributes, udi_ubit32_t pace, udi_index_t serialization_domain)
{
	LOG("gcb=%p,regset_idx=%i,base_offset=0x%x,length=0x%x,trans_list=%p,list_length=%i,...",
		gcb, regset_idx, base_offset, length, trans_list, list_length);
	char bus_type[16] = {0};
	udi_instance_attr_type_t	type;
	type = udi_instance_attr_get_internal(gcb, "bus_type", 0, bus_type, sizeof(bus_type), NULL);
	if(type != UDI_ATTR_STRING) {
		Log_Warning("UDI", "No/invalid bus_type attribute");
		callback(gcb, UDI_NULL_PIO_HANDLE);
		return ;
	}


	_udi_pio_do_io_op_t	*io_op;
	if( strcmp(bus_type, "pci") == 0 ) {
		// Ask PCI binding
		io_op = pci_pio_do_io;
	}
	else {
		// Oops, unknown
		Log_Warning("UDI", "Unknown bus type %s", bus_type);
		callback(gcb, UDI_NULL_PIO_HANDLE);
		return ;
	}

	udi_pio_handle_t ret = malloc( sizeof(struct udi_pio_handle_s) );
	ret->Inst = UDI_int_ChannelGetInstance(gcb, false, &ret->RegionIdx);
	ret->ChildID = ret->Inst->ParentChildBinding->ChildID;
	ret->RegSet = regset_idx;
	ret->IOFunc = io_op;
	ret->Offset = base_offset;
	ret->Length = length;
	ret->TransOps = trans_list;
	// TODO: Pre-scan to get labels
	ret->nTransOps = list_length;
	ret->PIOAttributes = pio_attributes;
	ret->Pace = pace;
	// TODO: Validate serialization_domain
	ret->Domain = serialization_domain;
	
	callback(gcb, ret);
}

void udi_pio_unmap(udi_pio_handle_t pio_handle)
{
	UNIMPLEMENTED();
}

udi_ubit32_t udi_pio_atmic_sizes(udi_pio_handle_t pio_handle)
{
	UNIMPLEMENTED();
	return 0;
}

void udi_pio_abort_sequence(udi_pio_handle_t pio_handle, udi_size_t scratch_requirement)
{
	UNIMPLEMENTED();
}

size_t _get_label(udi_pio_handle_t pio_handle, udi_index_t label)
{
	udi_pio_trans_t	*ops = pio_handle->TransOps;
	size_t	ip = 0;
	if( label == 0 )
		return 0;
	for( ; ip < pio_handle->nTransOps; ip ++ ) {
		if( ops[ip].pio_op == UDI_PIO_LABEL && ops[ip].operand == label )
			return ip;
	}
	return 0;
}

typedef union
{
	udi_ubit32_t	words[32/4];;
} _trans_value_t;

void _zero_upper(int sizelog2, _trans_value_t *val)
{
	switch(sizelog2)
	{
	case UDI_PIO_1BYTE:
		val->words[0] &= 0x00FF;
	case UDI_PIO_2BYTE:
		val->words[0] &= 0xFFFF;
	case UDI_PIO_4BYTE:
		val->words[1] = 0;
	case UDI_PIO_8BYTE:
		val->words[2] = 0;
		val->words[3] = 0;
	case UDI_PIO_16BYTE:
		val->words[4] = 0;
		val->words[5] = 0;
		val->words[6] = 0;
		val->words[7] = 0;
	case UDI_PIO_32BYTE:
		break;
	}
}

int _compare_zero(_trans_value_t *val, int sizelog2)
{
	 int	is_z=1;
	 int	is_n=0;
	switch(sizelog2)
	{
	case UDI_PIO_32BYTE:
		is_z = is_z && (val->words[7] == 0);
		is_z = is_z && (val->words[6] == 0);
		is_z = is_z && (val->words[5] == 0);
		is_z = is_z && (val->words[4] == 0);
	case UDI_PIO_16BYTE:
		is_z = is_z && (val->words[3] == 0);
		is_z = is_z && (val->words[2] == 0);
	case UDI_PIO_8BYTE:
		is_z = is_z && (val->words[1] == 0);
	case UDI_PIO_4BYTE:
		is_z = is_z && (val->words[0] == 0);
		is_n = (val->words[ (1<<sizelog2)/4-1 ] >> 31) != 0;
		break;
	case UDI_PIO_2BYTE:
		is_z = (val->words[0] & 0xFFFF) == 0;
		is_n = (val->words[0] >> 15) != 0;
		break;
	case UDI_PIO_1BYTE:
		is_z = (val->words[0] & 0xFFFF) == 0;
		is_n = (val->words[0] >> 7) != 0;
		break;
	}
	
	return is_z | (is_n << 1);
}

static inline void _operation_shift_left(int sizelog2, _trans_value_t *dstval, int count)
{
	const int nwords = (1<<sizelog2)/4;
	ASSERTC(count, <=, 32);
	if( sizelog2 <= UDI_PIO_4BYTE ) {
		dstval->words[0] <<= count;
	}
	else if( count == 0 ) {
		// nop
	}
	else if( count == 32 ) {
		for(int i = nwords - 1; i --; )
			dstval->words[i+1] = dstval->words[i];
		dstval->words[0] = 0;
	}
	else {
		for( int i = nwords - 1; i --; )
			dstval->words[i+1] = (dstval->words[i+1] << count)
				| (dstval->words[i] >> (32 - count));
		dstval->words[0] = dstval->words[0] << count;
	}
	_zero_upper(sizelog2, dstval);
}
static inline void _operation_shift_right(int sizelog2, _trans_value_t *dstval, int count)
{
	const int nwords = (1<<sizelog2)/4;
	ASSERTC(count, <=, 32);
	if( sizelog2 <= UDI_PIO_4BYTE ) {
		dstval->words[0] >>= count;
	}
	else if( count == 0 ) {
		// nop
	}
	else if( count == 32 ) {
		for(int i = 0; i < nwords-1; i++ )
			dstval->words[i] = dstval->words[i+1];
		dstval->words[nwords-1] = 0;
	}
	else {
		for(int i = 0; i < nwords-1; i++ )
			dstval->words[i] = (dstval->words[i] >> count)
				| (dstval->words[i+1] << (32 - count));
		dstval->words[nwords-1] = dstval->words[nwords-1] >> count;
	}
	_zero_upper(sizelog2, dstval);
}
static inline void _operation_and(int sizelog2, _trans_value_t *dstval, const _trans_value_t *srcval)
{
	for( int i = 0; i < ((1 <<sizelog2)+3)/4; i ++ )
		dstval->words[i] &= srcval->words[i];
	_zero_upper(sizelog2, dstval);
}
static inline void _operation_or(int sizelog2, _trans_value_t *dstval, const _trans_value_t *srcval)
{
	for( int i = 0; i < ((1 <<sizelog2)+3)/4; i ++ )
		dstval->words[i] |= srcval->words[i];
	_zero_upper(sizelog2, dstval);
}
static inline void _operation_xor(int sizelog2, _trans_value_t *dstval, const _trans_value_t *srcval)
{
	for( int i = 0; i < ((1 <<sizelog2)+3)/4; i ++ )
		dstval->words[i] ^= srcval->words[i];
	_zero_upper(sizelog2, dstval);
}
static inline void _operation_add(int sizelog2, _trans_value_t *dstval, const _trans_value_t *srcval)
{
	 int	c = 0;
	for( int i = 0; i < ((1 <<sizelog2)+3)/4; i ++ ) {
		 int	new_c = (dstval->words[i] + srcval->words[i]) < srcval->words[i];
		dstval->words[i] += srcval->words[i] + c;
		c = new_c;
	}
	_zero_upper(sizelog2, dstval);
}
static inline void _operation_sub(int sizelog2, _trans_value_t *dstval, const _trans_value_t *srcval)
{
	if( sizelog2 <= UDI_PIO_4BYTE ) {
		// Single word doesn't need the borrow logic
		// - including it would cause bugs
		dstval->words[0] -= srcval->words[0];
	}
	else {
		 int	b = 0;
		for( int i = ((1 <<sizelog2)+3)/4; i --; ) {
			 int	new_b = (dstval->words[i] < srcval->words[i] + b);
			dstval->words[i] -= srcval->words[i] + b;
			b = new_b;
		}
	}
	_zero_upper(sizelog2, dstval);
}

#if DEBUG
static const char *caMEM_MODES[] = {"DIRECT","SCRATCH","BUF","MEM"};
#endif
static inline int _read_mem(udi_cb_t *gcb, udi_buf_t *buf, void *mem_ptr,
	udi_ubit8_t op, int sizelog2,
	const _trans_value_t *reg, _trans_value_t *val)
{
	udi_ubit32_t	ofs = reg->words[0];
	udi_size_t	size = 1 << sizelog2;
	switch(op)
	{
	case UDI_PIO_DIRECT:
		memcpy(val, reg, size);
		_zero_upper(sizelog2, val);
		break;
	case UDI_PIO_SCRATCH:
		ASSERTCR( (ofs & (size-1)), ==, 0, 1);
		memcpy(val, gcb->scratch + ofs, size);
		//LOG("scr %p+%i => %i %x,...", gcb->scratch, ofs, size, val->words[0]);
		break;
	case UDI_PIO_BUF:
		udi_buf_read(buf, ofs, size, val);
		//LOG("buf %p+%i => %i %x,...", buf, ofs, size, val->words[0]);
		break;
	case UDI_PIO_MEM:
		ASSERTCR( (ofs & (size-1)), ==, 0, 1 );
		memcpy(val, mem_ptr + ofs, size);
		//LOG("mem %p+%i => %i %x,...", mem_ptr, ofs, size, val->words[0]);
		break;
	}
	return 0;
}

static inline int _write_mem(udi_cb_t *gcb, udi_buf_t *buf, void *mem_ptr,
	udi_ubit8_t op, int sizelog2,
	_trans_value_t *reg, const _trans_value_t *val)
{
	udi_ubit32_t	ofs = reg->words[0];
	udi_size_t	size = 1 << sizelog2;
	switch(op)
	{
	case UDI_PIO_DIRECT:
		memcpy(reg, val, size);
		_zero_upper(sizelog2, reg);
		break;
	case UDI_PIO_SCRATCH:
		ASSERTCR( (ofs & (size-1)), ==, 0, 1);
		LOG("scr %p+%i = %i %x,...", gcb->scratch, ofs, size, val->words[0]);
		memcpy(gcb->scratch + ofs, val, size);
		break;
	case UDI_PIO_BUF:
		LOG("buf %p+%i = %i %x,...", buf, ofs, size, val->words[0]);
		udi_buf_write(NULL,NULL, val, size, buf, ofs, size, UDI_NULL_BUF_PATH);
		break;
	case UDI_PIO_MEM:
		ASSERTCR( (ofs & (size-1)), ==, 0, 1);
		LOG("mem %p+%i = %i %x,...", mem_ptr, ofs, size, val->words[0]);
		memcpy(mem_ptr + ofs, val, size);
		break;
	}
	return 0;
}

void udi_pio_trans(udi_pio_trans_call_t *callback, udi_cb_t *gcb,
	udi_pio_handle_t pio_handle, udi_index_t start_label,
	udi_buf_t *buf, void *mem_ptr)
{
	LOG("pio_handle=%p,start_label=%i,buf=%p,mem_ptr=%p",
		pio_handle, start_label, buf, mem_ptr);
	_trans_value_t	registers[8];
	_trans_value_t	tmpval;

	ASSERT(pio_handle);
	
	udi_ubit16_t	ret_status = 0;
	
	udi_pio_trans_t	*ops = pio_handle->TransOps;
	size_t	ip = _get_label(pio_handle, start_label);
	
	while( ip < pio_handle->nTransOps )
	{
		udi_ubit8_t	pio_op = ops[ip].pio_op;
		udi_ubit8_t	tran_size = ops[ip].tran_size;
		udi_ubit16_t	operand = ops[ip].operand;
		LOG("%2i: %02x %i 0x%04x", ip, pio_op, tran_size, operand);
		ASSERTC(tran_size, <=, UDI_PIO_32BYTE);
		ip ++;
		if( !(pio_op & 0x80) )
		{
			// Class A
			_trans_value_t	*reg = &registers[pio_op&7];
			switch(pio_op & 0x60)
			{
			case UDI_PIO_IN:
				pio_handle->IOFunc(pio_handle->ChildID, pio_handle->RegSet,
					operand, tran_size, &tmpval, false);
				_write_mem(gcb, buf, mem_ptr, (pio_op&0x18), tran_size, reg, &tmpval);
				break;
			case UDI_PIO_OUT:
				_read_mem(gcb, buf, mem_ptr, (pio_op&0x18), tran_size, reg, &tmpval);
				pio_handle->IOFunc(pio_handle->ChildID, pio_handle->RegSet,
					operand, tran_size, &tmpval, true);
				break;
			case UDI_PIO_LOAD:
				_read_mem(gcb, buf, mem_ptr, (pio_op&0x18), tran_size, reg, &registers[operand]);
				_zero_upper(tran_size, &registers[operand]);
				break;
			case UDI_PIO_STORE:
				_write_mem(gcb, buf, mem_ptr, (pio_op&0x18), tran_size, reg, &registers[operand]);
				break;
			}
		}
		else if( pio_op < 0xF0 )
		{
			 int	reg_num = pio_op & 7;
			_trans_value_t	*reg = &registers[reg_num];
			// Class B
			switch(pio_op & 0xF8)
			{
			case UDI_PIO_LOAD_IMM:
				ASSERTC(ip + (1<<tran_size)/2-1, <=, pio_handle->nTransOps);
				if( tran_size == UDI_PIO_1BYTE ) {
					Log_Error("UDI", "udi_pio_trans - %p [%i] LOAD_IMM with 1 byte",
						ops, ip-1);
					goto error;
				}
				switch(tran_size)
				{
				case UDI_PIO_32BYTE:
					reg->words[7] = (ops[ip+14].operand << 16) | ops[ip+13].operand;
					reg->words[6] = (ops[ip+12].operand << 16) | ops[ip+11].operand;
					reg->words[5] = (ops[ip+10].operand << 16) | ops[ip+ 9].operand;
					reg->words[4] = (ops[ip+ 8].operand << 16) | ops[ip+ 7].operand;
				case UDI_PIO_16BYTE:
					reg->words[3] = (ops[ip+ 6].operand << 16) | ops[ip+ 5].operand;
					reg->words[2] = (ops[ip+ 4].operand << 16) | ops[ip+ 3].operand;
				case UDI_PIO_8BYTE:
					reg->words[1] = (ops[ip+ 2].operand << 16) | ops[ip+ 1].operand;
				case UDI_PIO_4BYTE:
					reg->words[0] = (ops[ip+ 0].operand << 16) | operand;
				case UDI_PIO_2BYTE:
				case UDI_PIO_1BYTE:
					reg->words[0] = operand;
					break;
				}
				_zero_upper(tran_size, reg);
				ip += (1<<tran_size)/2-1;
				break;
			case UDI_PIO_CSKIP: {
				int cnd = _compare_zero(reg, tran_size);
				switch(operand)
				{
				case UDI_PIO_NZ:
					LOG("CSKIP NZ R%i", reg_num);
					if( !(cnd & 1) )
						ip ++;
					break;
				case UDI_PIO_Z:
					LOG("CSKIP Z R%i", reg_num);
					if( cnd & 1 )
						ip ++;
					break;
				case UDI_PIO_NNEG:
					LOG("CSKIP NNEG R%i", reg_num);
					if( !(cnd & 2) )
						ip ++;
				case UDI_PIO_NEG:
					LOG("CSKIP NEG R%i", reg_num);
					if( cnd & 2 )
						ip ++;
					break;
				}
				break; }
			case UDI_PIO_IN_IND:
				pio_handle->IOFunc(pio_handle->ChildID, pio_handle->RegSet,
					registers[operand].words[0], tran_size, reg, false);
				_zero_upper(tran_size, reg);
				break;
			case UDI_PIO_OUT_IND:
				pio_handle->IOFunc(pio_handle->ChildID, pio_handle->RegSet,
					registers[operand].words[0], tran_size, reg, true);
				break;
			case UDI_PIO_SHIFT_LEFT:
				_operation_shift_left(tran_size, reg, operand);
				break;
			case UDI_PIO_SHIFT_RIGHT:
				_operation_shift_right(tran_size, reg, operand);
				break;
			case UDI_PIO_AND:
				_operation_and(tran_size, reg, &registers[operand]);
				break;
			case UDI_PIO_AND_IMM:
				tmpval.words[0] = operand;
				_zero_upper(UDI_PIO_2BYTE, &tmpval);
				_operation_and(tran_size, reg, &tmpval);
				break;
			case UDI_PIO_OR:
				_operation_or(tran_size, reg, &registers[operand]);
				break;
			case UDI_PIO_OR_IMM:
				tmpval.words[0] = operand;
				_zero_upper(UDI_PIO_4BYTE, &tmpval);
				_operation_or(tran_size, reg, &tmpval);
				break;
			case UDI_PIO_XOR:
				_operation_xor(tran_size, reg, &registers[operand]);
				break;
			case UDI_PIO_ADD:
				_operation_add(tran_size, reg, &registers[operand]);
				break;
			case UDI_PIO_ADD_IMM:
				tmpval.words[0] = operand;
				if( operand & (1 <<16) ) {
					tmpval.words[0] |= 0xFFFF0000;
					memset(&tmpval.words[1], 0xFF, 4*(8-1));
				}
				else {
					_zero_upper(UDI_PIO_4BYTE, &tmpval);
				}
				_operation_add(tran_size, reg, &tmpval);
				break;
			case UDI_PIO_SUB:
				_operation_sub(tran_size, reg, &registers[operand]);
				break;
			default:
				Log_Error("UDI", "udi_pio_trans - Class B %x unhandled!",
					pio_op & 0xF8);
				goto error;
			}
		}
		else
		{
			// Class C
			switch(pio_op)
			{
			case UDI_PIO_BRANCH:
				ip = _get_label(pio_handle, operand);
				break;
			case UDI_PIO_LABEL:
				// nop
				break;
			case UDI_PIO_REP_IN_IND:
			case UDI_PIO_REP_OUT_IND: {
				bool dir_is_out = (pio_op == UDI_PIO_REP_OUT_IND);
				udi_ubit8_t	mode    = operand & 0x18;
				udi_ubit32_t	cnt     = registers[(operand>>13) & 7].words[0];
				 int	pio_stride      =           (operand>>10) & 3;
				udi_ubit32_t	pio_ofs	= registers[(operand>>7) & 7].words[0];
				 int	mem_stride      =           (operand>>5) & 3;
				_trans_value_t	*mem_reg=&registers[(operand>>0) & 7];
				udi_ubit32_t	saved_mem_reg = mem_reg->words[0];

				if( pio_stride > 0 )
					pio_stride = 1<<(tran_size+pio_stride-1);
				if( mem_stride > 0 )
					mem_stride = 1<<(tran_size+mem_stride-1);
			
				LOG("REP_%s_IND %i IO=%x+%i %s Mem=%x+%i",
					(dir_is_out ? "OUT": "IN"), cnt,
					pio_ofs, pio_stride,
					caMEM_MODES[mode>>3], saved_mem_reg, mem_stride
					);

				while( cnt -- )
				{
					if( dir_is_out )
						_read_mem(gcb,buf,mem_ptr, mode, tran_size,
							mem_reg, &tmpval);
					pio_handle->IOFunc(pio_handle->ChildID, pio_handle->RegSet,
						pio_ofs, tran_size, &tmpval, dir_is_out);
					if( !dir_is_out )
						_write_mem(gcb,buf,mem_ptr, mode, tran_size,
							mem_reg, &tmpval);
					pio_ofs += pio_stride;
					if( mode != UDI_PIO_DIRECT )
						mem_reg->words[0] += mem_stride;
				}
				if( mode != UDI_PIO_DIRECT )
					mem_reg->words[0] = saved_mem_reg;
				
				break; }
			case UDI_PIO_DELAY:
				LOG("DELAY %i", operand);
				Log_Notice("UDI", "udi_pio_trans - TODO: DELAY");
				break;
			case UDI_PIO_BARRIER:
				LOG("BARRIER");
				Log_Notice("UDI", "udi_pio_trans - TODO: BARRIER");
				break;
			case UDI_PIO_SYNC:
				LOG("SYNC");
				Log_Notice("UDI", "udi_pio_trans - TODO: SYNC");
				break;
			case UDI_PIO_SYNC_OUT:
				LOG("SYNC_OUT");
				Log_Notice("UDI", "udi_pio_trans - TODO: SYNC_OUT");
				break;
			case UDI_PIO_DEBUG:
				LOG("DEBUG %x", operand);
				// nop
				break;
			case UDI_PIO_END:
				ASSERTC(operand, <, 8);
				ASSERTC(tran_size, <=, UDI_PIO_2BYTE);
				if( tran_size == UDI_PIO_2BYTE )
					ret_status = registers[operand].words[0] & 0xFFFF;
				else
					ret_status = registers[operand].words[0] & 0xFF;
				goto end;
			case UDI_PIO_END_IMM:
				ASSERTC(tran_size, ==, UDI_PIO_2BYTE);
				ret_status = operand;
				goto end;
			default:
				Log_Error("UDI", "udi_pio_trans - Class C %x unimplemented",
					pio_op);
				goto error;
			}
		}
	}
	
	if( ip == pio_handle->nTransOps ) {
		Log_Notice("UDI", "udi_pio_trans - %p: Overran transaction list",
			ops);
	}
end:
	callback(gcb, NULL, UDI_OK, ret_status);
	return ;
error:
	callback(gcb, NULL, UDI_STAT_HW_PROBLEM, 0);
}

void udi_pio_probe(udi_pio_probe_call_t *callback, udi_cb_t *gcb,
	udi_pio_handle_t pio_handle, void *mem_ptr, udi_ubit32_t pio_offset,
	udi_ubit8_t tran_size, udi_ubit8_t direction)
{
	UNIMPLEMENTED();
}

