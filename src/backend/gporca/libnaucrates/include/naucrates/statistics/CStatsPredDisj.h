//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CStatsPredDisj.h
//
//	@doc:
//		Disjunctive filter on statistics
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_CStatsPredDisj_H
#define GPNAUCRATES_CStatsPredDisj_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/statistics/CPoint.h"
#include "naucrates/statistics/CStatsPred.h"

namespace gpnaucrates
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CStatsPredDisj
//
//	@doc:
//		Disjunctive filter on statistics
//---------------------------------------------------------------------------
class CStatsPredDisj : public CStatsPred
{
private:
	// private copy ctor
	CStatsPredDisj(const CStatsPredDisj &);

	// private assignment operator
	CStatsPredDisj &operator=(CStatsPredDisj &);

	// array of filters
	gpos::Ref<CStatsPredPtrArry> m_disj_pred_stats_array;

public:
	// ctor
	explicit CStatsPredDisj(gpos::Ref<CStatsPredPtrArry> disj_pred_stats_array);

	// dtor
	~CStatsPredDisj() override
	{
		;
	}

	// the column identifier on which the predicates are on
	ULONG GetColId() const override;

	// total number of predicates in the disjunction
	ULONG
	GetNumPreds() const
	{
		return m_disj_pred_stats_array->Size();
	}

	// return the array of predicate filters
	gpos::Ref<CStatsPredPtrArry>
	GetDisjPredStatsArray() const
	{
		return m_disj_pred_stats_array.get();
	}

	// sort the components of the disjunction
	void Sort() const;

	// return the point filter at a particular position
	gpos::Ref<CStatsPred> GetPredStats(ULONG pos) const;

	// filter type id
	EStatsPredType
	GetPredStatsType() const override
	{
		return CStatsPred::EsptDisj;
	}

	// return the column id of the filter based on the column ids of its child filters
	static ULONG GetColId(const gpos::Ref<CStatsPredPtrArry> pdrgpstatspred);

	// conversion function
	static gpos::Ref<CStatsPredDisj>
	ConvertPredStats(gpos::Ref<CStatsPred> pred_stats)
	{
		GPOS_ASSERT(nullptr != pred_stats);
		GPOS_ASSERT(CStatsPred::EsptDisj == pred_stats->GetPredStatsType());

		return gpos::dyn_cast<CStatsPredDisj>(pred_stats);
	}

};	// class CStatsPredDisj
}  // namespace gpnaucrates

#endif	// !GPNAUCRATES_CStatsPredDisj_H

// EOF
