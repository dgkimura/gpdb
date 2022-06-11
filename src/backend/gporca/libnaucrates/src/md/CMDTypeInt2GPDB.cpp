//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CMDTypeInt2GPDB.cpp
//
//	@doc:
//		Implementation of the class for representing GPDB-specific int2 type in the
//		MD cache
//---------------------------------------------------------------------------

#include "naucrates/md/CMDTypeInt2GPDB.h"

#include "gpos/common/owner.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/base/CDatumInt2GPDB.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLDatum.h"
#include "naucrates/dxl/operators/CDXLDatumInt2.h"
#include "naucrates/dxl/operators/CDXLScalarConstValue.h"

using namespace gpdxl;
using namespace gpmd;
using namespace gpnaucrates;

// static member initialization
CWStringConst CMDTypeInt2GPDB::m_str = CWStringConst(GPOS_WSZ_LIT("int2"));
CMDName CMDTypeInt2GPDB::m_mdname(&m_str);

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt2GPDB::CMDTypeInt2GPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDTypeInt2GPDB::CMDTypeInt2GPDB(CMemoryPool *mp) : m_mp(mp)
{
	m_mdid = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_OID);
	if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution))
	{
		m_distr_opfamily = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_OPFAMILY);
		m_legacy_distr_opfamily =
			GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_LEGACY_OPFAMILY);
	}
	else
	{
		m_distr_opfamily = nullptr;
		m_legacy_distr_opfamily = nullptr;
	}
	m_mdid_op_eq = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_EQ_OP);
	m_mdid_op_neq = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_NEQ_OP);
	m_mdid_op_lt = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_LT_OP);
	m_mdid_op_leq = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_LEQ_OP);
	m_mdid_op_gt = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_GT_OP);
	m_mdid_op_geq = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_GEQ_OP);
	m_mdid_op_cmp = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_COMP_OP);
	m_mdid_type_array = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_ARRAY_TYPE);
	m_mdid_min = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_AGG_MIN);
	m_mdid_max = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_AGG_MAX);
	m_mdid_avg = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_AGG_AVG);
	m_mdid_sum = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_AGG_SUM);
	m_mdid_count = GPOS_NEW(mp) CMDIdGPDB(GPDB_INT2_AGG_COUNT);

	m_dxl_str = CDXLUtils::SerializeMDObj(
		m_mp, this, false /*fSerializeHeader*/, false /*indentation*/);

	GPOS_ASSERT(GPDB_INT2_OID == gpos::dyn_cast<CMDIdGPDB>(m_mdid)->Oid());
	;
	m_datum_null =
		GPOS_NEW(mp) CDatumInt2GPDB(m_mdid, 1 /* value */, true /* is_null */);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt2GPDB::~CMDTypeInt2GPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDTypeInt2GPDB::~CMDTypeInt2GPDB()
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
//		CMDTypeInt2GPDB::GetDatum
//
//	@doc:
//		Factory function for creating INT2 datums
//
//---------------------------------------------------------------------------
gpos::Ref<IDatumInt2>
CMDTypeInt2GPDB::CreateInt2Datum(CMemoryPool *mp, SINT value,
								 BOOL is_null) const
{
	return GPOS_NEW(mp) CDatumInt2GPDB(m_mdid->Sysid(), value, is_null);
}


//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt2GPDB::MDId
//
//	@doc:
//		Returns the metadata id of this type
//
//---------------------------------------------------------------------------
IMDId *
CMDTypeInt2GPDB::MDId() const
{
	return m_mdid.get();
}

IMDId *
CMDTypeInt2GPDB::GetDistrOpfamilyMdid() const
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
//		CMDTypeInt2GPDB::Mdname
//
//	@doc:
//		Returns the name of this type
//
//---------------------------------------------------------------------------
CMDName
CMDTypeInt2GPDB::Mdname() const
{
	return CMDTypeInt2GPDB::m_mdname;
}


//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt2GPDB::GetMdidForCmpType
//
//	@doc:
//		Return mdid of specified comparison operator type
//
//---------------------------------------------------------------------------
IMDId *
CMDTypeInt2GPDB::GetMdidForCmpType(ECmpType cmp_type) const
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
//		CMDTypeInt2GPDB::GetMdidForAggType
//
//	@doc:
//		Return mdid of specified aggregate type
//
//---------------------------------------------------------------------------
IMDId *
CMDTypeInt2GPDB::GetMdidForAggType(EAggType agg_type) const
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
//		CMDTypeInt2GPDB::Serialize
//
//	@doc:
//		Serialize relation metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDTypeInt2GPDB::Serialize(CXMLSerializer *xml_serializer) const
{
	CGPDBTypeHelper<CMDTypeInt2GPDB>::Serialize(xml_serializer, this);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt2GPDB::GetDatumForDXLConstVal
//
//	@doc:
//		Transformation method for generating int2 datum from CDXLScalarConstValue
//
//---------------------------------------------------------------------------
gpos::Ref<IDatum>
CMDTypeInt2GPDB::GetDatumForDXLConstVal(
	const CDXLScalarConstValue *dxl_op) const
{
	CDXLDatumInt2 *dxl_datum = gpos::dyn_cast<CDXLDatumInt2>(
		const_cast<CDXLDatum *>(dxl_op->GetDatumVal()));

	return GPOS_NEW(m_mp) CDatumInt2GPDB(m_mdid->Sysid(), dxl_datum->Value(),
										 dxl_datum->IsNull());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt2GPDB::GetDatumForDXLDatum
//
//	@doc:
//		Construct an int2 datum from a DXL datum
//
//---------------------------------------------------------------------------
gpos::Ref<IDatum>
CMDTypeInt2GPDB::GetDatumForDXLDatum(CMemoryPool *mp,
									 const CDXLDatum *dxl_datum) const
{
	CDXLDatumInt2 *int2_dxl_datum =
		gpos::dyn_cast<CDXLDatumInt2>(const_cast<CDXLDatum *>(dxl_datum));
	SINT val = int2_dxl_datum->Value();
	BOOL is_null = int2_dxl_datum->IsNull();

	return GPOS_NEW(mp) CDatumInt2GPDB(m_mdid->Sysid(), val, is_null);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt2GPDB::GetDatumVal
//
//	@doc:
// 		Generate dxl datum
//
//---------------------------------------------------------------------------
gpos::Ref<CDXLDatum>
CMDTypeInt2GPDB::GetDatumVal(CMemoryPool *mp, IDatum *datum) const
{
	;
	CDatumInt2GPDB *int2_datum = dynamic_cast<CDatumInt2GPDB *>(datum);

	return GPOS_NEW(mp)
		CDXLDatumInt2(mp, m_mdid, int2_datum->IsNull(), int2_datum->Value());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt2GPDB::GetDXLOpScConst
//
//	@doc:
// 		Generate a dxl scalar constant from a datum
//
//---------------------------------------------------------------------------
gpos::Ref<CDXLScalarConstValue>
CMDTypeInt2GPDB::GetDXLOpScConst(CMemoryPool *mp, IDatum *datum) const
{
	CDatumInt2GPDB *int2gpdb_datum = dynamic_cast<CDatumInt2GPDB *>(datum);

	;
	gpos::Ref<CDXLDatumInt2> dxl_datum = GPOS_NEW(mp) CDXLDatumInt2(
		mp, m_mdid, int2gpdb_datum->IsNull(), int2gpdb_datum->Value());

	return GPOS_NEW(mp) CDXLScalarConstValue(mp, std::move(dxl_datum));
}

//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt2GPDB::GetDXLDatumNull
//
//	@doc:
// 		Generate dxl datum
//
//---------------------------------------------------------------------------
gpos::Ref<CDXLDatum>
CMDTypeInt2GPDB::GetDXLDatumNull(CMemoryPool *mp) const
{
	;

	return GPOS_NEW(mp)
		CDXLDatumInt2(mp, m_mdid, true /*is_null*/, 1 /* a dummy value */);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CMDTypeInt2GPDB::DebugPrint
//
//	@doc:
//		Prints a metadata cache relation to the provided output
//
//---------------------------------------------------------------------------
void
CMDTypeInt2GPDB::DebugPrint(IOstream &os) const
{
	CGPDBTypeHelper<CMDTypeInt2GPDB>::DebugPrint(os, this);
}

#endif	// GPOS_DEBUG

// EOF
