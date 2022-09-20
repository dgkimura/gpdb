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

	gpos::Ref<CConstExprEvaluatorDefault> pceevaldefault =
		GPOS_NEW(mp) CConstExprEvaluatorDefault();
	GPOS_ASSERT(!pceevaldefault->FCanEvalExpressions());

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	// Test evaluation of an integer constant
	{
		ULONG ulVal = 123456;
		gpos::Ref<CExpression> pexprUl =
			CUtils::PexprScalarConstInt4(mp, ulVal);
#ifdef GPOS_DEBUG
		gpos::Ref<CExpression> pexprUlResult =
			pceevaldefault->PexprEval(pexprUl.get());
		CScalarConst *pscalarconstUl =
			gpos::dyn_cast<CScalarConst>(pexprUl->Pop());
		CScalarConst *pscalarconstUlResult =
			gpos::dyn_cast<CScalarConst>(pexprUlResult->Pop());
		GPOS_ASSERT(pscalarconstUl->Matches(pscalarconstUlResult));
		;
#endif	// GPOS_DEBUG
		;
	}

	// Test evaluation of a null test expression
	{
		ULONG ulVal = 123456;
		gpos::Ref<CExpression> pexprUl =
			CUtils::PexprScalarConstInt4(mp, ulVal);
		gpos::Ref<CExpression> pexprIsNull =
			CUtils::PexprIsNull(mp, std::move(pexprUl));
#ifdef GPOS_DEBUG
		gpos::Ref<CExpression> pexprResult =
			pceevaldefault->PexprEval(pexprIsNull.get());
		gpopt::CScalarNullTest *pscalarnulltest =
			gpos::dyn_cast<CScalarNullTest>(pexprIsNull->Pop());
		GPOS_ASSERT(pscalarnulltest->Matches(pexprResult->Pop()));
		;
#endif	// GPOS_DEBUG
		;
	};

	return GPOS_OK;
}

// EOF
