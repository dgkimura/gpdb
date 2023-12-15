//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (c) 2023 VMware, Inc. or its affiliates. All Rights Reserved.
//
//	@filename:
//		CPlanHint.cpp
//
//	@doc:
//		Container of plan hint objects
//---------------------------------------------------------------------------

#include "gpopt/hints/CPlanHint.h"

#include "gpos/base.h"

using namespace gpopt;

FORCE_GENERATE_DBGSTR(CPlanHint);

//---------------------------------------------------------------------------
//	@function:
//		CPlanHint::CPlanHint
//---------------------------------------------------------------------------
CPlanHint::CPlanHint(CMemoryPool *mp)
	: m_mp(mp), m_scan_hint(GPOS_NEW(mp) ScanHintList(mp))
{
}

CPlanHint::~CPlanHint()
{
	m_scan_hint->Release();
}

void
CPlanHint::AddHint(CScanHint *hint)
{
	m_scan_hint->Append(hint);
}

CScanHint *
CPlanHint::GetScanHint(const char *relname)
{
	CWStringConst *name = GPOS_NEW(m_mp) CWStringConst(m_mp, relname);
	CScanHint *hint = GetScanHint(name);
	GPOS_DELETE(name);
	return hint;
}

CScanHint *
CPlanHint::GetScanHint(const CWStringBase *name)
{
	for (ULONG ul = 0; ul < m_scan_hint->Size(); ul++)
	{
		CScanHint *hint = (*m_scan_hint)[ul];
		if (name->Equals(hint->GetName()))
		{
			return hint;
		}
	}
	return nullptr;
}

IOstream &
CPlanHint::OsPrint(IOstream &os) const
{
	os << "PlanHint: [";
	if (nullptr == m_scan_hint)
	{
		os << "]";
		return os;
	}

	os << "\n";
	for (ULONG ul = 0; ul < m_scan_hint->Size(); ul++)
	{
		os << "  ";
		(*m_scan_hint)[ul]->OsPrint(os) << "\n";
	}
	os << "]";
	return os;
}

void
CPlanHint::Serialize(CXMLSerializer *xml_serializer) const
{
	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenHint));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName),
								 "scan");

	for (ULONG ul = 0; ul < m_scan_hint->Size(); ul++)
	{
		(*m_scan_hint)[ul]->Serialize(xml_serializer);
	}

	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenHint));
}


// EOF
