//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CMDTypeInt8GPDB.h
//
//	@doc:
//		Class for representing Int8 types in GPDB
//---------------------------------------------------------------------------



#ifndef GPMD_CMDTypeInt8GPDB_H
#define GPMD_CMDTypeInt8GPDB_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/base/IDatumInt4.h"
#include "naucrates/md/CGPDBTypeHelper.h"
#include "naucrates/md/IMDTypeInt8.h"

#define GPDB_INT8_OID OID(20)
#define GPDB_INT8_OPFAMILY OID(1977)
#define GPDB_INT8_LEGACY_OPFAMILY OID(7100)
#define GPDB_INT8_LENGTH 8
#define GPDB_INT8_EQ_OP OID(410)
#define GPDB_INT8_NEQ_OP OID(411)
#define GPDB_INT8_LT_OP OID(412)
#define GPDB_INT8_LEQ_OP OID(414)
#define GPDB_INT8_GT_OP OID(413)
#define GPDB_INT8_GEQ_OP OID(415)
#define GPDB_INT8_COMP_OP OID(351)
#define GPDB_INT8_ARRAY_TYPE OID(1016)

#define GPDB_INT8_AGG_MIN OID(2131)
#define GPDB_INT8_AGG_MAX OID(2115)
#define GPDB_INT8_AGG_AVG OID(2100)
#define GPDB_INT8_AGG_SUM OID(2107)
#define GPDB_INT8_AGG_COUNT OID(2147)

// fwd decl
namespace gpdxl
{
class CXMLSerializer;
}



namespace gpmd
{
using namespace gpos;
using namespace gpnaucrates;

//---------------------------------------------------------------------------
//	@class:
//		CMDTypeInt8GPDB
//
//	@doc:
//		Class for representing Int8 types in GPDB
//
//---------------------------------------------------------------------------
class CMDTypeInt8GPDB : public IMDTypeInt8
{
	friend class CGPDBTypeHelper<CMDTypeInt8GPDB>;

private:
	// memory pool
	CMemoryPool *m_mp;

	// type id
	gpos::owner<IMDId *> m_mdid;
	gpos::owner<IMDId *> m_distr_opfamily;
	gpos::owner<IMDId *> m_legacy_distr_opfamily;

	// mdids of different operators
	gpos::owner<IMDId *> m_mdid_op_eq;
	gpos::owner<IMDId *> m_mdid_op_neq;
	gpos::owner<IMDId *> m_mdid_op_lt;
	gpos::owner<IMDId *> m_mdid_op_leq;
	gpos::owner<IMDId *> m_mdid_op_gt;
	gpos::owner<IMDId *> m_mdid_op_geq;
	gpos::owner<IMDId *> m_mdid_op_cmp;
	gpos::owner<IMDId *> m_mdid_type_array;

	// min aggregate
	gpos::owner<IMDId *> m_mdid_min;

	// max aggregate
	gpos::owner<IMDId *> m_mdid_max;

	// avg aggregate
	gpos::owner<IMDId *> m_mdid_avg;

	// sum aggregate
	gpos::owner<IMDId *> m_mdid_sum;

	// count aggregate
	gpos::owner<IMDId *> m_mdid_count;

	// DXL for object
	const CWStringDynamic *m_dxl_str;

	// type name
	static CWStringConst m_str;
	static CMDName m_mdname;

	// a null datum of this type (used for statistics comparison)
	gpos::owner<IDatum *> m_datum_null;

public:
	CMDTypeInt8GPDB(const CMDTypeInt8GPDB &) = delete;

	// ctor/dtor
	explicit CMDTypeInt8GPDB(CMemoryPool *mp);

	~CMDTypeInt8GPDB() override;

	// factory method for creating Int8 datums
	gpos::owner<IDatumInt8 *> CreateInt8Datum(CMemoryPool *mp, LINT value,
											  BOOL is_null) const override;

	// accessors
	const CWStringDynamic *
	GetStrRepr() const override
	{
		return m_dxl_str;
	}

	// type id
	gpos::pointer<IMDId *> MDId() const override;

	gpos::pointer<IMDId *> GetDistrOpfamilyMdid() const override;

	// type name
	CMDName Mdname() const override;

	// id of specified comparison operator type
	gpos::pointer<IMDId *> GetMdidForCmpType(ECmpType cmp_type) const override;

	// id of specified specified aggregate type
	gpos::pointer<IMDId *> GetMdidForAggType(EAggType agg_type) const override;

	BOOL
	IsRedistributable() const override
	{
		return true;
	}

	BOOL
	IsFixedLength() const override
	{
		return true;
	}

	// is type composite
	BOOL
	IsComposite() const override
	{
		return false;
	}

	ULONG
	Length() const override
	{
		return GPDB_INT8_LENGTH;
	}

	// return the GPDB length
	virtual INT
	GetGPDBLength() const
	{
		return GPDB_INT8_LENGTH;
	}

	BOOL
	IsPassedByValue() const override
	{
		return true;
	}

	gpos::pointer<const IMDId *>
	CmpOpMdid() const override
	{
		return m_mdid_op_cmp;
	}

	// is type hashable
	BOOL
	IsHashable() const override
	{
		return true;
	}

	// is type merge joinable
	BOOL
	IsMergeJoinable() const override
	{
		return true;
	}

	gpos::pointer<IMDId *>
	GetArrayTypeMdid() const override
	{
		return m_mdid_type_array;
	}

	// id of the relation corresponding to a composite type
	IMDId *
	GetBaseRelMdid() const override
	{
		return nullptr;
	}

	// serialize object in DXL format
	void Serialize(gpdxl::CXMLSerializer *xml_serializer) const override;

	// return the null constant for this type
	gpos::pointer<IDatum *>
	DatumNull() const override
	{
		return m_datum_null;
	}

	// transformation method for generating datum from CDXLScalarConstValue
	gpos::owner<IDatum *> GetDatumForDXLConstVal(
		gpos::pointer<const CDXLScalarConstValue *> dxl_op) const override;

	// create typed datum from DXL datum
	gpos::owner<IDatum *> GetDatumForDXLDatum(
		CMemoryPool *mp,
		gpos::pointer<const CDXLDatum *> dxl_datum) const override;

	// generate the DXL datum from IDatum
	gpos::owner<CDXLDatum *> GetDatumVal(CMemoryPool *mp,
										 IDatum *datum) const override;

	// generate the DXL datum representing null value
	gpos::owner<CDXLDatum *> GetDXLDatumNull(CMemoryPool *mp) const override;

	// generate the DXL scalar constant from IDatum
	gpos::owner<CDXLScalarConstValue *> GetDXLOpScConst(
		CMemoryPool *mp, IDatum *datum) const override;

#ifdef GPOS_DEBUG
	// debug print of the type in the provided stream
	void DebugPrint(IOstream &os) const override;
#endif
};
}  // namespace gpmd

#endif	// !GPMD_CMDTypeInt8GPDB_H

// EOF
