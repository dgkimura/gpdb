//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (c) 2023 VMware, Inc. or its affiliates. All Rights Reserved.
//
//	@filename:
//		CJoinHint.cpp
//
//	@doc:
//		Container of join hint objects
//---------------------------------------------------------------------------

#include "gpopt/hints/CJoinHint.h"

#include "gpopt/exception.h"
#include "gpopt/hints/CHintUtils.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpopt;

FORCE_GENERATE_DBGSTR(CJoinHint);
FORCE_GENERATE_DBGSTR(CJoinHint::JoinPair);

IOstream &
CJoinHint::JoinPair::OsPrint(IOstream &os) const
{
	if (nullptr != m_name)
	{
		os << m_name->GetBuffer();
	}
	else
	{
		os << "(";
		m_outer->OsPrint(os);
		os << " ";
		m_inner->OsPrint(os);
		os << ")";

		os << "{";
		CJoinHint::CWStringConstHashSetIter iter(m_aliases);
		while (iter.Advance())
		{
			const CWStringConst *cwstr = iter.Get();
			os << " " << cwstr->GetBuffer();
		}
		os << " }";
	}
	return os;
}


IOstream &
CJoinHint::OsPrint(IOstream &os) const
{
	os << "JoinHint:";

	m_join_pair->OsPrint(os);

	return os;
}

void
CJoinHint::Serialize(CXMLSerializer *xml_serializer) const
{
	GPOS_ASSERT(nullptr != xml_serializer);
}
