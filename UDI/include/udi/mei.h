/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * include/udi/mei.h
 * - Metalanguage-to-Environment Interface
 */
#ifndef _UDI__MEI_H_
#define _UDI__MEI_H_

typedef const struct udi_mei_init_s	udi_mei_init_t;
typedef const struct udi_mei_ops_vec_template_s	udi_mei_ops_vec_template_t;
typedef const struct uid_mei_op_template_s	uid_mei_op_template_t;

typedef udi_ubit8_t udi_mei_enumeration_rank_func_t(udi_ubit32_t attr_device_match, void **attr_value_list);
typedef void udi_mei_direct_stub_t(udi_op_t *op, udi_cb_t *gcb, va_list arglist);
typedef void udi_mei_backend_stub_t(udi_op_t *op, udi_cb_t *gcb, void *marshal_space);

struct udi_mei_init_s
{
	udi_mei_ops_vec_template_t	*ops_vec_template_list;
	udi_mei_enumeration_rank_func_t	*mei_enumeration_rank;
};

struct udi_mei_ops_vec_template_s
{
	udi_index_t	meta_ops_num;
	udi_ubit8_t	relationship;
	const udi_mei_op_template_t	*op_template_list;
};
// Flags for `relationship`
#define UDI_MEI_REL_INITIATOR	(1U<<0)
#define UDI_MEI_REL_BIND	(1U<<1)
#define UDI_MEI_REL_EXTERNAL	(1U<<2)
#define UDI_MEI_REL_INTERNAL	(1U<<3)
#define UDI_MEI_REL_SINGLE	(1U<<4)

struct uid_mei_op_template_s
{
	const char	*op_name;
	udi_ubit8_t	op_category;
	udi_ubit8_t	op_flags;
	udi_index_t	meta_cb_num;
	udi_index_t	completion_ops_num;
	udi_index_t	completion_vec_idx;
	udi_index_t	exception_ops_num;
	udi_index_t	exception_vec_idx;
	udi_mei_direct_stub_t	*direct_stub;
	udi_mei_backend_stub_t	*backend_stub;
	udi_layout_t	*visible_layout;
	udi_layout_t	*marshal_layout;
};
/* Values for op_category */
#define UDI_MEI_OPCAT_REQ	1
#define UDI_MEI_OPCAT_ACK	2
#define UDI_MEI_OPCAT_NAK	3
#define UDI_MEI_OPCAT_IND	4
#define UDI_MEI_OPCAT_RES	5
#define UDI_MEI_OPCAT_RDY	6
/* Values for op_flags */
#define UDI_MEI_OP_ABORTABLE	(1U<<0)
#define UDI_MEI_OP_RECOVERABLE	(1U<<1)
#define UDI_MEI_OP_STATE_CHANGE	(1U<<2)
/* Maximum Sizes For Control Block Layouts */
#define UDI_MEI_MAX_VISIBLE_SIZE	2000
#define UDI_MEI_MAX_MARSHAL_SIZE	4000

#define _UDI_MEI_FIRST(lst, ...)	lst
#define _UDI_MEI_OTHER(lst, ...)	__VA_ARGS__
#define _UDI_MEI_VARG(type,name,vatype) \
	type name = UDI_VA_ARG(arglist, type, vatype);
#define _UDI_MEI_VARGS0()
#define _UDI_MEI_VARGS1(args,argt,argva) \
	_UDI_MEI_VARG(_UDI_MEI_FIRST(argt), _UDI_MEI_FIRST(args),_UDI_MEI_FIRST(argva))
#define _UDI_MEI_VARGS2(args,argt,argva) \
	_UDI_MEI_VARG(_UDI_MEI_FIRST(argt), _UDI_MEI_FIRST(args),_UDI_MEI_FIRST(argva)) \
	_UDI_MEI_VARGS1( _UDI_MEI_OTHER(argt), _UDI_MEI_OTHER(args), _UDI_MEI_OTHER(argva) )
	
#define UDI_MEI_STUBS(op_name, cb_type, argc, args, arg_types, arg_va_list, meta_ops_num, vec_idx) \
	void op_name(cb_type *cb, _UDI_ARG_LIST_##argc args ) {\
		udi_mei_call(UDI_GCB(cb), &udi_mei_info, meta_ops_num, vec_idx, args);\
	}\
	void op_name##_direct(udi_op_t *op, udi_cb_t *gcb, va_lis arglist) {\
		_UDI_MEI_VARGS##argc(args ,## arg_types ,## arg_va_list)\
		(*(op_name##_op_t)op)(UDI_MCB(gcb, cb_type) ,## args);\
	}\
	void op_name##_backend(udi_op_t *op, udi_cb_t *gcb, void *marshal_space) {\
	}

#endif

