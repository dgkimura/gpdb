//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (c) 2023 VMware, Inc. or its affiliates. All Rights Reserved.
//
//	@filename:
//		CScanHint.cpp
//
//	@doc:
//		Container of plan hint objects
//---------------------------------------------------------------------------

#include "gpopt/hints/CScanHint.h"

using namespace gpopt;

FORCE_GENERATE_DBGSTR(CScanHint);

IOstream &
CScanHint::OsPrint(IOstream &os) const
{
	os << "ScanHint: " << m_name->GetBuffer() << "[";
	os << "indexes:";
	if (m_indexnames->Size() > 0)
	{
		os << (*m_indexnames)[0]->GetBuffer();
	}
	for (ULONG ul = 1; ul < m_indexnames->Size(); ul++)
	{
		os << ", " << (*m_indexnames)[ul]->GetBuffer();
	}
	if (m_isnot)
	{
		os << "|isnot=true";
	}
	os << "]";
	return os;
}

static CWStringDynamic *
GetIndexesCommaSeparatedStr(const StringPtrArray *indexes, CMemoryPool *mp)
{
	CWStringDynamic *str = GPOS_NEW(mp) CWStringDynamic(mp);

	ULONG num_of_indexes = indexes == nullptr ? 0 : indexes->Size();
	for (ULONG idx = 0; idx < num_of_indexes; idx++)
	{
		const WCHAR *index = ((*indexes)[idx])->GetBuffer();
		str->AppendWideCharArray(index);
		if (idx != num_of_indexes - 1)
		{
			str->AppendFormat(
				GPOS_WSZ_LIT("%ls"),
				CDXLTokens::GetDXLTokenStr(EdxltokenComma)->GetBuffer());
		}
	}

	return str;
}

void
CScanHint::Serialize(CXMLSerializer *xml_serializer) const
{
	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenScanHint));

	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenAlias),
								 GetName());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenOpName),
								 GetOperator());
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenScalarBoolNot), GetIsNot());

	CWStringDynamic *indexes =
		GetIndexesCommaSeparatedStr(GetIndexNames(), m_mp);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenIndexName),
								 indexes);
	GPOS_DELETE(indexes);

	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenScanHint));
}
