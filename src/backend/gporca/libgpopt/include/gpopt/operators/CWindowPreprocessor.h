//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2015 VMware, Inc. or its affiliates.
//
//	@filename:
//		CWindowPreprocessor.h
//
//	@doc:
//		Preprocessing routines of window functions
//---------------------------------------------------------------------------
#ifndef GPOPT_CWindowPreprocessor_H
#define GPOPT_CWindowPreprocessor_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CWindowFrame.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CWindowPreprocessor
//
//	@doc:
//		Preprocessing routines of window functions
//
//---------------------------------------------------------------------------
class CWindowPreprocessor
{
private:
	// iterate over project elements and split them elements between Distinct Aggs
	// list, and Others list
	static void SplitPrjList(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprSeqPrj,
		gpos::owner<CExpressionArray *> *ppdrgpexprDistinctAggsPrjElems,
		gpos::owner<CExpressionArray *> *ppdrgpexprOtherPrjElems,
		gpos::owner<gpopt::COrderSpecArray *> *ppdrgposOther,
		gpos::owner<gpopt::CWindowFrameArray *> *ppdrgpwfOther);

	// split given SeqPrj expression into:
	//	- A GbAgg expression containing distinct Aggs, and
	//	- A SeqPrj expression containing all other window functions
	static void SplitSeqPrj(CMemoryPool *mp,
							gpos::pointer<CExpression *> pexprSeqPrj,
							gpos::owner<CExpression *> *ppexprGbAgg,
							gpos::owner<CExpression *> *ppexprOutputSeqPrj);

	// create a CTE with two consumers using the child expression of Sequence
	// Project
	static void CreateCTE(CMemoryPool *mp,
						  gpos::pointer<CExpression *> pexprSeqPrj,
						  gpos::owner<CExpression *> *ppexprFirstConsumer,
						  gpos::owner<CExpression *> *ppexprSecondConsumer);

	// extract grouping columns from given expression
	static CColRefArray *PdrgpcrGrpCols(
		gpos::pointer<CExpression *> pexprJoinDQAs);

	// transform sequence project expression into an inner join expression
	static gpos::owner<CExpression *> PexprSeqPrj2Join(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprSeqPrj);

public:
	CWindowPreprocessor(const CWindowPreprocessor &) = delete;

	// main driver
	static gpos::owner<CExpression *> PexprPreprocess(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

};	// class CWindowPreprocessor
}  // namespace gpopt


#endif	// !GPOPT_CWindowPreprocessor_H

// EOF
