//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2022 VMware, Inc. or its affiliates.
//
//	@filename:
//		COrderedAggPreprocessor.h
//
//	@doc:
//		Preprocessing routines of ordered-set aggregates
//---------------------------------------------------------------------------
#ifndef GPOPT_COrderedAggPreprocessor_H
#define GPOPT_COrderedAggPreprocessor_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CWindowFrame.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		COrderedAggPreprocessor
//
//	@doc:
//		Preprocessing routines of window functions
//
//---------------------------------------------------------------------------
class COrderedAggPreprocessor
{
private:
	// iterate over project elements and split elements between Ordered Aggs
	// list, and Others list
	static void SplitPrjList(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprInputAggPrj,
		gpos::Ref<CExpressionArray> *ppdrgpexprOrderedAggsPrEl,
		gpos::Ref<CExpressionArray> *ppdrgpexprOtherPrjElems);

	// split given InputAgg expression into:
	//	- A GbAgg expression containing ordered aggs split to gp_percentile agg, and
	//	- A GbAgg expression containing all other non-ordered agg functions
	static void SplitOrderedAggsPrj(CMemoryPool *mp,
									gpos::Ref<CExpression> pexprInputAggPrj,
									gpos::Ref<CExpression> *ppexprGbAgg,
									gpos::Ref<CExpression> *ppexprOutputSeqPrj);

	// create a CTE with two consumers using the child expression of Sequence
	// Project
	static void CreateCTE(CMemoryPool *mp,
						  gpos::Ref<CExpression> pexprInputAggPrj,
						  gpos::Ref<CExpression> *ppexprFirstConsumer,
						  gpos::Ref<CExpression> *ppexprSecondConsumer);

	// create an expression for final agg function
	static gpos::Ref<CExpression> PexprFinalAgg(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprAggFunc,
		CColRef *arg_col_ref, CColRef *total_count_colref,
		CColRef *peer_count_colref);

	// transform sequence project expression into an inner join expression
	static gpos::Ref<CExpression> PexprInputAggPrj2Join(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprInputAggPrj);

public:
	COrderedAggPreprocessor(const COrderedAggPreprocessor &) = delete;

	// main driver
	static gpos::Ref<CExpression> PexprPreprocess(CMemoryPool *mp,
												  gpos::Ref<CExpression> pexpr);

};	// class COrderedAggPreprocessor
}  // namespace gpopt


#endif	// !GPOPT_COrderedAggPreprocessor_H

// EOF
