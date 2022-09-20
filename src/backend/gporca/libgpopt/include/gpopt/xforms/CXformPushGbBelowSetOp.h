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
		gpos::pointer<CLogicalGbAgg *> popGbAgg =
			gpos::dyn_cast<CLogicalGbAgg>(exprhdl.Pop());
		if (popGbAgg->FGlobal())
		{
			return ExfpHigh;
		}

		return ExfpNone;
	}

	// actual transform
	void
	Transform(gpos::pointer<CXformContext *> pxfctxt,
			  gpos::pointer<CXformResult *> pxfres,
			  gpos::pointer<CExpression *> pexpr) const override
	{
		GPOS_ASSERT(nullptr != pxfctxt);
		GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
		GPOS_ASSERT(FCheckPattern(pexpr));

		CMemoryPool *mp = pxfctxt->Pmp();
		gpos::pointer<COptimizerConfig *> optconfig =
			COptCtxt::PoctxtFromTLS()->GetOptimizerConfig();

		gpos::pointer<CExpression *> pexprSetOp = (*pexpr)[0];
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
		gpos::pointer<CLogicalSetOp *> popSetOp =
			gpos::dyn_cast<CLogicalSetOp>(pexprSetOp->Pop());

		CColRefArray *pdrgpcrGb = popGbAgg->Pdrgpcr();
		gpos::pointer<CColRefArray *> pdrgpcrOutput = popSetOp->PdrgpcrOutput();
		gpos::owner<CColRefSet *> pcrsOutput =
			GPOS_NEW(mp) CColRefSet(mp, pdrgpcrOutput);
		gpos::pointer<CColRef2dArray *> pdrgpdrgpcrInput =
			popSetOp->PdrgpdrgpcrInput();
		gpos::owner<CExpressionArray *> pdrgpexprNewChildren =
			GPOS_NEW(mp) CExpressionArray(mp);
		gpos::owner<CColRef2dArray *> pdrgpdrgpcrNewInput =
			GPOS_NEW(mp) CColRef2dArray(mp);
		const ULONG arity = pexprSetOp->Arity();

		BOOL fNewChild = false;

		for (ULONG ulChild = 0; ulChild < arity; ulChild++)
		{
			CExpression *pexprChild = (*pexprSetOp)[ulChild];
			gpos::pointer<CColRefArray *> pdrgpcrChild =
				(*pdrgpdrgpcrInput)[ulChild];
			gpos::owner<CColRefSet *> pcrsChild =
				GPOS_NEW(mp) CColRefSet(mp, pdrgpcrChild);

			gpos::owner<CColRefArray *> pdrgpcrChildGb = nullptr;
			if (!pcrsChild->Equals(pcrsOutput))
			{
				// use column mapping in SetOp to set child grouping colums
				gpos::owner<UlongToColRefMap *> colref_mapping =
					CUtils::PhmulcrMapping(mp, pdrgpcrOutput, pdrgpcrChild);
				pdrgpcrChildGb = CUtils::PdrgpcrRemap(
					mp, pdrgpcrGb, colref_mapping, true /*must_exist*/);
				colref_mapping->Release();
			}
			else
			{
				// use grouping columns directly as child grouping colums
				pdrgpcrGb->AddRef();
				pdrgpcrChildGb = pdrgpcrGb;
			}

			pexprChild->AddRef();
			pcrsChild->Release();

			// if child of setop is already an Agg with the same grouping columns
			// that we want to use, there is no need to add another agg on top of it
			gpos::pointer<COperator *> popChild = pexprChild->Pop();
			if (COperator::EopLogicalGbAgg == popChild->Eopid())
			{
				gpos::pointer<CLogicalGbAgg *> popGbAgg =
					gpos::dyn_cast<CLogicalGbAgg>(popChild);
				if (CColRef::Equals(popGbAgg->Pdrgpcr(), pdrgpcrChildGb))
				{
					pdrgpexprNewChildren->Append(pexprChild);
					pdrgpdrgpcrNewInput->Append(pdrgpcrChildGb);

					continue;
				}
			}

			fNewChild = true;
			pexprPrjList->AddRef();
			gpos::owner<CExpression *> pexprChildGb =
				CUtils::PexprLogicalGbAggGlobal(mp, pdrgpcrChildGb, pexprChild,
												pexprPrjList);
			pdrgpexprNewChildren->Append(pexprChildGb);

			pdrgpcrChildGb->AddRef();
			pdrgpdrgpcrNewInput->Append(pdrgpcrChildGb);
		}

		pcrsOutput->Release();

		if (!fNewChild)
		{
			// all children of the union were already Aggs with the same grouping
			// columns that we would have created. No new alternative expressions
			pdrgpdrgpcrNewInput->Release();
			pdrgpexprNewChildren->Release();

			return;
		}

		pdrgpcrGb->AddRef();
		gpos::owner<TSetOp *> popSetOpNew =
			GPOS_NEW(mp) TSetOp(mp, pdrgpcrGb, std::move(pdrgpdrgpcrNewInput));
		gpos::owner<CExpression *> pexprNewSetOp = GPOS_NEW(mp) CExpression(
			mp, std::move(popSetOpNew), std::move(pdrgpexprNewChildren));

		popGbAgg->AddRef();
		pexprPrjList->AddRef();
		gpos::owner<CExpression *> pexprResult = GPOS_NEW(mp)
			CExpression(mp, popGbAgg, std::move(pexprNewSetOp), pexprPrjList);

		pxfres->Add(std::move(pexprResult));
	}

};	// class CXformPushGbBelowSetOp

}  // namespace gpopt

#endif	// !GPOPT_CXformPushGbBelowSetOp_H

// EOF
