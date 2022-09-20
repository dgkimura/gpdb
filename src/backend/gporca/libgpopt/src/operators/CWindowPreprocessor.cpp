//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2015 VMware, Inc. or its affiliates.
//
//	@filename:
//		CWindowPreprocessor.cpp
//
//	@doc:
//		Preprocessing routines of window functions
//---------------------------------------------------------------------------

#include "gpopt/operators/CWindowPreprocessor.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalCTEAnchor.h"
#include "gpopt/operators/CLogicalCTEConsumer.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalSequenceProject.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarWindowFunc.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CWindowPreprocessor::SplitPrjList
//
//	@doc:
//		Iterate over project elements and split them elements between
//		Distinct Aggs list, and Others list
//
//---------------------------------------------------------------------------
void
CWindowPreprocessor::SplitPrjList(
	CMemoryPool *mp, CExpression *pexprSeqPrj,
	gpos::Ref<CExpressionArray> *
		ppdrgpexprDistinctAggsPrEl,	 // output: list of project elements with Distinct Aggs
	gpos::Ref<CExpressionArray> *
		ppdrgpexprOtherPrEl,  // output: list of project elements with Other window functions
	gpos::Ref<COrderSpecArray> *
		ppdrgposOther,	// output: array of order specs of window functions used in Others list
	gpos::Ref<CWindowFrameArray> *
		ppdrgpwfOther  // output: array of frame specs of window functions used in Others list
)
{
	GPOS_ASSERT(nullptr != pexprSeqPrj);
	GPOS_ASSERT(nullptr != ppdrgpexprDistinctAggsPrEl);
	GPOS_ASSERT(nullptr != ppdrgpexprOtherPrEl);
	GPOS_ASSERT(nullptr != ppdrgposOther);
	GPOS_ASSERT(nullptr != ppdrgpwfOther);

	CLogicalSequenceProject *popSeqPrj =
		gpos::dyn_cast<CLogicalSequenceProject>(pexprSeqPrj->Pop());
	CExpression *pexprPrjList = (*pexprSeqPrj)[1];

	COrderSpecArray *pdrgpos = popSeqPrj->Pdrgpos();
	BOOL fHasOrderSpecs = popSeqPrj->FHasOrderSpecs();

	CWindowFrameArray *pdrgpwf = popSeqPrj->Pdrgpwf();
	BOOL fHasFrameSpecs = popSeqPrj->FHasFrameSpecs();

	gpos::Ref<CExpressionArray> pdrgpexprDistinctAggsPrEl =
		GPOS_NEW(mp) CExpressionArray(mp);

	gpos::Ref<CExpressionArray> pdrgpexprOtherPrEl =
		GPOS_NEW(mp) CExpressionArray(mp);
	gpos::Ref<COrderSpecArray> pdrgposOther = GPOS_NEW(mp) COrderSpecArray(mp);
	gpos::Ref<CWindowFrameArray> pdrgpwfOther =
		GPOS_NEW(mp) CWindowFrameArray(mp);

	// iterate over project list and split project elements between
	// Distinct Aggs list, and Others list
	const ULONG arity = pexprPrjList->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		CExpression *pexprPrjEl = (*pexprPrjList)[ul];
		CExpression *pexprWinFunc = (*pexprPrjEl)[0];
		CScalarWindowFunc *popScWinFunc =
			gpos::dyn_cast<CScalarWindowFunc>(pexprWinFunc->Pop());
		CScalarProjectElement *popScPrjElem =
			gpos::dyn_cast<CScalarProjectElement>(pexprPrjEl->Pop());
		CColRef *pcrPrjElem = popScPrjElem->Pcr();

		if (popScWinFunc->IsDistinct() && popScWinFunc->FAgg())
		{
			gpos::Ref<CExpression> pexprAgg =
				CXformUtils::PexprWinFuncAgg2ScalarAgg(mp, pexprWinFunc);
			gpos::Ref<CExpression> pexprNewPrjElem =
				CUtils::PexprScalarProjectElement(mp, pcrPrjElem, pexprAgg);
			pdrgpexprDistinctAggsPrEl->Append(pexprNewPrjElem);
		}
		else
		{
			if (fHasOrderSpecs)
			{
				;
				pdrgposOther->Append((*pdrgpos)[ul]);
			}

			if (fHasFrameSpecs)
			{
				;
				pdrgpwfOther->Append((*pdrgpwf)[ul]);
			}

			;
			gpos::Ref<CExpression> pexprNewPrjElem =
				CUtils::PexprScalarProjectElement(mp, pcrPrjElem, pexprWinFunc);
			pdrgpexprOtherPrEl->Append(pexprNewPrjElem);
		}
	}

	*ppdrgpexprDistinctAggsPrEl = pdrgpexprDistinctAggsPrEl;
	*ppdrgpexprOtherPrEl = pdrgpexprOtherPrEl;
	*ppdrgposOther = pdrgposOther;
	*ppdrgpwfOther = pdrgpwfOther;
}


//---------------------------------------------------------------------------
//	@function:
//		CWindowPreprocessor::SplitSeqPrj
//
//	@doc:
//		Split SeqPrj expression into:
//		- A GbAgg expression containing distinct Aggs, and
//		- A SeqPrj expression containing all remaining window functions
//
//---------------------------------------------------------------------------
void
CWindowPreprocessor::SplitSeqPrj(
	CMemoryPool *mp, CExpression *pexprSeqPrj,
	gpos::Ref<CExpression>
		*ppexprGbAgg,  // output: GbAgg expression containing distinct Aggs
	gpos::Ref<CExpression> *
		ppexprOutputSeqPrj	// output: SeqPrj expression containing all remaining window functions
)
{
	GPOS_ASSERT(nullptr != pexprSeqPrj);
	GPOS_ASSERT(nullptr != ppexprGbAgg);
	GPOS_ASSERT(nullptr != ppexprOutputSeqPrj);

	// split project elements between Distinct Aggs list, and Others list
	gpos::Ref<CExpressionArray> pdrgpexprDistinctAggsPrEl = nullptr;
	gpos::Ref<CExpressionArray> pdrgpexprOtherPrEl = nullptr;
	gpos::Ref<COrderSpecArray> pdrgposOther = nullptr;
	gpos::Ref<CWindowFrameArray> pdrgpwfOther = nullptr;
	SplitPrjList(mp, pexprSeqPrj, &pdrgpexprDistinctAggsPrEl,
				 &pdrgpexprOtherPrEl, &pdrgposOther, &pdrgpwfOther);

	// check distribution spec of original SeqPrj and extract grouping columns
	// from window (PARTITION BY) clause
	CLogicalSequenceProject *popSeqPrj =
		gpos::dyn_cast<CLogicalSequenceProject>(pexprSeqPrj->Pop());
	CDistributionSpec *pds = popSeqPrj->Pds();
	gpos::Ref<CColRefArray> pdrgpcrGrpCols = nullptr;
	if (CDistributionSpec::EdtHashed == pds->Edt())
	{
		gpos::Ref<CColRefSet> pcrs = CUtils::PcrsExtractColumns(
			mp, gpos::dyn_cast<CDistributionSpecHashed>(pds)->Pdrgpexpr());
		pdrgpcrGrpCols = pcrs->Pdrgpcr(mp);
		;
	}
	else
	{
		// no (PARTITION BY) clause
		pdrgpcrGrpCols = GPOS_NEW(mp) CColRefArray(mp);
	}

	gpos::Ref<CExpression> pexprSeqPrjChild = (*pexprSeqPrj)[0];
	;
	*ppexprGbAgg = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp)
			CLogicalGbAgg(mp, pdrgpcrGrpCols, COperator::EgbaggtypeGlobal),
		pexprSeqPrjChild,
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
								 pdrgpexprDistinctAggsPrEl));

	;
	if (0 == pdrgpexprOtherPrEl->Size())
	{
		// no remaining window functions after excluding distinct aggs,
		// reuse the original SeqPrj child in this case
		;
		;
		;
		*ppexprOutputSeqPrj = pexprSeqPrjChild;

		return;
	}

	// create a new SeqPrj expression for remaining window functions
	;
	*ppexprOutputSeqPrj = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalSequenceProject(mp, pds, std::move(pdrgposOther),
											 std::move(pdrgpwfOther)),
		pexprSeqPrjChild,
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
								 std::move(pdrgpexprOtherPrEl)));
}


//---------------------------------------------------------------------------
//	@function:
//		CWindowPreprocessor::CreateCTE
//
//	@doc:
//		Create a CTE with two consumers using the child expression of
//		Sequence Project
//
//---------------------------------------------------------------------------
void
CWindowPreprocessor::CreateCTE(CMemoryPool *mp, CExpression *pexprSeqPrj,
							   gpos::Ref<CExpression> *ppexprFirstConsumer,
							   gpos::Ref<CExpression> *ppexprSecondConsumer)
{
	GPOS_ASSERT(nullptr != pexprSeqPrj);
	GPOS_ASSERT(COperator::EopLogicalSequenceProject ==
				pexprSeqPrj->Pop()->Eopid());
	GPOS_ASSERT(nullptr != ppexprFirstConsumer);
	GPOS_ASSERT(nullptr != ppexprSecondConsumer);

	CExpression *pexprChild = (*pexprSeqPrj)[0];
	CColRefSet *pcrsChildOutput = pexprChild->DeriveOutputColumns();
	gpos::Ref<CColRefArray> pdrgpcrChildOutput = pcrsChildOutput->Pdrgpcr(mp);

	// create a CTE producer based on SeqPrj child expression
	CCTEInfo *pcteinfo = COptCtxt::PoctxtFromTLS()->Pcteinfo();
	const ULONG ulCTEId = pcteinfo->next_id();
	CExpression *pexprCTEProd = CXformUtils::PexprAddCTEProducer(
		mp, ulCTEId, pdrgpcrChildOutput.get(), pexprChild);
	gpos::Ref<CColRefArray> pdrgpcrProducerOutput =
		pexprCTEProd->DeriveOutputColumns()->Pdrgpcr(mp);

	// first consumer creates new output columns to be used later as input to GbAgg expression
	*ppexprFirstConsumer = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalCTEConsumer(
			mp, ulCTEId, CUtils::PdrgpcrCopy(mp, pdrgpcrProducerOutput.get())));
	pcteinfo->IncrementConsumers(ulCTEId);
	;

	// second consumer reuses the same output columns of SeqPrj child to be able to provide any requested columns upstream
	*ppexprSecondConsumer = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CLogicalCTEConsumer(
							mp, ulCTEId, std::move(pdrgpcrChildOutput)));
	pcteinfo->IncrementConsumers(ulCTEId);
}


//---------------------------------------------------------------------------
//	@function:
//		CWindowPreprocessor::PdrgpcrGrpCols
//
//	@doc:
//		Extract grouping columns from given expression,
//		we expect expression to be either a GbAgg expression or a join
//		whose inner child is a GbAgg expression
//
//---------------------------------------------------------------------------
CColRefArray *
CWindowPreprocessor::PdrgpcrGrpCols(CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	COperator *pop = pexpr->Pop();

	if (COperator::EopLogicalGbAgg == pop->Eopid())
	{
		// passed expression is a Group By, return grouping columns
		return gpos::dyn_cast<CLogicalGbAgg>(pop)->Pdrgpcr();
	}

	if (CUtils::FLogicalJoin(pop))
	{
		// pass expression is a join, we expect a Group By on the inner side
		COperator *popInner = (*pexpr)[1]->Pop();
		if (COperator::EopLogicalGbAgg == popInner->Eopid())
		{
			return gpos::dyn_cast<CLogicalGbAgg>(popInner)->Pdrgpcr();
		}
	}

	return nullptr;
}


//---------------------------------------------------------------------------
//	@function:
//		CWindowPreprocessor::PexprSeqPrj2Join
//
//	@doc:
//		Transform sequence project expression with distinct aggregates
//		into an inner join expression,
//			- the outer child of the join is a GbAgg expression that computes
//			distinct aggs,
//			- the inner child of the join is a SeqPrj expression that computes
//			remaining window functions
//
//		we use a CTE to compute the input to both join children, while maintaining
//		all column references upstream by reusing the same computed columns in the
//		original SeqPrj expression
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CWindowPreprocessor::PexprSeqPrj2Join(CMemoryPool *mp, CExpression *pexprSeqPrj)
{
	GPOS_ASSERT(nullptr != pexprSeqPrj);
	GPOS_ASSERT(COperator::EopLogicalSequenceProject ==
				pexprSeqPrj->Pop()->Eopid());
	GPOS_ASSERT(0 < (*pexprSeqPrj)[1]->DeriveTotalDistinctAggs());

	// split SeqPrj expression into a GbAgg expression (for distinct Aggs), and
	// another SeqPrj expression (for remaining window functions)
	gpos::Ref<CExpression> pexprGbAgg = nullptr;
	gpos::Ref<CExpression> pexprWindow = nullptr;
	SplitSeqPrj(mp, pexprSeqPrj, &pexprGbAgg, &pexprWindow);

	// create CTE using SeqPrj child expression
	gpos::Ref<CExpression> pexprGbAggConsumer = nullptr;
	gpos::Ref<CExpression> pexprWindowConsumer = nullptr;
	CreateCTE(mp, pexprSeqPrj, &pexprGbAggConsumer, &pexprWindowConsumer);

	// extract output columns of SeqPrj child expression
	CExpression *pexprChild = (*pexprSeqPrj)[0];
	gpos::Ref<CColRefArray> pdrgpcrChildOutput =
		pexprChild->DeriveOutputColumns()->Pdrgpcr(mp);

	// to match requested columns upstream, we have to re-use the same computed
	// columns that define the aggregates, we avoid recreating new columns during
	// expression copy by passing must_exist as false
	CColRefArray *pdrgpcrConsumerOutput =
		gpos::dyn_cast<CLogicalCTEConsumer>(pexprGbAggConsumer->Pop())
			->Pdrgpcr();
	gpos::Ref<UlongToColRefMap> colref_mapping = CUtils::PhmulcrMapping(
		mp, pdrgpcrChildOutput.get(), pdrgpcrConsumerOutput);
	gpos::Ref<CExpression> pexprGbAggRemapped =
		pexprGbAgg->PexprCopyWithRemappedColumns(mp, colref_mapping,
												 false /*must_exist*/);
	;
	;
	;

	// finalize GbAgg expression by replacing its child with CTE consumer
	;
	;
	gpos::Ref<CExpression> pexprGbAggWithConsumer =
		GPOS_NEW(mp) CExpression(mp, pexprGbAggRemapped->Pop(),
								 pexprGbAggConsumer, (*pexprGbAggRemapped)[1]);
	;

	// in case of multiple Distinct Aggs, we need to expand the GbAgg expression
	// into a join expression where leaves carry single Distinct Aggs
	gpos::Ref<CExpression> pexprJoinDQAs =
		CXformUtils::PexprGbAggOnCTEConsumer2Join(mp,
												  pexprGbAggWithConsumer.get());
	;

	gpos::Ref<CExpression> pexprWindowFinal = nullptr;
	if (COperator::EopLogicalSequenceProject == pexprWindow->Pop()->Eopid())
	{
		// create a new SeqPrj expression for remaining window functions,
		// and replace expression child withCTE consumer
		;
		;
		pexprWindowFinal = GPOS_NEW(mp) CExpression(
			mp, pexprWindow->Pop(), pexprWindowConsumer, (*pexprWindow)[1]);
	}
	else
	{
		// no remaining window functions, simply reuse created CTE consumer
		pexprWindowFinal = pexprWindowConsumer;
	};

	// extract grouping columns from created join expression
	CColRefArray *pdrgpcrGrpCols = PdrgpcrGrpCols(pexprJoinDQAs.get());

	// create final join condition
	gpos::Ref<CExpression> pexprJoinCondition = nullptr;

	if (nullptr != pdrgpcrGrpCols && 0 < pdrgpcrGrpCols->Size())
	{
		// extract PARTITION BY columns from original SeqPrj expression
		CLogicalSequenceProject *popSeqPrj =
			gpos::dyn_cast<CLogicalSequenceProject>(pexprSeqPrj->Pop());
		CDistributionSpec *pds = popSeqPrj->Pds();
		gpos::Ref<CColRefSet> pcrs = CUtils::PcrsExtractColumns(
			mp, gpos::dyn_cast<CDistributionSpecHashed>(pds)->Pdrgpexpr());
		gpos::Ref<CColRefArray> pdrgpcrPartitionBy = pcrs->Pdrgpcr(mp);
		;
		GPOS_ASSERT(
			pdrgpcrGrpCols->Size() == pdrgpcrPartitionBy->Size() &&
			"Partition By columns in window function are not the same as grouping columns in created Aggs");

		// create a conjunction of INDF expressions comparing a GROUP BY column to a PARTITION BY column
		pexprJoinCondition = CPredicateUtils::PexprINDFConjunction(
			mp, pdrgpcrGrpCols, pdrgpcrPartitionBy.get());
		;
	}
	else
	{
		// no PARTITION BY, join condition is const True
		pexprJoinCondition = CUtils::PexprScalarConstBool(mp, true /*value*/);
	}

	// create a join between expanded DQAs and Window expressions
	gpos::Ref<CExpression> pexprJoin =
		CUtils::PexprLogicalJoin<CLogicalInnerJoin>(
			mp, std::move(pexprJoinDQAs), std::move(pexprWindowFinal),
			std::move(pexprJoinCondition));

	ULONG ulCTEId =
		gpos::dyn_cast<CLogicalCTEConsumer>(pexprGbAggConsumer->Pop())
			->UlCTEId();
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalCTEAnchor(mp, ulCTEId), std::move(pexprJoin));
}


//---------------------------------------------------------------------------
//	@function:
//		CWindowPreprocessor::PexprPreprocess
//
//	@doc:
//		Main driver
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CWindowPreprocessor::PexprPreprocess(CMemoryPool *mp, CExpression *pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	COperator *pop = pexpr->Pop();
	if (COperator::EopLogicalSequenceProject == pop->Eopid() &&
		0 < (*pexpr)[1]->DeriveTotalDistinctAggs())
	{
		gpos::Ref<CExpression> pexprJoin = PexprSeqPrj2Join(mp, pexpr);

		// recursively process the resulting expression
		gpos::Ref<CExpression> pexprResult =
			PexprPreprocess(mp, pexprJoin.get());
		;

		return pexprResult;
	}

	// recursively process child expressions
	const ULONG arity = pexpr->Arity();
	gpos::Ref<CExpressionArray> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild = PexprPreprocess(mp, (*pexpr)[ul]);
		pdrgpexprChildren->Append(pexprChild);
	}

	;
	return GPOS_NEW(mp) CExpression(mp, pop, std::move(pdrgpexprChildren));
}

// EOF
