//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CPointTest.cpp
//
//	@doc:
//		Tests for CPoint
//---------------------------------------------------------------------------

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#include "gpos/common/owner.h"
#endif

#include <stdint.h>

#include "gpos/io/COstreamString.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/statistics/CPoint.h"

#include "unittest/base.h"
#include "unittest/dxl/statistics/CPointTest.h"
#include "unittest/gpopt/CTestUtils.h"

using namespace gpopt;

// unittest for statistics objects
GPOS_RESULT
CPointTest::EresUnittest()
{
	// tests that use shared optimization context
	CUnittest rgutSharedOptCtxt[] = {
		GPOS_UNITTEST_FUNC(CPointTest::EresUnittest_CPointInt4),
		GPOS_UNITTEST_FUNC(CPointTest::EresUnittest_CPointBool),
	};

	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr /* pceeval */,
					 CTestUtils::GetCostModel(mp));

	return CUnittest::EresExecute(rgutSharedOptCtxt,
								  GPOS_ARRAY_SIZE(rgutSharedOptCtxt));
}

// basic int4 point tests;
GPOS_RESULT
CPointTest::EresUnittest_CPointInt4()
{
	// create memory pool
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// generate integer points
	gpos::Ref<CPoint> point1 = CTestUtils::PpointInt4(mp, 1);
	gpos::Ref<CPoint> point2 = CTestUtils::PpointInt4(mp, 2);

	GPOS_RTL_ASSERT_MSG(point1->Equals(point1.get()), "1 == 1");
	GPOS_RTL_ASSERT_MSG(point1->IsLessThan(point2.get()), "1 < 2");
	GPOS_RTL_ASSERT_MSG(point2->IsGreaterThan(point1.get()), "2 > 1");
	GPOS_RTL_ASSERT_MSG(point1->IsLessThanOrEqual(point2.get()), "1 <= 2");
	GPOS_RTL_ASSERT_MSG(point2->IsGreaterThanOrEqual(point2.get()), "2 >= 2");

	CDouble dDistance = point2->Distance(point1.get());

	// should be 1.0
	GPOS_RTL_ASSERT_MSG(0.99 < dDistance && dDistance < 1.01,
						"incorrect distance calculation");

	;
	;

	return GPOS_OK;
}

// basic bool point tests;
GPOS_RESULT
CPointTest::EresUnittest_CPointBool()
{
	// create memory pool
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// generate boolean points
	gpos::Ref<CPoint> point1 = CTestUtils::PpointBool(mp, true);
	gpos::Ref<CPoint> point2 = CTestUtils::PpointBool(mp, false);

	// true == true
	GPOS_RTL_ASSERT_MSG(point1->Equals(point1.get()),
						"true must be equal to true");

	// true != false
	GPOS_RTL_ASSERT_MSG(point1->IsNotEqual(point2.get()),
						"true must not be equal to false");

	;
	;

	return GPOS_OK;
}

// EOF
