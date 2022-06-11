//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CXformImplementation.cpp
//
//	@doc:
//		Implementation of basic implementation transformation
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformImplementation.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementation::CXformImplementation
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CXformImplementation::CXformImplementation(gpos::owner<CExpression *> pexpr)
	: CXform(std::move(pexpr))
{
	GPOS_ASSERT(nullptr != pexpr);
}


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementation::~CXformImplementation
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CXformImplementation::~CXformImplementation() = default;


// EOF
