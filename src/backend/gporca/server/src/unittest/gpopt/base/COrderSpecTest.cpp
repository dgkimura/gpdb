//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		COrderSpecTest.cpp
//
//	@doc:
//		Tests for order specification
//---------------------------------------------------------------------------
#include "unittest/gpopt/base/COrderSpecTest.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CColumnFactory.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CQueryContext.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"
#include "naucrates/md/CMDProviderMemory.h"
#include "naucrates/md/IMDId.h"
#include "naucrates/md/IMDTypeInt4.h"

#include "unittest/base.h"
#include "unittest/gpopt/CTestUtils.h"
#include "unittest/gpopt/translate/CTranslatorExprToDXLTest.h"

//---------------------------------------------------------------------------
//	@function:
//		COrderSpecTest::EresUnittest
//
//	@doc:
//		Unittest for order spec classes
//
//---------------------------------------------------------------------------
GPOS_RESULT
COrderSpecTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(COrderSpecTest::EresUnittest_Basics)};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

//---------------------------------------------------------------------------
//	@function:
//		COrderSpecTest::EresUnittest_Basics
//
//	@doc:
//		Basic order spec tests
//
//---------------------------------------------------------------------------
GPOS_RESULT
COrderSpecTest::EresUnittest_Basics()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// Setup an MD cache with a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	// get column factory from optimizer context object
	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();

	CWStringConst strName(GPOS_WSZ_LIT("Test Column"));
	CName name(&strName);

	const IMDTypeInt4 *pmdtypeint4 =
		mda.PtMDType<IMDTypeInt4>(CTestUtils::m_sysidDefault);

	CColRef *pcr1 =
		col_factory->PcrCreate(pmdtypeint4, default_type_modifier, name);
	CColRef *pcr2 =
		col_factory->PcrCreate(pmdtypeint4, default_type_modifier, name);
	CColRef *pcr3 =
		col_factory->PcrCreate(pmdtypeint4, default_type_modifier, name);


	gpos::Ref<COrderSpec> pos1 = GPOS_NEW(mp) COrderSpec(mp);

	gpos::Ref<IMDId> pmdidInt4LT =
		pmdtypeint4->GetMdidForCmpType(IMDType::EcmptL);
	;
	;

	pos1->Append(pmdidInt4LT, pcr1, COrderSpec::EntFirst);
	pos1->Append(pmdidInt4LT, pcr2, COrderSpec::EntLast);

	GPOS_ASSERT(pos1->Matches(pos1.get()));
	GPOS_ASSERT(pos1->FSatisfies(pos1.get()));

	gpos::Ref<COrderSpec> pos2 = GPOS_NEW(mp) COrderSpec(mp);
	;
	;
	;

	pos2->Append(pmdidInt4LT, pcr1, COrderSpec::EntFirst);
	pos2->Append(pmdidInt4LT, pcr2, COrderSpec::EntLast);
	pos2->Append(pmdidInt4LT, pcr3, COrderSpec::EntAuto);

	(void) pos1->HashValue();
	(void) pos2->HashValue();

	GPOS_ASSERT(pos2->Matches(pos2.get()));
	GPOS_ASSERT(pos2->FSatisfies(pos2.get()));


	GPOS_ASSERT(!pos1->Matches(pos2.get()));
	GPOS_ASSERT(!pos2->Matches(pos1.get()));

	GPOS_ASSERT(pos2->FSatisfies(pos1.get()));
	GPOS_ASSERT(!pos1->FSatisfies(pos2.get()));

	// iterate over the components of the order spec
	for (ULONG ul = 0; ul < pos1->UlSortColumns(); ul++)
	{
		const CColRef *colref GPOS_ASSERTS_ONLY = pos1->Pcr(ul);

		GPOS_ASSERT(nullptr != colref);

		const IMDId *mdid GPOS_ASSERTS_ONLY = pos1->GetMdIdSortOp(ul);

		GPOS_ASSERT(mdid->IsValid());

		(void) pos1->Ent(ul);
	}

	;
	;

	return GPOS_OK;
}


// EOF
