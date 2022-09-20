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
CSubqueryTestUtils::GenerateGetExpressions(CMemoryPool *mp,
										   gpos::Ref<CExpression> *ppexprOuter,
										   gpos::Ref<CExpression> *ppexprInner)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != ppexprOuter);
	GPOS_ASSERT(nullptr != ppexprInner);

	// outer expression
	CWStringConst strNameR(GPOS_WSZ_LIT("Rel1"));
	gpos::Ref<CMDIdGPDB> pmdidR =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID1, 1, 1);
	gpos::Ref<CTableDescriptor> ptabdescR = CTestUtils::PtabdescCreate(
		mp, 3 /*num_cols*/, std::move(pmdidR), CName(&strNameR));
	*ppexprOuter =
		CTestUtils::PexprLogicalGet(mp, std::move(ptabdescR), &strNameR);

	// inner expression
	CWStringConst strNameS(GPOS_WSZ_LIT("Rel2"));
	gpos::Ref<CMDIdGPDB> pmdidS =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID2, 1, 1);
	gpos::Ref<CTableDescriptor> ptabdescS = CTestUtils::PtabdescCreate(
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprJoinWithAggSubquery(CMemoryPool *mp, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::Ref<CExpression> pexprLeft = nullptr;
	gpos::Ref<CExpression> pexprRight = nullptr;
	GenerateGetExpressions(mp, &pexprLeft, &pexprRight);

	gpos::Ref<CExpression> pexprInner = CTestUtils::PexprLogicalGet(mp);
	gpos::Ref<CExpression> pexprSelect = PexprSelectWithAggSubquery(
		mp, std::move(pexprLeft), std::move(pexprInner), fCorrelated);

	;
	;

	gpos::Ref<CExpression> pexpr = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CLogicalInnerJoin(mp), (*pexprSelect)[0],
					std::move(pexprRight), (*pexprSelect)[1]);

	;

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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithAggSubquery(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
	gpos::Ref<CExpression> pexprInner, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	// get any column
	CColRefSet *pcrs = pexprOuter->DeriveOutputColumns();
	CColRef *pcrLeft = pcrs->PcrAny();

	// generate agg subquery
	gpos::Ref<CExpression> pexprSubq = PexprSubqueryAgg(
		mp, pexprOuter.get(), std::move(pexprInner), fCorrelated);

	// generate equality predicate
	gpos::Ref<CExpression> pexprPredicate =
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithAggSubqueryConstComparison(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
	gpos::Ref<CExpression> pexprInner, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	// generate agg subquery
	gpos::Ref<CExpression> pexprSubq = PexprSubqueryAgg(
		mp, pexprOuter.get(), std::move(pexprInner), fCorrelated);

	gpos::Ref<CExpression> pexprConst =
		CUtils::PexprScalarConstInt8(mp, 0 /*val*/);

	// generate equality predicate
	gpos::Ref<CExpression> pexprPredicate = CUtils::PexprScalarEqCmp(
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprProjectWithAggSubquery(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
	gpos::Ref<CExpression> pexprInner, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	// generate agg subquery
	gpos::Ref<CExpression> pexprSubq = PexprSubqueryAgg(
		mp, pexprOuter.get(), std::move(pexprInner), fCorrelated);

	// generate a computed column
	CScalarSubquery *popSubquery =
		gpos::dyn_cast<CScalarSubquery>(pexprSubq->Pop());
	const IMDType *pmdtype = md_accessor->RetrieveType(popSubquery->MdidType());
	CColRef *pcrComputed =
		col_factory->PcrCreate(pmdtype, popSubquery->TypeModifier());

	// generate a scalar project list
	gpos::Ref<CExpression> pexprPrjElem = CUtils::PexprScalarProjectElement(
		mp, pcrComputed, std::move(pexprSubq));
	gpos::Ref<CExpression> pexprPrjList = GPOS_NEW(mp) CExpression(
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithAggSubquery(CMemoryPool *mp,
											   BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithAggSubqueryConstComparison(CMemoryPool *mp,
															  BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprProjectWithAggSubquery(CMemoryPool *mp,
												BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithAggSubqueryOverJoin(CMemoryPool *mp,
													   BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	// generate a pair of get expressions
	gpos::Ref<CExpression> pexprR = nullptr;
	gpos::Ref<CExpression> pexprS = nullptr;
	GenerateGetExpressions(mp, &pexprR, &pexprS);

	// generate outer expression
	CWStringConst strNameT(GPOS_WSZ_LIT("Rel3"));

	gpos::Ref<CMDIdGPDB> pmdidT =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID3, 1, 1);
	gpos::Ref<CTableDescriptor> ptabdescT = CTestUtils::PtabdescCreate(
		mp, 3 /*num_cols*/, pmdidT, CName(&strNameT));
	gpos::Ref<CExpression> pexprT =
		CTestUtils::PexprLogicalGet(mp, ptabdescT, &strNameT);
	CColRef *pcrInner = pexprR->DeriveOutputColumns()->PcrAny();
	CColRef *pcrOuter = pexprT->DeriveOutputColumns()->PcrAny();

	gpos::Ref<CExpression> pexprPred = nullptr;
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
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexpr->Append(std::move(pexprR));
	pdrgpexpr->Append(std::move(pexprS));
	pdrgpexpr->Append(std::move(pexprPred));
	gpos::Ref<CExpression> pexprJoin =
		CTestUtils::PexprLogicalNAryJoin(mp, std::move(pdrgpexpr));

	gpos::Ref<CExpression> pexprSubq = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CScalarSubquery(mp, pcrInner, false /*fGeneratedByExist*/,
									 false /*fGeneratedByQuantified*/),
		std::move(pexprJoin));

	gpos::Ref<CExpression> pexprPredOuter =
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithSubqueryQuantifiedOverWindow(
	CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id);

	gpos::Ref<CExpression> pexprOuter = CTestUtils::PexprLogicalGet(mp);
	gpos::Ref<CExpression> pexprInner = CTestUtils::PexprOneWindowFunction(mp);
	gpos::Ref<CExpression> pexprSubqueryQuantified = PexprSubqueryQuantified(
		mp, op_id, pexprOuter.get(), std::move(pexprInner), fCorrelated);

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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprProjectWithAnySubquery(CMemoryPool *mp,
												BOOL fCorrelated)
{
	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprProjectWithAllSubquery(CMemoryPool *mp,
												BOOL fCorrelated)
{
	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSubqueriesInDifferentContexts(CMemoryPool *mp,
													   BOOL fCorrelated)
{
	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);
	gpos::Ref<CExpression> pexprSelect = PexprSelectWithAggSubquery(
		mp, std::move(pexprOuter), std::move(pexprInner), fCorrelated);

	gpos::Ref<CExpression> pexprGet = CTestUtils::PexprLogicalGet(mp);
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSubqueriesInNullTestContext(CMemoryPool *mp,
													 BOOL fCorrelated)
{
	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);

	// generate agg subquery
	gpos::Ref<CExpression> pexprSubq = PexprSubqueryAgg(
		mp, pexprOuter.get(), std::move(pexprInner), fCorrelated);

	// generate Is Not Null predicate
	gpos::Ref<CExpression> pexprPredicate =
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithSubqueryDisjuncts(CMemoryPool *mp,
													 BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithNestedCmpSubquery(CMemoryPool *mp,
													 BOOL fCorrelated)
{
	gpos::Ref<CExpression> pexprSelectWithSubquery =
		PexprSelectWithAggSubquery(mp, fCorrelated);

	CExpression *pexprLogical = (*pexprSelectWithSubquery)[0];
	gpos::Ref<CExpression> pexprSubqueryPred = (*pexprSelectWithSubquery)[1];

	// generate a parent equality predicate
	;
	gpos::Ref<CExpression> pexprPredicate1 = CUtils::PexprScalarEqCmp(
		mp, CUtils::PexprScalarConstBool(mp, true /*value*/),
		std::move(pexprSubqueryPred));

	// add another nesting level
	gpos::Ref<CExpression> pexprPredicate = CUtils::PexprScalarEqCmp(
		mp, CUtils::PexprScalarConstBool(mp, true /*value*/),
		std::move(pexprPredicate1));

	;
	;

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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithCmpSubqueries(CMemoryPool *mp,
												 BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);
	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);

	// generate a scalar subquery
	gpos::Ref<CExpression> pexprScalarSubquery1 = PexprSubqueryAgg(
		mp, pexprOuter.get(), std::move(pexprInner), fCorrelated);

	// generate get expression
	CWStringConst strNameT(GPOS_WSZ_LIT("Rel3"));

	gpos::Ref<CMDIdGPDB> pmdidT =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID3, 1, 1);
	gpos::Ref<CTableDescriptor> ptabdescT = CTestUtils::PtabdescCreate(
		mp, 3 /*num_cols*/, std::move(pmdidT), CName(&strNameT));
	gpos::Ref<CExpression> pexprT =
		CTestUtils::PexprLogicalGet(mp, std::move(ptabdescT), &strNameT);

	// generate another scalar subquery
	gpos::Ref<CExpression> pexprScalarSubquery2 =
		PexprSubqueryAgg(mp, pexprOuter.get(), std::move(pexprT), fCorrelated);

	// generate equality predicate between both subqueries
	gpos::Ref<CExpression> pexprPredicate = CUtils::PexprScalarEqCmp(
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithNestedSubquery(CMemoryPool *mp,
												  BOOL fCorrelated)
{
	gpos::Ref<CExpression> pexprInner =
		PexprSelectWithAggSubquery(mp, fCorrelated);
	CColRef *pcrInner = pexprInner->DeriveOutputColumns()->PcrAny();

	gpos::Ref<CExpression> pexprOuter = CTestUtils::PexprLogicalGet(mp);
	CColRef *pcrOuter = pexprOuter->DeriveOutputColumns()->PcrAny();

	gpos::Ref<CExpression> pexprSubq = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CScalarSubquery(mp, pcrInner, false /*fGeneratedByExist*/,
									 false /*fGeneratedByQuantified*/),
		std::move(pexprInner));

	gpos::Ref<CExpression> pexprPred =
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithNestedQuantifiedSubqueries(
	CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated)
{
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id);

	CWStringConst strName1(GPOS_WSZ_LIT("Rel1"));
	CWStringConst strAlias1(GPOS_WSZ_LIT("Rel1Alias"));
	gpos::Ref<CExpression> pexprOuter1 = CTestUtils::PexprLogicalGetNullable(
		mp, GPOPT_TEST_REL_OID1, &strName1, &strAlias1);

	CWStringConst strName2(GPOS_WSZ_LIT("Rel2"));
	CWStringConst strAlias2(GPOS_WSZ_LIT("Rel2Alias"));
	gpos::Ref<CExpression> pexprOuter2 = CTestUtils::PexprLogicalGetNullable(
		mp, GPOPT_TEST_REL_OID2, &strName2, &strAlias2);

	CWStringConst strName3(GPOS_WSZ_LIT("Rel3"));
	CWStringConst strAlias3(GPOS_WSZ_LIT("Rel3Alias"));
	gpos::Ref<CExpression> pexprOuter3 = CTestUtils::PexprLogicalGetNullable(
		mp, GPOPT_TEST_REL_OID3, &strName3, &strAlias3);

	CWStringConst strName4(GPOS_WSZ_LIT("Rel4"));
	CWStringConst strAlias4(GPOS_WSZ_LIT("Rel4Alias"));
	gpos::Ref<CExpression> pexprInner = CTestUtils::PexprLogicalGetNullable(
		mp, GPOPT_TEST_REL_OID4, &strName4, &strAlias4);

	gpos::Ref<CExpression> pexprSubqueryQuantified1 = PexprSubqueryQuantified(
		mp, op_id, pexprOuter3.get(), std::move(pexprInner), fCorrelated);
	gpos::Ref<CExpression> pexprSelect1 = CUtils::PexprLogicalSelect(
		mp, std::move(pexprOuter3), std::move(pexprSubqueryQuantified1));
	gpos::Ref<CExpression> pexprSubqueryQuantified2 = PexprSubqueryQuantified(
		mp, op_id, pexprOuter2.get(), std::move(pexprSelect1), fCorrelated);
	gpos::Ref<CExpression> pexprSelect2 = CUtils::PexprLogicalSelect(
		mp, std::move(pexprOuter2), std::move(pexprSubqueryQuantified2));
	gpos::Ref<CExpression> pexprSubqueryQuantified3 = PexprSubqueryQuantified(
		mp, op_id, pexprOuter1.get(), std::move(pexprSelect2), fCorrelated);

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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWith2LevelsCorrSubquery(CMemoryPool *mp,
													   BOOL fCorrelated)
{
	gpos::Ref<CExpression> pexpr =
		PexprSelectWithNestedSubquery(mp, fCorrelated);
	if (fCorrelated)
	{
		// add a 2-level correlation
		CExpression *pexprOuterSubq = (*(*pexpr)[1])[1];
		CExpression *pexprInnerSubq = (*(*(*pexprOuterSubq)[0])[1])[1];
		CExpression *pexprInnerSelect = (*(*pexprInnerSubq)[0])[0];
		CExpressionArray *pdrgpexpr = (*pexprInnerSelect)[1]->PdrgPexpr();

		CColRef *pcrOuter = pexpr->DeriveOutputColumns()->PcrAny();
		CColRef *pcrInner = pexprInnerSelect->DeriveOutputColumns()->PcrAny();
		gpos::Ref<CExpression> pexprPred =
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithSubqueryConjuncts(CMemoryPool *mp,
													 BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprProjectWithSubqueries(CMemoryPool *mp,
											   BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSubquery(
	CMemoryPool *mp, CExpression *pexprOuter, gpos::Ref<CExpression> pexprInner,
	BOOL
		fCorrelated	 // add a predicate to inner expression correlated with outer expression?
)
{
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	// get a random column from inner expression
	CColRef *pcrInner = pexprInner->DeriveOutputColumns()->PcrAny();

	// generate a non-correlated predicate to be added to inner expression
	gpos::Ref<CExpression> pexprNonCorrelated = CUtils::PexprScalarEqCmp(
		mp, pcrInner, CUtils::PexprScalarConstInt4(mp, 5 /*val*/));

	// predicate for the inner expression
	gpos::Ref<CExpression> pexprPred = nullptr;
	if (fCorrelated)
	{
		// get a random column from outer expression
		CColRef *pcrOuter = pexprOuter->DeriveOutputColumns()->PcrAny();

		// generate correlated predicate
		gpos::Ref<CExpression> pexprCorrelated =
			CUtils::PexprScalarEqCmp(mp, pcrOuter, pcrInner);

		// generate AND expression of correlated and non-correlated predicates
		gpos::Ref<CExpressionArray> pdrgpexpr =
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSubqueryQuantified(
	CMemoryPool *mp, COperator::EOperatorId op_id, CExpression *pexprOuter,
	gpos::Ref<CExpression> pexprInner,
	BOOL
		fCorrelated	 // add a predicate to inner expression correlated with outer expression?
)
{
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id);
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	gpos::Ref<CExpression> pexprSelect =
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprUndecorrelatableSubquery(CMemoryPool *mp,
												  COperator::EOperatorId op_id,
												  BOOL fCorrelated)
{
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id ||
				COperator::EopScalarSubqueryExists == op_id ||
				COperator::EopScalarSubqueryNotExists == op_id);

	CWStringConst strNameR(GPOS_WSZ_LIT("Rel1"));
	gpos::Ref<CMDIdGPDB> pmdidR = GPOS_NEW(mp) CMDIdGPDB(
		GPOPT_TEST_REL_OID1, 1 /*version_major*/, 1 /*version_minor*/);
	gpos::Ref<CTableDescriptor> ptabdescR =
		CTestUtils::PtabdescPlain(mp, 3 /*num_cols*/, pmdidR, CName(&strNameR));
	gpos::Ref<CExpression> pexprOuter =
		CTestUtils::PexprLogicalGet(mp, ptabdescR, &strNameR);

	// generate quantified subquery predicate
	gpos::Ref<CExpression> pexprInner = CTestUtils::PexprLogicalGet(mp);
	gpos::Ref<CExpression> pexprSubquery = nullptr;
	switch (op_id)
	{
		case COperator::EopScalarSubqueryAny:
		case COperator::EopScalarSubqueryAll:
			pexprSubquery = PexprSubqueryQuantified(mp, op_id, pexprOuter.get(),
													pexprInner, fCorrelated);
			break;

		case COperator::EopScalarSubqueryExists:
		case COperator::EopScalarSubqueryNotExists:
			pexprSubquery = PexprSubqueryExistential(
				mp, op_id, pexprOuter.get(), pexprInner, fCorrelated);
			break;

		default:
			GPOS_ASSERT(!"Invalid subquery type");
	}

	// generate a regular predicate
	CColRef *pcrOuter = pexprOuter->DeriveOutputColumns()->PcrAny();
	gpos::Ref<CExpression> pexprPred = CUtils::PexprScalarEqCmp(
		mp, pcrOuter, CUtils::PexprScalarConstInt4(mp, 5 /*val*/));

	// generate OR expression of  predicates
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprUndecorrelatableScalarSubquery(CMemoryPool *mp,
														BOOL fCorrelated)
{
	gpos::Ref<CExpression> pexprOuter = CTestUtils::PexprLogicalGet(mp);
	gpos::Ref<CExpression> pexprInner = CTestUtils::PexprLogicalGet(mp);
	gpos::Ref<CExpression> pexprSelect =
		PexprSubquery(mp, pexprOuter.get(), pexprInner, fCorrelated);

	// get a random column from inner expression
	CColRefSet *pcrs = pexprInner->DeriveOutputColumns();
	CColRef *pcrInner = pcrs->PcrAny();

	gpos::Ref<CExpression> pexprSubquery = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CScalarSubquery(mp, pcrInner, false /*fGeneratedByExist*/,
									 false /*fGeneratedByQuantified*/),
		std::move(pexprSelect));

	CColRef *pcrOuter = pexprOuter->DeriveOutputColumns()->PcrAny();
	gpos::Ref<CExpression> pexprPred =
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSubqueryExistential(
	CMemoryPool *mp, COperator::EOperatorId op_id, CExpression *pexprOuter,
	gpos::Ref<CExpression> pexprInner,
	BOOL
		fCorrelated	 // add a predicate to inner expression correlated with outer expression?
)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryExists == op_id ||
				COperator::EopScalarSubqueryNotExists == op_id);
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	gpos::Ref<CExpression> pexprSelect =
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSubqueryAgg(
	CMemoryPool *mp, CExpression *pexprOuter, gpos::Ref<CExpression> pexprInner,
	BOOL
		fCorrelated	 // add a predicate to inner expression correlated with outer expression?
)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	gpos::Ref<CExpression> pexprSelect =
		PexprSubquery(mp, pexprOuter, pexprInner, fCorrelated);

	// get a random column from inner expression
	CColRefSet *pcrs = pexprInner->DeriveOutputColumns();
	CColRef *pcrInner = pcrs->PcrAny();

	// generate a SUM expression
	gpos::Ref<CExpression> pexprProjElem =
		CTestUtils::PexprPrjElemWithSum(mp, pcrInner);
	CColRef *pcrComputed =
		gpos::dyn_cast<CScalarProjectElement>(pexprProjElem->Pop())->Pcr();

	// add SUM expression to a project list
	gpos::Ref<CExpression> pexprProjList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), std::move(pexprProjElem));

	// generate empty grouping columns list
	gpos::Ref<CColRefArray> colref_array = GPOS_NEW(mp) CColRefArray(mp);

	// generate a group by on top of select expression
	gpos::Ref<CExpression> pexprLogicalGbAgg = CUtils::PexprLogicalGbAggGlobal(
		mp, std::move(colref_array), std::move(pexprSelect),
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithSubqueryBoolOp(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
	gpos::Ref<CExpression> pexprInner, BOOL fCorrelated,
	CScalarBoolOp::EBoolOperator eboolop)
{
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	GPOS_ASSERT(CScalarBoolOp::EboolopAnd == eboolop ||
				CScalarBoolOp::EboolopOr == eboolop);

	// get any two columns
	CColRefSet *pcrs = pexprOuter->DeriveOutputColumns();
	CColRef *pcrLeft = pcrs->PcrAny();

	// generate agg subquery
	gpos::Ref<CExpression> pexprAggSubquery = PexprSubqueryAgg(
		mp, pexprOuter.get(), std::move(pexprInner), fCorrelated);

	// generate equality predicate involving a subquery
	gpos::Ref<CExpression> pexprPred1 =
		CUtils::PexprScalarEqCmp(mp, pcrLeft, std::move(pexprAggSubquery));

	// generate a regular predicate
	gpos::Ref<CExpression> pexprPred2 = CUtils::PexprScalarEqCmp(
		mp, pcrLeft, CUtils::PexprScalarConstInt4(mp, 5 /*val*/));

	// generate ALL subquery
	gpos::Ref<CExpression> pexprGet = CTestUtils::PexprLogicalGet(mp);
	gpos::Ref<CExpression> pexprSubqueryAll = PexprSubqueryQuantified(
		mp, COperator::EopScalarSubqueryAll, pexprOuter.get(),
		std::move(pexprGet), fCorrelated);

	// generate EXISTS subquery
	gpos::Ref<CExpression> pexprGet2 = CTestUtils::PexprLogicalGet(mp);
	gpos::Ref<CExpression> pexprSubqueryExists = PexprSubqueryExistential(
		mp, COperator::EopScalarSubqueryExists, pexprOuter.get(),
		std::move(pexprGet2), fCorrelated);

	// generate AND expression of all predicates
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexpr->Append(std::move(pexprPred1));
	pdrgpexpr->Append(std::move(pexprPred2));
	pdrgpexpr->Append(std::move(pexprSubqueryExists));
	pdrgpexpr->Append(std::move(pexprSubqueryAll));

	gpos::Ref<CExpression> pexprPred =
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprProjectWithSubqueries(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
	gpos::Ref<CExpression> pexprInner, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != pexprOuter);
	GPOS_ASSERT(nullptr != pexprInner);

	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	// generate an array of project elements holding subquery expressions
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	CColRef *pcrComputed = nullptr;
	gpos::Ref<CExpression> pexprPrjElem = nullptr;
	gpos::Ref<CExpression> pexprGet = nullptr;

	const IMDTypeBool *pmdtypebool = md_accessor->PtMDType<IMDTypeBool>();

	// generate agg subquery
	gpos::Ref<CExpression> pexprAggSubquery = PexprSubqueryAgg(
		mp, pexprOuter.get(), std::move(pexprInner), fCorrelated);
	const CColRef *colref =
		gpos::dyn_cast<CScalarSubquery>(pexprAggSubquery->Pop())->Pcr();
	pcrComputed =
		col_factory->PcrCreate(colref->RetrieveType(), colref->TypeModifier());
	pexprPrjElem = CUtils::PexprScalarProjectElement(
		mp, pcrComputed, std::move(pexprAggSubquery));
	pdrgpexpr->Append(pexprPrjElem);

	// generate ALL subquery
	pexprGet = CTestUtils::PexprLogicalGet(mp);
	gpos::Ref<CExpression> pexprSubqueryAll =
		PexprSubqueryQuantified(mp, COperator::EopScalarSubqueryAll,
								pexprOuter.get(), pexprGet, fCorrelated);
	pcrComputed = col_factory->PcrCreate(pmdtypebool, default_type_modifier);
	pexprPrjElem = CUtils::PexprScalarProjectElement(
		mp, pcrComputed, std::move(pexprSubqueryAll));
	pdrgpexpr->Append(pexprPrjElem);

	// generate existential subquery
	pexprGet = CTestUtils::PexprLogicalGet(mp);
	gpos::Ref<CExpression> pexprSubqueryExists = PexprSubqueryExistential(
		mp, COperator::EopScalarSubqueryExists, pexprOuter.get(),
		std::move(pexprGet), fCorrelated);
	pcrComputed = col_factory->PcrCreate(pmdtypebool, default_type_modifier);
	pexprPrjElem = CUtils::PexprScalarProjectElement(
		mp, pcrComputed, std::move(pexprSubqueryExists));
	pdrgpexpr->Append(std::move(pexprPrjElem));

	gpos::Ref<CExpression> pexprPrjList = GPOS_NEW(mp) CExpression(
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithSubqueryQuantified(
	CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id);

	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);
	gpos::Ref<CExpression> pexprSubqueryQuantified = PexprSubqueryQuantified(
		mp, op_id, pexprOuter.get(), std::move(pexprInner), fCorrelated);

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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithQuantifiedAggSubquery(
	CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id);

	gpos::Ref<CExpression> pexprOuter = CTestUtils::PexprLogicalGet(mp);
	gpos::Ref<CExpression> pexprSubq = PexprSubqueryAgg(
		mp, pexprOuter.get(), CTestUtils::PexprLogicalGet(mp), fCorrelated);
	gpos::Ref<CExpression> pexprGb = (*pexprSubq)[0];
	;
	;

	CColRef *pcrInner = pexprGb->DeriveOutputColumns()->PcrAny();
	CColRef *pcrOuter = pexprOuter->DeriveOutputColumns()->PcrAny();
	gpos::Ref<CExpression> pexprSubqueryQuantified = nullptr;
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprProjectWithSubqueryQuantified(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprOuter,
	gpos::Ref<CExpression> pexprInner, COperator::EOperatorId op_id,
	BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id);

	gpos::Ref<CExpression> pexprSubqueryQuantified = PexprSubqueryQuantified(
		mp, op_id, pexprOuter.get(), std::move(pexprInner), fCorrelated);

	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	// generate a computed column
	CScalarSubqueryQuantified *pop = gpos::dyn_cast<CScalarSubqueryQuantified>(
		pexprSubqueryQuantified->Pop());
	const IMDType *pmdtype = md_accessor->RetrieveType(pop->MdidType());
	CColRef *pcrComputed = col_factory->PcrCreate(pmdtype, pop->TypeModifier());

	// generate a scalar project list
	gpos::Ref<CExpression> pexprPrjElem = CUtils::PexprScalarProjectElement(
		mp, pcrComputed, std::move(pexprSubqueryQuantified));
	gpos::Ref<CExpression> pexprPrjList = GPOS_NEW(mp) CExpression(
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithSubqueryExistential(
	CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryExists == op_id ||
				COperator::EopScalarSubqueryNotExists == op_id);

	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);
	gpos::Ref<CExpression> pexprSubqueryExistential = PexprSubqueryExistential(
		mp, op_id, pexprOuter.get(), std::move(pexprInner), fCorrelated);

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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSelectWithTrimmableExistentialSubquery(
	CMemoryPool *mp, COperator::EOperatorId op_id,
	BOOL  // fCorrelated
)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryExists == op_id ||
				COperator::EopScalarSubqueryNotExists == op_id);

	gpos::Ref<CExpression> pexprOuter = CTestUtils::PexprLogicalGet(mp);
	gpos::Ref<CExpression> pexprInner =
		CTestUtils::PexprLogicalGbAggWithSum(mp);

	// remove grouping columns
	;
	;
	gpos::Ref<CExpression> pexprGbAgg = CUtils::PexprLogicalGbAggGlobal(
		mp, GPOS_NEW(mp) CColRefArray(mp), (*pexprInner)[0], (*pexprInner)[1]);
	;

	// create existential subquery
	gpos::Ref<CExpression> pexprSubqueryExistential = nullptr;
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
	CColRefSet *pcrs = pexprOuter->DeriveOutputColumns();
	gpos::Ref<CExpression> pexprEqPred = CUtils::PexprScalarEqCmp(
		mp, pcrs->PcrAny(), CUtils::PexprScalarConstInt4(mp, 5 /*val*/));

	gpos::Ref<CExpression> pexprConjunction = CPredicateUtils::PexprConjunction(
		mp, pexprSubqueryExistential.get(), pexprEqPred.get());
	;
	;

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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprProjectWithSubqueryExistential(
	CMemoryPool *mp, COperator::EOperatorId op_id, BOOL fCorrelated)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryExists == op_id ||
				COperator::EopScalarSubqueryNotExists == op_id);

	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
	GenerateGetExpressions(mp, &pexprOuter, &pexprInner);
	gpos::Ref<CExpression> pexprSubqueryExistential = PexprSubqueryExistential(
		mp, op_id, pexprOuter.get(), std::move(pexprInner), fCorrelated);

	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();

	// generate a computed column
	CScalarSubqueryExistential *pop =
		gpos::dyn_cast<CScalarSubqueryExistential>(
			pexprSubqueryExistential->Pop());
	const IMDType *pmdtype = md_accessor->RetrieveType(pop->MdidType());
	CColRef *pcrComputed = col_factory->PcrCreate(pmdtype, pop->TypeModifier());

	// generate a scalar project list
	gpos::Ref<CExpression> pexprPrjElem = CUtils::PexprScalarProjectElement(
		mp, pcrComputed, std::move(pexprSubqueryExistential));
	gpos::Ref<CExpression> pexprPrjList = GPOS_NEW(mp) CExpression(
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSubqueryWithConstTableGet(CMemoryPool *mp,
												   COperator::EOperatorId op_id)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(COperator::EopScalarSubqueryAny == op_id ||
				COperator::EopScalarSubqueryAll == op_id);

	CWStringConst strNameR(GPOS_WSZ_LIT("Rel1"));
	gpos::Ref<CMDIdGPDB> pmdidR =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID1, 1, 1);
	gpos::Ref<CTableDescriptor> ptabdescR = CTestUtils::PtabdescCreate(
		mp, 3 /*num_cols*/, pmdidR, CName(&strNameR));

	gpos::Ref<CExpression> pexprOuter =
		CTestUtils::PexprLogicalGet(mp, ptabdescR, &strNameR);
	gpos::Ref<CExpression> pexprConstTableGet =
		CTestUtils::PexprConstTableGet(mp, 3 /* ulElements */);

	// get random columns from inner expression
	CColRefSet *pcrs = pexprConstTableGet->DeriveOutputColumns();
	const CColRef *pcrInner = pcrs->PcrAny();

	// get random columns from outer expression
	pcrs = pexprOuter->DeriveOutputColumns();
	const CColRef *pcrOuter = pcrs->PcrAny();

	const CWStringConst *str = GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("="));

	gpos::Ref<CExpression> pexprSubquery = nullptr;
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
gpos::Ref<CExpression>
CSubqueryTestUtils::PexprSubqueryWithDisjunction(CMemoryPool *mp)
{
	GPOS_ASSERT(nullptr != mp);

	CWStringConst strNameR(GPOS_WSZ_LIT("Rel1"));
	gpos::Ref<CMDIdGPDB> pmdidR =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID1, 1, 1);
	gpos::Ref<CTableDescriptor> ptabdescR = CTestUtils::PtabdescCreate(
		mp, 3 /*num_cols*/, pmdidR, CName(&strNameR));

	gpos::Ref<CExpression> pexprOuter =
		CTestUtils::PexprLogicalGet(mp, ptabdescR, &strNameR);

	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	for (int i = 0; i < 2; i++)
	{
		gpos::Ref<CExpression> pexprConstTableGet =
			CTestUtils::PexprConstTableGet(mp, 3 /* ulElements */);
		// get random columns from inner expression
		CColRefSet *pcrs = pexprConstTableGet->DeriveOutputColumns();
		const CColRef *pcrInner = pcrs->PcrAny();

		// get random columns from outer expression
		pcrs = pexprOuter->DeriveOutputColumns();
		const CColRef *pcrOuter = pcrs->PcrAny();

		const CWStringConst *str =
			GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("="));

		gpos::Ref<CExpression> pexprSubquery = GPOS_NEW(mp) CExpression(
			mp,
			GPOS_NEW(mp) gpopt::CScalarSubqueryAny(
				mp, GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_EQ_OP), str, pcrInner),
			pexprConstTableGet, CUtils::PexprScalarIdent(mp, pcrOuter));
		pdrgpexpr->Append(pexprSubquery);
	}



	// generate a disjunction of the subquery with itself
	gpos::Ref<CExpression> pexprBoolOp = CUtils::PexprScalarBoolOp(
		mp, CScalarBoolOp::EboolopOr, std::move(pdrgpexpr));

	return CUtils::PexprLogicalSelect(mp, std::move(pexprOuter),
									  std::move(pexprBoolOp));
}

// EOF
