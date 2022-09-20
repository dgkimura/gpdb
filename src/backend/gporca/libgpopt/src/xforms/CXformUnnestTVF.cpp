//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CXformUnnestTVF.cpp
//
//	@doc:
//		Implementation of TVF unnesting xform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformUnnestTVF.h"

#include "gpos/base.h"
#include "gpos/common/CHashMap.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalCTEAnchor.h"
#include "gpopt/operators/CLogicalCTEConsumer.h"
#include "gpopt/operators/CLogicalLeftOuterCorrelatedApply.h"
#include "gpopt/operators/CLogicalTVF.h"
#include "gpopt/operators/CPatternMultiTree.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformUnnestTVF::CXformUnnestTVF
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformUnnestTVF::CXformUnnestTVF(CMemoryPool *mp)
	: CXformExploration(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalTVF(mp),
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternMultiTree(
						  mp))	// variable number of args, each is a deep tree
			  ))
{
}


//---------------------------------------------------------------------------
//	@function:
//		CXformUnnestTVF::Exfp
//
//	@doc:
//		Compute xform promise for a given expression handle
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformUnnestTVF::Exfp(CExpressionHandle &exprhdl) const
{
	const ULONG arity = exprhdl.Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		if (exprhdl.DeriveHasSubquery(ul))
		{
			// xform is applicable if TVF argument is a subquery
			return CXform::ExfpHigh;
		}
	}

	return CXform::ExfpNone;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformUnnestTVF::PdrgpcrSubqueries
//
//	@doc:
//		Return array of subquery column references in CTE consumer output
//		after mapping to consumer output
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefArray>
CXformUnnestTVF::PdrgpcrSubqueries(CMemoryPool *mp, CExpression *pexprCTEProd,
								   CExpression *pexprCTECons)
{
	CExpression *pexprProject = (*pexprCTEProd)[0];
	GPOS_ASSERT(COperator::EopLogicalProject == pexprProject->Pop()->Eopid());

	gpos::Ref<CColRefArray> pdrgpcrProdOutput =
		pexprCTEProd->DeriveOutputColumns()->Pdrgpcr(mp);
	gpos::Ref<CColRefArray> pdrgpcrConsOutput =
		pexprCTECons->DeriveOutputColumns()->Pdrgpcr(mp);
	GPOS_ASSERT(pdrgpcrProdOutput->Size() == pdrgpcrConsOutput->Size());

	gpos::Ref<CColRefArray> colref_array = GPOS_NEW(mp) CColRefArray(mp);
	const ULONG ulPrjElems = (*pexprProject)[1]->Arity();
	for (ULONG ulOuter = 0; ulOuter < ulPrjElems; ulOuter++)
	{
		CExpression *pexprPrjElem = (*(*pexprProject)[1])[ulOuter];
		if ((*pexprPrjElem)[0]->DeriveHasSubquery())
		{
			CColRef *pcrProducer =
				gpos::dyn_cast<CScalarProjectElement>(pexprPrjElem->Pop())
					->Pcr();
			CColRef *pcrConsumer = CUtils::PcrMap(
				pcrProducer, pdrgpcrProdOutput.get(), pdrgpcrConsOutput.get());
			GPOS_ASSERT(nullptr != pcrConsumer);

			colref_array->Append(pcrConsumer);
		}
	}

	;
	;

	return colref_array;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformUnnestTVF::PexprProjectSubqueries
//
//	@doc:
//		Collect subquery arguments and return a Project expression with
//		collected subqueries in project list
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CXformUnnestTVF::PexprProjectSubqueries(CMemoryPool *mp, CExpression *pexprTVF)
{
	GPOS_ASSERT(COperator::EopLogicalTVF == pexprTVF->Pop()->Eopid());

	// collect subquery arguments
	gpos::Ref<CExpressionArray> pdrgpexprSubqueries =
		GPOS_NEW(mp) CExpressionArray(mp);
	const ULONG arity = pexprTVF->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		CExpression *pexprScalarChild = (*pexprTVF)[ul];
		if (pexprScalarChild->DeriveHasSubquery())
		{
			;
			pdrgpexprSubqueries->Append(pexprScalarChild);
		}
	}
	GPOS_ASSERT(0 < pdrgpexprSubqueries->Size());

	gpos::Ref<CExpression> pexprCTG = CUtils::PexprLogicalCTGDummy(mp);
	gpos::Ref<CExpression> pexprProject = CUtils::PexprAddProjection(
		mp, std::move(pexprCTG), pdrgpexprSubqueries.get());
	;

	return pexprProject;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformUnnestTVF::Transform
//
//	@doc:
//		Actual transformation
//		for queries of the form 'SELECT * FROM func(1, (select 5))'
//		we generate a correlated CTE expression to execute subquery args
//		in different subplans, the resulting expression looks like this
//
//		+--CLogicalCTEAnchor (0)
//		   +--CLogicalLeftOuterCorrelatedApply
//			  |--CLogicalTVF (func) Columns: ["a" (0), "b" (1)]
//			  |  |--CScalarConst (1)     <-- constant arg
//			  |  +--CScalarIdent "ColRef_0005" (11)    <-- subquery arg replaced by column
//			  |--CLogicalCTEConsumer (0), Columns: ["ColRef_0005" (11)]
//			  +--CScalarConst (1)
//
//		where CTE(0) is a Project expression on subquery args
//
//
//---------------------------------------------------------------------------
void
CXformUnnestTVF::Transform(CXformContext *pxfctxt, CXformResult *pxfres,
						   CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	// create a project expression on subquery arguments
	gpos::Ref<CExpression> pexprProject = PexprProjectSubqueries(mp, pexpr);

	// create a CTE producer on top of the project
	CCTEInfo *pcteinfo = COptCtxt::PoctxtFromTLS()->Pcteinfo();
	const ULONG ulCTEId = pcteinfo->next_id();

	// construct CTE producer output from subquery columns
	gpos::Ref<CColRefArray> pdrgpcrOutput = GPOS_NEW(mp) CColRefArray(mp);
	const ULONG ulPrjElems = (*pexprProject)[1]->Arity();
	for (ULONG ulOuter = 0; ulOuter < ulPrjElems; ulOuter++)
	{
		CExpression *pexprPrjElem = (*(*pexprProject)[1])[ulOuter];
		if ((*pexprPrjElem)[0]->DeriveHasSubquery())
		{
			CColRef *pcrSubq =
				gpos::dyn_cast<CScalarProjectElement>(pexprPrjElem->Pop())
					->Pcr();
			pdrgpcrOutput->Append(pcrSubq);
		}
	}

	CExpression *pexprCTEProd = CXformUtils::PexprAddCTEProducer(
		mp, ulCTEId, pdrgpcrOutput.get(), pexprProject.get());
	;
	;

	// create CTE consumer
	gpos::Ref<CColRefArray> pdrgpcrProducerOutput =
		pexprCTEProd->DeriveOutputColumns()->Pdrgpcr(mp);
	gpos::Ref<CColRefArray> pdrgpcrConsumerOutput =
		CUtils::PdrgpcrCopy(mp, pdrgpcrProducerOutput.get());
	gpos::Ref<CLogicalCTEConsumer> popConsumer =
		GPOS_NEW(mp) CLogicalCTEConsumer(mp, ulCTEId, pdrgpcrConsumerOutput);
	gpos::Ref<CExpression> pexprCTECons =
		GPOS_NEW(mp) CExpression(mp, popConsumer);
	pcteinfo->IncrementConsumers(ulCTEId);
	;

	// find columns corresponding to subqueries in consumer's output
	gpos::Ref<CColRefArray> pdrgpcrSubqueries =
		PdrgpcrSubqueries(mp, pexprCTEProd, pexprCTECons.get());

	// create new function arguments by replacing subqueries with columns in CTE consumer output
	gpos::Ref<CExpressionArray> pdrgpexprNewArgs =
		GPOS_NEW(mp) CExpressionArray(mp);
	ULONG ulIndex = 0;
	const ULONG arity = pexpr->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		CExpression *pexprScalarChild = (*pexpr)[ul];
		if (pexprScalarChild->DeriveHasSubquery())
		{
			CColRef *colref = (*pdrgpcrSubqueries)[ulIndex];
			pdrgpexprNewArgs->Append(CUtils::PexprScalarIdent(mp, colref));
			ulIndex++;
		}
		else
		{
			;
			pdrgpexprNewArgs->Append((*pexpr)[ul]);
		}
	}

	// finally, create correlated apply expression
	gpos::Ref<CLogicalTVF> popTVF = gpos::dyn_cast<CLogicalTVF>(pexpr->Pop());
	;
	gpos::Ref<CExpression> pexprCorrApply =
		CUtils::PexprLogicalApply<CLogicalLeftOuterCorrelatedApply>(
			mp,
			GPOS_NEW(mp)
				CExpression(mp, std::move(popTVF), std::move(pdrgpexprNewArgs)),
			std::move(pexprCTECons), std::move(pdrgpcrSubqueries),
			COperator::EopScalarSubquery,
			CPredicateUtils::PexprConjunction(
				mp, nullptr /*pdrgpexpr*/)	// scalar expression is const True
		);

	gpos::Ref<CExpression> pexprAlt = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CLogicalCTEAnchor(mp, ulCTEId),
					std::move(pexprCorrApply));

	// add alternative to transformation result
	pxfres->Add(std::move(pexprAlt));
}


// EOF
