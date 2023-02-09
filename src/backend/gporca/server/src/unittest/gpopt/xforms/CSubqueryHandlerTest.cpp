//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CSubqueryHandlerTest.cpp
//
//	@doc:
//		Test for subquery handling
//---------------------------------------------------------------------------
#include "unittest/gpopt/xforms/CSubqueryHandlerTest.h"

#include "gpos/common/owner.h"
#include "gpos/io/COstreamString.h"
#include "gpos/string/CWStringDynamic.h"

#include "gpopt/base/CQueryContext.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/xforms/CSubqueryHandler.h"
#include "gpopt/xforms/CXformFactory.h"
#include "naucrates/md/CMDIdGPDB.h"

#include "unittest/base.h"
#include "unittest/gpopt/CSubqueryTestUtils.h"
#include "unittest/gpopt/CTestUtils.h"

ULONG CSubqueryHandlerTest::m_ulSubqueryHandlerMinidumpTestCounter =
	0;	// start from first test

// minidump files
const CHAR *rgszSubqueryHandlerMinidumpFileNames[] = {
	"../data/dxl/minidump/SemiJoinWithWindowsFuncInSubquery.mdp",
	"../data/dxl/minidump/CorrelatedSubqueryWithAggWindowFunc.mdp",
	"../data/dxl/minidump/AllSubqueryWithSubqueryInScalar.mdp",
	"../data/dxl/minidump/AnySubqueryWithAllSubqueryInScalar.mdp",
	"../data/dxl/minidump/AnySubqueryWithSubqueryInScalar.mdp",
	"../data/dxl/minidump/ScalarSubq-Eq-SubqAll-1.mdp",
	"../data/dxl/minidump/ScalarSubq-Eq-SubqAll-2.mdp"};

//---------------------------------------------------------------------------
//	@function:
//		CSubqueryHandlerTest::EresUnittest
//
//	@doc:
//		Unittest for predicate utilities
//
//---------------------------------------------------------------------------
GPOS_RESULT
CSubqueryHandlerTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CSubqueryHandlerTest::EresUnittest_Subquery2Apply),
		GPOS_UNITTEST_FUNC(
			CSubqueryHandlerTest::EresUnittest_SubqueryWithDisjunction),
		GPOS_UNITTEST_FUNC(CSubqueryHandlerTest::EresUnittest_RunMinidumpTests),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

// run minidump tests
GPOS_RESULT
CSubqueryHandlerTest::EresUnittest_RunMinidumpTests()
{
	return CTestUtils::EresUnittest_RunTestsWithoutAdditionalTraceFlags(
		rgszSubqueryHandlerMinidumpFileNames,
		&m_ulSubqueryHandlerMinidumpTestCounter,
		GPOS_ARRAY_SIZE(rgszSubqueryHandlerMinidumpFileNames),
		true, /* fMatchPlans */
		true  /* fTestSpacePruning */
	);
}

//---------------------------------------------------------------------------
//	@function:
//		CSubqueryHandlerTest::EresUnittest_Subquery2Apply
//
//	@doc:
//		Test of subquery handler
//
//---------------------------------------------------------------------------
GPOS_RESULT
CSubqueryHandlerTest::EresUnittest_Subquery2Apply()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	using Pfpexpr = gpos::Ref<CExpression> (*)(CMemoryPool *, BOOL);
	Pfpexpr rgpf[] = {
		CSubqueryTestUtils::PexprSelectWithAggSubquery,
		CSubqueryTestUtils::PexprSelectWithAggSubqueryConstComparison,
		CSubqueryTestUtils::PexprProjectWithAggSubquery,
		CSubqueryTestUtils::PexprSelectWithAnySubquery,
		CSubqueryTestUtils::PexprProjectWithAnySubquery,
		CSubqueryTestUtils::PexprSelectWithAllSubquery,
		CSubqueryTestUtils::PexprProjectWithAllSubquery,
		CSubqueryTestUtils::PexprSelectWithExistsSubquery,
		CSubqueryTestUtils::PexprProjectWithExistsSubquery,
		CSubqueryTestUtils::PexprSelectWithNotExistsSubquery,
		CSubqueryTestUtils::PexprProjectWithNotExistsSubquery,
		CSubqueryTestUtils::PexprSelectWithNestedCmpSubquery,
		CSubqueryTestUtils::PexprSelectWithCmpSubqueries,
		CSubqueryTestUtils::PexprSelectWithSubqueryConjuncts,
		CSubqueryTestUtils::PexprProjectWithSubqueries,
		CSubqueryTestUtils::PexprSelectWith2LevelsCorrSubquery,
		CSubqueryTestUtils::PexprJoinWithAggSubquery,
		CSubqueryTestUtils::PexprSelectWithAggSubqueryOverJoin,
		CSubqueryTestUtils::PexprSelectWithNestedSubquery,
		CSubqueryTestUtils::PexprSubqueriesInNullTestContext,
		CSubqueryTestUtils::PexprSubqueriesInDifferentContexts,
		CSubqueryTestUtils::PexprSelectWithSubqueryDisjuncts,
		CSubqueryTestUtils::PexprSelectWithNestedAnySubqueries,
		CSubqueryTestUtils::PexprSelectWithNestedAllSubqueries,
		CSubqueryTestUtils::PexprUndecorrelatableAnySubquery,
		CSubqueryTestUtils::PexprUndecorrelatableAllSubquery,
		CSubqueryTestUtils::PexprUndecorrelatableExistsSubquery,
		CSubqueryTestUtils::PexprUndecorrelatableNotExistsSubquery,
		CSubqueryTestUtils::PexprUndecorrelatableScalarSubquery,
	};

	// xforms to test
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfSubqJoin2Apply);
	(void) xform_set->ExchangeSet(CXform::ExfSelect2Apply);
	(void) xform_set->ExchangeSet(CXform::ExfProject2Apply);

	BOOL fCorrelated = true;
	// we generate two expressions using each generator
	const ULONG size = 2 * GPOS_ARRAY_SIZE(rgpf);
	for (ULONG ul = 0; ul < size; ul++)
	{
		ULONG ulIndex = ul / 2;
		// install opt context in TLS
		CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
						 CTestUtils::GetCostModel(mp));

		// generate expression
		gpos::Ref<CExpression> pexpr = rgpf[ulIndex](mp, fCorrelated);

		// check for subq xforms
		gpos::Ref<CXformSet> pxfsCand =
			gpos::dyn_cast<CLogical>(pexpr->Pop())->PxfsCandidates(mp);
		pxfsCand->Intersection(xform_set.get());

		CXformSetIter xsi(*pxfsCand);
		while (xsi.Advance())
		{
			gpos::Ref<CXform> pxform = CXformFactory::Pxff()->Pxf(xsi.TBit());
			GPOS_ASSERT(nullptr != pxform);

			CWStringDynamic str(mp);
			COstreamString oss(&str);

			oss << std::endl << "INPUT:" << std::endl << *pexpr << std::endl;

			gpos::Ref<CXformContext> pxfctxt = GPOS_NEW(mp) CXformContext(mp);
			gpos::Ref<CXformResult> pxfres = GPOS_NEW(mp) CXformResult(mp);

			// calling the xform to perform subquery to Apply transformation
			pxform->Transform(pxfctxt.get(), pxfres.get(), pexpr.get());
			gpos::Ref<CExpression> pexprResult = pxfres->PexprNext();

			oss << std::endl << "OUTPUT:" << std::endl;
			if (nullptr != pexprResult)
			{
				oss << *pexprResult << std::endl;
			}
			else
			{
				oss << "\tNo subquery unnesting output" << std::endl;
			}

			GPOS_TRACE(str.GetBuffer());
			str.Reset();

			;
			;
		}

		;
		;
		fCorrelated = !fCorrelated;
	}

	;

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CSubqueryHandlerTest::EresUnittest_SubqueryWithDisjunction
//
//	@doc:
//		Test case of subqueries in a disjunctive tree
//
//---------------------------------------------------------------------------
GPOS_RESULT
CSubqueryHandlerTest::EresUnittest_SubqueryWithDisjunction()
{
	// use own memory pool
	CAutoMemoryPool amp(CAutoMemoryPool::ElcNone);
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache());
	mda.RegisterProvider(CTestUtils::m_sysidDefault, std::move(pmdp));

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	// create a subquery with const table get expression

	gpos::Ref<CExpression> pexprOuter = nullptr;
	gpos::Ref<CExpression> pexprInner = nullptr;
	CSubqueryTestUtils::GenerateGetExpressions(mp, &pexprOuter, &pexprInner);

	gpos::Ref<CExpression> pexpr =
		CSubqueryTestUtils::PexprSelectWithSubqueryBoolOp(
			mp, std::move(pexprOuter), std::move(pexprInner),
			true /*fCorrelated*/, CScalarBoolOp::EboolopOr);

	gpos::Ref<CXform> pxform =
		CXformFactory::Pxff()->Pxf(CXform::ExfSelect2Apply);

	CWStringDynamic str(mp);
	COstreamString oss(&str);

	oss << std::endl << "EXPRESSION:" << std::endl << *pexpr << std::endl;

	gpos::Ref<CExpression> pexprLogical = (*pexpr)[0];
	gpos::Ref<CExpression> pexprScalar = (*pexpr)[1];
	oss << std::endl << "LOGICAL:" << std::endl << *pexprLogical << std::endl;
	oss << std::endl << "SCALAR:" << std::endl << *pexprScalar << std::endl;

	GPOS_TRACE(str.GetBuffer());
	str.Reset();

	gpos::Ref<CXformContext> pxfctxt = GPOS_NEW(mp) CXformContext(mp);
	gpos::Ref<CXformResult> pxfres = GPOS_NEW(mp) CXformResult(mp);

	// calling the xform to perform subquery to Apply transformation
	pxform->Transform(pxfctxt.get(), pxfres.get(), pexpr.get());
	gpos::Ref<CExpression> pexprResult = pxfres->PexprNext();

	oss << std::endl
		<< "NEW LOGICAL:" << std::endl
		<< *((*pexprResult)[0]) << std::endl;
	oss << std::endl
		<< "RESIDUAL SCALAR:" << std::endl
		<< *((*pexprResult)[1]) << std::endl;

	GPOS_TRACE(str.GetBuffer());
	str.Reset();

	;
	;
	;

	return GPOS_OK;
}


// EOF
