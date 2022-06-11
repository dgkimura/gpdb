//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CStatsPredLike.cpp
//
//	@doc:
//		Implementation of statistics LIKE filter
//---------------------------------------------------------------------------

#include "naucrates/statistics/CStatsPredLike.h"

#include "gpos/common/owner.h"

#include "gpopt/operators/CExpression.h"

using namespace gpnaucrates;
using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredLike::CStatisticsFilterLike
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CStatsPredLike::CStatsPredLike(ULONG colid, gpos::Ref<CExpression> expr_left,
							   gpos::Ref<CExpression> expr_right,
							   CDouble default_scale_factor)
	: CStatsPred(colid),
	  m_expr_left(std::move(expr_left)),
	  m_expr_right(std::move(expr_right)),
	  m_default_scale_factor(default_scale_factor)
{
	GPOS_ASSERT(gpos::ulong_max != colid);
	GPOS_ASSERT(nullptr != m_expr_left);
	GPOS_ASSERT(nullptr != m_expr_right);
	GPOS_ASSERT(0 < default_scale_factor);
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredLike::~CStatisticsFilterLike
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CStatsPredLike::~CStatsPredLike()
{
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredLike::GetColId
//
//	@doc:
//		Return the column identifier on which the predicates are on
//
//---------------------------------------------------------------------------
ULONG
CStatsPredLike::GetColId() const
{
	return m_colid;
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredLike::DefaultScaleFactor
//
//	@doc:
//		Return the default like scale factor
//
//---------------------------------------------------------------------------
CDouble
CStatsPredLike::DefaultScaleFactor() const
{
	return m_default_scale_factor;
}

// EOF
