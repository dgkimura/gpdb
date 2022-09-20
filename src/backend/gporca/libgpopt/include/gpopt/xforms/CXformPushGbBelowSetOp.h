//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 VMware, Inc. or its affiliates.
//
//	@filename:
//		CXformPushGbBelowSetOp.h
//
//	@doc:
//		Push grouping below set operation
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformPushGbBelowSetOp_H
#define GPOPT_CXformPushGbBelowSetOp_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CPatternMultiLeaf.h"
#include "gpopt/operators/CPatternTree.h"
#include "gpopt/optimizer/COptimizerConfig.h"
#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CXformPushGbBelowSetOp
//
//	@doc:
//		Push grouping below set operation
//
//---------------------------------------------------------------------------
template <class TSetOp>
class CXformPushGbBelowSetOp : public CXformExploration
{
private:
public:
	CXformPushGbBelowSetOp(const CXformPushGbBelowSetOp &) = delete;

	// ctor
	explicit CXformPushGbBelowSetOp(CMemoryPool *mp)
		: CXformExploration(
			  // pattern
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CLogicalGbAgg(mp),
				  GPOS_NEW(mp) CExpression	// left child is a set operation
				  (mp, GPOS_NEW(mp) TSetOp(mp),
				   GPOS_NEW(mp)
					   CExpression(mp, GPOS_NEW(mp) CPatternMultiLeaf(mp))),
				  GPOS_NEW(mp) CExpression(
					  mp, GPOS_NEW(mp)
							  CPatternTree(mp))	 // project list of group-by
				  ))
	{
	}

	// dtor
	~CXformPushGbBelowSetOp() override = default;

	// compute xform promise for a given expression handle
	EXformPromise
	Exfp(CExpressionHandle &exprhdl) const override
	{
		CLogicalGbAgg *popGbAgg = gpos::dyn_cast<CLogicalGbAgg>(exprhdl.Pop());
		if (popGbAgg->FGlobal())
		{
			return ExfpHigh;
		}

		return ExfpNone;
	}

	// actual transform
	void
	Transform(CXformContext *pxfctxt, CXformResult *pxfres,
			  CExpression *pexpr) const override
	{
		GPOS_ASSERT(nullptr != pxfctxt);
		GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
		GPOS_ASSERT(FCheckPattern(pexpr));

		CMemoryPool *mp = pxfctxt->Pmp();
		COptimizerConfig *optconfig =
			COptCtxt::PoctxtFromTLS()->GetOptimizerConfig();

		CExpression *pexprSetOp = (*pexpr)[0];
		if (pexprSetOp->Arity() >
			optconfig->GetHint()->UlPushGroupByBelowSetopThreshold())
		{
			// bail-out if set op has many children
			return;
		}
		CExpression *pexprPrjList = (*pexpr)[1];
		if (0 < pexprPrjList->Arity())
		{
			// bail-out if group-by has any aggregate functions
			return;
		}

		CLogicalGbAgg *popGbAgg = gpos::dyn_cast<CLogicalGbAgg>(pexpr->Pop());
		CLogicalSetOp *popSetOp =
			gpos::dyn_cast<CLogicalSetOp>(pexprSetOp->Pop());

		CColRefArray *pdrgpcrGb = popGbAgg->Pdrgpcr();
		CColRefArray *pdrgpcrOutput = popSetOp->PdrgpcrOutput();
		gpos::Ref<CColRefSet> pcrsOutput =
			GPOS_NEW(mp) CColRefSet(mp, pdrgpcrOutput);
		CColRef2dArray *pdrgpdrgpcrInput = popSetOp->PdrgpdrgpcrInput();
		gpos::Ref<CExpressionArray> pdrgpexprNewChildren =
			GPOS_NEW(mp) CExpressionArray(mp);
		gpos::Ref<CColRef2dArray> pdrgpdrgpcrNewInput =
			GPOS_NEW(mp) CColRef2dArray(mp);
		const ULONG arity = pexprSetOp->Arity();

		BOOL fNewChild = false;

		for (ULONG ulChild = 0; ulChild < arity; ulChild++)
		{
			CExpression *pexprChild = (*pexprSetOp)[ulChild];
			CColRefArray *pdrgpcrChild = (*pdrgpdrgpcrInput)[ulChild].get();
			gpos::Ref<CColRefSet> pcrsChild =
				GPOS_NEW(mp) CColRefSet(mp, pdrgpcrChild);

			gpos::Ref<CColRefArray> pdrgpcrChildGb = nullptr;
			if (!pcrsChild->Equals(pcrsOutput.get()))
			{
				// use column mapping in SetOp to set child grouping colums
				gpos::Ref<UlongToColRefMap> colref_mapping =
					CUtils::PhmulcrMapping(mp, pdrgpcrOutput, pdrgpcrChild);
				pdrgpcrChildGb = CUtils::PdrgpcrRemap(
					mp, pdrgpcrGb, colref_mapping.get(), true /*must_exist*/);
				;
			}
			else
			{
				// use grouping columns directly as child grouping colums
				;
				pdrgpcrChildGb = pdrgpcrGb;
			}

			;
			;

			// if child of setop is already an Agg with the same grouping columns
			// that we want to use, there is no need to add another agg on top of it
			COperator *popChild = pexprChild->Pop();
			if (COperator::EopLogicalGbAgg == popChild->Eopid())
			{
				CLogicalGbAgg *popGbAgg =
					gpos::dyn_cast<CLogicalGbAgg>(popChild);
				if (CColRef::Equals(popGbAgg->Pdrgpcr(), pdrgpcrChildGb.get()))
				{
					pdrgpexprNewChildren->Append(pexprChild);
					pdrgpdrgpcrNewInput->Append(pdrgpcrChildGb);

					continue;
				}
			}

			fNewChild = true;
			;
			gpos::Ref<CExpression> pexprChildGb =
				CUtils::PexprLogicalGbAggGlobal(mp, pdrgpcrChildGb, pexprChild,
												pexprPrjList);
			pdrgpexprNewChildren->Append(pexprChildGb);

			;
			pdrgpdrgpcrNewInput->Append(pdrgpcrChildGb);
		}

		;

		if (!fNewChild)
		{
			// all children of the union were already Aggs with the same grouping
			// columns that we would have created. No new alternative expressions
			;
			;

			return;
		}

		;
		gpos::Ref<TSetOp> popSetOpNew =
			GPOS_NEW(mp) TSetOp(mp, pdrgpcrGb, std::move(pdrgpdrgpcrNewInput));
		gpos::Ref<CExpression> pexprNewSetOp = GPOS_NEW(mp) CExpression(
			mp, std::move(popSetOpNew), std::move(pdrgpexprNewChildren));

		;
		;
		gpos::Ref<CExpression> pexprResult = GPOS_NEW(mp)
			CExpression(mp, popGbAgg, std::move(pexprNewSetOp), pexprPrjList);

		pxfres->Add(std::move(pexprResult));
	}

};	// class CXformPushGbBelowSetOp

}  // namespace gpopt

#endif	// !GPOPT_CXformPushGbBelowSetOp_H

// EOF
