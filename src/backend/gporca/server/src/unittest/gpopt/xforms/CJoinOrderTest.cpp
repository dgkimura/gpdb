//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CJoinOrderTest.cpp
//
//	@doc:
//		Test for join ordering
//---------------------------------------------------------------------------
#include "unittest/gpopt/xforms/CJoinOrderTest.h"

#include "gpos/common/owner.h"
#include "gpos/error/CAutoTrace.h"
#include "gpos/io/COstreamString.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/CQueryContext.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/xforms/CJoinOrder.h"
#include "gpopt/xforms/CJoinOrderMinCard.h"

#include "unittest/base.h"
#include "unittest/gpopt/CTestUtils.h"

ULONG CJoinOrderTest::m_ulTestCounter = 0;	// start from first test

// minidump files
const CHAR *rgszJoinOrderFileNames[] = {
	"../data/dxl/minidump/JoinOptimizationLevelGreedyNonPartTblInnerJoin.mdp",
	"../data/dxl/minidump/JoinOptimizationLevelQueryNonPartTblInnerJoin.mdp"};

//---------------------------------------------------------------------------
//	@function:
//		CJoinOrderTest::EresUnittest
//
//	@doc:
//		Unittest for predicate utilities
//
//---------------------------------------------------------------------------
GPOS_RESULT
CJoinOrderTest::EresUnittest()
{
	CUnittest rgut[] = {GPOS_UNITTEST_FUNC(EresUnittest_ExpandMinCard),
						GPOS_UNITTEST_FUNC(EresUnittest_RunTests)};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}


//---------------------------------------------------------------------------
//	@function:
//		CJoinOrderTest::EresUnittest_ExpandMinCard
//
//	@doc:
//		Expansion expansion based on cardinality of intermediate results
//
//---------------------------------------------------------------------------
GPOS_RESULT
CJoinOrderTest::EresUnittest_ExpandMinCard()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// array of relation names
	CWStringConst rgscRel[] = {
		GPOS_WSZ_LIT("Rel10"), GPOS_WSZ_LIT("Rel3"),  GPOS_WSZ_LIT("Rel4"),
		GPOS_WSZ_LIT("Rel6"),  GPOS_WSZ_LIT("Rel7"),  GPOS_WSZ_LIT("Rel8"),
		GPOS_WSZ_LIT("Rel12"), GPOS_WSZ_LIT("Rel13"), GPOS_WSZ_LIT("Rel5"),
		GPOS_WSZ_LIT("Rel14"), GPOS_WSZ_LIT("Rel15"), GPOS_WSZ_LIT("Rel1"),
		GPOS_WSZ_LIT("Rel11"), GPOS_WSZ_LIT("Rel2"),  GPOS_WSZ_LIT("Rel9"),
	};

	// array of relation IDs
	ULONG rgulRel[] = {
		GPOPT_TEST_REL_OID10, GPOPT_TEST_REL_OID3,	GPOPT_TEST_REL_OID4,
		GPOPT_TEST_REL_OID6,  GPOPT_TEST_REL_OID7,	GPOPT_TEST_REL_OID8,
		GPOPT_TEST_REL_OID12, GPOPT_TEST_REL_OID13, GPOPT_TEST_REL_OID5,
		GPOPT_TEST_REL_OID14, GPOPT_TEST_REL_OID15, GPOPT_TEST_REL_OID1,
		GPOPT_TEST_REL_OID11, GPOPT_TEST_REL_OID2,	GPOPT_TEST_REL_OID9,
	};

	const ULONG ulRels = GPOS_ARRAY_SIZE(rgscRel);
	GPOS_ASSERT(GPOS_ARRAY_SIZE(rgulRel) == ulRels);

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache());
	mda.RegisterProvider(CTestUtils::m_sysidDefault, pmdp);

	{
		// install opt context in TLS
		CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
						 CTestUtils::GetCostModel(mp));

		gpos::Ref<CExpression> pexprNAryJoin = CTestUtils::PexprLogicalNAryJoin(
			mp, rgscRel, rgulRel, ulRels, false /*fCrossProduct*/);

		// derive stats on input expression
		CExpressionHandle exprhdl(mp);
		exprhdl.Attach(pexprNAryJoin.get());
		exprhdl.DeriveStats(mp, mp, nullptr /*prprel*/, nullptr /*stats_ctxt*/);

		gpos::Ref<CExpressionArray> pdrgpexpr =
			GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG ul = 0; ul < ulRels; ul++)
		{
			gpos::Ref<CExpression> pexprChild = (*pexprNAryJoin)[ul];
			;
			pdrgpexpr->Append(pexprChild);
		}
		gpos::Ref<CExpressionArray> pdrgpexprPred =
			CPredicateUtils::PdrgpexprConjuncts(mp, (*pexprNAryJoin)[ulRels]);
		;
		;
		CJoinOrderMinCard jomc(mp, pdrgpexpr, pdrgpexprPred);
		gpos::Ref<CExpression> pexprResult = jomc.PexprExpand();
		{
			CAutoTrace at(mp);
			at.Os() << std::endl
					<< "INPUT:" << std::endl
					<< *pexprNAryJoin << std::endl;
			at.Os() << std::endl
					<< "OUTPUT:" << std::endl
					<< *pexprResult << std::endl;
		};
		;
		;
		;
	}

	return GPOS_OK;
}

//	run all Minidump-based tests with plan matching
GPOS_RESULT
CJoinOrderTest::EresUnittest_RunTests()
{
	return CTestUtils::EresUnittest_RunTests(
		rgszJoinOrderFileNames, &m_ulTestCounter,
		GPOS_ARRAY_SIZE(rgszJoinOrderFileNames));
}
// EOF
