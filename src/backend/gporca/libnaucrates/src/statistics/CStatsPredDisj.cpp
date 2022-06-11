//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CStatsPredDisj.cpp
//
//	@doc:
//		Implementation of statistics Disjunctive filter
//---------------------------------------------------------------------------

#include "naucrates/statistics/CStatsPredDisj.h"

#include "gpos/common/owner.h"

#include "naucrates/statistics/CStatisticsUtils.h"
#include "naucrates/statistics/StatsPredLess.h"

using namespace gpnaucrates;
using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CStatsPrefDisj::CStatsPrefDisj
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CStatsPredDisj::CStatsPredDisj(
	gpos::Ref<CStatsPredPtrArry> disj_pred_stats_array)
	: CStatsPred(gpos::ulong_max),
	  m_disj_pred_stats_array(std::move(disj_pred_stats_array))
{
	GPOS_ASSERT(nullptr != m_disj_pred_stats_array);
	m_colid = CStatisticsUtils::GetColId(m_disj_pred_stats_array.get());
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPrefDisj::GetPredStats
//
//	@doc:
//		Return the point filter at a particular position
//
//---------------------------------------------------------------------------
CStatsPred *
CStatsPredDisj::GetPredStats(ULONG pos) const
{
	return (*m_disj_pred_stats_array)[pos].get();
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPrefDisj::Sort
//
//	@doc:
//		Sort the components of the disjunction
//
//---------------------------------------------------------------------------
void
CStatsPredDisj::Sort() const
{
	if (1 < GetNumPreds())
	{
		// sort the filters on column ids
		m_disj_pred_stats_array->Sort(gpnaucrates::StatsPredColIdLess{});
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPrefDisj::GetColId
//
//	@doc:
//		Return the column identifier on which the predicates are on
//
//---------------------------------------------------------------------------
ULONG
CStatsPredDisj::GetColId() const
{
	return m_colid;
}

// EOF
