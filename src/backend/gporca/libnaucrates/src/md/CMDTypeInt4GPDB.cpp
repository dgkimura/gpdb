//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMDTypeInt4GPDB.cpp
//
//	@doc:
//		Implementation of the class for representing GPDB-specific int4 type in the
//		MD cache
//---------------------------------------------------------------------------

#include "naucrates/md/CMDTypeInt4GPDB.h"

#include "gpos/common/owner.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/base/CDatumInt4GPDB.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLDatum.h"
#include "naucrates/dxl/operators/CDXLDatumInt4.h"
#include "naucrates/dxl/operators/CDXLScalarConstValue.h"

using namespace gpdxl;
using namespace gpmd;
using namespace gpnaucrates;

// static member initialization
CWStringConst CMDTypeInt4GPDB::m_str = CWStringConst(GPOS_WSZ_LIT("int4"));
CMDName CMDTypeInt4GPDB::m_mdname(&m_str);

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt4GPDB::CMDTypeInt4GPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDTypeInt4GPDB::CMDTypeInt4GPDB(CMemoryPool *mp) : m_mp(mp)
{
	m_mdid = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_OID);
	if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution))
	{
		m_distr_opfamily = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_OPFAMILY);
		m_legacy_distr_opfamily =
			GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_LEGACY_OPFAMILY);
	}
	else
	{
		m_distr_opfamily = nullptr;
		m_legacy_distr_opfamily = nullptr;
	}
	m_mdid_op_eq = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_EQ_OP);
	m_mdid_op_neq = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_NEQ_OP);
	m_mdid_op_lt = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_LT_OP);
	m_mdid_op_leq = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_LEQ_OP);
	m_mdid_op_gt = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_GT_OP);
	m_mdid_op_geq = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_GEQ_OP);
	m_mdid_op_cmp = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_COMP_OP);
	m_mdid_type_array = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_ARRAY_TYPE);
	m_mdid_min = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_AGG_MIN);
	m_mdid_max = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_AGG_MAX);
	m_mdid_avg = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_AGG_AVG);
	m_mdid_sum = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_AGG_SUM);
	m_mdid_count = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_AGG_COUNT);

	m_dxl_str = CDXLUtils::SerializeMDObj(
		m_mp, this, false /*fSerializeHeader*/, false /*indentation*/);

	GPOS_ASSERT(GPDB_INT4_OID == gpos::dyn_cast<CMDIdGPDB>(m_mdid)->Oid());
	;
	m_datum_null =
		GPOS_NEW(mp) CDatumInt4GPDB(m_mdid, 1 /* value */, true /* is_null */);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt4GPDB::~CMDTypeInt4GPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDTypeInt4GPDB::~CMDTypeInt4GPDB()
{
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;

	GPOS_DELETE(m_dxl_str);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt4GPDB::GetDatum
//
//	@doc:
//		Factory function for creating INT4 datums
//
//---------------------------------------------------------------------------
gpos::Ref<IDatumInt4>
CMDTypeInt4GPDB::CreateInt4Datum(CMemoryPool *mp, INT value, BOOL is_null) const
{
	return GPOS_NEW(mp) CDatumInt4GPDB(m_mdid->Sysid(), value, is_null);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt4GPDB::MDId
//
//	@doc:
//		Returns the metadata id of this type
//
//---------------------------------------------------------------------------
IMDId *
CMDTypeInt4GPDB::MDId() const
{
	return m_mdid.get();
}

IMDId *
CMDTypeInt4GPDB::GetDistrOpfamilyMdid() const
{
	if (GPOS_FTRACE(EopttraceUseLegacyOpfamilies))
	{
		return m_legacy_distr_opfamily.get();
	}
	else
	{
		return m_distr_opfamily.get();
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt4GPDB::Mdname
//
//	@doc:
//		Returns the name of this type
//
//---------------------------------------------------------------------------
CMDName
CMDTypeInt4GPDB::Mdname() const
{
	return CMDTypeInt4GPDB::m_mdname;
}


//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt4GPDB::GetMdidForCmpType
//
//	@doc:
//		Return mdid of specified comparison operator type
//
//---------------------------------------------------------------------------
IMDId *
CMDTypeInt4GPDB::GetMdidForCmpType(ECmpType cmp_type) const
{
	switch (cmp_type)
	{
		case EcmptEq:
			return m_mdid_op_eq.get();
		case EcmptNEq:
			return m_mdid_op_neq.get();
		case EcmptL:
			return m_mdid_op_lt.get();
		case EcmptLEq:
			return m_mdid_op_leq.get();
		case EcmptG:
			return m_mdid_op_gt.get();
		case EcmptGEq:
			return m_mdid_op_geq.get();
		default:
			GPOS_ASSERT(!"Invalid operator type");
			return nullptr;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt4GPDB::GetMdidForAggType
//
//	@doc:
//		Return mdid of specified aggregate type
//
//---------------------------------------------------------------------------
IMDId *
CMDTypeInt4GPDB::GetMdidForAggType(EAggType agg_type) const
{
	switch (agg_type)
	{
		case EaggMin:
			return m_mdid_min.get();
		case EaggMax:
			return m_mdid_max.get();
		case EaggAvg:
			return m_mdid_avg.get();
		case EaggSum:
			return m_mdid_sum.get();
		case EaggCount:
			return m_mdid_count.get();
		default:
			GPOS_ASSERT(!"Invalid aggregate type");
			return nullptr;
	}
}
//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt4GPDB::Serialize
//
//	@doc:
//		Serialize relation metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDTypeInt4GPDB::Serialize(CXMLSerializer *xml_serializer) const
{
	CGPDBTypeHelper<CMDTypeInt4GPDB>::Serialize(xml_serializer, this);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt4GPDB::GetDatumForDXLConstVal
//
//	@doc:
//		Transformation method for generating int4 datum from CDXLScalarConstValue
//
//---------------------------------------------------------------------------
gpos::Ref<IDatum>
CMDTypeInt4GPDB::GetDatumForDXLConstVal(
	const CDXLScalarConstValue *dxl_op) const
{
	CDXLDatumInt4 *dxl_datum = gpos::dyn_cast<CDXLDatumInt4>(
		const_cast<CDXLDatum *>(dxl_op->GetDatumVal()));

	return GPOS_NEW(m_mp) CDatumInt4GPDB(m_mdid->Sysid(), dxl_datum->Value(),
										 dxl_datum->IsNull());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt4GPDB::GetDatumForDXLDatum
//
//	@doc:
//		Construct an int4 datum from a DXL datum
//
//---------------------------------------------------------------------------
gpos::Ref<IDatum>
CMDTypeInt4GPDB::GetDatumForDXLDatum(CMemoryPool *mp,
									 const CDXLDatum *dxl_datum) const
{
	CDXLDatumInt4 *int4_dxl_datum =
		gpos::dyn_cast<CDXLDatumInt4>(const_cast<CDXLDatum *>(dxl_datum));
	INT val = int4_dxl_datum->Value();
	BOOL is_null = int4_dxl_datum->IsNull();

	return GPOS_NEW(mp) CDatumInt4GPDB(m_mdid->Sysid(), val, is_null);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt4GPDB::GetDatumVal
//
//	@doc:
// 		Generate dxl datum
//
//---------------------------------------------------------------------------
gpos::Ref<CDXLDatum>
CMDTypeInt4GPDB::GetDatumVal(CMemoryPool *mp, IDatum *datum) const
{
	;
	CDatumInt4GPDB *int4_datum = dynamic_cast<CDatumInt4GPDB *>(datum);

	return GPOS_NEW(mp)
		CDXLDatumInt4(mp, m_mdid, int4_datum->IsNull(), int4_datum->Value());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt4GPDB::GetDXLOpScConst
//
//	@doc:
// 		Generate a dxl scalar constant from a datum
//
//---------------------------------------------------------------------------
gpos::Ref<CDXLScalarConstValue>
CMDTypeInt4GPDB::GetDXLOpScConst(CMemoryPool *mp, IDatum *datum) const
{
	CDatumInt4GPDB *int4gpdb_datum = dynamic_cast<CDatumInt4GPDB *>(datum);

	;
	gpos::Ref<CDXLDatumInt4> dxl_datum = GPOS_NEW(mp) CDXLDatumInt4(
		mp, m_mdid, int4gpdb_datum->IsNull(), int4gpdb_datum->Value());

	return GPOS_NEW(mp) CDXLScalarConstValue(mp, std::move(dxl_datum));
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt4GPDB::GetDXLDatumNull
//
//	@doc:
// 		Generate dxl datum
//
//---------------------------------------------------------------------------
gpos::Ref<CDXLDatum>
CMDTypeInt4GPDB::GetDXLDatumNull(CMemoryPool *mp) const
{
	;

	return GPOS_NEW(mp) CDXLDatumInt4(mp, m_mdid, true /*is_null*/, 1);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt4GPDB::DebugPrint
//
//	@doc:
//		Prints a metadata cache relation to the provided output
//
//---------------------------------------------------------------------------
void
CMDTypeInt4GPDB::DebugPrint(IOstream &os) const
{
	CGPDBTypeHelper<CMDTypeInt4GPDB>::DebugPrint(os, this);
}

#endif	// GPOS_DEBUG

// EOF
