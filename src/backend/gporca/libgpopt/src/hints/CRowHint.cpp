//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (c) 2023 VMware, Inc. or its affiliates. All Rights Reserved.
//
//	@filename:
//		CRowHint.cpp
//
//	@doc:
//		Container of plan hint objects
//---------------------------------------------------------------------------

#include "gpopt/hints/CRowHint.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/hints/CHintUtils.h"
#include "gpopt/optimizer/COptimizerConfig.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpopt;

FORCE_GENERATE_DBGSTR(CRowHint);

IOstream &
CRowHint::OsPrint(IOstream &os) const
{
	CWStringDynamic *aliases =
		CDXLUtils::SerializeToCommaSeparatedString(m_mp, m_aliases);

	os << "RowHint: " << aliases->GetBuffer();

	switch (m_type)
	{
		case CRowHint::RVT_ABSOLUTE:
		{
			os << " " << GPOS_WSZ_LIT("#");
			break;
		}
		case CRowHint::RVT_ADD:
		{
			os << " " << GPOS_WSZ_LIT("+");
			break;
		}
		case CRowHint::RVT_SUB:
		{
			os << " " << GPOS_WSZ_LIT("-");
			break;
		}
		case CRowHint::RVT_MULTI:
		{
			os << " " << GPOS_WSZ_LIT("*");
			break;
		}
		default:
		{
		}
	}

	os << m_rows;


	GPOS_DELETE(aliases);
	return os;
}

const StringPtrArray *
CRowHint::GetAliasNames()
{
	if (nullptr == m_aliases_converted)
	{
		m_aliases_converted =
			CHintUtils::ConvertSubqueryAliasesToTableNames(m_mp, m_aliases);
	}
	return m_aliases_converted;
}

void
CRowHint::Serialize(CXMLSerializer *xml_serializer) const
{
	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenRowHint));

	CWStringDynamic *aliases =
		CDXLUtils::SerializeToCommaSeparatedString(m_mp, m_aliases);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenAlias),
								 aliases);
	GPOS_DELETE(aliases);

	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenRows),
								 m_rows);

	const CWStringConst *hint_type = nullptr;
	switch (GetType())
	{
		case CRowHint::RVT_ABSOLUTE:
		{
			hint_type = CDXLTokens::GetDXLTokenStr(EdxltokenAbsolute);
			break;
		}
		case CRowHint::RVT_ADD:
		{
			hint_type = CDXLTokens::GetDXLTokenStr(EdxltokenAdd);
			break;
		}
		case CRowHint::RVT_SUB:
		{
			hint_type = CDXLTokens::GetDXLTokenStr(EdxltokenSubtract);
			break;
		}
		case CRowHint::RVT_MULTI:
		{
			hint_type = CDXLTokens::GetDXLTokenStr(EdxltokenMultiply);
			break;
		}
		default:
		{
			CWStringDynamic *error_message = GPOS_NEW(m_mp)
				CWStringDynamic(m_mp, GPOS_WSZ_LIT("Unknown row type"));

			GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp,
					   error_message->GetBuffer());
		}
	}
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenKind),
								 hint_type);

	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenRowHint));
}
