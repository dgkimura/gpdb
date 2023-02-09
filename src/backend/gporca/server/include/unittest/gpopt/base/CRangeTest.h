//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CRangeTest.h
//
//	@doc:
//		Test for ranges
//---------------------------------------------------------------------------
#ifndef GPOPT_CRangeTest_H
#define GPOPT_CRangeTest_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CRange.h"

#include "unittest/gpopt/CTestUtils.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CRangeTest
//
//	@doc:
//		Static unit tests for ranges
//
//---------------------------------------------------------------------------
class CRangeTest
{
	using PfPdatum = gpos::Ref<IDatum> (*)(CMemoryPool *, INT);

private:
	static GPOS_RESULT EresInitAndCheckRanges(CMemoryPool *mp,
											  gpos::Ref<IMDId> mdid,
											  PfPdatum pf);

	static void TestRangeRelationship(CMemoryPool *mp,
									  gpos::Ref<CRange> prange1,
									  gpos::Ref<CRange> prange2,
									  gpos::Ref<CRange> prange3,
									  gpos::Ref<CRange> prange4,
									  gpos::Ref<CRange> prange5);

	static void PrintRange(CMemoryPool *mp, CColRef *colref,
						   gpos::Ref<CRange> prange);

	// int2 datum
	static gpos::Ref<IDatum> CreateInt2Datum(CMemoryPool *mp, INT i);

	// int4 datum
	static gpos::Ref<IDatum> CreateInt4Datum(CMemoryPool *mp, INT i);

	// int8 datum
	static gpos::Ref<IDatum> CreateInt8Datum(CMemoryPool *mp, INT li);

public:
	// unittests
	static GPOS_RESULT EresUnittest();
	static GPOS_RESULT EresUnittest_CRangeInt2();
	static GPOS_RESULT EresUnittest_CRangeInt4();
	static GPOS_RESULT EresUnittest_CRangeInt8();
	static GPOS_RESULT EresUnittest_CRangeFromScalar();

};	// class CRangeTest
}  // namespace gpopt

#endif	// !GPOPT_CRangeTest_H


// EOF
