//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMDScalarOpGPDB.cpp
//
//	@doc:
//		Implementation of the class for representing GPDB-specific scalar ops
//		in the MD cache
//---------------------------------------------------------------------------


#include "naucrates/md/CMDScalarOpGPDB.h"

#include "gpos/common/owner.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::CMDScalarOpGPDB
//
//	@doc:
//		Constructs a metadata scalar op
//
//---------------------------------------------------------------------------
CMDScalarOpGPDB::CMDScalarOpGPDB(
	CMemoryPool *mp, gpos::Ref<IMDId> mdid, CMDName *mdname,
	gpos::Ref<IMDId> mdid_type_left, gpos::Ref<IMDId> mdid_type_right,
	gpos::Ref<IMDId> result_type_mdid, gpos::Ref<IMDId> mdid_func,
	gpos::Ref<IMDId> mdid_commute_opr, gpos::Ref<IMDId> m_mdid_inverse_opr,
	IMDType::ECmpType cmp_type, BOOL returns_null_on_null_input,
	gpos::Ref<IMdIdArray> mdid_opfamilies_array,
	gpos::Ref<IMDId> mdid_hash_opfamily,
	gpos::Ref<IMDId> mdid_legacy_hash_opfamily, BOOL is_ndv_preserving)
	: m_mp(mp),
	  m_mdid(std::move(mdid)),
	  m_mdname(mdname),
	  m_mdid_type_left(std::move(mdid_type_left)),
	  m_mdid_type_right(std::move(mdid_type_right)),
	  m_mdid_type_result(std::move(result_type_mdid)),
	  m_func_mdid(std::move(mdid_func)),
	  m_mdid_commute_opr(std::move(mdid_commute_opr)),
	  m_mdid_inverse_opr(std::move(m_mdid_inverse_opr)),
	  m_comparision_type(cmp_type),
	  m_returns_null_on_null_input(returns_null_on_null_input),
	  m_mdid_opfamilies_array(std::move(mdid_opfamilies_array)),
	  m_mdid_hash_opfamily(std::move(mdid_hash_opfamily)),
	  m_mdid_legacy_hash_opfamily(std::move(mdid_legacy_hash_opfamily)),
	  m_is_ndv_preserving(is_ndv_preserving)
{
	GPOS_ASSERT(nullptr != m_mdid_opfamilies_array);
	m_dxl_str = CDXLUtils::SerializeMDObj(
		m_mp, this, false /*fSerializeHeader*/, false /*indentation*/);
}


//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::~CMDScalarOpGPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDScalarOpGPDB::~CMDScalarOpGPDB()
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

	GPOS_DELETE(m_mdname);
	GPOS_DELETE(m_dxl_str);
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::MDId
//
//	@doc:
//		Operator id
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::MDId() const
{
	return m_mdid.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::Mdname
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
CMDName
CMDScalarOpGPDB::Mdname() const
{
	return *m_mdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::GetLeftMdid
//
//	@doc:
//		Type id of left operand
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::GetLeftMdid() const
{
	return m_mdid_type_left.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::GetRightMdid
//
//	@doc:
//		Type id of right operand
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::GetRightMdid() const
{
	return m_mdid_type_right.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::GetResultTypeMdid
//
//	@doc:
//		Type id of result
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::GetResultTypeMdid() const
{
	return m_mdid_type_result.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::FuncMdId
//
//	@doc:
//		Id of function which implements the operator
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::FuncMdId() const
{
	return m_func_mdid.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::GetCommuteOpMdid
//
//	@doc:
//		Id of commute operator
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::GetCommuteOpMdid() const
{
	return m_mdid_commute_opr.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::GetInverseOpMdid
//
//	@doc:
//		Id of inverse operator
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::GetInverseOpMdid() const
{
	return m_mdid_inverse_opr.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::IsEqualityOp
//
//	@doc:
//		Is this an equality operator
//
//---------------------------------------------------------------------------
BOOL
CMDScalarOpGPDB::IsEqualityOp() const
{
	return IMDType::EcmptEq == m_comparision_type;
}


//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::ReturnsNullOnNullInput
//
//	@doc:
//		Does operator return NULL when all inputs are NULL?
//		STRICT implies NULL-returning, but the opposite is not always true,
//		the implementation in GPDB returns what STRICT property states
//
//---------------------------------------------------------------------------
BOOL
CMDScalarOpGPDB::ReturnsNullOnNullInput() const
{
	return m_returns_null_on_null_input;
}


BOOL
CMDScalarOpGPDB::IsNDVPreserving() const
{
	return m_is_ndv_preserving;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::ParseCmpType
//
//	@doc:
//		Comparison type
//
//---------------------------------------------------------------------------
IMDType::ECmpType
CMDScalarOpGPDB::ParseCmpType() const
{
	return m_comparision_type;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::Serialize
//
//	@doc:
//		Serialize scalar op metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDScalarOpGPDB::Serialize(CXMLSerializer *xml_serializer) const
{
	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenGPDBScalarOp));

	m_mdid->Serialize(xml_serializer,
					  CDXLTokens::GetDXLTokenStr(EdxltokenMdid));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName),
								 m_mdname->GetMDName());
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenGPDBScalarOpCmpType),
		IMDType::GetCmpTypeStr(m_comparision_type));
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenReturnsNullOnNullInput),
		m_returns_null_on_null_input);
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenIsNDVPreserving),
		m_is_ndv_preserving);

	Edxltoken dxl_token_array[8] = {EdxltokenGPDBScalarOpLeftTypeId,
									EdxltokenGPDBScalarOpRightTypeId,
									EdxltokenGPDBScalarOpResultTypeId,
									EdxltokenGPDBScalarOpFuncId,
									EdxltokenGPDBScalarOpCommOpId,
									EdxltokenGPDBScalarOpInverseOpId,
									EdxltokenGPDBScalarOpHashOpfamily,
									EdxltokenGPDBScalarOpLegacyHashOpfamily};

	IMDId *mdid_array[8] = {m_mdid_type_left,	  m_mdid_type_right,
							m_mdid_type_result,	  m_func_mdid,
							m_mdid_commute_opr,	  m_mdid_inverse_opr,
							m_mdid_hash_opfamily, m_mdid_legacy_hash_opfamily};

	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(dxl_token_array); ul++)
	{
		SerializeMDIdAsElem(xml_serializer,
							CDXLTokens::GetDXLTokenStr(dxl_token_array[ul]),
							mdid_array[ul]);

		GPOS_CHECK_ABORT;
	}

	// serialize opfamilies information
	if (0 < m_mdid_opfamilies_array->Size())
	{
		SerializeMDIdList(xml_serializer, m_mdid_opfamilies_array.get(),
						  CDXLTokens::GetDXLTokenStr(EdxltokenOpfamilies),
						  CDXLTokens::GetDXLTokenStr(EdxltokenOpfamily));
	}

	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenGPDBScalarOp));
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::OpfamiliesCount
//
//	@doc:
//		Number of opfamilies this operator belongs to
//
//---------------------------------------------------------------------------
ULONG
CMDScalarOpGPDB::OpfamiliesCount() const
{
	return m_mdid_opfamilies_array->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::OpfamilyMdidAt
//
//	@doc:
//		Operator family at given position
//
//---------------------------------------------------------------------------
IMDId *
CMDScalarOpGPDB::OpfamilyMdidAt(ULONG pos) const
{
	GPOS_ASSERT(pos < m_mdid_opfamilies_array->Size());

	return (*m_mdid_opfamilies_array)[pos].get();
}

// compatible hash opfamily
IMDId *
CMDScalarOpGPDB::HashOpfamilyMdid() const
{
	if (GPOS_FTRACE(EopttraceUseLegacyOpfamilies))
	{
		return m_mdid_legacy_hash_opfamily.get();
	}
	else
	{
		return m_mdid_hash_opfamily.get();
	}
}


#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CMDScalarOpGPDB::DebugPrint
//
//	@doc:
//		Prints a metadata cache relation to the provided output
//
//---------------------------------------------------------------------------
void
CMDScalarOpGPDB::DebugPrint(IOstream &os) const
{
	os << "Operator id: ";
	MDId()->OsPrint(os);
	os << std::endl;

	os << "Operator name: " << (Mdname()).GetMDName()->GetBuffer() << std::endl;

	os << "Left operand type id: ";
	GetLeftMdid()->OsPrint(os);
	os << std::endl;

	os << "Right operand type id: ";
	GetRightMdid()->OsPrint(os);
	os << std::endl;

	os << "Result type id: ";
	GetResultTypeMdid()->OsPrint(os);
	os << std::endl;

	os << "Operator func id: ";
	FuncMdId()->OsPrint(os);
	os << std::endl;

	os << "Commute operator id: ";
	GetCommuteOpMdid()->OsPrint(os);
	os << std::endl;

	os << "Inverse operator id: ";
	GetInverseOpMdid()->OsPrint(os);
	os << std::endl;

	os << std::endl;
}

#endif	// GPOS_DEBUG


// EOF
