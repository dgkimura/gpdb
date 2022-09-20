//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CSubqueryTestUtils.cpp
//
//	@doc:
//		Implementation of test utility functions
//---------------------------------------------------------------------------

#include "unittest/gpopt/CSubqueryTestUtils.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CAutoOptCtxt.h"
#include "gpopt/base/CQueryContext.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/mdcache/CMDCache.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarSubqueryAll.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "gpopt/operators/CScalarSubqueryExists.h"
#include "gpopt/operators/CScalarSubqueryNotExists.h"
#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/IMDTypeBool.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::GenerateGetExpressions
//
//	@doc:
//		Helper for generating a pair of randomized Get expressions
//
//---------------------------------------------------------------------------
void
CSubqueryTestUtils::GenerateGetExpressions(
	CMemoryPool *mp, gpos::owner<CExpression *> *ppexprOuter,
	gpos::owner<CExpression *> *ppexprInner)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != ppexprOuter);
	GPOS_ASSERT(nullptr != ppexprInner);

	// outer expression
	CWStringConst strNameR(GPOS_WSZ_LIT("Rel1"));
	gpos::owner<CMDIdGPDB *> pmdidR =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID1, 1, 1);
	gpos::owner<CTableDescriptor *> ptabdescR = CTestUtils::PtabdescCreate(
		mp, 3 /*num_cols*/, std::move(pmdidR), CName(&strNameR));
	*ppexprOuter =
		CTestUtils::PexprLogicalGet(mp, std::move(ptabdescR), &strNameR);

	// inner expression
	CWStringConst strNameS(GPOS_WSZ_LIT("Rel2"));
	gpos::owner<CMDIdGPDB *> pmdidS =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID2, 1, 1);
	gpos::owner<CTableDescriptor *> ptabdescS = CTestUtils::PtabdescCreate(
		mp, 3 /*num_cols*/, std::move(pmdidS), CName(&strNameS));
	*ppexprInner =
		CTestUtils::PexprLogicalGet(mp, std::move(ptabdescS), &strNameS);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprJoinWithAggSubquery
//
//	@doc:
//		Generate randomized join expression with a subquery predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprJoinWithAggSubquery(CMemoryPool *mp, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::owner<CExpression *> pexprLeft = nullptr;
	gpos::owner<CExpression *> pexprRight = nullptr;
	GenerateGetExpressions(mp, &pexprLeft, &pexprRight);

	gpos::owner<CExpression *> pexprInner = CTestUtils::PexprLogicalGet(mp);
	gpos::owner<CExpression *> pexprSelect = PexprSelectWithAggSubquery(
		mp, std::move(pexprLeft), std::move(pexprInner), fCorrelated);

	(*pexprSelect)[0]->AddRef();
	(*pexprSelect)[1]->AddRef();

	gpos::owner<CExpression *> pexpr = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CLogicalInnerJoin(mp), (*pexprSelect)[0],
					std::move(pexprRight), (*pexprSelect)[1]);

	pexprSelect->Release();

	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithAggSubquery
//
//	@doc:
//		Generate a Select expression with a subquery equality predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithAggSubquery(
	CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
	gpos::owner<CExpression *> pexprInner, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	// get any column
	gpos::pointer<CColRefSet *> pcrs = pexprOuter->DeriveOutputColumns();
	CColRef *pcrLeft = pcrs->PcrAny();

	// generate agg subquery
	gpos::owner<CExpression *> pexprSubq =
		PexprSubqueryAgg(mp, pexprOuter, std::move(pexprInner), fCorrelated);

	// generate equality predicate
	gpos::owner<CExpression *> pexprPredicate =
		CUtils::PexprScalarEqCmp(mp, pcrLeft, std::move(pexprSubq));

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprPredicate));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithAggSubqueryConstComparison
//
//	@doc:
//		Generate a Select expression with a subquery equality predicate
//		involving constant
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithAggSubqueryConstComparison(
	CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
	gpos::owner<CExpression *> pexprInner, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	// generate agg subquery
	gpos::owner<CExpression *> pexprSubq =
		PexprSubqueryAgg(mp, pexprOuter, std::move(pexprInner), fCorrelated);

	gpos::owner<CExpression *> pexprConst =
		CUtils::PexprScalarConstInt8(mp, 0 /*val*/);

	// generate equality predicate
	gpos::owner<CExpression *> pexprPredicate = CUtils::PexprScalarEqCmp(
		mp, std::move(pexprConst), std::move(pexprSubq));

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprPredicate));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprProjectWithAggSubquery
//
//	@doc:
//		Generate a Project expression with a subquery equality predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprProjectWithAggSubquery(
	CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
	gpos::owner<CExpression *> pexprInner, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	// generate agg subquery
	gpos::owner<CExpression *> pexprSubq =
		PexprSubqueryAgg(mp, pexprOuter, std::move(pexprInner), fCorrelated);

	// generate a computed column
	gpos::pointer<CScalarSubquery *> popSubquery =
		gpos::dyn_cast<CScalarSubquery>(pexprSubq->Pop());
	gpos::pointer<const IMDType *> pmdtype =
		md_accessor->RetrieveType(popSubquery->MdidType());
	CColRef *pcrComputed =
		col_factory->PcrCreate(pmdtype, popSubquery->TypeModifier());

	// generate a scalar project list
	gpos::owner<CExpression *> pexprPrjElem = CUtils::PexprScalarProjectElement(
		mp, pcrComputed, std::move(pexprSubq));
	gpos::owner<CExpression *> pexprPrjList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), std::move(pexprPrjElem));

	return CUtils::PexprLogicalProject(mp, std::move(pexprOuter),
									   std::move(pexprPrjList),
									   false /*fNewComputedCol*/);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithAggSubquery
//
//	@doc:
//		Generate randomized Select expression with a subquery predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithAggSubquery(CMemoryPool *mp,
											   BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::owner<CExpression *> pexprOuter = nullptr;
	gpos::owner<CExpression *> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);

	return PexprSelectWithAggSubquery(mp, std::move(pexprOuter),
									  std::move(pexprInner), fCorrelated);
}

//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithAggSubqueryConstComparison
//
//	@doc:
//		Generate randomized Select expression with a subquery predicate
//		involving constant
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithAggSubqueryConstComparison(CMemoryPool *mp,
															  BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::owner<CExpression *> pexprOuter = nullptr;
	gpos::owner<CExpression *> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);

	return PexprSelectWithAggSubqueryConstComparison(
		mp, std::move(pexprOuter), std::move(pexprInner), fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprProjectWithAggSubquery
//
//	@doc:
//		Generate randomized Project expression with a subquery in project
//		element
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprProjectWithAggSubquery(CMemoryPool *mp,
												BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::owner<CExpression *> pexprOuter = nullptr;
	gpos::owner<CExpression *> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);

	return PexprProjectWithAggSubquery(mp, std::move(pexprOuter),
									   std::move(pexprInner), fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithAggSubqueryOverJoin
//
//	@doc:
//		Generate a random select expression with a subquery over join predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithAggSubqueryOverJoin(CMemoryPool *mp,
													   BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	// generate a pair of get expressions
	gpos::owner<CExpression *> pexprR = nullptr;
	gpos::owner<CExpression *> pexprS = nullptr;
	GenerateGetExpressions(mp, &pexprR, &pexprS);

	// generate outer expression
	CWStringConst strNameT(GPOS_WSZ_LIT("Rel3"));

	gpos::owner<CMDIdGPDB *> pmdidT =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID3, 1, 1);
	gpos::owner<CTableDescriptor *> ptabdescT = CTestUtils::PtabdescCreate(
		mp, 3 /*num_cols*/, pmdidT, CName(&strNameT));
	gpos::owner<CExpression *> pexprT =
		CTestUtils::PexprLogicalGet(mp, ptabdescT, &strNameT);
	CColRef *pcrInner = pexprR->DeriveOutputColumns()->PcrAny();
	CColRef *pcrOuter = pexprT->DeriveOutputColumns()->PcrAny();

	gpos::owner<CExpression *> pexprPred = nullptr;
	if (fCorrelated)
	{
		// generate correlation predicate
		pexprPred = CUtils::PexprScalarEqCmp(mp, pcrInner, pcrOuter);
	}
	else
	{
		pexprPred = CUtils::PexprScalarConstBool(mp, true /*value*/);
	}

	// generate N-Ary join
	gpos::owner<CExpressionArray *> pdrgpexpr =
		GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexpr->Append(std::move(pexprR));
	pdrgpexpr->Append(std::move(pexprS));
	pdrgpexpr->Append(std::move(pexprPred));
	gpos::owner<CExpression *> pexprJoin =
		CTestUtils::PexprLogicalNAryJoin(mp, std::move(pdrgpexpr));

	gpos::owner<CExpression *> pexprSubq = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CScalarSubquery(mp, pcrInner, false /*fGeneratedByExist*/,
									 false /*fGeneratedByQuantified*/),
		std::move(pexprJoin));

	gpos::owner<CExpression *> pexprPredOuter =
		CUtils::PexprScalarEqCmp(mp, pcrOuter, std::move(pexprSubq));
	return CUtils::PexprLogicalSelect(mp, std::move(pexprT),
									  std::move(pexprPredOuter));
}

//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithAnySubquery
//
//	@doc:
//		Generate randomized Select expression with Any subquery predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithAnySubquery(CMemoryPool *mp,
											   BOOL fCorrelated)
{
	return PexprSelectWithSubqueryQuantified(
		mp, COperator::EopScalarSubqueryAny, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithAnySubqueryOverWindow
//
//	@doc:
//		Generate randomized Select expression with Any subquery predicate
//		over window operation
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithAnySubqueryOverWindow(CMemoryPool *mp,
														 BOOL fCorrelated)
{
	return PexprSelectWithSubqueryQuantifiedOverWindow(
		mp, COperator::EopScalarSubqueryAny, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithSubqueryQuantifiedOverWindow
//
//	@doc:
//		Generate randomized Select expression with quantified subquery
//		predicate over window operations
//
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithSubqueryQuantifiedOverWindow(
	CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id);

	gpos::owner<CExpression *> pexprOuter = CTestUtils::PexprLogicalGet(mp);
	gpos::owner<CExpression *> pexprInner =
		CTestUtils::PexprOneWindowFunction(mp);
	gpos::owner<CExpression *> pexprSubqueryQuantified =
		PexprSubqueryQuantified(mp, op_id, pexprOuter, std::move(pexprInner),
								fCorrelated);

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprSubqueryQuantified));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithAllSubquery
//
//	@doc:
//		Generate randomized Select expression with All subquery predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithAllSubquery(CMemoryPool *mp,
											   BOOL fCorrelated)
{
	return PexprSelectWithSubqueryQuantified(
		mp, COperator::EopScalarSubqueryAll, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithAllSubqueryOverWindow
//
//	@doc:
//		Generate randomized Select expression with All subquery predicate
//		over window operation
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithAllSubqueryOverWindow(CMemoryPool *mp,
														 BOOL fCorrelated)
{
	return PexprSelectWithSubqueryQuantifiedOverWindow(
		mp, COperator::EopScalarSubqueryAll, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithAnyAggSubquery
//
//	@doc:
//		Generate randomized Select expression with Any subquery whose inner
//		expression is a GbAgg
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithAnyAggSubquery(CMemoryPool *mp,
												  BOOL fCorrelated)
{
	return PexprSelectWithQuantifiedAggSubquery(
		mp, COperator::EopScalarSubqueryAny, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithAllAggSubquery
//
//	@doc:
//		Generate randomized Select expression with All subquery whose inner
//		expression is a GbAgg
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithAllAggSubquery(CMemoryPool *mp,
												  BOOL fCorrelated)
{
	return PexprSelectWithQuantifiedAggSubquery(
		mp, COperator::EopScalarSubqueryAll, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprProjectWithAnySubquery
//
//	@doc:
//		Generate randomized Project expression with Any subquery predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprProjectWithAnySubquery(CMemoryPool *mp,
												BOOL fCorrelated)
{
	gpos::owner<CExpression *> pexprOuter = nullptr;
	gpos::owner<CExpression *> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);

	return PexprProjectWithSubqueryQuantified(
		mp, std::move(pexprOuter), std::move(pexprInner),
		COperator::EopScalarSubqueryAny, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprProjectWithAllSubquery
//
//	@doc:
//		Generate randomized Project expression with All subquery predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprProjectWithAllSubquery(CMemoryPool *mp,
												BOOL fCorrelated)
{
	gpos::owner<CExpression *> pexprOuter = nullptr;
	gpos::owner<CExpression *> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);

	return PexprProjectWithSubqueryQuantified(
		mp, std::move(pexprOuter), std::move(pexprInner),
		COperator::EopScalarSubqueryAll, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSubqueriesInDifferentContexts
//
//	@doc:
//		Generate a randomized expression with subqueries in both value
//		and filter contexts
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSubqueriesInDifferentContexts(CMemoryPool *mp,
													   BOOL fCorrelated)
{
	gpos::owner<CExpression *> pexprOuter = nullptr;
	gpos::owner<CExpression *> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);
	gpos::owner<CExpression *> pexprSelect = PexprSelectWithAggSubquery(
		mp, std::move(pexprOuter), std::move(pexprInner), fCorrelated);

	gpos::owner<CExpression *> pexprGet = CTestUtils::PexprLogicalGet(mp);
	return PexprProjectWithSubqueryQuantified(
		mp, std::move(pexprSelect), std::move(pexprGet),
		COperator::EopScalarSubqueryAny, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSubqueriesInNullTestContext
//
//	@doc:
//		Generate a randomized expression expression with subquery in null
//		test context
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSubqueriesInNullTestContext(CMemoryPool *mp,
													 BOOL fCorrelated)
{
	gpos::owner<CExpression *> pexprOuter = nullptr;
	gpos::owner<CExpression *> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);

	// generate agg subquery
	gpos::owner<CExpression *> pexprSubq =
		PexprSubqueryAgg(mp, pexprOuter, std::move(pexprInner), fCorrelated);

	// generate Is Not Null predicate
	gpos::owner<CExpression *> pexprPredicate =
		CUtils::PexprIsNotNull(mp, std::move(pexprSubq));

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprPredicate));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithExistsSubquery
//
//	@doc:
//		Generate randomized Select expression with Exists subquery predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithExistsSubquery(CMemoryPool *mp,
												  BOOL fCorrelated)
{
	return PexprSelectWithSubqueryExistential(
		mp, COperator::EopScalarSubqueryExists, fCorrelated);
}

//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithNotExistsSubquery
//
//	@doc:
//		Generate randomized Select expression with Not Exists subquery predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithNotExistsSubquery(CMemoryPool *mp,
													 BOOL fCorrelated)
{
	return PexprSelectWithSubqueryExistential(
		mp, COperator::EopScalarSubqueryNotExists, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithSubqueryDisjuncts
//
//	@doc:
//		Generate randomized select expression with subquery predicates in
//		an OR tree
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithSubqueryDisjuncts(CMemoryPool *mp,
													 BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::owner<CExpression *> pexprOuter = nullptr;
	gpos::owner<CExpression *> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);

	return PexprSelectWithSubqueryBoolOp(mp, std::move(pexprOuter),
										 std::move(pexprInner), fCorrelated,
										 CScalarBoolOp::EboolopOr);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithTrimmableExists
//
//	@doc:
//		Generate randomized Select expression with trimmable Exists
//		subquery predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithTrimmableExists(CMemoryPool *mp,
												   BOOL fCorrelated)
{
	return PexprSelectWithTrimmableExistentialSubquery(
		mp, COperator::EopScalarSubqueryExists, fCorrelated);
}

//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithTrimmableNotExists
//
//	@doc:
//		Generate randomized Select expression with trimmable Not Exists
//		subquery predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithTrimmableNotExists(CMemoryPool *mp,
													  BOOL fCorrelated)
{
	return PexprSelectWithTrimmableExistentialSubquery(
		mp, COperator::EopScalarSubqueryNotExists, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprProjectWithExistsSubquery
//
//	@doc:
//		Generate randomized Project expression with Exists subquery predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprProjectWithExistsSubquery(CMemoryPool *mp,
												   BOOL fCorrelated)
{
	return PexprProjectWithSubqueryExistential(
		mp, COperator::EopScalarSubqueryExists, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprProjectWithNotExistsSubquery
//
//	@doc:
//		Generate randomized Project expression with Not Exists subquery predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprProjectWithNotExistsSubquery(CMemoryPool *mp,
													  BOOL fCorrelated)
{
	return PexprProjectWithSubqueryExistential(
		mp, COperator::EopScalarSubqueryNotExists, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithNestedCmpSubquery
//
//	@doc:
//		Generate randomized Select expression with nested comparisons
//		involving subqueries
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithNestedCmpSubquery(CMemoryPool *mp,
													 BOOL fCorrelated)
{
	gpos::owner<CExpression *> pexprSelectWithSubquery =
		PexprSelectWithAggSubquery(mp, fCorrelated);

	CExpression *pexprLogical = (*pexprSelectWithSubquery)[0];
	gpos::owner<CExpression *> pexprSubqueryPred =
		(*pexprSelectWithSubquery)[1];

	// generate a parent equality predicate
	pexprSubqueryPred->AddRef();
	gpos::owner<CExpression *> pexprPredicate1 = CUtils::PexprScalarEqCmp(
		mp, CUtils::PexprScalarConstBool(mp, true /*value*/),
		std::move(pexprSubqueryPred));

	// add another nesting level
	gpos::owner<CExpression *> pexprPredicate = CUtils::PexprScalarEqCmp(
		mp, CUtils::PexprScalarConstBool(mp, true /*value*/),
		std::move(pexprPredicate1));

	pexprLogical->AddRef();
	pexprSelectWithSubquery->Release();

	return CUtils::PexprLogicalSelect(mp, pexprLogical,
									  std::move(pexprPredicate));
}

//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithCmpSubqueries
//
//	@doc:
//		Generate randomized Select expression with comparison between
//		two subqueries
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithCmpSubqueries(CMemoryPool *mp,
												 BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);
	gpos::owner<CExpression *> pexprOuter = nullptr;
	gpos::owner<CExpression *> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);

	// generate a scalar subquery
	gpos::owner<CExpression *> pexprScalarSubquery1 =
		PexprSubqueryAgg(mp, pexprOuter, std::move(pexprInner), fCorrelated);

	// generate get expression
	CWStringConst strNameT(GPOS_WSZ_LIT("Rel3"));

	gpos::owner<CMDIdGPDB *> pmdidT =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID3, 1, 1);
	gpos::owner<CTableDescriptor *> ptabdescT = CTestUtils::PtabdescCreate(
		mp, 3 /*num_cols*/, std::move(pmdidT), CName(&strNameT));
	gpos::owner<CExpression *> pexprT =
		CTestUtils::PexprLogicalGet(mp, std::move(ptabdescT), &strNameT);

	// generate another scalar subquery
	gpos::owner<CExpression *> pexprScalarSubquery2 =
		PexprSubqueryAgg(mp, pexprOuter, std::move(pexprT), fCorrelated);

	// generate equality predicate between both subqueries
	gpos::owner<CExpression *> pexprPredicate = CUtils::PexprScalarEqCmp(
		mp, std::move(pexprScalarSubquery1), std::move(pexprScalarSubquery2));

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprPredicate));
}

//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithNestedSubquery
//
//	@doc:
//		Generate randomized Select expression with nested subquery predicate
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithNestedSubquery(CMemoryPool *mp,
												  BOOL fCorrelated)
{
	gpos::owner<CExpression *> pexprInner =
		PexprSelectWithAggSubquery(mp, fCorrelated);
	CColRef *pcrInner = pexprInner->DeriveOutputColumns()->PcrAny();

	gpos::owner<CExpression *> pexprOuter = CTestUtils::PexprLogicalGet(mp);
	CColRef *pcrOuter = pexprOuter->DeriveOutputColumns()->PcrAny();

	gpos::owner<CExpression *> pexprSubq = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CScalarSubquery(mp, pcrInner, false /*fGeneratedByExist*/,
									 false /*fGeneratedByQuantified*/),
		std::move(pexprInner));

	gpos::owner<CExpression *> pexprPred =
		CUtils::PexprScalarEqCmp(mp, pcrOuter, std::move(pexprSubq));

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprPred));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithNestedQuantifiedSubqueries
//
//	@doc:
//		Generate a random select expression with nested quantified subqueries
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithNestedQuantifiedSubqueries(
	CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated)
{
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id);

	CWStringConst strName1(GPOS_WSZ_LIT("Rel1"));
	CWStringConst strAlias1(GPOS_WSZ_LIT("Rel1Alias"));
	gpos::owner<CExpression *> pexprOuter1 =
		CTestUtils::PexprLogicalGetNullable(mp, GPOPT_TEST_REL_OID1, &strName1,
											&strAlias1);

	CWStringConst strName2(GPOS_WSZ_LIT("Rel2"));
	CWStringConst strAlias2(GPOS_WSZ_LIT("Rel2Alias"));
	gpos::owner<CExpression *> pexprOuter2 =
		CTestUtils::PexprLogicalGetNullable(mp, GPOPT_TEST_REL_OID2, &strName2,
											&strAlias2);

	CWStringConst strName3(GPOS_WSZ_LIT("Rel3"));
	CWStringConst strAlias3(GPOS_WSZ_LIT("Rel3Alias"));
	gpos::owner<CExpression *> pexprOuter3 =
		CTestUtils::PexprLogicalGetNullable(mp, GPOPT_TEST_REL_OID3, &strName3,
											&strAlias3);

	CWStringConst strName4(GPOS_WSZ_LIT("Rel4"));
	CWStringConst strAlias4(GPOS_WSZ_LIT("Rel4Alias"));
	gpos::owner<CExpression *> pexprInner = CTestUtils::PexprLogicalGetNullable(
		mp, GPOPT_TEST_REL_OID4, &strName4, &strAlias4);

	gpos::owner<CExpression *> pexprSubqueryQuantified1 =
		PexprSubqueryQuantified(mp, op_id, pexprOuter3, std::move(pexprInner),
								fCorrelated);
	gpos::owner<CExpression *> pexprSelect1 = CUtils::PexprLogicalSelect(
		mp, std::move(pexprOuter3), std::move(pexprSubqueryQuantified1));
	gpos::owner<CExpression *> pexprSubqueryQuantified2 =
		PexprSubqueryQuantified(mp, op_id, pexprOuter2, std::move(pexprSelect1),
								fCorrelated);
	gpos::owner<CExpression *> pexprSelect2 = CUtils::PexprLogicalSelect(
		mp, std::move(pexprOuter2), std::move(pexprSubqueryQuantified2));
	gpos::owner<CExpression *> pexprSubqueryQuantified3 =
		PexprSubqueryQuantified(mp, op_id, pexprOuter1, std::move(pexprSelect2),
								fCorrelated);

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter1),
									  std::move(pexprSubqueryQuantified3));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithNestedAnySubqueries
//
//	@doc:
//		Generate a random select expression with nested Any subqueries
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithNestedAnySubqueries(CMemoryPool *mp,
													   BOOL fCorrelated)
{
	return PexprSelectWithNestedQuantifiedSubqueries(
		mp, COperator::EopScalarSubqueryAny, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithNestedAllSubqueries
//
//	@doc:
//		Generate a random select expression with nested All subqueries
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithNestedAllSubqueries(CMemoryPool *mp,
													   BOOL fCorrelated)
{
	return PexprSelectWithNestedQuantifiedSubqueries(
		mp, COperator::EopScalarSubqueryAll, fCorrelated);
}



//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWith2LevelsCorrSubquery
//
//	@doc:
//		Generate randomized select expression with 2-levels correlated subqueries
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWith2LevelsCorrSubquery(CMemoryPool *mp,
													   BOOL fCorrelated)
{
	gpos::owner<CExpression *> pexpr =
		PexprSelectWithNestedSubquery(mp, fCorrelated);
	if (fCorrelated)
	{
		// add a 2-level correlation
		gpos::pointer<CExpression *> pexprOuterSubq = (*(*pexpr)[1])[1];
		gpos::pointer<CExpression *> pexprInnerSubq =
			(*(*(*pexprOuterSubq)[0])[1])[1];
		gpos::pointer<CExpression *> pexprInnerSelect =
			(*(*pexprInnerSubq)[0])[0];
		gpos::pointer<CExpressionArray *> pdrgpexpr =
			(*pexprInnerSelect)[1]->PdrgPexpr();

		CColRef *pcrOuter = pexpr->DeriveOutputColumns()->PcrAny();
		CColRef *pcrInner = pexprInnerSelect->DeriveOutputColumns()->PcrAny();
		gpos::owner<CExpression *> pexprPred =
			CUtils::PexprScalarEqCmp(mp, pcrOuter, pcrInner);
		pdrgpexpr->Append(pexprPred);
	}

	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithSubqueryConjuncts
//
//	@doc:
//		Generate randomized select expression with subquery predicates in
//		an AND tree
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithSubqueryConjuncts(CMemoryPool *mp,
													 BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::owner<CExpression *> pexprOuter = nullptr;
	gpos::owner<CExpression *> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);

	return PexprSelectWithSubqueryBoolOp(mp, std::move(pexprOuter),
										 std::move(pexprInner), fCorrelated,
										 CScalarBoolOp::EboolopAnd);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprProjectWithSubqueries
//
//	@doc:
//		Generate randomized project expression with multiple subqueries
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprProjectWithSubqueries(CMemoryPool *mp,
											   BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::owner<CExpression *> pexprOuter = nullptr;
	gpos::owner<CExpression *> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);

	return PexprProjectWithSubqueries(mp, std::move(pexprOuter),
									  std::move(pexprInner), fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSubquery
//
//	@doc:
//		Helper for generating a randomized Select expression with correlated
//		predicates to be used for building subquery examples:
//
//			SELECT inner_column
//			FROM inner_expression
//			WHERE inner_column = 5 [AND outer_column = inner_column]
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSubquery(
	CMemoryPool *mp, gpos::pointer<CExpression *> pexprOuter,
	gpos::owner<CExpression *> pexprInner,
	BOOL
		fCorrelated	 // add a predicate to inner expression correlated with outer expression?
)
{
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	// get a random column from inner expression
	CColRef *pcrInner = pexprInner->DeriveOutputColumns()->PcrAny();

	// generate a non-correlated predicate to be added to inner expression
	gpos::owner<CExpression *> pexprNonCorrelated = CUtils::PexprScalarEqCmp(
		mp, pcrInner, CUtils::PexprScalarConstInt4(mp, 5 /*val*/));

	// predicate for the inner expression
	gpos::owner<CExpression *> pexprPred = nullptr;
	if (fCorrelated)
	{
		// get a random column from outer expression
		CColRef *pcrOuter = pexprOuter->DeriveOutputColumns()->PcrAny();

		// generate correlated predicate
		gpos::owner<CExpression *> pexprCorrelated =
			CUtils::PexprScalarEqCmp(mp, pcrOuter, pcrInner);

		// generate AND expression of correlated and non-correlated predicates
		gpos::owner<CExpressionArray *> pdrgpexpr =
			GPOS_NEW(mp) CExpressionArray(mp);
		pdrgpexpr->Append(pexprCorrelated);
		pdrgpexpr->Append(pexprNonCorrelated);
		pexprPred =
			CUtils::PexprScalarBoolOp(mp, CScalarBoolOp::EboolopAnd, pdrgpexpr);
	}
	else
	{
		pexprPred = pexprNonCorrelated;
	}

	// generate a select on top of inner expression
	return CUtils::PexprLogicalSelect(mp, std::move(pexprInner),
									  std::move(pexprPred));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSubqueryQuantified
//
//	@doc:
//		Generate a quantified subquery expression
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSubqueryQuantified(
	CMemoryPool *mp, COperator::EOperatorId op_id,
	gpos::pointer<CExpression *> pexprOuter,
	gpos::owner<CExpression *> pexprInner,
	BOOL
		fCorrelated	 // add a predicate to inner expression correlated with outer expression?
)
{
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id);
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	gpos::owner<CExpression *> pexprSelect =
		PexprSubquery(mp, pexprOuter, pexprInner, fCorrelated);

	// get random columns from inner expression
	CColRefSet *pcrs = pexprInner->DeriveOutputColumns();
	const CColRef *pcrInner = pcrs->PcrAny();

	// get random columns from outer expression
	pcrs = pexprOuter->DeriveOutputColumns();
	const CColRef *pcrOuter = pcrs->PcrAny();

	// return a quantified subquery expression
	if (COperator::EopScalarSubqueryAny == op_id)
	{
		const CWStringConst *str =
			GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("="));
		return GPOS_NEW(mp) CExpression(
			mp,
			GPOS_NEW(mp) gpopt::CScalarSubqueryAny(
				mp, GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_EQ_OP), str, pcrInner),
			std::move(pexprSelect), CUtils::PexprScalarIdent(mp, pcrOuter));
	}

	const CWStringConst *str = GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("<>"));
	return GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CScalarSubqueryAll(
			mp, GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_NEQ_OP), str, pcrInner),
		std::move(pexprSelect), CUtils::PexprScalarIdent(mp, pcrOuter));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprUndecorrelatableSubquery
//
//	@doc:
//		Generate an expression with undecorrelatable quantified subquery
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprUndecorrelatableSubquery(CMemoryPool *mp,
												  COperator::EOperatorId op_id,
												  BOOL fCorrelated)
{
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id ||
				COperator::EopScalarSubqueryExists == op_id ||
				COperator::EopScalarSubqueryNotExists == op_id);

	CWStringConst strNameR(GPOS_WSZ_LIT("Rel1"));
	gpos::owner<CMDIdGPDB *> pmdidR = GPOS_NEW(mp) CMDIdGPDB(
		GPOPT_TEST_REL_OID1, 1 /*version_major*/, 1 /*version_minor*/);
	gpos::owner<CTableDescriptor *> ptabdescR =
		CTestUtils::PtabdescPlain(mp, 3 /*num_cols*/, pmdidR, CName(&strNameR));
	gpos::owner<CExpression *> pexprOuter =
		CTestUtils::PexprLogicalGet(mp, ptabdescR, &strNameR);

	// generate quantified subquery predicate
	gpos::owner<CExpression *> pexprInner = CTestUtils::PexprLogicalGet(mp);
	gpos::owner<CExpression *> pexprSubquery = nullptr;
	switch (op_id)
	{
		case COperator::EopScalarSubqueryAny:
		case COperator::EopScalarSubqueryAll:
			pexprSubquery = PexprSubqueryQuantified(mp, op_id, pexprOuter,
													pexprInner, fCorrelated);
			break;

		case COperator::EopScalarSubqueryExists:
		case COperator::EopScalarSubqueryNotExists:
			pexprSubquery = PexprSubqueryExistential(mp, op_id, pexprOuter,
													 pexprInner, fCorrelated);
			break;

		default:
			GPOS_ASSERT(!"Invalid subquery type");
	}

	// generate a regular predicate
	CColRef *pcrOuter = pexprOuter->DeriveOutputColumns()->PcrAny();
	gpos::owner<CExpression *> pexprPred = CUtils::PexprScalarEqCmp(
		mp, pcrOuter, CUtils::PexprScalarConstInt4(mp, 5 /*val*/));

	// generate OR expression of  predicates
	gpos::owner<CExpressionArray *> pdrgpexpr =
		GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexpr->Append(std::move(pexprSubquery));
	pdrgpexpr->Append(std::move(pexprPred));

	return CUtils::PexprLogicalSelect(
		mp, std::move(pexprOuter),
		CPredicateUtils::PexprDisjunction(mp, std::move(pdrgpexpr)));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprUndecorrelatableAnySubquery
//
//	@doc:
//		Generate an expression with undecorrelatable ANY subquery
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprUndecorrelatableAnySubquery(CMemoryPool *mp,
													 BOOL fCorrelated)
{
	return PexprUndecorrelatableSubquery(mp, COperator::EopScalarSubqueryAny,
										 fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprUndecorrelatableAllSubquery
//
//	@doc:
//		Generate an expression with undecorrelatable ALL subquery
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprUndecorrelatableAllSubquery(CMemoryPool *mp,
													 BOOL fCorrelated)
{
	return PexprUndecorrelatableSubquery(mp, COperator::EopScalarSubqueryAll,
										 fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprUndecorrelatableExistsSubquery
//
//	@doc:
//		Generate an expression with undecorrelatable Exists subquery
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprUndecorrelatableExistsSubquery(CMemoryPool *mp,
														BOOL fCorrelated)
{
	return PexprUndecorrelatableSubquery(mp, COperator::EopScalarSubqueryExists,
										 fCorrelated);
}

//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprUndecorrelatableNotExistsSubquery
//
//	@doc:
//		Generate an expression with undecorrelatable Not Exists subquery
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprUndecorrelatableNotExistsSubquery(CMemoryPool *mp,
														   BOOL fCorrelated)
{
	return PexprUndecorrelatableSubquery(
		mp, COperator::EopScalarSubqueryNotExists, fCorrelated);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprUndecorrelatableScalarSubquery
//
//	@doc:
//		Generate an expression with undecorrelatable Scalar subquery
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprUndecorrelatableScalarSubquery(CMemoryPool *mp,
														BOOL fCorrelated)
{
	gpos::owner<CExpression *> pexprOuter = CTestUtils::PexprLogicalGet(mp);
	gpos::owner<CExpression *> pexprInner = CTestUtils::PexprLogicalGet(mp);
	gpos::owner<CExpression *> pexprSelect =
		PexprSubquery(mp, pexprOuter, pexprInner, fCorrelated);

	// get a random column from inner expression
	gpos::pointer<CColRefSet *> pcrs = pexprInner->DeriveOutputColumns();
	CColRef *pcrInner = pcrs->PcrAny();

	gpos::owner<CExpression *> pexprSubquery = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CScalarSubquery(mp, pcrInner, false /*fGeneratedByExist*/,
									 false /*fGeneratedByQuantified*/),
		std::move(pexprSelect));

	CColRef *pcrOuter = pexprOuter->DeriveOutputColumns()->PcrAny();
	gpos::owner<CExpression *> pexprPred =
		CUtils::PexprScalarEqCmp(mp, pcrOuter, std::move(pexprSubquery));

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprPred));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSubqueryExistential
//
//	@doc:
//		Generate an EXISTS/NOT EXISTS subquery expression
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSubqueryExistential(
	CMemoryPool *mp, COperator::EOperatorId op_id,
	gpos::pointer<CExpression *> pexprOuter,
	gpos::owner<CExpression *> pexprInner,
	BOOL
		fCorrelated	 // add a predicate to inner expression correlated with outer expression?
)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryExists == op_id ||
				COperator::EopScalarSubqueryNotExists == op_id);
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	gpos::owner<CExpression *> pexprSelect =
		PexprSubquery(mp, pexprOuter, pexprInner, fCorrelated);

	// return a quantified subquery expression
	if (COperator::EopScalarSubqueryExists == op_id)
	{
		return GPOS_NEW(mp)
			CExpression(mp, GPOS_NEW(mp) gpopt::CScalarSubqueryExists(mp),
						std::move(pexprSelect));
	}

	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryNotExists(mp), std::move(pexprSelect));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSubqueryAgg
//
//	@doc:
//		Generate a randomized ScalarSubquery aggregate expression for
//		the following query:
//
//			SELECT sum(inner_column)
//			FROM inner_expression
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSubqueryAgg(
	CMemoryPool *mp, gpos::pointer<CExpression *> pexprOuter,
	gpos::owner<CExpression *> pexprInner,
	BOOL
		fCorrelated	 // add a predicate to inner expression correlated with outer expression?
)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	gpos::owner<CExpression *> pexprSelect =
		PexprSubquery(mp, pexprOuter, pexprInner, fCorrelated);

	// get a random column from inner expression
	gpos::pointer<CColRefSet *> pcrs = pexprInner->DeriveOutputColumns();
	CColRef *pcrInner = pcrs->PcrAny();

	// generate a SUM expression
	gpos::owner<CExpression *> pexprProjElem =
		CTestUtils::PexprPrjElemWithSum(mp, pcrInner);
	CColRef *pcrComputed =
		gpos::dyn_cast<CScalarProjectElement>(pexprProjElem->Pop())->Pcr();

	// add SUM expression to a project list
	gpos::owner<CExpression *> pexprProjList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), std::move(pexprProjElem));

	// generate empty grouping columns list
	gpos::owner<CColRefArray *> colref_array = GPOS_NEW(mp) CColRefArray(mp);

	// generate a group by on top of select expression
	gpos::owner<CExpression *> pexprLogicalGbAgg =
		CUtils::PexprLogicalGbAggGlobal(mp, std::move(colref_array),
										std::move(pexprSelect),
										std::move(pexprProjList));

	// return a subquery expression on top of group by
	return GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp)
			CScalarSubquery(mp, pcrComputed, false /*fGeneratedByExist*/,
							false /*fGeneratedByQuantified*/),
		std::move(pexprLogicalGbAgg));
}

//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithSubqueryBoolOp
//
//	@doc:
//		Generate a Select expression with a BoolOp (AND/OR) predicate tree involving
//		subqueries
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithSubqueryBoolOp(
	CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
	gpos::owner<CExpression *> pexprInner, BOOL fCorrelated,
	CScalarBoolOp::EBoolOperator eboolop)
{
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	GPOS_ASSERT(CScalarBoolOp::EboolopAnd == eboolop ||
				CScalarBoolOp::EboolopOr == eboolop);

	// get any two columns
	gpos::pointer<CColRefSet *> pcrs = pexprOuter->DeriveOutputColumns();
	CColRef *pcrLeft = pcrs->PcrAny();

	// generate agg subquery
	gpos::owner<CExpression *> pexprAggSubquery =
		PexprSubqueryAgg(mp, pexprOuter, std::move(pexprInner), fCorrelated);

	// generate equality predicate involving a subquery
	gpos::owner<CExpression *> pexprPred1 =
		CUtils::PexprScalarEqCmp(mp, pcrLeft, std::move(pexprAggSubquery));

	// generate a regular predicate
	gpos::owner<CExpression *> pexprPred2 = CUtils::PexprScalarEqCmp(
		mp, pcrLeft, CUtils::PexprScalarConstInt4(mp, 5 /*val*/));

	// generate ALL subquery
	gpos::owner<CExpression *> pexprGet = CTestUtils::PexprLogicalGet(mp);
	gpos::owner<CExpression *> pexprSubqueryAll =
		PexprSubqueryQuantified(mp, COperator::EopScalarSubqueryAll, pexprOuter,
								std::move(pexprGet), fCorrelated);

	// generate EXISTS subquery
	gpos::owner<CExpression *> pexprGet2 = CTestUtils::PexprLogicalGet(mp);
	gpos::owner<CExpression *> pexprSubqueryExists =
		PexprSubqueryExistential(mp, COperator::EopScalarSubqueryExists,
								 pexprOuter, std::move(pexprGet2), fCorrelated);

	// generate AND expression of all predicates
	gpos::owner<CExpressionArray *> pdrgpexpr =
		GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexpr->Append(std::move(pexprPred1));
	pdrgpexpr->Append(std::move(pexprPred2));
	pdrgpexpr->Append(std::move(pexprSubqueryExists));
	pdrgpexpr->Append(std::move(pexprSubqueryAll));

	gpos::owner<CExpression *> pexprPred =
		CUtils::PexprScalarBoolOp(mp, eboolop, std::move(pdrgpexpr));

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprPred));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprProjectWithSubqueries
//
//	@doc:
//		Generate a Project expression with multiple subqueries in project list
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprProjectWithSubqueries(
	CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
	gpos::owner<CExpression *> pexprInner, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	// generate an array of project elements holding subquery expressions
	gpos::owner<CExpressionArray *> pdrgpexpr =
		GPOS_NEW(mp) CExpressionArray(mp);

	CColRef *pcrComputed = nullptr;
	gpos::owner<CExpression *> pexprPrjElem = nullptr;
	gpos::owner<CExpression *> pexprGet = nullptr;

	gpos::pointer<const IMDTypeBool *> pmdtypebool =
		md_accessor->PtMDType<IMDTypeBool>();

	// generate agg subquery
	gpos::owner<CExpression *> pexprAggSubquery =
		PexprSubqueryAgg(mp, pexprOuter, std::move(pexprInner), fCorrelated);
	const CColRef *colref =
		gpos::dyn_cast<CScalarSubquery>(pexprAggSubquery->Pop())->Pcr();
	pcrComputed =
		col_factory->PcrCreate(colref->RetrieveType(), colref->TypeModifier());
	pexprPrjElem = CUtils::PexprScalarProjectElement(
		mp, pcrComputed, std::move(pexprAggSubquery));
	pdrgpexpr->Append(pexprPrjElem);

	// generate ALL subquery
	pexprGet = CTestUtils::PexprLogicalGet(mp);
	gpos::owner<CExpression *> pexprSubqueryAll = PexprSubqueryQuantified(
		mp, COperator::EopScalarSubqueryAll, pexprOuter, pexprGet, fCorrelated);
	pcrComputed = col_factory->PcrCreate(pmdtypebool, default_type_modifier);
	pexprPrjElem = CUtils::PexprScalarProjectElement(
		mp, pcrComputed, std::move(pexprSubqueryAll));
	pdrgpexpr->Append(pexprPrjElem);

	// generate existential subquery
	pexprGet = CTestUtils::PexprLogicalGet(mp);
	gpos::owner<CExpression *> pexprSubqueryExists =
		PexprSubqueryExistential(mp, COperator::EopScalarSubqueryExists,
								 pexprOuter, std::move(pexprGet), fCorrelated);
	pcrComputed = col_factory->PcrCreate(pmdtypebool, default_type_modifier);
	pexprPrjElem = CUtils::PexprScalarProjectElement(
		mp, pcrComputed, std::move(pexprSubqueryExists));
	pdrgpexpr->Append(std::move(pexprPrjElem));

	gpos::owner<CExpression *> pexprPrjList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), std::move(pdrgpexpr));

	return CUtils::PexprLogicalProject(mp, std::move(pexprOuter),
									   std::move(pexprPrjList),
									   false /*fNewComputedCol*/);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithSubqueryQuantified
//
//	@doc:
//		Generate randomized Select expression with quantified subquery
//		predicate
//
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithSubqueryQuantified(
	CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id);

	gpos::owner<CExpression *> pexprOuter = nullptr;
	gpos::owner<CExpression *> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);
	gpos::owner<CExpression *> pexprSubqueryQuantified =
		PexprSubqueryQuantified(mp, op_id, pexprOuter, std::move(pexprInner),
								fCorrelated);

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprSubqueryQuantified));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithQuantifiedAggSubquery
//
//	@doc:
//		Generate randomized Select expression with quantified subquery
//		whose inner expression is an aggregate
//
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithQuantifiedAggSubquery(
	CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id);

	gpos::owner<CExpression *> pexprOuter = CTestUtils::PexprLogicalGet(mp);
	gpos::owner<CExpression *> pexprSubq = PexprSubqueryAgg(
		mp, pexprOuter, CTestUtils::PexprLogicalGet(mp), fCorrelated);
	gpos::owner<CExpression *> pexprGb = (*pexprSubq)[0];
	pexprGb->AddRef();
	pexprSubq->Release();

	CColRef *pcrInner = pexprGb->DeriveOutputColumns()->PcrAny();
	CColRef *pcrOuter = pexprOuter->DeriveOutputColumns()->PcrAny();
	gpos::owner<CExpression *> pexprSubqueryQuantified = nullptr;
	if (COperator::EopScalarSubqueryAny == op_id)
	{
		const CWStringConst *str =
			GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("="));
		pexprSubqueryQuantified = GPOS_NEW(mp) CExpression(
			mp,
			GPOS_NEW(mp) gpopt::CScalarSubqueryAny(
				mp, GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_EQ_OP), str, pcrInner),
			pexprGb, CUtils::PexprScalarIdent(mp, pcrOuter));
	}

	const CWStringConst *str = GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("<>"));
	pexprSubqueryQuantified = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CScalarSubqueryAll(
			mp, GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_NEQ_OP), str, pcrInner),
		pexprGb, CUtils::PexprScalarIdent(mp, pcrOuter));


	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprSubqueryQuantified));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprProjectWithSubqueryQuantified
//
//	@doc:
//		Generate a randomized Project expression with a quantified subquery
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprProjectWithSubqueryQuantified(
	CMemoryPool *mp, gpos::owner<CExpression *> pexprOuter,
	gpos::owner<CExpression *> pexprInner, COperator::EOperatorId op_id,
	BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id);

	gpos::owner<CExpression *> pexprSubqueryQuantified =
		PexprSubqueryQuantified(mp, op_id, pexprOuter, std::move(pexprInner),
								fCorrelated);

	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	// generate a computed column
	gpos::pointer<CScalarSubqueryQuantified *> pop =
		gpos::dyn_cast<CScalarSubqueryQuantified>(
			pexprSubqueryQuantified->Pop());
	gpos::pointer<const IMDType *> pmdtype =
		md_accessor->RetrieveType(pop->MdidType());
	CColRef *pcrComputed = col_factory->PcrCreate(pmdtype, pop->TypeModifier());

	// generate a scalar project list
	gpos::owner<CExpression *> pexprPrjElem = CUtils::PexprScalarProjectElement(
		mp, pcrComputed, std::move(pexprSubqueryQuantified));
	gpos::owner<CExpression *> pexprPrjList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), std::move(pexprPrjElem));

	return CUtils::PexprLogicalProject(mp, std::move(pexprOuter),
									   std::move(pexprPrjList),
									   false /*fNewComputedCol*/);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithSubqueryExistential
//
//	@doc:
//		Generate randomized Select expression with existential subquery
//		predicate
//
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithSubqueryExistential(
	CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryExists == op_id ||
				COperator::EopScalarSubqueryNotExists == op_id);

	gpos::owner<CExpression *> pexprOuter = nullptr;
	gpos::owner<CExpression *> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);
	gpos::owner<CExpression *> pexprSubqueryExistential =
		PexprSubqueryExistential(mp, op_id, pexprOuter, std::move(pexprInner),
								 fCorrelated);

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprSubqueryExistential));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSelectWithTrimmableExistentialSubquery
//
//	@doc:
//		Generate randomized Select expression with existential subquery
//		predicate that can be trimmed
//
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSelectWithTrimmableExistentialSubquery(
	CMemoryPool *mp, COperator::EOperatorId op_id,
	BOOL  // fCorrelated
)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryExists == op_id ||
				COperator::EopScalarSubqueryNotExists == op_id);

	gpos::owner<CExpression *> pexprOuter = CTestUtils::PexprLogicalGet(mp);
	gpos::owner<CExpression *> pexprInner =
		CTestUtils::PexprLogicalGbAggWithSum(mp);

	// remove grouping columns
	(*pexprInner)[0]->AddRef();
	(*pexprInner)[1]->AddRef();
	gpos::owner<CExpression *> pexprGbAgg = CUtils::PexprLogicalGbAggGlobal(
		mp, GPOS_NEW(mp) CColRefArray(mp), (*pexprInner)[0], (*pexprInner)[1]);
	pexprInner->Release();

	// create existential subquery
	gpos::owner<CExpression *> pexprSubqueryExistential = nullptr;
	if (COperator::EopScalarSubqueryExists == op_id)
	{
		pexprSubqueryExistential = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) gpopt::CScalarSubqueryExists(mp), pexprGbAgg);
	}
	else
	{
		pexprSubqueryExistential = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarSubqueryNotExists(mp), pexprGbAgg);
	}

	// generate a regular predicate
	gpos::pointer<CColRefSet *> pcrs = pexprOuter->DeriveOutputColumns();
	gpos::owner<CExpression *> pexprEqPred = CUtils::PexprScalarEqCmp(
		mp, pcrs->PcrAny(), CUtils::PexprScalarConstInt4(mp, 5 /*val*/));

	gpos::owner<CExpression *> pexprConjunction =
		CPredicateUtils::PexprConjunction(mp, pexprSubqueryExistential,
										  pexprEqPred);
	pexprEqPred->Release();
	pexprSubqueryExistential->Release();

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprConjunction));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprProjectWithSubqueryExistential
//
//	@doc:
//		Generate randomized Project expression with existential subquery
//
//
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprProjectWithSubqueryExistential(
	CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryExists == op_id ||
				COperator::EopScalarSubqueryNotExists == op_id);

	gpos::owner<CExpression *> pexprOuter = nullptr;
	gpos::owner<CExpression *> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);
	gpos::owner<CExpression *> pexprSubqueryExistential =
		PexprSubqueryExistential(mp, op_id, pexprOuter, std::move(pexprInner),
								 fCorrelated);

	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	// generate a computed column
	gpos::pointer<CScalarSubqueryExistential *> pop =
		gpos::dyn_cast<CScalarSubqueryExistential>(
			pexprSubqueryExistential->Pop());
	gpos::pointer<const IMDType *> pmdtype =
		md_accessor->RetrieveType(pop->MdidType());
	CColRef *pcrComputed = col_factory->PcrCreate(pmdtype, pop->TypeModifier());

	// generate a scalar project list
	gpos::owner<CExpression *> pexprPrjElem = CUtils::PexprScalarProjectElement(
		mp, pcrComputed, std::move(pexprSubqueryExistential));
	gpos::owner<CExpression *> pexprPrjList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), std::move(pexprPrjElem));

	return CUtils::PexprLogicalProject(mp, std::move(pexprOuter),
									   std::move(pexprPrjList),
									   false /*fNewComputedCol*/);
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSubqueryWithConstTableGet
//
//	@doc:
//		Generate Select expression with Any subquery predicate over a const
//		table get
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSubqueryWithConstTableGet(CMemoryPool *mp,
												   COperator::EOperatorId op_id)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id);

	CWStringConst strNameR(GPOS_WSZ_LIT("Rel1"));
	gpos::owner<CMDIdGPDB *> pmdidR =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID1, 1, 1);
	gpos::owner<CTableDescriptor *> ptabdescR = CTestUtils::PtabdescCreate(
		mp, 3 /*num_cols*/, pmdidR, CName(&strNameR));

	gpos::owner<CExpression *> pexprOuter =
		CTestUtils::PexprLogicalGet(mp, ptabdescR, &strNameR);
	gpos::owner<CExpression *> pexprConstTableGet =
		CTestUtils::PexprConstTableGet(mp, 3 /* ulElements */);

	// get random columns from inner expression
	CColRefSet *pcrs = pexprConstTableGet->DeriveOutputColumns();
	const CColRef *pcrInner = pcrs->PcrAny();

	// get random columns from outer expression
	pcrs = pexprOuter->DeriveOutputColumns();
	const CColRef *pcrOuter = pcrs->PcrAny();

	const CWStringConst *str = GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("="));

	gpos::owner<CExpression *> pexprSubquery = nullptr;
	if (COperator::EopScalarSubqueryAny == op_id)
	{
		// construct ANY subquery expression
		pexprSubquery = GPOS_NEW(mp) CExpression(
			mp,
			GPOS_NEW(mp) gpopt::CScalarSubqueryAny(
				mp, GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_EQ_OP), str, pcrInner),
			pexprConstTableGet, CUtils::PexprScalarIdent(mp, pcrOuter));
	}
	else
	{
		// construct ALL subquery expression
		pexprSubquery = GPOS_NEW(mp) CExpression(
			mp,
			GPOS_NEW(mp) CScalarSubqueryAll(
				mp, GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_EQ_OP), str, pcrInner),
			pexprConstTableGet, CUtils::PexprScalarIdent(mp, pcrOuter));
	}

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprSubquery));
}


//---------------------------------------------------------------------------
//	@function:
//		CSubqueryTestUtils::PexprSubqueryWithDisjunction
//
//	@doc:
//		Generate Select expression with a disjunction of two Any subqueries over
//		const table get
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CSubqueryTestUtils::PexprSubqueryWithDisjunction(CMemoryPool *mp)
{
	GPOS_ASSERT(nullptr != mp);

	CWStringConst strNameR(GPOS_WSZ_LIT("Rel1"));
	gpos::owner<CMDIdGPDB *> pmdidR =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID1, 1, 1);
	gpos::owner<CTableDescriptor *> ptabdescR = CTestUtils::PtabdescCreate(
		mp, 3 /*num_cols*/, pmdidR, CName(&strNameR));

	gpos::owner<CExpression *> pexprOuter =
		CTestUtils::PexprLogicalGet(mp, ptabdescR, &strNameR);

	gpos::owner<CExpressionArray *> pdrgpexpr =
		GPOS_NEW(mp) CExpressionArray(mp);

	for (int i = 0; i < 2; i++)
	{
		gpos::owner<CExpression *> pexprConstTableGet =
			CTestUtils::PexprConstTableGet(mp, 3 /* ulElements */);
		// get random columns from inner expression
		CColRefSet *pcrs = pexprConstTableGet->DeriveOutputColumns();
		const CColRef *pcrInner = pcrs->PcrAny();

		// get random columns from outer expression
		pcrs = pexprOuter->DeriveOutputColumns();
		const CColRef *pcrOuter = pcrs->PcrAny();

		const CWStringConst *str =
			GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("="));

		gpos::owner<CExpression *> pexprSubquery = GPOS_NEW(mp) CExpression(
			mp,
			GPOS_NEW(mp) gpopt::CScalarSubqueryAny(
				mp, GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_EQ_OP), str, pcrInner),
			pexprConstTableGet, CUtils::PexprScalarIdent(mp, pcrOuter));
		pdrgpexpr->Append(pexprSubquery);
	}



	// generate a disjunction of the subquery with itself
	gpos::owner<CExpression *> pexprBoolOp = CUtils::PexprScalarBoolOp(
		mp, CScalarBoolOp::EboolopOr, std::move(pdrgpexpr));

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprBoolOp));
}

// EOF
