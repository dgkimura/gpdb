//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMDTypeGenericGPDB.h
//
//	@doc:
//		Class representing GPDB generic types
//---------------------------------------------------------------------------

#ifndef GPMD_CMDTypeGenericGPDB_H
#define GPMD_CMDTypeGenericGPDB_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/gpdb_types.h"
#include "naucrates/md/CGPDBTypeHelper.h"
#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/IMDTypeGeneric.h"

// some metadata ids for types that don't have their specific header files (yet)
// keep this in sync with Postgres file pg_operator.h
#define GPDB_TEXT_EQ_OP OID(98)
#define GPDB_TEXT_NEQ_OP OID(531)
#define GPDB_TEXT_LT_OP OID(664)
#define GPDB_TEXT_LEQ_OP OID(665)
#define GPDB_TEXT_GT_OP OID(666)
#define GPDB_TEXT_GEQ_OP OID(667)


// fwd decl
namespace gpdxl
{
class CXMLSerializer;
}

namespace gpmd
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CMDTypeGenericGPDB
//
//	@doc:
//		Class representing GPDB generic types
//
//---------------------------------------------------------------------------
class CMDTypeGenericGPDB : public IMDTypeGeneric
{
	friend class CGPDBTypeHelper<CMDTypeGenericGPDB>;

private:
	// memory pool
	CMemoryPool *m_mp;

	// DXL for object
	const CWStringDynamic *m_dxl_str;

	// metadata id
	gpos::owner<IMDId *> m_mdid;

	// type name
	CMDName *m_mdname;

	// can type be redistributed based on non-legacy distr opfamily
	BOOL m_is_redistributable;

	// is this a fixed-length type
	BOOL m_is_fixed_length;

	// FIXME: we seem to only use m_gpdb_length here, why?
#ifdef GPOS_DEBUG
	// type length in number of bytes for fixed-length types, 0 otherwise
	ULONG m_length;
#endif

	// is type passed by value or by reference
	BOOL m_is_passed_by_value;

	gpos::owner<IMDId *> m_distr_opfamily;

	gpos::owner<IMDId *> m_legacy_distr_opfamily;

	// id of equality operator for type
	gpos::owner<IMDId *> m_mdid_op_eq;

	// id of inequality operator for type
	gpos::owner<IMDId *> m_mdid_op_neq;

	// id of less than operator for type
	gpos::owner<IMDId *> m_mdid_op_lt;

	// id of less than equals operator for type
	gpos::owner<IMDId *> m_mdid_op_leq;

	// id of greater than operator for type
	gpos::owner<IMDId *> m_mdid_op_gt;

	// id of greater than equals operator for type
	gpos::owner<IMDId *> m_mdid_op_geq;

	// id of comparison operator for type used in btree lookups
	gpos::owner<IMDId *> m_mdid_op_cmp;

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

	// is type hashable
	BOOL m_is_hashable;

	// is type merge joinable using '='
	BOOL m_is_merge_joinable;

	// is type composite
	BOOL m_is_composite_type;

	// is type text related
	BOOL m_is_text_related;

	// id of the relation corresponding to a composite type
	gpos::owner<IMDId *> m_mdid_base_relation;

	// id of array type for type
	gpos::owner<IMDId *> m_mdid_type_array;

	// GPDB specific length
	INT m_gpdb_length;

	// a null datum of this type (used for statistics comparison)
	gpos::owner<IDatum *> m_datum_null;

public:
	CMDTypeGenericGPDB(const CMDTypeGenericGPDB &) = delete;

	// ctor
	CMDTypeGenericGPDB(
		CMemoryPool *mp, gpos::owner<IMDId *> mdid, CMDName *mdname,
		BOOL is_redistributable, BOOL is_fixed_length, ULONG length,
		BOOL is_passed_by_value, gpos::owner<IMDId *> mdid_distr_opfamily,
		gpos::owner<IMDId *> mdid_legacy_distr_opfamily,
		gpos::owner<IMDId *> mdid_op_eq, gpos::owner<IMDId *> mdid_op_neq,
		gpos::owner<IMDId *> mdid_op_lt, gpos::owner<IMDId *> mdid_op_leq,
		gpos::owner<IMDId *> mdid_op_gt, gpos::owner<IMDId *> mdid_op_geq,
		gpos::owner<IMDId *> mdid_op_cmp, gpos::owner<IMDId *> pmdidMin,
		gpos::owner<IMDId *> pmdidMax, gpos::owner<IMDId *> pmdidAvg,
		gpos::owner<IMDId *> pmdidSum, gpos::owner<IMDId *> pmdidCount,
		BOOL is_hashable, BOOL is_merge_joinable, BOOL is_composite_type,
		BOOL is_text_related, gpos::owner<IMDId *> mdid_base_relation,
		gpos::owner<IMDId *> mdid_type_array, INT gpdb_length);

	// dtor
	~CMDTypeGenericGPDB() override;

	// accessors
	const CWStringDynamic *
	GetStrRepr() const override
	{
		return m_dxl_str;
	}

	gpos::pointer<IMDId *> MDId() const override;

	CMDName Mdname() const override;

	BOOL IsRedistributable() const override;

	BOOL
	IsFixedLength() const override
	{
		return m_is_fixed_length;
	}

	// is type composite
	BOOL
	IsComposite() const override
	{
		return m_is_composite_type;
	}

	ULONG
	Length() const override
	{
		return m_gpdb_length;
	}

	BOOL
	IsPassedByValue() const override
	{
		return m_is_passed_by_value;
	}

	// id of specified comparison operator type
	gpos::pointer<IMDId *> GetMdidForCmpType(ECmpType ecmpt) const override;

	// id of specified specified aggregate type
	gpos::pointer<IMDId *> GetMdidForAggType(EAggType agg_type) const override;

	gpos::pointer<const IMDId *>
	CmpOpMdid() const override
	{
		return m_mdid_op_cmp;
	}

	// is type hashable
	BOOL
	IsHashable() const override
	{
		return m_is_hashable;
	}

	BOOL
	IsTextRelated() const override
	{
		return m_is_text_related;
	}

	// is type merge joinable on '='
	BOOL
	IsMergeJoinable() const override
	{
		return m_is_merge_joinable;
	}

	// id of the relation corresponding to a composite type
	gpos::pointer<IMDId *>
	GetBaseRelMdid() const override
	{
		return m_mdid_base_relation;
	}

	gpos::pointer<IMDId *>
	GetArrayTypeMdid() const override
	{
		return m_mdid_type_array;
	}

	gpos::pointer<IMDId *> GetDistrOpfamilyMdid() const override;

	// serialize object in DXL format
	void Serialize(gpdxl::CXMLSerializer *xml_serializer) const override;

	// factory method for generating generic datum from CDXLScalarConstValue
	gpos::owner<IDatum *> GetDatumForDXLConstVal(
		gpos::pointer<const CDXLScalarConstValue *> dxl_op) const override;

	// create typed datum from DXL datum
	gpos::owner<IDatum *> GetDatumForDXLDatum(
		CMemoryPool *mp,
		gpos::pointer<const CDXLDatum *> dxl_datum) const override;

	// return the GPDB length
	virtual INT
	GetGPDBLength() const
	{
		return m_gpdb_length;
	}

	// return the null constant for this type
	gpos::pointer<IDatum *>
	DatumNull() const override
	{
		return m_datum_null;
	}

	// generate the DXL datum from IDatum
	gpos::owner<CDXLDatum *> GetDatumVal(
		CMemoryPool *mp, gpos::pointer<IDatum *> datum) const override;

	// generate the DXL datum representing null value
	gpos::owner<CDXLDatum *> GetDXLDatumNull(CMemoryPool *mp) const override;

	// generate the DXL scalar constant from IDatum
	gpos::owner<CDXLScalarConstValue *> GetDXLOpScConst(
		CMemoryPool *mp, gpos::pointer<IDatum *> datum) const override;

#ifdef GPOS_DEBUG
	// debug print of the type in the provided stream
	void DebugPrint(IOstream &os) const override;
#endif

	// is type an ambiguous one? e.g., AnyElement in GPDB
	BOOL IsAmbiguous() const override;

	// create a dxl datum
	static gpos::owner<CDXLDatum *> CreateDXLDatumVal(
		CMemoryPool *mp, gpos::owner<IMDId *> mdid,
		gpos::pointer<const IMDType *> md_type, INT type_modifier, BOOL is_null,
		BYTE *byte_array, ULONG length, LINT lint_Value, CDouble double_Value);

	// create a dxl datum of types having double mapping
	static gpos::owner<CDXLDatum *> CreateDXLDatumStatsDoubleMappable(
		CMemoryPool *mp, IMDId *mdid, INT type_modifier, BOOL is_null,
		BYTE *byte_array, ULONG length, LINT lint_Value, CDouble double_Value);

	// create a dxl datum of types having lint mapping
	static gpos::owner<CDXLDatum *> CreateDXLDatumStatsIntMappable(
		CMemoryPool *mp, IMDId *mdid, INT type_modifier, BOOL is_null,
		BYTE *byte_array, ULONG length, LINT lint_Value, CDouble double_Value);

	// create a NULL constant for this type
	gpos::owner<IDatum *> CreateGenericNullDatum(
		CMemoryPool *mp, INT type_modifier) const override;

	// does a datum of this type need bytea to Lint mapping for statistics computation
	static BOOL HasByte2IntMapping(gpos::pointer<const IMDType *> mdtype);

	// does a datum of this type need bytea to double mapping for statistics computation
	static BOOL HasByte2DoubleMapping(gpos::pointer<const IMDId *> mdid);

	// is this a time-related type
	static BOOL IsTimeRelatedType(gpos::pointer<const IMDId *> mdid);

	// is this a time-related type mappable to DOUBLE
	static BOOL
	IsTimeRelatedTypeMappableToDouble(gpos::pointer<const IMDId *> mdid)
	{
		return IsTimeRelatedType(mdid) &&
			   !IsTimeRelatedTypeMappableToLint(mdid);
	}

	// is this a time-related type mappable to LINT
	static inline BOOL
	IsTimeRelatedTypeMappableToLint(gpos::pointer<const IMDId *> mdid)
	{
		return mdid->Equals(&CMDIdGPDB::m_mdid_date);
	}

	// is this a network-related type
	static BOOL IsNetworkRelatedType(gpos::pointer<const IMDId *> mdid);
};
}  // namespace gpmd

#endif	// !GPMD_CMDTypeGenericGPDB_H

// EOF
