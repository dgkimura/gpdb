//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CConstExprEvaluatorDefaultTest.cpp
//
//	@doc:
//		Unit tests for CConstExprEvaluatorDefault
//
//	@owner:
//
//
//	@test:
//
//---------------------------------------------------------------------------

#include "unittest/gpopt/eval/CConstExprEvaluatorDefaultTest.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CAutoOptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/operators/CScalarNullTest.h"
#include "naucrates/md/CMDProviderMemory.h"

#include "unittest/base.h"
#include "unittest/gpopt/CTestUtils.h"

using namespace gpos;
using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CConstExprEvaluatorDefaultTest::EresUnittest
//
//	@doc:
//		Executes all unit tests for CConstExprEvaluatorDefault
//
//---------------------------------------------------------------------------
GPOS_RESULT
CConstExprEvaluatorDefaultTest::EresUnittest()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	gpos::owner<CConstExprEvaluatorDefault *> pceevaldefault =
		GPOS_NEW(mp) CConstExprEvaluatorDefault();
	GPOS_ASSERT(!pceevaldefault->FCanEvalExpressions());

	// setup a file-based provider
	gpos::owner<CMDProviderMemory *> pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	// Test evaluation of an integer constant
	{
		ULONG ulVal = 123456;
		gpos::owner<CExpression *> pexprUl =
			CUtils::PexprScalarConstInt4(mp, ulVal);
#ifdef GPOS_DEBUG
		gpos::owner<CExpression *> pexprUlResult =
			pceevaldefault->PexprEval(pexprUl);
		gpos::pointer<CScalarConst *> pscalarconstUl =
			gpos::dyn_cast<CScalarConst>(pexprUl->Pop());
		gpos::pointer<CScalarConst *> pscalarconstUlResult =
			gpos::dyn_cast<CScalarConst>(pexprUlResult->Pop());
		GPOS_ASSERT(pscalarconstUl->Matches(pscalarconstUlResult));
		pexprUlResult->Release();
#endif	// GPOS_DEBUG
		pexprUl->Release();
	}

	// Test evaluation of a null test expression
	{
		ULONG ulVal = 123456;
		gpos::owner<CExpression *> pexprUl =
			CUtils::PexprScalarConstInt4(mp, ulVal);
		gpos::owner<CExpression *> pexprIsNull =
			CUtils::PexprIsNull(mp, std::move(pexprUl));
#ifdef GPOS_DEBUG
		gpos::owner<CExpression *> pexprResult =
			pceevaldefault->PexprEval(pexprIsNull);
		gpos::pointer<gpopt::CScalarNullTest *> pscalarnulltest =
			gpos::dyn_cast<CScalarNullTest>(pexprIsNull->Pop());
		GPOS_ASSERT(pscalarnulltest->Matches(pexprResult->Pop()));
		pexprResult->Release();
#endif	// GPOS_DEBUG
		pexprIsNull->Release();
	}
	pceevaldefault->Release();

	return GPOS_OK;
}

// EOF
