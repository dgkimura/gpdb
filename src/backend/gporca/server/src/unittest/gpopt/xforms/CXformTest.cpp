//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CXformTest.cpp
//
//	@doc:
//		Test for CXForm
//---------------------------------------------------------------------------
#include "unittest/gpopt/xforms/CXformTest.h"

#include "gpos/io/COstreamString.h"
#include "gpos/string/CWStringDynamic.h"

#include "gpopt/base/CQueryContext.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"
#include "gpopt/operators/CLogicalCTEAnchor.h"
#include "gpopt/operators/CLogicalCTEConsumer.h"
#include "gpopt/operators/CLogicalCTEProducer.h"
#include "gpopt/operators/CLogicalFullOuterJoin.h"
#include "gpopt/xforms/CXform.h"
#include "gpopt/xforms/xforms.h"

#include "unittest/base.h"
#include "unittest/gpopt/CTestUtils.h"
#include "unittest/gpopt/translate/CTranslatorExprToDXLTest.h"
#include "unittest/gpopt/xforms/CDecorrelatorTest.h"


//---------------------------------------------------------------------------
//	@function:
//		CXformTest::EresUnittest
//
//	@doc:
//		Unittest driver
//
//---------------------------------------------------------------------------
GPOS_RESULT
CXformTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CXformTest::EresUnittest_ApplyXforms),
		GPOS_UNITTEST_FUNC(CXformTest::EresUnittest_ApplyXforms_CTE),
#ifdef GPOS_DEBUG
		GPOS_UNITTEST_FUNC(CXformTest::EresUnittest_Mapping),
#endif	// GPOS_DEBUG
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}


//---------------------------------------------------------------------------
//	@function:
//		CXformTest::EresUnittest_ApplyXforms
//
//	@doc:
//		Test application of different xforms
//
//---------------------------------------------------------------------------
GPOS_RESULT
CXformTest::EresUnittest_ApplyXforms()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	using Pfpexpr = gpos::Ref<CExpression> (*)(CMemoryPool *);
	Pfpexpr rgpf[] = {
		CTestUtils::PexprLogicalApplyWithOuterRef<CLogicalInnerApply>,
		CTestUtils::PexprLogicalApply<CLogicalLeftSemiApply>,
		CTestUtils::PexprLogicalApply<CLogicalLeftAntiSemiApply>,
		CTestUtils::PexprLogicalApply<CLogicalLeftAntiSemiApplyNotIn>,
		CTestUtils::PexprLogicalApplyWithOuterRef<CLogicalLeftOuterApply>,
		CTestUtils::PexprLogicalGet,
		CTestUtils::PexprLogicalForeignGet,
		CTestUtils::PexprLogicalSelect,
		CTestUtils::PexprLogicalLimit,
		CTestUtils::PexprLogicalJoin<CLogicalInnerJoin>,
		CTestUtils::PexprLogicalJoin<CLogicalLeftOuterJoin>,
		CTestUtils::PexprLogicalJoin<CLogicalLeftSemiJoin>,
		CTestUtils::PexprLogicalJoin<CLogicalLeftAntiSemiJoin>,
		CTestUtils::PexprLogicalJoin<CLogicalLeftAntiSemiJoinNotIn>,
		CTestUtils::PexprLogicalGbAgg,
		CTestUtils::PexprLogicalGbAggOverJoin,
		CTestUtils::PexprLogicalGbAggWithSum,
		CTestUtils::PexprLogicalGbAggDedupOverInnerJoin,
		CTestUtils::PexprLogicalNAryJoin,
		CTestUtils::PexprLeftOuterJoinOnNAryJoin,
		CTestUtils::PexprLogicalProject,
		CTestUtils::PexprLogicalSequence,
		CTestUtils::PexprLogicalGetPartitioned,
		CTestUtils::PexprLogicalSelectPartitioned,
		CTestUtils::PexprLogicalDynamicGet,
		CTestUtils::PexprJoinPartitionedInner<CLogicalInnerJoin>,
		CTestUtils::PexprLogicalSelectCmpToConst,
		CTestUtils::PexprLogicalTVFTwoArgs,
		CTestUtils::PexprLogicalTVFNoArgs,
		CTestUtils::PexprLogicalInsert,
		CTestUtils::PexprLogicalDelete,
		CTestUtils::PexprLogicalUpdate,
		CTestUtils::PexprLogicalAssert,
		CTestUtils::PexprLogicalJoin<CLogicalFullOuterJoin>,
		PexprJoinTree,
		CTestUtils::PexprLogicalJoinWithPartitionedAndIndexedInnerChild,
	};

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;

	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(rgpf); ul++)
	{
		CWStringDynamic str(mp);
		COstreamString oss(&str);

		// generate simple expression
		gpos::Ref<CExpression> pexpr = rgpf[ul](mp);
		ApplyExprXforms(mp, oss, pexpr);

		GPOS_TRACE(str.GetBuffer());
	}

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformTest::EresUnittest_ApplyXforms_CTE
//
//	@doc:
//		Test application of CTE-related xforms
//
//---------------------------------------------------------------------------
GPOS_RESULT
CXformTest::EresUnittest_ApplyXforms_CTE()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;

	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	// create producer
	ULONG ulCTEId = 0;
	gpos::Ref<CExpression> pexprProducer =
		CTestUtils::PexprLogicalCTEProducerOverSelect(mp, ulCTEId);
	COptCtxt::PoctxtFromTLS()->Pcteinfo()->AddCTEProducer(pexprProducer);

	pdrgpexpr->Append(pexprProducer);

	gpos::Ref<CColRefArray> pdrgpcrProducer =
		CLogicalCTEProducer::PopConvert(pexprProducer->Pop())->Pdrgpcr();
	gpos::Ref<CColRefArray> pdrgpcrConsumer =
		CUtils::PdrgpcrCopy(mp, pdrgpcrProducer);

	gpos::Ref<CExpression> pexprConsumer = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalCTEConsumer(mp, ulCTEId, pdrgpcrConsumer));

	pdrgpexpr->Append(pexprConsumer);
	COptCtxt::PoctxtFromTLS()->Pcteinfo()->IncrementConsumers(ulCTEId);


	gpos::Ref<CExpression> pexprSelect =
		CTestUtils::PexprLogicalSelect(mp, pexprConsumer);
	pdrgpexpr->Append(pexprSelect);


	gpos::Ref<CExpression> pexprAnchor = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalCTEAnchor(mp, ulCTEId), pexprSelect);

	pdrgpexpr->Append(pexprAnchor);

	const ULONG length = pdrgpexpr->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		CWStringDynamic str(mp);
		COstreamString oss(&str);

		ApplyExprXforms(mp, oss, (*pdrgpexpr)[ul]);

		GPOS_TRACE(str.GetBuffer());
	}


	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformTest::ApplyExprXforms
//
//	@doc:
//		Apply different xforms for the given expression
//
//---------------------------------------------------------------------------
void
CXformTest::ApplyExprXforms(CMemoryPool *mp, IOstream &os,
							gpos::Ref<CExpression> pexpr)
{
	os << std::endl << "EXPR:" << std::endl;
	(void) pexpr->OsPrint(os);

	for (ULONG ulXformId = 0; ulXformId < CXform::ExfSentinel; ulXformId++)
	{
		if (CXformFactory::Pxff()->IsXformIdUsed((CXform::EXformId) ulXformId))
		{
			gpos::Ref<CXform> pxform =
				CXformFactory::Pxff()->Pxf((CXform::EXformId) ulXformId);
			os << std::endl << "XFORM " << pxform->SzId() << ":" << std::endl;

			gpos::Ref<CXformContext> pxfctxt = GPOS_NEW(mp) CXformContext(mp);
			gpos::Ref<CXformResult> pxfres = GPOS_NEW(mp) CXformResult(mp);

#ifdef GPOS_DEBUG
			if (pxform->FCheckPattern(pexpr) &&
				CXform::FPromising(mp, pxform, pexpr))
			{
				if (CXform::ExfExpandNAryJoinMinCard == pxform->Exfid())
				{
					GPOS_ASSERT(COperator::EopLogicalNAryJoin ==
								pexpr->Pop()->Eopid());

					// derive stats on NAry join expression
					CExpressionHandle exprhdl(mp);
					exprhdl.Attach(pexpr);
					exprhdl.DeriveStats(mp, mp, nullptr /*prprel*/,
										nullptr /*stats_ctxt*/);
				}

				pxform->Transform(pxfctxt, pxfres, pexpr);

				gpos::Ref<CExpression> pexprResult = pxfres->PexprNext();
				while (nullptr != pexprResult)
				{
					GPOS_ASSERT(pexprResult->FMatchDebug(pexprResult));

					pexprResult = pxfres->PexprNext();
				}
				(void) pxfres->OsPrint(os);
			}
#endif	// GPOS_DEBUG
		}
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CXformTest::PexprStarJoinTree
//
//	@doc:
//		Generate a randomized star join tree
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CXformTest::PexprStarJoinTree(CMemoryPool *mp, ULONG ulTabs)
{
	gpos::Ref<CExpression> pexprLeft = CTestUtils::PexprLogicalGet(mp);

	for (ULONG ul = 1; ul < ulTabs; ul++)
	{
		gpos::Ref<CColRef> pcrLeft = pexprLeft->DeriveOutputColumns()->PcrAny();
		gpos::Ref<CExpression> pexprRight = CTestUtils::PexprLogicalGet(mp);
		gpos::Ref<CColRef> pcrRight =
			pexprRight->DeriveOutputColumns()->PcrAny();

		gpos::Ref<CExpression> pexprPred =
			CUtils::PexprScalarEqCmp(mp, pcrLeft, pcrRight);

		pexprLeft = CUtils::PexprLogicalJoin<CLogicalInnerJoin>(
			mp, pexprLeft, pexprRight, pexprPred);
	}

	return pexprLeft;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformTest::PexprJoinTree
//
//	@doc:
//		Generate a randomized star join tree
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CXformTest::PexprJoinTree(CMemoryPool *mp)
{
	return PexprStarJoinTree(mp, 3);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CXformTest::EresUnittest_Mapping
//
//	@doc:
//		Test name -> xform mapping
//
//---------------------------------------------------------------------------
GPOS_RESULT
CXformTest::EresUnittest_Mapping()
{
	for (ULONG ul = 0; ul < CXform::ExfSentinel; ul++)
	{
		if (CXformFactory::Pxff()->IsXformIdUsed((CXform::EXformId) ul))
		{
			CXform::EXformId exfid = (CXform::EXformId) ul;
			gpos::Ref<CXform> pxform = CXformFactory::Pxff()->Pxf(exfid);
			gpos::Ref<CXform> pxformMapped =
				CXformFactory::Pxff()->Pxf(pxform->SzId());
			GPOS_ASSERT(pxform == pxformMapped);
		}
	}

	return GPOS_OK;
}
#endif	// GPOS_DEBUG


// EOF
