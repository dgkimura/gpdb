//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CXformExploration.cpp
//
//	@doc:
//		Implementation of basic exploration transformation
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformExploration.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformExploration::CXformExploration
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformExploration::CXformExploration(gpos::owner<CExpression *> pexpr)
	: CXform(std::move(pexpr))
{
	GPOS_ASSERT(nullptr != pexpr);
}


//---------------------------------------------------------------------------
//	@function:
//		CXformExploration::~CXformExploration
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformExploration::~CXformExploration() = default;


// EOF
