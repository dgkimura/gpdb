//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CDistributionSpecTest.cpp
//
//	@doc:
//		Tests for distribution specification
//---------------------------------------------------------------------------

#include "unittest/gpopt/base/CDistributionSpecTest.h"

#include "gpos/common/owner.h"
#include "gpos/error/CAutoTrace.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColumnFactory.h"
#include "gpopt/base/CDistributionSpecAny.h"
#include "gpopt/base/CDistributionSpecHashed.h"
#include "gpopt/base/CDistributionSpecRandom.h"
#include "gpopt/base/CDistributionSpecReplicated.h"
#include "gpopt/base/CDistributionSpecSingleton.h"
#include "gpopt/base/CDistributionSpecUniversal.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/mdcache/CMDCache.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "naucrates/md/CMDProviderMemory.h"
#include "naucrates/md/IMDTypeInt4.h"

#include "unittest/base.h"
#include "unittest/gpopt/CTestUtils.h"


const CHAR *szMDFileName = "../data/dxl/metadata/md.xml";

//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecTest::EresUnittest
//
//	@doc:
//		Unittest for distribution spec classes
//
//---------------------------------------------------------------------------
GPOS_RESULT
CDistributionSpecTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDistributionSpecTest::EresUnittest_Any),
		GPOS_UNITTEST_FUNC(CDistributionSpecTest::EresUnittest_Singleton),
		GPOS_UNITTEST_FUNC(CDistributionSpecTest::EresUnittest_Random),
		GPOS_UNITTEST_FUNC(CDistributionSpecTest::EresUnittest_Replicated),
		GPOS_UNITTEST_FUNC(CDistributionSpecTest::EresUnittest_Universal),
		GPOS_UNITTEST_FUNC(CDistributionSpecTest::EresUnittest_Hashed),
#ifdef GPOS_DEBUG
		GPOS_UNITTEST_FUNC_ASSERT(
			CDistributionSpecTest::EresUnittest_NegativeAny),
		GPOS_UNITTEST_FUNC_ASSERT(
			CDistributionSpecTest::EresUnittest_NegativeUniversal),
		GPOS_UNITTEST_FUNC_ASSERT(
			CDistributionSpecTest::EresUnittest_NegativeRandom),
#endif	// GPOS_DEBUG
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}


//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecTest::EresUnittest_Any
//
//	@doc:
//		Test for "any" distribution spec
//
//---------------------------------------------------------------------------
GPOS_RESULT
CDistributionSpecTest::EresUnittest_Any()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// any distribution
	gpos::Ref<CDistributionSpecAny> pdsany =
		GPOS_NEW(mp) CDistributionSpecAny(COperator::EopSentinel);

	GPOS_ASSERT(pdsany->FSatisfies(pdsany.get()));
	GPOS_ASSERT(pdsany->Matches(pdsany.get()));

	CAutoTrace at(mp);
	at.Os() << std::endl;
	at.Os() << *pdsany << std::endl;

	;

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecTest::EresUnittest_Random
//
//	@doc:
//		Test for forced random distribution spec
//
//---------------------------------------------------------------------------
GPOS_RESULT
CDistributionSpecTest::EresUnittest_Random()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr /*pceeval*/,
					 CTestUtils::GetCostModel(mp));
	COptCtxt *poptctxt = COptCtxt::PoctxtFromTLS();

	// basic tests with random distribution
	poptctxt->MarkDMLQuery(true /*fDMLQuery*/);
	gpos::Ref<CDistributionSpecRandom> pdsRandomDuplicateSensitive =
		GPOS_NEW(mp) CDistributionSpecRandom();

	poptctxt->MarkDMLQuery(false /*fDMLQuery*/);
	gpos::Ref<CDistributionSpecRandom> pdsRandomNonDuplicateSensitive =
		GPOS_NEW(mp) CDistributionSpecRandom();

	GPOS_ASSERT(pdsRandomDuplicateSensitive->FSatisfies(
		pdsRandomDuplicateSensitive.get()));
	GPOS_ASSERT(pdsRandomDuplicateSensitive->Matches(
		pdsRandomDuplicateSensitive.get()));
	GPOS_ASSERT(pdsRandomDuplicateSensitive->FSatisfies(
		pdsRandomNonDuplicateSensitive.get()));
	GPOS_ASSERT(!pdsRandomNonDuplicateSensitive->FSatisfies(
		pdsRandomDuplicateSensitive.get()));

	// random and universal
	gpos::Ref<CDistributionSpecUniversal> pdsUniversal =
		GPOS_NEW(mp) CDistributionSpecUniversal();
	GPOS_ASSERT(pdsUniversal->FSatisfies(pdsRandomNonDuplicateSensitive.get()));
	GPOS_ASSERT(!pdsUniversal->FSatisfies(pdsRandomDuplicateSensitive.get()));

	// random and any
	gpos::Ref<CDistributionSpecAny> pdsany =
		GPOS_NEW(mp) CDistributionSpecAny(COperator::EopSentinel);

	GPOS_ASSERT(!pdsany->FSatisfies(pdsRandomDuplicateSensitive.get()));
	GPOS_ASSERT(pdsRandomDuplicateSensitive->FSatisfies(pdsany.get()));

	CAutoTrace at(mp);
	at.Os() << std::endl;
	at.Os() << *pdsRandomDuplicateSensitive << std::endl;

	;
	;
	;
	;

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecTest::EresUnittest_Replicated
//
//	@doc:
//		Test for replicated distribution spec
//
//---------------------------------------------------------------------------
GPOS_RESULT
CDistributionSpecTest::EresUnittest_Replicated()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr /*pceeval*/,
					 CTestUtils::GetCostModel(mp));

	// basic tests with replicated distributions
	gpos::Ref<CDistributionSpecReplicated> pdsreplicated = GPOS_NEW(mp)
		CDistributionSpecReplicated(CDistributionSpec::EdtStrictReplicated);

	GPOS_ASSERT(pdsreplicated->FSatisfies(pdsreplicated.get()));
	GPOS_ASSERT(pdsreplicated->Matches(pdsreplicated.get()));

	// replicated and random
	gpos::Ref<CDistributionSpecRandom> pdsrandom =
		GPOS_NEW(mp) CDistributionSpecRandom();

	GPOS_ASSERT(!pdsrandom->FSatisfies(pdsreplicated.get()));
	GPOS_ASSERT(pdsreplicated->FSatisfies(pdsrandom.get()));

	// replicated and any
	gpos::Ref<CDistributionSpecAny> pdsany =
		GPOS_NEW(mp) CDistributionSpecAny(COperator::EopSentinel);

	GPOS_ASSERT(!pdsany->FSatisfies(pdsreplicated.get()));
	GPOS_ASSERT(pdsreplicated->FSatisfies(pdsany.get()));

	CAutoTrace at(mp);
	at.Os() << std::endl;
	at.Os() << *pdsreplicated << std::endl;

	;
	;
	;

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecTest::EresUnittest_Singleton
//
//	@doc:
//		Test for singleton distribution spec
//
//---------------------------------------------------------------------------
GPOS_RESULT
CDistributionSpecTest::EresUnittest_Singleton()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr /*pceeval*/,
					 CTestUtils::GetCostModel(mp));

	// basic tests with singleton distributions
	gpos::Ref<CDistributionSpecSingleton> pdssSegment = GPOS_NEW(mp)
		CDistributionSpecSingleton(CDistributionSpecSingleton::EstSegment);
	gpos::Ref<CDistributionSpecSingleton> pdssMaster = GPOS_NEW(mp)
		CDistributionSpecSingleton(CDistributionSpecSingleton::EstMaster);

	GPOS_ASSERT(pdssMaster->FSatisfies(pdssMaster.get()));
	GPOS_ASSERT(pdssMaster->Matches(pdssMaster.get()));

	GPOS_ASSERT(pdssSegment->FSatisfies(pdssSegment.get()));
	GPOS_ASSERT(pdssSegment->Matches(pdssSegment.get()));

	GPOS_ASSERT(!pdssMaster->FSatisfies(pdssSegment.get()) &&
				!pdssSegment->FSatisfies(pdssMaster.get()));

	// singleton and replicated
	gpos::Ref<CDistributionSpecReplicated> pdsreplicated = GPOS_NEW(mp)
		CDistributionSpecReplicated(CDistributionSpec::EdtStrictReplicated);

	GPOS_ASSERT(pdsreplicated->FSatisfies(pdssSegment.get()));
	GPOS_ASSERT(!pdsreplicated->FSatisfies(pdssMaster.get()));

	GPOS_ASSERT(!pdssSegment->FSatisfies(pdsreplicated.get()));

	// singleton and random
	gpos::Ref<CDistributionSpecRandom> pdsrandom =
		GPOS_NEW(mp) CDistributionSpecRandom();

	GPOS_ASSERT(!pdsrandom->FSatisfies(pdssSegment.get()));
	GPOS_ASSERT(!pdsrandom->FSatisfies(pdssMaster.get()));
	GPOS_ASSERT(!pdssSegment->FSatisfies(pdsrandom.get()));
	GPOS_ASSERT(!pdssMaster->FSatisfies(pdsrandom.get()));

	// singleton and any
	gpos::Ref<CDistributionSpecAny> pdsany =
		GPOS_NEW(mp) CDistributionSpecAny(COperator::EopSentinel);

	GPOS_ASSERT(!pdsany->FSatisfies(pdssSegment.get()));
	GPOS_ASSERT(pdssSegment->FSatisfies(pdsany.get()));

	CAutoTrace at(mp);
	at.Os() << std::endl;
	at.Os() << *pdssMaster << std::endl;
	at.Os() << *pdssSegment << std::endl;

	;
	;
	;
	;
	;

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecTest::EresUnittest_Universal
//
//	@doc:
//		Test for universal distribution spec
//
//---------------------------------------------------------------------------
GPOS_RESULT
CDistributionSpecTest::EresUnittest_Universal()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr /*pceeval*/,
					 CTestUtils::GetCostModel(mp));

	// basic tests with universal distributions
	gpos::Ref<CDistributionSpecUniversal> pdsuniversal =
		GPOS_NEW(mp) CDistributionSpecUniversal();

	GPOS_ASSERT(pdsuniversal->FSatisfies(pdsuniversal.get()));
	GPOS_ASSERT(pdsuniversal->Matches(pdsuniversal.get()));

	// universal and singleton
	gpos::Ref<CDistributionSpecSingleton> pdssSegment = GPOS_NEW(mp)
		CDistributionSpecSingleton(CDistributionSpecSingleton::EstSegment);

	GPOS_ASSERT(pdsuniversal->FSatisfies(pdssSegment.get()));

	// universal and replicated
	gpos::Ref<CDistributionSpecReplicated> pdsreplicated = GPOS_NEW(mp)
		CDistributionSpecReplicated(CDistributionSpec::EdtStrictReplicated);

	GPOS_ASSERT(pdsuniversal->FSatisfies(pdsreplicated.get()));

	// universal and random
	gpos::Ref<CDistributionSpecRandom> pdsrandom =
		GPOS_NEW(mp) CDistributionSpecRandom();

	GPOS_ASSERT(pdsuniversal->FSatisfies(pdsrandom.get()));

	// universal and any
	gpos::Ref<CDistributionSpecAny> pdsany =
		GPOS_NEW(mp) CDistributionSpecAny(COperator::EopSentinel);

	GPOS_ASSERT(pdsuniversal->FSatisfies(pdsany.get()));

	CAutoTrace at(mp);
	at.Os() << std::endl;
	at.Os() << *pdsuniversal << std::endl;

	;
	;
	;
	;
	;

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecTest::EresUnittest_Hashed
//
//	@doc:
//		Test for hashed distribution spec
//
//---------------------------------------------------------------------------
GPOS_RESULT
CDistributionSpecTest::EresUnittest_Hashed()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));

	// install opt context
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	// get column factory from optimizer context object
	COptCtxt *poptctxt = COptCtxt::PoctxtFromTLS();
	CColumnFactory *col_factory = poptctxt->Pcf();

	CWStringConst strA(GPOS_WSZ_LIT("A"));
	CWStringConst strB(GPOS_WSZ_LIT("B"));

	CName nameA(&strA);
	CName nameB(&strB);

	const IMDTypeInt4 *pmdtypeint4 =
		mda.PtMDType<IMDTypeInt4>(CTestUtils::m_sysidDefault);

	CColRef *pcrA =
		col_factory->PcrCreate(pmdtypeint4, default_type_modifier, nameA);
	CColRef *pcrB =
		col_factory->PcrCreate(pmdtypeint4, default_type_modifier, nameB);

	gpos::Ref<CExpression> pexprScalarA = CUtils::PexprScalarIdent(mp, pcrA);
	gpos::Ref<CExpression> pexprScalarB = CUtils::PexprScalarIdent(mp, pcrB);

	gpos::Ref<CExpressionArray> prgpexpr1 = GPOS_NEW(mp) CExpressionArray(mp);
	prgpexpr1->Append(pexprScalarA);
	prgpexpr1->Append(std::move(pexprScalarB));

	gpos::Ref<CExpressionArray> prgpexpr2 = GPOS_NEW(mp) CExpressionArray(mp);
	;
	prgpexpr2->Append(pexprScalarA);

	// basic hash distribution tests

	// HD{A,B}, nulls colocated, duplicate insensitive
	poptctxt->MarkDMLQuery(false /*fDMLQuery*/);
	gpos::Ref<CDistributionSpecHashed> pdshashed1 =
		GPOS_NEW(mp) CDistributionSpecHashed(std::move(prgpexpr1),
											 true /* fNullsCollocated */);
	GPOS_ASSERT(pdshashed1->FSatisfies(pdshashed1.get()));
	GPOS_ASSERT(pdshashed1->Matches(pdshashed1.get()));

	// HD{A}, nulls colocated, duplicate sensitive
	poptctxt->MarkDMLQuery(true /*fDMLQuery*/);
	gpos::Ref<CDistributionSpecHashed> pdshashed2 = GPOS_NEW(mp)
		CDistributionSpecHashed(prgpexpr2, true /* fNullsCollocated */);
	GPOS_ASSERT(pdshashed2->FSatisfies(pdshashed2.get()));
	GPOS_ASSERT(pdshashed2->Matches(pdshashed2.get()));

	// HD{A}, nulls not colocated, duplicate sensitive
	;
	gpos::Ref<CDistributionSpecHashed> pdshashed3 = GPOS_NEW(mp)
		CDistributionSpecHashed(prgpexpr2, false /* fNullsCollocated */);
	GPOS_ASSERT(pdshashed3->FSatisfies(pdshashed3.get()));
	GPOS_ASSERT(pdshashed3->Matches(pdshashed3.get()));

	// ({A,B}, true, true) does not satisfy ({A}, true, false)
	GPOS_ASSERT(!pdshashed1->FMatchSubset(pdshashed2.get()));
	GPOS_ASSERT(!pdshashed1->FSatisfies(pdshashed2.get()));

	// ({A}, true) satisfies ({A, B}, true)
	GPOS_ASSERT(pdshashed2->FMatchSubset(pdshashed1.get()));
	GPOS_ASSERT(pdshashed2->FSatisfies(pdshashed1.get()));

	// ({A}, true) satisfies ({A}, false)
	GPOS_ASSERT(pdshashed2->FMatchSubset(pdshashed3.get()));
	GPOS_ASSERT(pdshashed2->FSatisfies(pdshashed3.get()));

	// ({A}, false) does not satisfy ({A}, true)
	GPOS_ASSERT(!pdshashed1->FMatchSubset(pdshashed3.get()));
	GPOS_ASSERT(!pdshashed1->FSatisfies(pdshashed3.get()));

	// hashed and universal
	gpos::Ref<CDistributionSpecUniversal> pdsuniversal =
		GPOS_NEW(mp) CDistributionSpecUniversal();

	GPOS_ASSERT(pdsuniversal->FSatisfies(pdshashed1.get()));

	// hashed and singleton
	gpos::Ref<CDistributionSpecSingleton> pdssSegment = GPOS_NEW(mp)
		CDistributionSpecSingleton(CDistributionSpecSingleton::EstSegment);

	GPOS_ASSERT(!pdshashed1->FSatisfies(pdssSegment.get()));
	GPOS_ASSERT(!pdssSegment->Matches(pdshashed1.get()));

	// hashed and replicated
	gpos::Ref<CDistributionSpecReplicated> pdsreplicated = GPOS_NEW(mp)
		CDistributionSpecReplicated(CDistributionSpec::EdtStrictReplicated);

	GPOS_ASSERT(pdsreplicated->FSatisfies(pdshashed1.get()));
	GPOS_ASSERT(!pdshashed1->FSatisfies(pdsreplicated.get()));

	// hashed and random
	gpos::Ref<CDistributionSpecRandom> pdsrandom =
		GPOS_NEW(mp) CDistributionSpecRandom();

	GPOS_ASSERT(!pdsrandom->FSatisfies(pdshashed1.get()));
	GPOS_ASSERT(!pdshashed1->FSatisfies(pdsrandom.get()));

	// hashed and any
	gpos::Ref<CDistributionSpecAny> pdsany =
		GPOS_NEW(mp) CDistributionSpecAny(COperator::EopSentinel);

	GPOS_ASSERT(!pdsany->FSatisfies(pdshashed1.get()));
	GPOS_ASSERT(pdshashed1->FSatisfies(pdsany.get()));

	CAutoTrace at(mp);
	at.Os() << std::endl;
	at.Os() << *pdshashed1 << std::endl;
	at.Os() << *pdshashed2 << std::endl;
	at.Os() << *pdshashed3 << std::endl;

	;
	;
	;

	;
	;
	;
	;
	;

	return GPOS_OK;
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecTest::EresUnittest_NegativeAny
//
//	@doc:
//		Negative test for ANY distribution spec
//
//---------------------------------------------------------------------------
GPOS_RESULT
CDistributionSpecTest::EresUnittest_NegativeAny()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// cannot add enforcers for ANY distribution
	gpos::Ref<CDistributionSpecAny> pdsany =
		GPOS_NEW(mp) CDistributionSpecAny(COperator::EopSentinel);
	CExpressionHandle *pexprhdl = GPOS_NEW(mp) CExpressionHandle(mp);
	pdsany->AppendEnforcers(nullptr /*mp*/, *pexprhdl, nullptr /*prpp*/,
							nullptr /*pdrgpexpr*/, nullptr /*pexpr*/);
	;
	GPOS_DELETE(pexprhdl);

	return GPOS_FAILED;
}


//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecTest::EresUnittest_NegativeUniversal
//
//	@doc:
//		Negative test for universal distribution spec
//
//---------------------------------------------------------------------------
GPOS_RESULT
CDistributionSpecTest::EresUnittest_NegativeUniversal()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// cannot add enforcers for Universal distribution
	gpos::Ref<CDistributionSpecUniversal> pdsuniversal =
		GPOS_NEW(mp) CDistributionSpecUniversal();
	CExpressionHandle *pexprhdl = GPOS_NEW(mp) CExpressionHandle(mp);

	pdsuniversal->AppendEnforcers(nullptr /*mp*/, *pexprhdl, nullptr /*prpp*/,
								  nullptr /*pdrgpexpr*/, nullptr /*pexpr*/);
	;
	GPOS_DELETE(pexprhdl);

	return GPOS_FAILED;
}

//---------------------------------------------------------------------------
//	@function:
//		CDistributionSpecTest::EresUnittest_NegativeRandom
//
//	@doc:
//		Negative test for random distribution spec
//
//---------------------------------------------------------------------------
GPOS_RESULT
CDistributionSpecTest::EresUnittest_NegativeRandom()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr /*pceeval*/,
					 CTestUtils::GetCostModel(mp));

	// cannot add enforcers for Random distribution
	gpos::Ref<CDistributionSpecRandom> pdsrandom =
		GPOS_NEW(mp) CDistributionSpecRandom();
	CExpressionHandle *pexprhdl = GPOS_NEW(mp) CExpressionHandle(mp);

	pdsrandom->AppendEnforcers(nullptr /*mp*/, *pexprhdl, nullptr /*prpp*/,
							   nullptr /*pdrgpexpr*/, nullptr /*pexpr*/);
	;
	GPOS_DELETE(pexprhdl);

	return GPOS_FAILED;
}
#endif	// GPOS_DEBUG

// EOF
