//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CFunctionalDependencyTest.cpp
//
//	@doc:
//		Tests for functional dependencies
//---------------------------------------------------------------------------
#include "unittest/gpopt/base/CFunctionalDependencyTest.h"

#include "gpos/common/owner.h"
#include "gpos/error/CAutoTrace.h"

#include "gpopt/base/CFunctionalDependency.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"

#include "unittest/base.h"
#include "unittest/gpopt/CTestUtils.h"


//---------------------------------------------------------------------------
//	@function:
//		CFunctionalDependencyTest::EresUnittest
//
//	@doc:
//		Unittest for functional dependencies
//
//---------------------------------------------------------------------------
GPOS_RESULT
CFunctionalDependencyTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CFunctionalDependencyTest::EresUnittest_Basics)};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

//---------------------------------------------------------------------------
//	@function:
//		CFunctionalDependencyTest::EresUnittest_Basics
//
//	@doc:
//		Basic test for functional dependencies
//
//---------------------------------------------------------------------------
GPOS_RESULT
CFunctionalDependencyTest::EresUnittest_Basics()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// Setup an MD cache with a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache());
	mda.RegisterProvider(CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	// get column factory from optimizer context object
	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();

	CWStringConst strName(GPOS_WSZ_LIT("Test Column"));
	CName name(&strName);

	const IMDTypeInt4 *pmdtypeint4 = mda.PtMDType<IMDTypeInt4>();

	const ULONG num_cols = 3;
	gpos::Ref<CColRefSet> pcrsLeft = GPOS_NEW(mp) CColRefSet(mp);
	gpos::Ref<CColRefSet> pcrsRight = GPOS_NEW(mp) CColRefSet(mp);
	for (ULONG ul = 0; ul < num_cols; ul++)
	{
		CColRef *colref =
			col_factory->PcrCreate(pmdtypeint4, default_type_modifier, name);
		pcrsLeft->Include(colref);

		colref =
			col_factory->PcrCreate(pmdtypeint4, default_type_modifier, name);
		pcrsRight->Include(colref);
	}

	;
	;
	gpos::Ref<CFunctionalDependency> pfdFst =
		GPOS_NEW(mp) CFunctionalDependency(pcrsLeft, pcrsRight);

	;
	;
	gpos::Ref<CFunctionalDependency> pfdSnd =
		GPOS_NEW(mp) CFunctionalDependency(pcrsLeft, pcrsRight);

	GPOS_ASSERT(pfdFst->Equals(pfdSnd.get()));
	GPOS_ASSERT(pfdFst->HashValue() == pfdSnd->HashValue());

	gpos::Ref<CFunctionalDependencyArray> pdrgpfd =
		GPOS_NEW(mp) CFunctionalDependencyArray(mp);
	;
	pdrgpfd->Append(pfdFst);
	;
	pdrgpfd->Append(pfdSnd);
	GPOS_ASSERT(CFunctionalDependency::Equals(pdrgpfd.get(), pdrgpfd.get()));

	gpos::Ref<CColRefArray> colref_array =
		CFunctionalDependency::PdrgpcrKeys(mp, pdrgpfd.get());
	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp);
	pcrs->Include(colref_array.get());
	gpos::Ref<CColRefSet> pcrsKeys =
		CFunctionalDependency::PcrsKeys(mp, pdrgpfd.get());

	GPOS_ASSERT(pcrsLeft->Equals(pcrs.get()));
	GPOS_ASSERT(pcrsKeys->Equals(pcrs.get()));

	CAutoTrace at(mp);
	at.Os() << "FD1:" << *pfdFst << std::endl << "FD2:" << *pfdSnd << std::endl;

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


// EOF
