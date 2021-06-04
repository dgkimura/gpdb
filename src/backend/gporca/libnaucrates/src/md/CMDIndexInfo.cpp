//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal Software Inc.
//
//	@filename:
//		CMDIndexInfo.cpp
//
//	@doc:
//		Implementation of the class for representing indexinfo
//---------------------------------------------------------------------------

#include "naucrates/md/CMDIndexInfo.h"

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpdxl;
using namespace gpmd;

// ctor
CMDIndexInfo::CMDIndexInfo(CMemoryPool *mp, IMDId *mdid, BOOL is_partial,
						   ULongPtrArray *included_cols_array)
	: m_mp(mp),
	  m_mdid(mdid),
	  m_is_partial(is_partial),
	  m_included_cols_array(included_cols_array)
{
	GPOS_ASSERT(mdid->IsValid());
}

// dtor
CMDIndexInfo::~CMDIndexInfo()
{
	m_mdid->Release();
}

// returns the metadata id of this index
IMDId *
CMDIndexInfo::MDId() const
{
	return m_mdid;
}

// is the index partial
BOOL
CMDIndexInfo::IsPartial() const
{
	return m_is_partial;
}

ULONG
CMDIndexInfo::IncludedCols() const
{
	return m_included_cols_array->Size();
}

ULONG
CMDIndexInfo::IncludedColAt(ULONG pos) const
{
	return *((*m_included_cols_array)[pos]);
}

// serialize indexinfo in DXL format
void
CMDIndexInfo::Serialize(gpdxl::CXMLSerializer *xml_serializer) const
{
	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenIndexInfo));

	m_mdid->Serialize(xml_serializer,
					  CDXLTokens::GetDXLTokenStr(EdxltokenMdid));
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenIndexPartial), m_is_partial);

	CWStringDynamic *available_cols_str =
		CDXLUtils::Serialize(m_mp, m_included_cols_array);
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenIndexIncludedCols),
		available_cols_str);
	GPOS_DELETE(available_cols_str);

	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenIndexInfo));
}

#ifdef GPOS_DEBUG
// prints a indexinfo to the provided output
void
CMDIndexInfo::DebugPrint(IOstream &os) const
{
	os << "Index id: ";
	MDId()->OsPrint(os);
	os << std::endl;
	os << "Is partial index: " << m_is_partial << std::endl;
}

#endif	// GPOS_DEBUG
