//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CPredicateUtilsTest.cpp
//
//	@doc:
//		Test for predicate utilities
//---------------------------------------------------------------------------
#include "unittest/gpopt/operators/CPredicateUtilsTest.h"

#include "gpos/common/owner.h"
#include "gpos/error/CAutoTrace.h"

#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"
#include "gpopt/operators/CExpressionPreprocessor.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "naucrates/md/CMDIdGPDB.h"

#include "unittest/base.h"
#include "unittest/gpopt/CTestUtils.h"


//---------------------------------------------------------------------------
//	@function:
//		CPredicateUtilsTest::EresUnittest
//
//	@doc:
//		Unittest for predicate utilities
//
//---------------------------------------------------------------------------
GPOS_RESULT
CPredicateUtilsTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CPredicateUtilsTest::EresUnittest_Conjunctions),
		GPOS_UNITTEST_FUNC(CPredicateUtilsTest::EresUnittest_Disjunctions),
		GPOS_UNITTEST_FUNC(CPredicateUtilsTest::EresUnittest_PlainEqualities),
		GPOS_UNITTEST_FUNC(CPredicateUtilsTest::EresUnittest_Implication),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}


//---------------------------------------------------------------------------
//	@function:
//		CPredicateUtilsTest::EresUnittest_Conjunctions
//
//	@doc:
//		Test extraction and construction of conjuncts
//
//---------------------------------------------------------------------------
GPOS_RESULT
CPredicateUtilsTest::EresUnittest_Conjunctions()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	// build conjunction
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	const ULONG ulConjs = 3;
	for (ULONG ul = 0; ul < ulConjs; ul++)
	{
		pdrgpexpr->Append(CUtils::PexprScalarConstBool(mp, true /*fValue*/));
	}
	gpos::Ref<CExpression> pexprConjunction = CUtils::PexprScalarBoolOp(
		mp, CScalarBoolOp::EboolopAnd, std::move(pdrgpexpr));

	// break into conjuncts
	gpos::Ref<CExpressionArray> pdrgpexprExtract =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprConjunction.get());
	GPOS_ASSERT(pdrgpexprExtract->Size() == ulConjs);

	// collapse into single conjunct
	gpos::Ref<CExpression> pexpr =
		CPredicateUtils::PexprConjunction(mp, pdrgpexprExtract);
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(CUtils::FScalarConstTrue(pexpr.get()));
	;

	// collapse empty input array to conjunct
	gpos::Ref<CExpression> pexprSingleton =
		CPredicateUtils::PexprConjunction(mp, nullptr /*pdrgpexpr*/);
	GPOS_ASSERT(nullptr != pexprSingleton);
	;

	;

	// conjunction on scalar comparisons
	gpos::Ref<CExpression> pexprGet = CTestUtils::PexprLogicalGet(mp);
	CColRefSet *pcrs = pexprGet->DeriveOutputColumns();
	CColRef *pcr1 = pcrs->PcrAny();
	CColRef *pcr2 = pcrs->PcrFirst();
	gpos::Ref<CExpression> pexprCmp1 =
		CUtils::PexprScalarCmp(mp, pcr1, pcr2, IMDType::EcmptEq);
	gpos::Ref<CExpression> pexprCmp2 = CUtils::PexprScalarCmp(
		mp, pcr1, CUtils::PexprScalarConstInt4(mp, 1 /*val*/),
		IMDType::EcmptEq);

	gpos::Ref<CExpression> pexprConj =
		CPredicateUtils::PexprConjunction(mp, pexprCmp1.get(), pexprCmp2.get());
	pdrgpexprExtract = CPredicateUtils::PdrgpexprConjuncts(mp, pexprConj.get());
	GPOS_ASSERT(2 == pdrgpexprExtract->Size());
	;

	;
	;
	;
	;

	return GPOS_OK;
}


//---------------------------------------------------------------------------
//	@function:
//		CPredicateUtilsTest::EresUnittest_Disjunctions
//
//	@doc:
//		Test extraction and construction of disjuncts
//
//---------------------------------------------------------------------------
GPOS_RESULT
CPredicateUtilsTest::EresUnittest_Disjunctions()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	// build disjunction
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	const ULONG ulDisjs = 3;
	for (ULONG ul = 0; ul < ulDisjs; ul++)
	{
		pdrgpexpr->Append(CUtils::PexprScalarConstBool(mp, false /*fValue*/));
	}
	gpos::Ref<CExpression> pexprDisjunction = CUtils::PexprScalarBoolOp(
		mp, CScalarBoolOp::EboolopOr, std::move(pdrgpexpr));

	// break into disjuncts
	gpos::Ref<CExpressionArray> pdrgpexprExtract =
		CPredicateUtils::PdrgpexprDisjuncts(mp, pexprDisjunction.get());
	GPOS_ASSERT(pdrgpexprExtract->Size() == ulDisjs);

	// collapse into single disjunct
	gpos::Ref<CExpression> pexpr =
		CPredicateUtils::PexprDisjunction(mp, pdrgpexprExtract);
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(CUtils::FScalarConstFalse(pexpr.get()));
	;

	// collapse empty input array to disjunct
	gpos::Ref<CExpression> pexprSingleton =
		CPredicateUtils::PexprDisjunction(mp, nullptr /*pdrgpexpr*/);
	GPOS_ASSERT(nullptr != pexprSingleton);
	;

	;

	// disjunction on scalar comparisons
	gpos::Ref<CExpression> pexprGet = CTestUtils::PexprLogicalGet(mp);
	CColRefSet *pcrs = pexprGet->DeriveOutputColumns();
	CColRefSetIter crsi(*pcrs);

	BOOL fAdvance GPOS_ASSERTS_ONLY = crsi.Advance();
	GPOS_ASSERT(fAdvance);
	CColRef *pcr1 = crsi.Pcr();

#ifdef GPOS_DEBUG
	fAdvance =
#endif
		crsi.Advance();
	GPOS_ASSERT(fAdvance);
	CColRef *pcr2 = crsi.Pcr();

#ifdef GPOS_DEBUG
	fAdvance =
#endif
		crsi.Advance();
	GPOS_ASSERT(fAdvance);
	CColRef *pcr3 = crsi.Pcr();

	gpos::Ref<CExpression> pexprCmp1 =
		CUtils::PexprScalarCmp(mp, pcr1, pcr2, IMDType::EcmptEq);
	gpos::Ref<CExpression> pexprCmp2 = CUtils::PexprScalarCmp(
		mp, pcr1, CUtils::PexprScalarConstInt4(mp, 1 /*val*/),
		IMDType::EcmptEq);

	{
		gpos::Ref<CExpression> pexprDisj = CPredicateUtils::PexprDisjunction(
			mp, pexprCmp1.get(), pexprCmp2.get());
		pdrgpexprExtract =
			CPredicateUtils::PdrgpexprDisjuncts(mp, pexprDisj.get());
		GPOS_ASSERT(2 == pdrgpexprExtract->Size());
		;
		;
	}


	{
		gpos::Ref<CExpressionArray> pdrgpexpr =
			GPOS_NEW(mp) CExpressionArray(mp);
		gpos::Ref<CExpression> pexprCmp3 =
			CUtils::PexprScalarCmp(mp, pcr2, pcr1, IMDType::EcmptG);
		gpos::Ref<CExpression> pexprCmp4 = CUtils::PexprScalarCmp(
			mp, CUtils::PexprScalarConstInt4(mp, 200 /*val*/), pcr3,
			IMDType::EcmptL);
		;
		;

		pdrgpexpr->Append(std::move(pexprCmp3));
		pdrgpexpr->Append(std::move(pexprCmp4));
		pdrgpexpr->Append(pexprCmp1);
		pdrgpexpr->Append(pexprCmp2);

		gpos::Ref<CExpression> pexprDisj =
			CPredicateUtils::PexprDisjunction(mp, std::move(pdrgpexpr));
		pdrgpexprExtract =
			CPredicateUtils::PdrgpexprDisjuncts(mp, pexprDisj.get());
		GPOS_ASSERT(4 == pdrgpexprExtract->Size());
		;
		;
	}

	;
	;
	;

	return GPOS_OK;
}


//---------------------------------------------------------------------------
//	@function:
//		CPredicateUtilsTest::EresUnittest_PlainEqualities
//
//	@doc:
//		Test the extraction of equality predicates between scalar identifiers
//
//---------------------------------------------------------------------------
GPOS_RESULT
CPredicateUtilsTest::EresUnittest_PlainEqualities()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	gpos::Ref<CExpression> pexprLeft = CTestUtils::PexprLogicalGet(mp);
	gpos::Ref<CExpression> pexprRight = CTestUtils::PexprLogicalGet(mp);

	gpos::Ref<CExpressionArray> pdrgpexprOriginal =
		GPOS_NEW(mp) CExpressionArray(mp);

	CColRefSet *pcrsLeft = pexprLeft->DeriveOutputColumns();
	CColRefSet *pcrsRight = pexprRight->DeriveOutputColumns();

	CColRef *pcrLeft = pcrsLeft->PcrAny();
	CColRef *pcrRight = pcrsRight->PcrAny();

	// generate an equality predicate between two column reference
	gpos::Ref<CExpression> pexprScIdentEquality =
		CUtils::PexprScalarEqCmp(mp, pcrLeft, pcrRight);

	;
	pdrgpexprOriginal->Append(pexprScIdentEquality);

	// generate a non-equality predicate between two column reference
	gpos::Ref<CExpression> pexprScIdentInequality = CUtils::PexprScalarCmp(
		mp, pcrLeft, pcrRight, CWStringConst(GPOS_WSZ_LIT("<")),
		GPOS_NEW(mp) CMDIdGPDB(GPDB_INT4_LT_OP));

	;
	pdrgpexprOriginal->Append(pexprScIdentInequality);

	// generate an equality predicate between a column reference and a constant value
	gpos::Ref<CExpression> pexprScalarConstInt4 =
		CUtils::PexprScalarConstInt4(mp, 10 /*fValue*/);
	gpos::Ref<CExpression> pexprScIdentConstEquality =
		CUtils::PexprScalarEqCmp(mp, std::move(pexprScalarConstInt4), pcrRight);

	pdrgpexprOriginal->Append(std::move(pexprScIdentConstEquality));

	GPOS_ASSERT(3 == pdrgpexprOriginal->Size());

	gpos::Ref<CExpressionArray> pdrgpexprResult =
		CPredicateUtils::PdrgpexprPlainEqualities(mp, pdrgpexprOriginal.get());

	GPOS_ASSERT(1 == pdrgpexprResult->Size());

	// clean up
	;
	;
	;
	;
	;
	;

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CPredicateUtilsTest::EresUnittest_Implication
//
//	@doc:
//		Test removal of implied predicates
//
//---------------------------------------------------------------------------
GPOS_RESULT
CPredicateUtilsTest::EresUnittest_Implication()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	// generate a two cascaded joins
	CWStringConst strName1(GPOS_WSZ_LIT("Rel1"));
	gpos::Ref<CMDIdGPDB> pmdid1 =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID1, 1, 1);
	gpos::Ref<CTableDescriptor> ptabdesc1 =
		CTestUtils::PtabdescCreate(mp, 3, std::move(pmdid1), CName(&strName1));
	CWStringConst strAlias1(GPOS_WSZ_LIT("Rel1"));
	gpos::Ref<CExpression> pexprRel1 =
		CTestUtils::PexprLogicalGet(mp, std::move(ptabdesc1), &strAlias1);

	CWStringConst strName2(GPOS_WSZ_LIT("Rel2"));
	gpos::Ref<CMDIdGPDB> pmdid2 =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID2, 1, 1);
	gpos::Ref<CTableDescriptor> ptabdesc2 =
		CTestUtils::PtabdescCreate(mp, 3, std::move(pmdid2), CName(&strName2));
	CWStringConst strAlias2(GPOS_WSZ_LIT("Rel2"));
	gpos::Ref<CExpression> pexprRel2 =
		CTestUtils::PexprLogicalGet(mp, std::move(ptabdesc2), &strAlias2);

	CWStringConst strName3(GPOS_WSZ_LIT("Rel3"));
	gpos::Ref<CMDIdGPDB> pmdid3 =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_TEST_REL_OID3, 1, 1);
	gpos::Ref<CTableDescriptor> ptabdesc3 =
		CTestUtils::PtabdescCreate(mp, 3, std::move(pmdid3), CName(&strName3));
	CWStringConst strAlias3(GPOS_WSZ_LIT("Rel3"));
	gpos::Ref<CExpression> pexprRel3 =
		CTestUtils::PexprLogicalGet(mp, std::move(ptabdesc3), &strAlias3);

	gpos::Ref<CExpression> pexprJoin1 =
		CTestUtils::PexprLogicalJoin<CLogicalInnerJoin>(
			mp, std::move(pexprRel1), std::move(pexprRel2));
	gpos::Ref<CExpression> pexprJoin2 =
		CTestUtils::PexprLogicalJoin<CLogicalInnerJoin>(
			mp, std::move(pexprJoin1), std::move(pexprRel3));

	{
		CAutoTrace at(mp);
		at.Os() << "Original expression:" << std::endl
				<< *pexprJoin2 << std::endl;
	}

	// imply new predicates by deriving constraints
	gpos::Ref<CExpression> pexprConstraints =
		CExpressionPreprocessor::PexprAddPredicatesFromConstraints(
			mp, pexprJoin2.get());

	{
		CAutoTrace at(mp);
		at.Os() << "Expression with implied predicates:" << std::endl
				<< *pexprConstraints << std::endl;
		;
	}

	// minimize join predicates by removing implied conjuncts
	CExpressionHandle exprhdl(mp);
	exprhdl.Attach(pexprConstraints.get());
	gpos::Ref<CExpression> pexprMinimizedPred =
		CPredicateUtils::PexprRemoveImpliedConjuncts(mp, (*pexprConstraints)[2],
													 exprhdl);

	{
		CAutoTrace at(mp);
		at.Os() << "Minimized join predicate:" << std::endl
				<< *pexprMinimizedPred << std::endl;
	}

	gpos::Ref<CExpressionArray> pdrgpexprOriginalConjuncts =
		CPredicateUtils::PdrgpexprConjuncts(mp, (*pexprConstraints)[2]);
	gpos::Ref<CExpressionArray> pdrgpexprNewConjuncts =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprMinimizedPred.get());

	GPOS_ASSERT(pdrgpexprNewConjuncts->Size() <
				pdrgpexprOriginalConjuncts->Size());

	// clean up
	;
	;
	;
	;
	;

	return GPOS_OK;
}
// EOF
