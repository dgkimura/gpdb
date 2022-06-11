//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CStatisticsTest.h
//
//	@doc:
//		Test for CPoint
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_CStatisticsTest_H
#define GPNAUCRATES_CStatisticsTest_H

#include "gpos/common/owner.h"

#include "naucrates/statistics/CBucket.h"
#include "naucrates/statistics/CHistogram.h"
#include "naucrates/statistics/CPoint.h"
#include "naucrates/statistics/CStatistics.h"
#include "naucrates/statistics/CStatsPredDisj.h"

// fwd declarations
namespace gpopt
{
class CTableDescriptor;
}

namespace gpmd
{
class IMDTypeInt4;
}

namespace gpnaucrates
{
//---------------------------------------------------------------------------
//	@class:
//		CStatisticsTest
//
//	@doc:
//		Static unit tests for statistics objects
//
//---------------------------------------------------------------------------
class CStatisticsTest
{
private:
	// test case for union all evaluation
	struct SStatsUnionAllSTestCase
	{
		// input stats dxl file
		const CHAR *m_szInputFile;

		// output stats dxl file
		const CHAR *m_szOutputFile;
	};

	// create filter on int4 types
	static void StatsFilterInt4(
		CMemoryPool *mp, ULONG colid, INT iLower, INT iUpper,
		gpos::pointer<CStatsPredPtrArry *> pgrgpstatspred);

	// create filter on boolean types
	static void StatsFilterBool(
		CMemoryPool *mp, ULONG colid, BOOL fValue,
		gpos::pointer<CStatsPredPtrArry *> pgrgpstatspred);

	// create filter on numeric types
	static void StatsFilterNumeric(
		CMemoryPool *mp, ULONG colid, CWStringDynamic *pstrLowerEncoded,
		CWStringDynamic *pstrUpperEncoded, CDouble dValLower, CDouble dValUpper,
		gpos::pointer<CStatsPredPtrArry *> pdrgpstatspred);

	// create filter on generic types
	static void StatsFilterGeneric(
		CMemoryPool *mp, ULONG colid, OID oid,
		CWStringDynamic *pstrLowerEncoded, CWStringDynamic *pstrUpperEncoded,
		LINT lValLower, LINT lValUpper,
		gpos::pointer<CStatsPredPtrArry *> pgrgpstatspred);

	static CHistogram *PhistExampleInt4Dim(CMemoryPool *mp);

	// helper function that generates an array of ULONG pointers
	static gpos::owner<ULongPtrArray *>
	Pdrgpul(CMemoryPool *mp, ULONG ul1, ULONG ul2 = gpos::ulong_max)
	{
		gpos::owner<ULongPtrArray *> pdrgpul = GPOS_NEW(mp) ULongPtrArray(mp);
		pdrgpul->Append(GPOS_NEW(mp) ULONG(ul1));

		if (gpos::ulong_max != ul2)
		{
			pdrgpul->Append(GPOS_NEW(mp) ULONG(ul2));
		}

		return pdrgpul;
	}

	// create a table descriptor with two columns having the given names
	static gpos::owner<CTableDescriptor *> PtabdescTwoColumnSource(
		CMemoryPool *mp, const CName &nameTable,
		gpos::pointer<const IMDTypeInt4 *> pmdtype,
		const CWStringConst &strColA, const CWStringConst &strColB);

public:
	// example filter
	static gpos::owner<CStatsPredPtrArry *> Pdrgpstatspred1(CMemoryPool *mp);

	static gpos::owner<CStatsPredPtrArry *> Pdrgpstatspred2(CMemoryPool *mp);

	// unittests
	static GPOS_RESULT EresUnittest();

	// union all tests
	static GPOS_RESULT EresUnittest_UnionAll();

	// statistics basic tests
	static GPOS_RESULT EresUnittest_CStatisticsBasic();

	// statistics basic tests
	static GPOS_RESULT EresUnittest_CStatisticsBucketTest();

	// exercise stats derivation during optimization
	static GPOS_RESULT EresUnittest_CStatisticsSelectDerivation();

	// GbAgg test when grouping on repeated columns
	static GPOS_RESULT EresUnittest_GbAggWithRepeatedGbCols();


};	// class CStatisticsTest
}  // namespace gpnaucrates

#endif	// !GPNAUCRATES_CStatisticsTest_H


// EOF
