//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CPartConstraintTest.cpp
//
//	@doc:
//      Test for CPartConstraint
//---------------------------------------------------------------------------

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#include "gpos/common/owner.h"
#endif

#include <stdint.h>

#include "gpos/task/CAutoTraceFlag.h"

#include "gpopt/base/CColumnFactory.h"
#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/mdcache/CMDCache.h"
#include "gpopt/metadata/CPartConstraint.h"
#include "naucrates/md/CMDIdGPDB.h"

#include "unittest/base.h"
#include "unittest/gpopt/CConstExprEvaluatorForDates.h"
#include "unittest/gpopt/CTestUtils.h"
#include "unittest/gpopt/metadata/CPartConstraintTest.h"

// number of microseconds in one day
const LINT CPartConstraintTest::lMicrosecondsPerDay =
	24 * 60 * 60 * INT64_C(1000000);

// date for '01-01-2012'
const LINT CPartConstraintTest::lInternalRepresentationFor2012_01_01 =
	LINT(4383) * CPartConstraintTest::lMicrosecondsPerDay;

// date for '01-21-2012'
const LINT CPartConstraintTest::lInternalRepresentationFor2012_01_21 =
	LINT(5003) * CPartConstraintTest::lMicrosecondsPerDay;

// date for '01-22-2012'
const LINT CPartConstraintTest::lInternalRepresentationFor2012_01_22 =
	LINT(5004) * CPartConstraintTest::lMicrosecondsPerDay;

// byte representation for '01-01-2012'
const WCHAR *CPartConstraintTest::wszInternalRepresentationFor2012_01_01 =
	GPOS_WSZ_LIT("HxEAAA==");

// byte representation for '01-21-2012'
const WCHAR *CPartConstraintTest::wszInternalRepresentationFor2012_01_21 =
	GPOS_WSZ_LIT("MxEAAA==");

// byte representation for '01-22-2012'
const WCHAR *CPartConstraintTest::wszInternalRepresentationFor2012_01_22 =
	GPOS_WSZ_LIT("MhEAAA==");

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraintTest::EresUnittest
//
//	@doc:
//		Unittest for part constraints
//
//---------------------------------------------------------------------------
GPOS_RESULT
CPartConstraintTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CPartConstraintTest::EresUnittest_Basic),
		GPOS_UNITTEST_FUNC(CPartConstraintTest::EresUnittest_DateIntervals),
	};
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));
	gpos::Ref<CConstExprEvaluatorForDates> pceeval =
		GPOS_NEW(mp) CConstExprEvaluatorForDates(mp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, std::move(pceeval),
					 CTestUtils::GetCostModel(mp));

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}


//---------------------------------------------------------------------------
//	@function:
//		CPartConstraintTest::EresUnittest_Basic
//
//	@doc:
//		Basic test for subsumption, equality and overlap of part constraints
//
//---------------------------------------------------------------------------
GPOS_RESULT
CPartConstraintTest::EresUnittest_Basic()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup an MD accessor
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));

	const IMDTypeInt4 *pmdtypeint4 =
		mda.PtMDType<IMDTypeInt4>(CTestUtils::m_sysidDefault);
	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();
	CColRef *colref =
		col_factory->PcrCreate(pmdtypeint4, default_type_modifier);

	// create a constraint col \in [1,3)
	gpos::Ref<CConstraint> pcnstr13 =
		PcnstrInterval(mp, colref, 1 /*ulLeft*/, 3 /*ulRight*/);

	// create a constraint col \in [1,5)
	gpos::Ref<CConstraint> pcnstr15 =
		PcnstrInterval(mp, colref, 1 /*ulLeft*/, 5 /*ulRight*/);

	gpos::Ref<CPartConstraint> ppartcnstr13Default =
		GPOS_NEW(mp) CPartConstraint(mp, pcnstr13, true /*fDefaultPartition*/,
									 false /*is_unbounded*/);
	gpos::Ref<CPartConstraint> ppartcnstr15Default =
		GPOS_NEW(mp) CPartConstraint(mp, pcnstr15, true /*fDefaultPartition*/,
									 false /*is_unbounded*/);

	;
	gpos::Ref<CPartConstraint> ppartcnstr13NoDefault =
		GPOS_NEW(mp) CPartConstraint(mp, pcnstr13, false /*fDefaultPartition*/,
									 false /*is_unbounded*/);

	;
	gpos::Ref<CPartConstraint> ppartcnstr13DefaultUnbounded =
		GPOS_NEW(mp) CPartConstraint(mp, pcnstr13, true /*fDefaultPartition*/,
									 true /*is_unbounded*/);

	;
	gpos::Ref<CPartConstraint> ppartcnstr15DefaultUnbounded =
		GPOS_NEW(mp) CPartConstraint(mp, pcnstr15, true /*fDefaultPartition*/,
									 true /*is_unbounded*/);

	// tests

	// equivalence
	GPOS_ASSERT(ppartcnstr13Default->FEquivalent(ppartcnstr13Default.get()));
	GPOS_ASSERT(ppartcnstr13DefaultUnbounded->FEquivalent(
		ppartcnstr13DefaultUnbounded.get()));
	GPOS_ASSERT(ppartcnstr13DefaultUnbounded->FEquivalent(
		ppartcnstr15DefaultUnbounded.get()));
	GPOS_ASSERT(
		!ppartcnstr13DefaultUnbounded->FEquivalent(ppartcnstr13Default.get()));
	GPOS_ASSERT(!ppartcnstr13Default->FEquivalent(ppartcnstr13NoDefault.get()));
	GPOS_ASSERT(!ppartcnstr13Default->FEquivalent(ppartcnstr15Default.get()));

	// unboundedness
	GPOS_ASSERT(!ppartcnstr13Default->IsConstraintUnbounded());
	GPOS_ASSERT(ppartcnstr13DefaultUnbounded->IsConstraintUnbounded());

	// subsumption & overlap
	GPOS_ASSERT(
		ppartcnstr13DefaultUnbounded->FSubsume(ppartcnstr13Default.get()));
	GPOS_ASSERT(
		ppartcnstr13DefaultUnbounded->FSubsume(ppartcnstr15Default.get()));
	GPOS_ASSERT(ppartcnstr15Default->FSubsume(ppartcnstr13Default.get()));
	GPOS_ASSERT(!ppartcnstr13Default->FSubsume(ppartcnstr15Default.get()));
	GPOS_ASSERT(ppartcnstr13Default->FOverlap(mp, ppartcnstr15Default.get()));
	GPOS_ASSERT(ppartcnstr13NoDefault->FOverlap(mp, ppartcnstr13Default.get()));

	// cleanup
	;
	;
	;
	;
	;

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraintTest::PcnstrInterval
//
//	@doc:
//		Create an interval constraint for the given column and interval boundaries
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraint>
CPartConstraintTest::PcnstrInterval(CMemoryPool *mp, CColRef *colref,
									ULONG ulLeft, ULONG ulRight)
{
	gpos::Ref<CExpression> pexprConstLeft =
		CUtils::PexprScalarConstInt4(mp, ulLeft);
	gpos::Ref<CExpression> pexprConstRight =
		CUtils::PexprScalarConstInt4(mp, ulRight);

	// AND
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexpr->Append(CUtils::PexprScalarCmp(
		mp, colref, std::move(pexprConstLeft), IMDType::EcmptGEq));
	pdrgpexpr->Append(CUtils::PexprScalarCmp(
		mp, colref, std::move(pexprConstRight), IMDType::EcmptL));

	gpos::Ref<CExpression> pexpr = CUtils::PexprScalarBoolOp(
		mp, CScalarBoolOp::EboolopAnd, std::move(pdrgpexpr));
	gpos::Ref<CConstraint> pcnstr =
		CConstraintInterval::PciIntervalFromScalarExpr(mp, pexpr.get(), colref);

	;

	return pcnstr;
}


//---------------------------------------------------------------------------
//	@function:
//		CPartConstraintTest::EresUnittest_DateIntervals
//
//	@doc:
//		Test for subsumption, equality and overlap of part constraints on date intervals
//
//---------------------------------------------------------------------------
GPOS_RESULT
CPartConstraintTest::EresUnittest_DateIntervals()
{
	CAutoTraceFlag atf(EopttraceEnableConstantExpressionEvaluation,
					   true /*value*/);

	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup an MD accessor
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));

	const IMDType *pmdtype = mda.RetrieveType(&CMDIdGPDB::m_mdid_date);
	CWStringConst str(GPOS_WSZ_LIT("date_col"));
	CName name(mp, &str);
	CAutoP<CColRef> colref(COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
		pmdtype, default_type_modifier, name));

	// create a date interval: ['01-01-2012', '01-21-2012')
	CWStringDynamic pstrLowerDate1(mp, wszInternalRepresentationFor2012_01_01);
	CWStringDynamic pstrUpperDate1(mp, wszInternalRepresentationFor2012_01_21);
	gpos::Ref<CConstraintInterval> pciFirst = CTestUtils::PciGenericInterval(
		mp, &mda, CMDIdGPDB::m_mdid_date, colref.Value(), &pstrLowerDate1,
		lInternalRepresentationFor2012_01_01, CRange::EriIncluded,
		&pstrUpperDate1, lInternalRepresentationFor2012_01_21,
		CRange::EriExcluded);

	// create a date interval: ['01-01-2012', '01-22-2012')
	CWStringDynamic pstrLowerDate2(mp, wszInternalRepresentationFor2012_01_01);
	CWStringDynamic pstrUpperDate2(mp, wszInternalRepresentationFor2012_01_22);
	gpos::Ref<CConstraintInterval> pciSecond = CTestUtils::PciGenericInterval(
		mp, &mda, CMDIdGPDB::m_mdid_date, colref.Value(), &pstrLowerDate2,
		lInternalRepresentationFor2012_01_01, CRange::EriIncluded,
		&pstrUpperDate2, lInternalRepresentationFor2012_01_22,
		CRange::EriExcluded);

	gpos::Ref<CPartConstraint> ppartcnstr1Default =
		GPOS_NEW(mp) CPartConstraint(mp, pciFirst, true /*fDefaultPartition*/,
									 false /*is_unbounded*/);
	gpos::Ref<CPartConstraint> ppartcnstr2Default =
		GPOS_NEW(mp) CPartConstraint(mp, pciSecond, true /*fDefaultPartition*/,
									 false /*is_unbounded*/);

	;
	gpos::Ref<CPartConstraint> ppartcnstr1NoDefault =
		GPOS_NEW(mp) CPartConstraint(mp, pciFirst, false /*fDefaultPartition*/,
									 false /*is_unbounded*/);

	;
	gpos::Ref<CPartConstraint> ppartcnstr1DefaultUnbounded =
		GPOS_NEW(mp) CPartConstraint(mp, pciFirst, true /*fDefaultPartition*/,
									 true /*is_unbounded*/);

	;
	gpos::Ref<CPartConstraint> ppartcnstr2DefaultUnbounded =
		GPOS_NEW(mp) CPartConstraint(mp, pciSecond, true /*fDefaultPartition*/,
									 true /*is_unbounded*/);

	// tests

	// equivalence
	GPOS_ASSERT(ppartcnstr1Default->FEquivalent(ppartcnstr1Default.get()));
	GPOS_ASSERT(ppartcnstr1DefaultUnbounded->FEquivalent(
		ppartcnstr1DefaultUnbounded.get()));
	GPOS_ASSERT(ppartcnstr1DefaultUnbounded->FEquivalent(
		ppartcnstr2DefaultUnbounded.get()));
	GPOS_ASSERT(
		!ppartcnstr1DefaultUnbounded->FEquivalent(ppartcnstr1Default.get()));
	GPOS_ASSERT(!ppartcnstr1Default->FEquivalent(ppartcnstr1NoDefault.get()));
	GPOS_ASSERT(!ppartcnstr1Default->FEquivalent(ppartcnstr2Default.get()));

	// unboundedness
	GPOS_ASSERT(!ppartcnstr1Default->IsConstraintUnbounded());
	GPOS_ASSERT(ppartcnstr1DefaultUnbounded->IsConstraintUnbounded());

	// subsumption & overlap
	GPOS_ASSERT(
		ppartcnstr1DefaultUnbounded->FSubsume(ppartcnstr1Default.get()));
	GPOS_ASSERT(
		ppartcnstr1DefaultUnbounded->FSubsume(ppartcnstr2Default.get()));
	GPOS_ASSERT(pciSecond->FContainsInterval(mp, pciFirst.get()));
	GPOS_ASSERT(ppartcnstr2Default->FSubsume(ppartcnstr1Default.get()));
	GPOS_ASSERT(!ppartcnstr1Default->FSubsume(ppartcnstr2Default.get()));
	GPOS_ASSERT(ppartcnstr1Default->FOverlap(mp, ppartcnstr2Default.get()));
	GPOS_ASSERT(ppartcnstr1NoDefault->FOverlap(mp, ppartcnstr1Default.get()));

	// cleanup
	;
	;
	;
	;
	;

	return GPOS_OK;
}

// EOF
