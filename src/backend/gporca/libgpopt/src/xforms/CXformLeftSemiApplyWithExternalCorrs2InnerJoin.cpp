//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CXformLeftSemiApplyWithExternalCorrs2InnerJoin.cpp
//
//	@doc:
//		Implementation of transform;
//		external correlations are correlations in the inner child of LSA
//		that use columns not defined by the outer child of LSA
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformLeftSemiApplyWithExternalCorrs2InnerJoin.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CScalarProjectList.h"

using namespace gpopt;


//
//---------------------------------------------------------------------------
//	@function:
//		CXformLeftSemiApplyWithExternalCorrs2InnerJoin::FSplitCorrelations
//
//	@doc:
//		Helper for splitting correlations into external and residual
//
//---------------------------------------------------------------------------
BOOL
CXformLeftSemiApplyWithExternalCorrs2InnerJoin::FSplitCorrelations(
	CMemoryPool *mp, CExpression *pexprOuter, CExpression *pexprInner,
	CExpressionArray *pdrgpexprAllCorr,
	gpos::Ref<CExpressionArray> *ppdrgpexprExternal,
	gpos::Ref<CExpressionArray> *ppdrgpexprResidual,
	gpos::Ref<CColRefSet> *ppcrsInnerUsed)
{
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);
	GPOS_ASSERT(nullptr != pdrgpexprAllCorr);
	GPOS_ASSERT(nullptr != ppdrgpexprExternal);
	GPOS_ASSERT(nullptr != ppdrgpexprResidual);
	GPOS_ASSERT(nullptr != ppcrsInnerUsed);

	// collect output columns of all children
	CColRefSet *pcrsOuterOuput = pexprOuter->DeriveOutputColumns();
	CColRefSet *pcrsInnerOuput = pexprInner->DeriveOutputColumns();
	gpos::Ref<CColRefSet> pcrsChildren =
		GPOS_NEW(mp) CColRefSet(mp, *pcrsOuterOuput);
	pcrsChildren->Union(pcrsInnerOuput);

	// split correlations into external correlations and residual correlations
	gpos::Ref<CExpressionArray> pdrgpexprExternal =
		GPOS_NEW(mp) CExpressionArray(mp);
	gpos::Ref<CExpressionArray> pdrgpexprResidual =
		GPOS_NEW(mp) CExpressionArray(mp);
	const ULONG ulCorrs = pdrgpexprAllCorr->Size();
	gpos::Ref<CColRefSet> pcrsUsed = GPOS_NEW(mp)
		CColRefSet(mp);	 // set of inner columns used in external correlations
	BOOL fSuccess = true;
	for (ULONG ul = 0; fSuccess && ul < ulCorrs; ul++)
	{
		CExpression *pexprCurrent = (*pdrgpexprAllCorr)[ul];
		gpos::Ref<CColRefSet> pcrsCurrent =
			GPOS_NEW(mp) CColRefSet(mp, *pexprCurrent->DeriveUsedColumns());
		if (pcrsCurrent->IsDisjoint(pcrsOuterOuput) ||
			pcrsCurrent->IsDisjoint(pcrsInnerOuput))
		{
			// add current correlation to external correlation
			;
			pdrgpexprExternal->Append(pexprCurrent);

			// filter used columns from external columns
			pcrsCurrent->Intersection(pcrsInnerOuput);
			pcrsUsed->Union(pcrsCurrent.get());
		}
		else if (pcrsChildren->ContainsAll(pcrsCurrent.get()))
		{
			// add current correlation to regular correlations
			;
			pdrgpexprResidual->Append(pexprCurrent);
		}
		else
		{
			// a mixed correlation (containing both outer columns and external columns)
			fSuccess = false;
		};
	};

	if (!fSuccess || 0 == pdrgpexprExternal->Size())
	{
		// failed to find external correlations
		;
		;
		;

		return false;
	}

	*ppcrsInnerUsed = pcrsUsed;
	*ppdrgpexprExternal = pdrgpexprExternal;
	*ppdrgpexprResidual = pdrgpexprResidual;

	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformLeftSemiApplyWithExternalCorrs2InnerJoin::FDecorrelate
//
//	@doc:
//		Helper for collecting correlations from LSA expression
//
//---------------------------------------------------------------------------
BOOL
CXformLeftSemiApplyWithExternalCorrs2InnerJoin::FDecorrelate(
	CMemoryPool *mp, CExpression *pexpr, gpos::Ref<CExpression> *ppexprInnerNew,
	gpos::Ref<CExpressionArray> *ppdrgpexprCorr)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApply == pexpr->Pop()->Eopid() ||
				COperator::EopLogicalLeftSemiApplyIn == pexpr->Pop()->Eopid());

	GPOS_ASSERT(nullptr != ppexprInnerNew);
	GPOS_ASSERT(nullptr != ppdrgpexprCorr);

	// extract children
	CExpression *pexprOuter = (*pexpr)[0];
	CExpression *pexprInner = (*pexpr)[1];
	CExpression *pexprScalar = (*pexpr)[2];

	// collect all correlations from inner child
	pexprInner->ResetDerivedProperties();
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	if (!CDecorrelator::FProcess(mp, pexprInner, true /* fEqualityOnly */,
								 ppexprInnerNew, pdrgpexpr,
								 pexprOuter->DeriveOutputColumns()))
	{
		// decorrelation failed
		;
		;

		return false;
	}

	// add all original scalar conjuncts to correlations
	gpos::Ref<CExpressionArray> pdrgpexprOriginal =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprScalar);
	CUtils::AddRefAppend(pdrgpexpr.get(), pdrgpexprOriginal.get());
	;

	*ppdrgpexprCorr = pdrgpexpr;

	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformLeftSemiApplyWithExternalCorrs2InnerJoin::PexprDecorrelate
//
//	@doc:
//		Transform LSA into a Gb on top of an InnerJoin, where
//		the grouping columns of Gb are the key of LSA's outer child
//		in addition to columns from LSA's inner child that are used
//		in external correlation;
//		this transformation exposes LSA's inner child columns so that
//		correlations can be pulled-up
//
//		Example:
//
//		Input:
//			LS-Apply
//				|----Get(B)
//				+----Select (C.c = A.a)
//						+----Get(C)
//
//		Output:
//			Select (C.c = A.a)
//				+--Gb(B.key, c)
//					+--InnerJoin
//						|--Get(B)
//						+--Get(C)
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CXformLeftSemiApplyWithExternalCorrs2InnerJoin::PexprDecorrelate(
	CMemoryPool *mp, CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApply == pexpr->Pop()->Eopid() ||
				COperator::EopLogicalLeftSemiApplyIn == pexpr->Pop()->Eopid());

	CExpressionHandle exprhdl(mp);
	exprhdl.Attach(pexpr);

	if (nullptr == exprhdl.DeriveKeyCollection(0 /*child_index*/) ||
		!CUtils::FInnerUsesExternalCols(exprhdl))
	{
		// outer child must have a key and inner child must have external correlations
		return nullptr;
	}

	gpos::Ref<CExpression> pexprInnerNew = nullptr;
	gpos::Ref<CExpressionArray> pdrgpexprAllCorr = nullptr;
	if (!FDecorrelate(mp, pexpr, &pexprInnerNew, &pdrgpexprAllCorr))
	{
		// decorrelation failed
		return nullptr;
	}
	GPOS_ASSERT(nullptr != pdrgpexprAllCorr);

	gpos::Ref<CExpressionArray> pdrgpexprExternal = nullptr;
	gpos::Ref<CExpressionArray> pdrgpexprResidual = nullptr;
	gpos::Ref<CColRefSet> pcrsInnerUsed = nullptr;
	if (!FSplitCorrelations(mp, (*pexpr)[0], pexprInnerNew.get(),
							pdrgpexprAllCorr.get(), &pdrgpexprExternal,
							&pdrgpexprResidual, &pcrsInnerUsed))
	{
		// splitting correlations failed
		;
		;

		return nullptr;
	}
	GPOS_ASSERT(pdrgpexprExternal->Size() + pdrgpexprResidual->Size() ==
				pdrgpexprAllCorr->Size());

	;

	// create an inner join between outer child and decorrelated inner child
	gpos::Ref<CExpression> pexprOuter = (*pexpr)[0];
	;
	gpos::Ref<CExpression> pexprInnerJoin =
		CUtils::PexprLogicalJoin<CLogicalInnerJoin>(
			mp, pexprOuter, std::move(pexprInnerNew),
			CPredicateUtils::PexprConjunction(mp,
											  std::move(pdrgpexprResidual)));

	// add a group by on outer columns + inner columns appearing in external correlations
	gpos::Ref<CColRefArray> pdrgpcrUsed = pcrsInnerUsed->Pdrgpcr(mp);
	;

	gpos::Ref<CColRefArray> pdrgpcrKey = nullptr;
	gpos::Ref<CColRefArray> pdrgpcrGrpCols =
		CUtils::PdrgpcrGroupingKey(mp, pexprOuter.get(), &pdrgpcrKey);
	;  // key is not used here

	pdrgpcrGrpCols->AppendArray(pdrgpcrUsed.get());
	;
	gpos::Ref<CExpression> pexprGb = CUtils::PexprLogicalGbAggGlobal(
		mp, std::move(pdrgpcrGrpCols), std::move(pexprInnerJoin),
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp)));

	// add a top filter for external correlations
	return CUtils::PexprLogicalSelect(
		mp, std::move(pexprGb),
		CPredicateUtils::PexprConjunction(mp, std::move(pdrgpexprExternal)));
}


//---------------------------------------------------------------------------
//	@function:
//		CXformLeftSemiApplyWithExternalCorrs2InnerJoin::Exfp
//
//	@doc:
//		Compute xform promise for a given expression handle;
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformLeftSemiApplyWithExternalCorrs2InnerJoin::Exfp(
	CExpressionHandle &exprhdl) const
{
	// expression must have outer references with external correlations
	if (exprhdl.HasOuterRefs(1 /*child_index*/) &&
		CUtils::FInnerUsesExternalCols(exprhdl))
	{
		return CXform::ExfpHigh;
	}

	return CXform::ExfpNone;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformLeftSemiApplyWithExternalCorrs2InnerJoin::Transform
//
//	@doc:
//		Actual transformation
//
//
//---------------------------------------------------------------------------
void
CXformLeftSemiApplyWithExternalCorrs2InnerJoin::Transform(
	CXformContext *pxfctxt, CXformResult *pxfres, CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	gpos::Ref<CExpression> pexprResult = PexprDecorrelate(mp, pexpr);
	if (nullptr != pexprResult)
	{
		pxfres->Add(std::move(pexprResult));
	}
}

// EOF
