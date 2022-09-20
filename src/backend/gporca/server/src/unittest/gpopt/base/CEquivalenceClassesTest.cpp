//---------------------------------------------------------------------------
//	VMware, Inc. or its affiliates
//	Copyright (C) 2017 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#include "unittest/gpopt/base/CEquivalenceClassesTest.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColumnFactory.h"
#include "gpopt/base/CQueryContext.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"
#include "gpopt/mdcache/CMDCache.h"
#include "naucrates/md/CMDProviderMemory.h"
#include "naucrates/md/IMDTypeInt4.h"

#include "unittest/base.h"
#include "unittest/gpopt/CTestUtils.h"
#include "unittest/gpopt/translate/CTranslatorExprToDXLTest.h"


// Unittest for bit vectors
GPOS_RESULT
CEquivalenceClassesTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CEquivalenceClassesTest::
							   EresUnittest_NotDisjointEquivalanceClasses),
		GPOS_UNITTEST_FUNC(
			CEquivalenceClassesTest::EresUnittest_IntersectEquivalanceClasses)};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

// Check disjoint equivalence classes are detected
GPOS_RESULT
CEquivalenceClassesTest::EresUnittest_NotDisjointEquivalanceClasses()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	gpos::owner<CColRefSet *> pcrs = GPOS_NEW(mp) CColRefSet(mp);

	// Setup an MD cache with a file-based provider
	gpos::owner<CMDProviderMemory *> pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(mp, CMDCache::Pcache());
	mda.RegisterProvider(CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	// get column factory from optimizer context object
	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();

	CWStringConst strName(GPOS_WSZ_LIT("Test Column"));
	CName name(&strName);

	gpos::pointer<const IMDTypeInt4 *> pmdtypeint4 =
		mda.PtMDType<IMDTypeInt4>();

	ULONG num_cols = 10;
	for (ULONG i = 0; i < num_cols; i++)
	{
		CColRef *colref =
			col_factory->PcrCreate(pmdtypeint4, default_type_modifier, name);
		pcrs->Include(colref);

		GPOS_ASSERT(pcrs->FMember(colref));
	}

	GPOS_ASSERT(pcrs->Size() == num_cols);

	gpos::owner<CColRefSet *> pcrsTwo = GPOS_NEW(mp) CColRefSet(mp, *pcrs);
	GPOS_ASSERT(pcrsTwo->Size() == num_cols);

	gpos::owner<CColRefSet *> pcrsThree = GPOS_NEW(mp) CColRefSet(mp);
	GPOS_ASSERT(pcrsThree->Size() == 0);
	CColRef *pcrThree =
		col_factory->PcrCreate(pmdtypeint4, default_type_modifier, name);
	pcrsThree->Include(pcrThree);
	GPOS_ASSERT(pcrsThree->Size() == 1);

	gpos::owner<CColRefSetArray *> pdrgpcrs = GPOS_NEW(mp) CColRefSetArray(mp);
	pcrs->AddRef();
	pcrsTwo->AddRef();
	pdrgpcrs->Append(pcrs);
	pdrgpcrs->Append(pcrsTwo);
	GPOS_ASSERT(!CUtils::FEquivalanceClassesDisjoint(mp, pdrgpcrs));

	gpos::owner<CColRefSetArray *> pdrgpcrsTwo =
		GPOS_NEW(mp) CColRefSetArray(mp);
	pcrs->AddRef();
	pcrsThree->AddRef();
	pdrgpcrsTwo->Append(pcrs);
	pdrgpcrsTwo->Append(pcrsThree);
	GPOS_ASSERT(CUtils::FEquivalanceClassesDisjoint(mp, pdrgpcrsTwo));

	pcrsThree->Release();
	pcrsTwo->Release();
	pcrs->Release();
	pdrgpcrs->Release();
	pdrgpcrsTwo->Release();

	return GPOS_OK;
}

// Check disjoint equivalence classes are detected
GPOS_RESULT
CEquivalenceClassesTest::EresUnittest_IntersectEquivalanceClasses()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	gpos::owner<CColRefSet *> pcrs = GPOS_NEW(mp) CColRefSet(mp);

	// Setup an MD cache with a file-based provider
	gpos::owner<CMDProviderMemory *> pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(mp, CMDCache::Pcache());
	mda.RegisterProvider(CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	// get column factory from optimizer context object
	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();

	CWStringConst strName(GPOS_WSZ_LIT("Test Column"));
	CName name(&strName);

	gpos::pointer<const IMDTypeInt4 *> pmdtypeint4 =
		mda.PtMDType<IMDTypeInt4>();

	ULONG num_cols = 10;
	for (ULONG i = 0; i < num_cols; i++)
	{
		CColRef *colref =
			col_factory->PcrCreate(pmdtypeint4, default_type_modifier, name);
		pcrs->Include(colref);

		GPOS_ASSERT(pcrs->FMember(colref));
	}

	GPOS_ASSERT(pcrs->Size() == num_cols);

	// Generate equivalence classes
	INT setBoundaryFirst[] = {2, 5, 7};
	gpos::owner<CColRefSetArray *> pdrgpFirst =
		CTestUtils::createEquivalenceClasses(mp, pcrs, setBoundaryFirst);

	INT setBoundarySecond[] = {1, 4, 5, 6};
	gpos::owner<CColRefSetArray *> pdrgpSecond =
		CTestUtils::createEquivalenceClasses(mp, pcrs, setBoundarySecond);

	INT setBoundaryExpected[] = {1, 2, 4, 5, 6, 7};
	gpos::owner<CColRefSetArray *> pdrgpIntersectExpectedOp =
		CTestUtils::createEquivalenceClasses(mp, pcrs, setBoundaryExpected);

	gpos::owner<CColRefSetArray *> pdrgpResult =
		CUtils::PdrgpcrsIntersectEquivClasses(mp, pdrgpFirst, pdrgpSecond);
	GPOS_ASSERT(CUtils::FEquivalanceClassesDisjoint(mp, pdrgpResult));
	GPOS_ASSERT(CUtils::FEquivalanceClassesEqual(mp, pdrgpResult,
												 pdrgpIntersectExpectedOp));

	pcrs->Release();
	pdrgpFirst->Release();
	pdrgpResult->Release();
	pdrgpSecond->Release();
	pdrgpIntersectExpectedOp->Release();

	return GPOS_OK;
}
// EOF
