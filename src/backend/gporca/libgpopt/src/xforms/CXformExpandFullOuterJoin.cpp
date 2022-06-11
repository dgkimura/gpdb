//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CXformExpandFullOuterJoin.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformExpandFullOuterJoin.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CLogicalCTEAnchor.h"
#include "gpopt/operators/CLogicalCTEConsumer.h"
#include "gpopt/operators/CLogicalFullOuterJoin.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CPatternTree.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformExpandFullOuterJoin::CXformExpandFullOuterJoin
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformExpandFullOuterJoin::CXformExpandFullOuterJoin(CMemoryPool *mp)
	: CXformExploration(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalFullOuterJoin(mp),
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternTree(mp)),  // outer child
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternTree(mp)),  // inner child
			  GPOS_NEW(mp) CExpression(
				  mp, GPOS_NEW(mp) CPatternTree(mp))  // scalar child
			  ))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformExpandFullOuterJoin::Exfp
//
//	@doc:
//		Compute promise of xform
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformExpandFullOuterJoin::Exfp(CExpressionHandle &	 //exprhdl
) const
{
	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformExpandFullOuterJoin::Transform
//
//	@doc:
//		Actual transformation
// 		The expression A FOJ B is translated to:
//
//		CTEAnchor(cteA)
//		+-- CTEAnchor(cteB)
//			+--UnionAll
//				|--	LOJ
//				|	|--	CTEConsumer(cteA)
//				|	+--	CTEConsumer(cteB)
//				+--	Project
//					+--	LASJ
//					|	|--	CTEConsumer(cteB)
//					|	+--	CTEConsumer(cteA)
//					+-- (NULLS - same schema of A)
//
//		Also, two CTE producers for cteA and cteB are added to CTE info
//
//---------------------------------------------------------------------------
void
CXformExpandFullOuterJoin::Transform(gpos::pointer<CXformContext *> pxfctxt,
									 gpos::pointer<CXformResult *> pxfres,
									 gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	gpos::pointer<CExpression *> pexprA = (*pexpr)[0];
	gpos::pointer<CExpression *> pexprB = (*pexpr)[1];
	CExpression *pexprScalar = (*pexpr)[2];

	// 1. create the CTE producers
	const ULONG ulCTEIdA = COptCtxt::PoctxtFromTLS()->Pcteinfo()->next_id();
	gpos::owner<CColRefArray *> pdrgpcrOutA =
		pexprA->DeriveOutputColumns()->Pdrgpcr(mp);
	(void) CXformUtils::PexprAddCTEProducer(mp, ulCTEIdA, pdrgpcrOutA, pexprA);

	const ULONG ulCTEIdB = COptCtxt::PoctxtFromTLS()->Pcteinfo()->next_id();
	gpos::owner<CColRefArray *> pdrgpcrOutB =
		pexprB->DeriveOutputColumns()->Pdrgpcr(mp);
	(void) CXformUtils::PexprAddCTEProducer(mp, ulCTEIdB, pdrgpcrOutB, pexprB);

	// 2. create the right child (PROJECT over LASJ)
	gpos::owner<CColRefArray *> pdrgpcrRightA =
		CUtils::PdrgpcrCopy(mp, pdrgpcrOutA);
	gpos::owner<CColRefArray *> pdrgpcrRightB =
		CUtils::PdrgpcrCopy(mp, pdrgpcrOutB);
	gpos::owner<CExpression *> pexprScalarRight =
		CXformUtils::PexprRemapColumns(mp, pexprScalar, pdrgpcrOutA,
									   pdrgpcrRightA, pdrgpcrOutB,
									   pdrgpcrRightB);
	gpos::owner<CExpression *> pexprLASJ = PexprLogicalJoinOverCTEs(
		mp, EdxljtLeftAntiSemijoin, ulCTEIdB, pdrgpcrRightB, ulCTEIdA,
		pdrgpcrRightA, std::move(pexprScalarRight));
	gpos::owner<CExpression *> pexprProject = CUtils::PexprLogicalProjectNulls(
		mp, pdrgpcrRightA, std::move(pexprLASJ));

	// 3. create the left child (LOJ) - this has to use the original output
	//    columns and the original scalar expression
	pexprScalar->AddRef();
	gpos::owner<CExpression *> pexprLOJ =
		PexprLogicalJoinOverCTEs(mp, EdxljtLeft, ulCTEIdA, pdrgpcrOutA,
								 ulCTEIdB, pdrgpcrOutB, pexprScalar);

	// 4. create the UNION ALL expression

	// output columns of the union are the same as the outputs of the first child (LOJ)
	gpos::owner<CColRefArray *> pdrgpcrOutput = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrOutput->AppendArray(pdrgpcrOutA);
	pdrgpcrOutput->AppendArray(pdrgpcrOutB);

	// input columns of the union
	gpos::owner<CColRef2dArray *> pdrgdrgpcrInput =
		GPOS_NEW(mp) CColRef2dArray(mp);

	// inputs from the first child (LOJ)
	pdrgpcrOutput->AddRef();
	pdrgdrgpcrInput->Append(pdrgpcrOutput);

	// inputs from the second child have to be in the correct order
	// a. add new computed columns from the project only
	gpos::owner<CColRefSet *> pcrsProjOnly = GPOS_NEW(mp) CColRefSet(mp);
	pcrsProjOnly->Include(pexprProject->DeriveOutputColumns());
	pcrsProjOnly->Exclude(pdrgpcrRightB);
	gpos::owner<CColRefArray *> pdrgpcrProj = pcrsProjOnly->Pdrgpcr(mp);
	pcrsProjOnly->Release();
	// b. add columns from the LASJ expression
	pdrgpcrProj->AppendArray(pdrgpcrRightB);

	pdrgdrgpcrInput->Append(std::move(pdrgpcrProj));

	gpos::owner<CExpression *> pexprUnionAll = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp)
			CLogicalUnionAll(mp, pdrgpcrOutput, std::move(pdrgdrgpcrInput)),
		std::move(pexprLOJ), std::move(pexprProject));

	// 5. Add CTE anchor for the B subtree
	gpos::owner<CExpression *> pexprAnchorB = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CLogicalCTEAnchor(mp, ulCTEIdB),
					std::move(pexprUnionAll));

	// 6. Add CTE anchor for the A subtree
	gpos::owner<CExpression *> pexprAnchorA = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CLogicalCTEAnchor(mp, ulCTEIdA),
					std::move(pexprAnchorB));

	// add alternative to xform result
	pxfres->Add(std::move(pexprAnchorA));
}

//---------------------------------------------------------------------------
//	@function:
//		CXformExpandFullOuterJoin::PexprLogicalJoinOverCTEs
//
//	@doc:
//		Construct a join expression of two CTEs using the given CTE ids
// 		and output columns
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CXformExpandFullOuterJoin::PexprLogicalJoinOverCTEs(
	CMemoryPool *mp, EdxlJoinType edxljointype, ULONG ulLeftCTEId,
	gpos::owner<CColRefArray *> pdrgpcrLeft, ULONG ulRightCTEId,
	gpos::owner<CColRefArray *> pdrgpcrRight,
	gpos::owner<CExpression *> pexprScalar)
{
	GPOS_ASSERT(nullptr != pexprScalar);

	gpos::owner<CExpressionArray *> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	gpos::pointer<CCTEInfo *> pcteinfo = COptCtxt::PoctxtFromTLS()->Pcteinfo();

	gpos::owner<CLogicalCTEConsumer *> popConsumerLeft = GPOS_NEW(mp)
		CLogicalCTEConsumer(mp, ulLeftCTEId, std::move(pdrgpcrLeft));
	gpos::owner<CExpression *> pexprLeft =
		GPOS_NEW(mp) CExpression(mp, std::move(popConsumerLeft));
	pcteinfo->IncrementConsumers(ulLeftCTEId);

	gpos::owner<CLogicalCTEConsumer *> popConsumerRight = GPOS_NEW(mp)
		CLogicalCTEConsumer(mp, ulRightCTEId, std::move(pdrgpcrRight));
	gpos::owner<CExpression *> pexprRight =
		GPOS_NEW(mp) CExpression(mp, std::move(popConsumerRight));
	pcteinfo->IncrementConsumers(ulRightCTEId);

	pdrgpexprChildren->Append(std::move(pexprLeft));
	pdrgpexprChildren->Append(std::move(pexprRight));
	pdrgpexprChildren->Append(std::move(pexprScalar));

	return CUtils::PexprLogicalJoin(mp, edxljointype,
									std::move(pdrgpexprChildren));
}

// EOF
