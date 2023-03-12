//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CStatsPredConj.cpp
//
//	@doc:
//		Implementation of statistics Conjunctive filter
//---------------------------------------------------------------------------

#include "naucrates/statistics/CStatsPredConj.h"

#include "gpopt/search/CGroupExpression.h"
#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpnaucrates;
using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredConj::CStatsPredConj
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CStatsPredConj::CStatsPredConj(
	gpos::Ref<CStatsPredPtrArry> conj_pred_stats_array)
	: CStatsPred(gpos::ulong_max),
	  m_conj_pred_stats_array(conj_pred_stats_array)
{
	GPOS_ASSERT(nullptr != conj_pred_stats_array);
	m_colid = CStatisticsUtils::GetColId(conj_pred_stats_array);
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredConj::GetPredStats
//
//	@doc:
//		Return the filter at a particular position
//
//---------------------------------------------------------------------------
gpos::Ref<CStatsPred>
CStatsPredConj::GetPredStats(ULONG pos) const
{
	return (*m_conj_pred_stats_array)[pos];
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredConj::Sort
//
//	@doc:
//		Sort the components of the disjunction
//
//---------------------------------------------------------------------------
void
CStatsPredConj::Sort() const
{
	if (1 < GetNumPreds())
	{
		// sort the filters on column ids
		m_conj_pred_stats_array->Sort([](const gpos::Ref<CStatsPred> &lhs,
										 const gpos::Ref<CStatsPred> &rhs) {
			return (INT) lhs->GetColId() - (INT) rhs->GetColId();
		});
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CStatsPredConj::GetColId
//
//	@doc:
//		Return the column identifier on which the predicates are on
//
//---------------------------------------------------------------------------
ULONG
CStatsPredConj::GetColId() const
{
	return m_colid;
}

// EOF
