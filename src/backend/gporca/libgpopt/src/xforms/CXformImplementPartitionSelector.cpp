//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CXformImplementPartitionSelector.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformImplementPartitionSelector.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CLogicalPartitionSelector.h"
#include "gpopt/operators/CPatternLeaf.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformImplementPartitionSelector::CXformImplementPartitionSelector
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformImplementPartitionSelector::CXformImplementPartitionSelector(
	CMemoryPool *mp)
	:  // pattern
	  CXformImplementation(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalPartitionSelector(mp),
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CPatternLeaf(mp))  // relational child
		  ))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformImplementPartitionSelector::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformImplementPartitionSelector::Transform(
	gpos::pointer<CXformContext *> pxfctxt GPOS_UNUSED,
	gpos::pointer<CXformResult *> pxfres GPOS_UNUSED,
	gpos::pointer<CExpression *> pexpr GPOS_UNUSED) const
{
}

// EOF
