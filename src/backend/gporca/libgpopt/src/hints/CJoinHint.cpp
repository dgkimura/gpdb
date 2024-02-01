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


IOstream &
CJoinHint::OsPrint(IOstream &os) const
{
	os << "JoinHint:";
	return os;
}

void
CJoinHint::Serialize(CXMLSerializer *xml_serializer) const
{
	GPOS_ASSERT(nullptr != xml_serializer);
}
