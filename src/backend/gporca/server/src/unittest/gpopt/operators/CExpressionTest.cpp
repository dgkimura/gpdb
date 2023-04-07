//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CExpressionTest.cpp
//
//	@doc:
//		Test for CExpression
//---------------------------------------------------------------------------
#include "unittest/gpopt/operators/CExpressionTest.h"

#include "gpos/error/CAutoTrace.h"
#include "gpos/io/COstreamString.h"
#include "gpos/string/CWStringDynamic.h"

#include "gpopt/base/CCTEReq.h"
#include "gpopt/base/CDrvdPropCtxtPlan.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/engine/CEngine.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"
#include "gpopt/mdcache/CAutoMDAccessor.h"
#include "gpopt/metadata/CIndexDescriptor.h"
#include "gpopt/operators/CLogicalBitmapTableGet.h"
#include "gpopt/operators/CLogicalDynamicGetBase.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoinNotIn.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CLogicalUnion.h"
#include "gpopt/operators/CScalarBitmapBoolOp.h"
#include "gpopt/operators/CScalarBitmapIndexProbe.h"
#include "naucrates/base/CDatumInt8GPDB.h"
#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/IMDScalarOp.h"

#include "unittest/base.h"
#include "unittest/gpopt/CTestUtils.h"

using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresUnittest
//
//	@doc:
//		Create required properties which are empty, except for required column set, given by 'pcrs'.
// 		Caller takes ownership of returned pointer.
//
//---------------------------------------------------------------------------
gpos::Ref<CReqdPropPlan>
CExpressionTest::PrppCreateRequiredProperties(CMemoryPool *mp,
											  gpos::Ref<CColRefSet> pcrs)
{
	gpos::Ref<COrderSpec> pos = GPOS_NEW(mp) COrderSpec(mp);
	gpos::Ref<CDistributionSpec> pds = GPOS_NEW(mp)
		CDistributionSpecSingleton(CDistributionSpecSingleton::EstMaster);
	gpos::Ref<CRewindabilitySpec> prs = GPOS_NEW(mp) CRewindabilitySpec(
		CRewindabilitySpec::ErtNone, CRewindabilitySpec::EmhtNoMotion);
	gpos::Ref<CEnfdOrder> peo =
		GPOS_NEW(mp) CEnfdOrder(pos, CEnfdOrder::EomSatisfy);
	gpos::Ref<CPartitionPropagationSpec> pps =
		GPOS_NEW(mp) CPartitionPropagationSpec(mp);
	gpos::Ref<CEnfdDistribution> ped =
		GPOS_NEW(mp) CEnfdDistribution(pds, CEnfdDistribution::EdmSatisfy);
	gpos::Ref<CEnfdRewindability> per =
		GPOS_NEW(mp) CEnfdRewindability(prs, CEnfdRewindability::ErmSatisfy);
	gpos::Ref<CEnfdPartitionPropagation> pepp = GPOS_NEW(mp)
		CEnfdPartitionPropagation(pps, CEnfdPartitionPropagation::EppmSatisfy);
	gpos::Ref<CCTEReq> pcter = GPOS_NEW(mp) CCTEReq(mp);
	return GPOS_NEW(mp) CReqdPropPlan(pcrs, peo, ped, per, pepp, pcter);
}

//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresUnittest
//
//	@doc:
//		Creates a logical GroupBy and a Get as its child. The columns in the
//		Get follow the format wszColNameFormat.
//		Caller takes ownership of returned pointer.
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CExpressionTest::PexprCreateGbyWithColumnFormat(CMemoryPool *mp,
												const WCHAR *wszColNameFormat)
{
	CWStringConst strRelName(GPOS_WSZ_LIT("Rel1"));
	gpos::Ref<CMDIdGPDB> rel_mdid =
		GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidRel, GPOPT_TEST_REL_OID1, 1, 1);
	gpos::Ref<CTableDescriptor> ptabdesc =
		CTestUtils::PtabdescPlainWithColNameFormat(
			mp, 3, rel_mdid, wszColNameFormat, CName(&strRelName), false);
	CWStringConst strRelAlias(GPOS_WSZ_LIT("Rel1"));
	gpos::Ref<CExpression> pexprGet =
		CTestUtils::PexprLogicalGet(mp, ptabdesc, &strRelAlias);
	return CTestUtils::PexprLogicalGbAggWithInput(mp, pexprGet);
}

//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresUnittest
//
//	@doc:
//		Unittest for expressions
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CExpressionTest::EresUnittest_SimpleOps),
		GPOS_UNITTEST_FUNC(CExpressionTest::EresUnittest_Union),
		GPOS_UNITTEST_FUNC(CExpressionTest::EresUnittest_BitmapGet),
		GPOS_UNITTEST_FUNC(CExpressionTest::EresUnittest_Const),
#ifdef GPOS_DEBUG
		GPOS_UNITTEST_FUNC(CExpressionTest::EresUnittest_ComparisonTypes),
#endif	// GPOS_DEBUG
		GPOS_UNITTEST_FUNC(CExpressionTest::EresUnittest_FValidPlan),
		GPOS_UNITTEST_FUNC(
			CExpressionTest::EresUnittest_FValidPlan_InvalidOrder),
		GPOS_UNITTEST_FUNC(
			CExpressionTest::EresUnittest_FValidPlan_InvalidDistribution),
		GPOS_UNITTEST_FUNC(
			CExpressionTest::EresUnittest_FValidPlan_InvalidRewindability),
		GPOS_UNITTEST_FUNC(
			CExpressionTest::EresUnittest_FValidPlan_InvalidCTEs),

#ifdef GPOS_DEBUG
		GPOS_UNITTEST_FUNC(CExpressionTest::EresUnittest_FValidPlanError),
#endif	// GPOS_DEBUG
		GPOS_UNITTEST_FUNC(EresUnittest_ReqdCols),
#ifdef GPOS_DEBUG
		GPOS_UNITTEST_FUNC_ASSERT(CExpressionTest::EresUnittest_InvalidSetOp),
#endif	// GPOS_DEBUG
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresUnittest_SimpleOps
//
//	@doc:
//		Basic tree builder test
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresUnittest_SimpleOps()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;

	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	using Pfpexpr = gpos::Ref<CExpression> (*)(CMemoryPool *);

	Pfpexpr rgpf[] = {
		CTestUtils::PexprLogicalGet,
		CTestUtils::PexprLogicalForeignGet,
		CTestUtils::PexprLogicalGetPartitioned,
		CTestUtils::PexprLogicalSelect,
		CTestUtils::PexprLogicalSelectCmpToConst,
		CTestUtils::PexprLogicalSelectArrayCmp,
		CTestUtils::PexprLogicalSelectPartitioned,
		CTestUtils::PexprLogicalJoin<CLogicalInnerJoin>,
		CTestUtils::PexprLogicalJoin<CLogicalLeftOuterJoin>,
		CTestUtils::PexprLogicalJoin<CLogicalLeftSemiJoin>,
		CTestUtils::PexprLogicalJoin<CLogicalLeftAntiSemiJoin>,
		CTestUtils::PexprLogicalJoin<CLogicalLeftAntiSemiJoinNotIn>,
		CTestUtils::PexprLogicalGbAgg,
		CTestUtils::PexprLogicalGbAggOverJoin,
		CTestUtils::PexprLogicalGbAggWithSum,
		CTestUtils::PexprLogicalLimit,
		CTestUtils::PexprLogicalNAryJoin,
		CTestUtils::PexprLogicalProject,
		CTestUtils::PexprConstTableGet5,
		CTestUtils::PexprLogicalDynamicGet,
		CTestUtils::PexprLogicalSequence,
		CTestUtils::PexprLogicalTVFTwoArgs,
		CTestUtils::PexprLogicalAssert,
		PexprComplexJoinTree};

#ifdef GPOS_DEBUG
	// misestimation risk for the roots of the plans in rgpf
	ULONG rgulRisk[] = {1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2,
						2, 3, 2, 1, 2, 1, 1, 1, 1, 1, 1, 6};
#endif	// GPOS_DEBUG

	for (ULONG i = 0; i < GPOS_ARRAY_SIZE(rgpf); i++)
	{
		// install opt context in TLS
		CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
						 CTestUtils::GetCostModel(mp));

		// generate simple expression
		gpos::Ref<CExpression> pexpr = rgpf[i](mp);

		gpos::Ref<CLogicalGet> popGet =
			gpos::dyn_cast<CLogicalGet>(pexpr->Pop());
		gpos::Ref<CLogicalDynamicGetBase> popDynGet =
			gpos::dyn_cast<CLogicalDynamicGetBase>(pexpr->Pop());
		gpos::Ref<CColRefArray> colrefs = nullptr;

		if (nullptr != popGet)
		{
			colrefs = popGet->PdrgpcrOutput();
		}
		else if (nullptr != popDynGet)
		{
			colrefs = popDynGet->PdrgpcrOutput();
		}

		if (nullptr != colrefs)
		{
			for (ULONG ul = 0; ul < colrefs->Size(); ul++)
			{
				(*colrefs)[ul]->MarkAsUsed();
			}
		}

		// self-match
		GPOS_ASSERT(pexpr->FMatchDebug(pexpr));

		// debug print
		CWStringDynamic str(mp, GPOS_WSZ_LIT("\n"));
		COstreamString oss(&str);

		oss << "EXPR:" << std::endl << *pexpr << std::endl;
		GPOS_TRACE(str.GetBuffer());
		str.Reset();

#ifdef GPOS_DEBUG

		oss << std::endl << "DERIVED PROPS:" << std::endl;
		GPOS_TRACE(str.GetBuffer());
		str.Reset();
		pexpr->DbgPrint();

		// copy expression
		gpos::Ref<CColRef> pcrOld = pexpr->DeriveOutputColumns()->PcrAny();
		gpos::Ref<CColRef> new_colref =
			COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(pcrOld);
		gpos::Ref<UlongToColRefMap> colref_mapping =
			GPOS_NEW(mp) UlongToColRefMap(mp);

		BOOL result = colref_mapping->Insert(pcrOld->Id(), new_colref);
		GPOS_ASSERT(result);
		gpos::Ref<CExpression> pexprCopy = pexpr->PexprCopyWithRemappedColumns(
			mp, colref_mapping, true /*must_exist*/);

		oss << std::endl
			<< "COPIED EXPRESSION (AFTER MAPPING " << *pcrOld << " TO "
			<< *new_colref << "):" << std::endl;
		GPOS_TRACE(str.GetBuffer());
		str.Reset();
		pexprCopy->DbgPrint();


		// derive stats on expression
		gpos::Ref<CReqdPropRelational> prprel =
			GPOS_NEW(mp) CReqdPropRelational(GPOS_NEW(mp) CColRefSet(mp));
		gpos::Ref<IStatisticsArray> stats_ctxt =
			GPOS_NEW(mp) IStatisticsArray(mp);
		gpos::Ref<IStatistics> stats = pexpr->PstatsDerive(prprel, stats_ctxt);
		GPOS_ASSERT(nullptr != stats);

		oss << "Expected risk: " << rgulRisk[i] << std::endl;
		oss << std::endl << "STATS:" << *stats << std::endl;
		GPOS_TRACE(str.GetBuffer());

		GPOS_ASSERT(rgulRisk[i] == stats->StatsEstimationRisk());



#endif	// GPOS_DEBUG

		// cleanup
	}

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//		CExpressionTest::EresUnittest_Union
//
//	@doc:
//		Basic tree builder test w/ Unions
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresUnittest_Union()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;

	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	// build union tree of depth 2
	gpos::Ref<CExpression> pexpr = CTestUtils::PexprLogicalUnion(mp, 2);

	// debug print
	CWStringDynamic str(mp);
	COstreamString oss(&str);
	pexpr->OsPrint(oss);

	GPOS_TRACE(str.GetBuffer());

	// derive properties on expression
	(void) pexpr->PdpDerive();

#ifdef GPOS_DEBUG
	gpos::Ref<CReqdPropRelational> prprel =
		GPOS_NEW(mp) CReqdPropRelational(GPOS_NEW(mp) CColRefSet(mp));
	gpos::Ref<IStatisticsArray> stats_ctxt = GPOS_NEW(mp) IStatisticsArray(mp);
	gpos::Ref<IStatistics> stats = pexpr->PstatsDerive(prprel, stats_ctxt);
	GPOS_ASSERT(nullptr != stats);

	// We expect a risk of 3 because every Union increments the risk.
	GPOS_ASSERT(3 == stats->StatsEstimationRisk());


#endif	// GPOS_DEBUG

	// cleanup


	return GPOS_OK;
}

//---------------------------------------------------------------------------
//		CExpressionTest::EresUnittest_BitmapScan
//
//	@doc:
//		Basic tree builder test with bitmap index and table get.
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresUnittest_BitmapGet()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;

	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	CWStringConst strRelName(GPOS_WSZ_LIT("MyTable"));
	CWStringConst strRelAlias(GPOS_WSZ_LIT("T"));
	gpos::Ref<CMDIdGPDB> rel_mdid =
		GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidRel, GPOPT_MDCACHE_TEST_OID, 1, 1);
	const WCHAR *wszColNameFormat = GPOS_WSZ_LIT("column_%04d");
	gpos::Ref<CTableDescriptor> ptabdesc =
		CTestUtils::PtabdescPlainWithColNameFormat(
			mp, 3, rel_mdid, wszColNameFormat, CName(&strRelName), false);

	// get the index associated with the table
	const gpos::Ref<IMDRelation> pmdrel = mda.RetrieveRel(ptabdesc->MDId());
	GPOS_ASSERT(0 < pmdrel->IndexCount());

	// create an index descriptor
	gpos::Ref<IMDId> pmdidIndex = pmdrel->IndexMDidAt(0 /*pos*/);
	const gpos::Ref<IMDIndex> pmdindex = mda.RetrieveIndex(pmdidIndex);

	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();

	const ULONG num_cols = pmdrel->ColumnCount();
	GPOS_ASSERT(2 < num_cols);

	// create an index on the first column
	const gpos::Ref<IMDColumn> pmdcol = pmdrel->GetMdCol(0);
	const gpos::Ref<IMDType> pmdtype = mda.RetrieveType(pmdcol->MdidType());
	gpos::Ref<CColRef> pcrFirst =
		col_factory->PcrCreate(pmdtype, pmdcol->TypeModifier());

	gpos::Ref<CExpression> pexprIndexCond = CUtils::PexprScalarEqCmp(
		mp, pcrFirst, CUtils::PexprScalarConstInt4(mp, 20 /*val*/));

	gpos::Ref<CMDIdGPDB> mdid =
		GPOS_NEW(mp) CMDIdGPDB(CMDIdGPDB::m_mdid_unknown);
	gpos::Ref<CIndexDescriptor> pindexdesc =
		CIndexDescriptor::Pindexdesc(mp, ptabdesc, pmdindex);
	gpos::Ref<CExpression> pexprBitmapIndex = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarBitmapIndexProbe(mp, pindexdesc, mdid),
		pexprIndexCond);

	gpos::Ref<CColRefArray> pdrgpcrTable = GPOS_NEW(mp) CColRefArray(mp);
	for (ULONG ul = 0; ul < num_cols; ++ul)
	{
		const gpos::Ref<IMDColumn> pmdcol = pmdrel->GetMdCol(ul);
		const gpos::Ref<IMDType> pmdtype = mda.RetrieveType(pmdcol->MdidType());
		gpos::Ref<CColRef> colref =
			col_factory->PcrCreate(pmdtype, pmdcol->TypeModifier());
		pdrgpcrTable->Append(colref);
	}

	gpos::Ref<CExpression> pexprTableCond = CUtils::PexprScalarEqCmp(
		mp, pcrFirst, CUtils::PexprScalarConstInt4(mp, 20 /*val*/));

	gpos::Ref<CExpression> pexprBitmapTableGet = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalBitmapTableGet(
			mp, ptabdesc,
			gpos::ulong_max,  // pgexprOrigin
			GPOS_NEW(mp) CName(mp, CName(&strRelAlias)), pdrgpcrTable),
		pexprTableCond, pexprBitmapIndex);

	// debug print
	CWStringDynamic str(mp);
	COstreamString oss(&str);
	pexprBitmapTableGet->OsPrint(oss);

	CWStringConst strExpectedDebugPrint(GPOS_WSZ_LIT(
		"+--CLogicalBitmapTableGet , Table Name: (\"MyTable\"), Columns: [\"ColRef_0001\" (1), \"ColRef_0002\" (2), \"ColRef_0003\" (3)]\n"
		"   |--CScalarCmp (=)\n"
		"   |  |--CScalarIdent \"ColRef_0000\" (0)\n"
		"   |  +--CScalarConst (20)\n"
		"   +--CScalarBitmapIndexProbe   Bitmap Index Name: (\"T_a\")\n"
		"      +--CScalarCmp (=)\n"
		"         |--CScalarIdent \"ColRef_0000\" (0)\n"
		"         +--CScalarConst (20)\n"));

	GPOS_ASSERT(str.Equals(&strExpectedDebugPrint));

	// derive properties on expression
	(void) pexprBitmapTableGet->PdpDerive();

	// test matching of bitmap index probe expressions
	gpos::Ref<CMDIdGPDB> pmdid2 =
		GPOS_NEW(mp) CMDIdGPDB(CMDIdGPDB::m_mdid_unknown);
	gpos::Ref<CIndexDescriptor> pindexdesc2 =
		CIndexDescriptor::Pindexdesc(mp, ptabdesc, pmdindex);
	gpos::Ref<CExpression> pexprIndexCond2 = CUtils::PexprScalarEqCmp(
		mp, pcrFirst, CUtils::PexprScalarConstInt4(mp, 20 /*val*/));
	gpos::Ref<CExpression> pexprBitmapIndex2 = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarBitmapIndexProbe(mp, pindexdesc2, pmdid2),
		pexprIndexCond2);
	CWStringDynamic strIndex2(mp);
	COstreamString ossIndex2(&strIndex2);
	pexprBitmapIndex2->OsPrint(ossIndex2);
	CWStringConst strExpectedDebugPrintIndex2(GPOS_WSZ_LIT(
		// clang-format off
		"+--CScalarBitmapIndexProbe   Bitmap Index Name: (\"T_a\")\n"
		"   +--CScalarCmp (=)\n"
		"      |--CScalarIdent \"ColRef_0000\" (0)\n"
		"      +--CScalarConst (20)\n"
		// clang-format on
		));

	GPOS_ASSERT(strIndex2.Equals(&strExpectedDebugPrintIndex2));
	GPOS_ASSERT(pexprBitmapIndex2->Matches(pexprBitmapIndex));



	gpos::Ref<CExpression> pexprBitmapBoolOp1 = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp)
			CScalarBitmapBoolOp(mp, CScalarBitmapBoolOp::EbitmapboolAnd, mdid),
		pexprBitmapIndex, pexprBitmapIndex2);



	gpos::Ref<CExpression> pexprBitmapBoolOp2 = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp)
			CScalarBitmapBoolOp(mp, CScalarBitmapBoolOp::EbitmapboolAnd, mdid),
		pexprBitmapIndex, pexprBitmapIndex2);
	GPOS_ASSERT(pexprBitmapBoolOp2->Matches(pexprBitmapBoolOp1));

	// cleanup



	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresUnittest_Const
//
//	@doc:
//		Test of scalar constant expressions
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresUnittest_Const()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;

	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	BOOL value = true;
	gpos::Ref<CExpression> pexprTrue = CUtils::PexprScalarConstBool(mp, value);

	value = false;
	gpos::Ref<CExpression> pexprFalse = CUtils::PexprScalarConstBool(mp, value);

	ULONG ulVal = 123456;
	gpos::Ref<CExpression> pexprUl = CUtils::PexprScalarConstInt4(mp, ulVal);
	gpos::Ref<CExpression> pexprUl2nd = CUtils::PexprScalarConstInt4(mp, ulVal);

	ulVal = 1;
	gpos::Ref<CExpression> pexprUlOne = CUtils::PexprScalarConstInt4(mp, ulVal);

	CWStringDynamic str(mp);
	COstreamString oss(&str);
	oss << std::endl;
	pexprTrue->OsPrint(oss);
	pexprFalse->OsPrint(oss);
	pexprUl->OsPrint(oss);
	pexprUl2nd->OsPrint(oss);
	pexprUlOne->OsPrint(oss);

#ifdef GPOS_DEBUG
	gpos::Ref<CScalarConst> pscalarconstTrue =
		CScalarConst::PopConvert(pexprTrue->Pop());
	gpos::Ref<CScalarConst> pscalarconstFalse =
		CScalarConst::PopConvert(pexprFalse->Pop());
	gpos::Ref<CScalarConst> pscalarconstUl =
		CScalarConst::PopConvert(pexprUl->Pop());
	gpos::Ref<CScalarConst> pscalarconstUl2nd =
		CScalarConst::PopConvert(pexprUl2nd->Pop());
	gpos::Ref<CScalarConst> pscalarconstUlOne =
		CScalarConst::PopConvert(pexprUlOne->Pop());
#endif	// GPOS_DEBUG

	GPOS_ASSERT(pscalarconstUl->HashValue() == pscalarconstUl2nd->HashValue());
	GPOS_ASSERT(!pscalarconstTrue->Matches(pscalarconstFalse));
	GPOS_ASSERT(!pscalarconstTrue->Matches(pscalarconstUlOne));



	GPOS_TRACE(str.GetBuffer());

	return GPOS_OK;
}


#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresUnittest_ComparisonTypes
//
//	@doc:
//		Test of scalar comparison types
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresUnittest_ComparisonTypes()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;

	CMDAccessor mda(mp, CMDCache::Pcache());
	mda.RegisterProvider(CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	const gpos::Ref<IMDType> pmdtype = mda.PtMDType<IMDTypeInt4>();

	GPOS_ASSERT(
		IMDType::EcmptEq ==
		CUtils::ParseCmpType(pmdtype->GetMdidForCmpType(IMDType::EcmptEq)));
	GPOS_ASSERT(
		IMDType::EcmptL ==
		CUtils::ParseCmpType(pmdtype->GetMdidForCmpType(IMDType::EcmptL)));
	GPOS_ASSERT(
		IMDType::EcmptG ==
		CUtils::ParseCmpType(pmdtype->GetMdidForCmpType(IMDType::EcmptG)));

	const gpos::Ref<IMDScalarOp> pmdscopEq =
		mda.RetrieveScOp(pmdtype->GetMdidForCmpType(IMDType::EcmptEq));
	const gpos::Ref<IMDScalarOp> pmdscopLT =
		mda.RetrieveScOp(pmdtype->GetMdidForCmpType(IMDType::EcmptL));
	const gpos::Ref<IMDScalarOp> pmdscopGT =
		mda.RetrieveScOp(pmdtype->GetMdidForCmpType(IMDType::EcmptG));

	GPOS_ASSERT(IMDType::EcmptNEq ==
				CUtils::ParseCmpType(pmdscopEq->GetInverseOpMdid()));
	GPOS_ASSERT(IMDType::EcmptLEq ==
				CUtils::ParseCmpType(pmdscopGT->GetInverseOpMdid()));
	GPOS_ASSERT(IMDType::EcmptGEq ==
				CUtils::ParseCmpType(pmdscopLT->GetInverseOpMdid()));

	return GPOS_OK;
}
#endif	// GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::SetupPlanForFValidPlanTest
//
//	@doc:
//		Helper function for the FValidPlan tests
//
//---------------------------------------------------------------------------
void
CExpressionTest::SetupPlanForFValidPlanTest(CMemoryPool *mp,
											gpos::Ref<CExpression> *ppexprGby,
											gpos::Ref<CColRefSet> *ppcrs,
											gpos::Ref<CExpression> *ppexprPlan,
											gpos::Ref<CReqdPropPlan> *pprpp)
{
	*ppexprGby =
		PexprCreateGbyWithColumnFormat(mp, GPOS_WSZ_LIT("Test Column%d"));

	// Create a column requirement using the first output column of the group by.
	gpos::Ref<CColRefSet> pcrsGby = (*ppexprGby)->DeriveOutputColumns();
	*ppcrs = GPOS_NEW(mp) CColRefSet(mp);
	(*ppcrs)->Include(pcrsGby->PcrFirst());

	*pprpp = PrppCreateRequiredProperties(mp, *ppcrs);
	CExpressionHandle exprhdl(mp);
	exprhdl.Attach(*ppexprGby);
	exprhdl.InitReqdProps(*pprpp);

	// Optimize the logical plan under default required properties, which are always satisfied.
	CEngine eng(mp);
	CAutoP<CQueryContext> pqc;
	pqc = CTestUtils::PqcGenerate(mp, *ppexprGby);
	eng.Init(pqc.Value(), nullptr /*search_stage_array*/);
	eng.Optimize();
	*ppexprPlan = eng.PexprExtractPlan();
}

//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresUnittest_FValidPlan
//
//	@doc:
//		Test for CExpression::FValidPlan
// 		Test now just very basic cases. More complex cases are covered by minidump tests
// 		in CEnumeratorTest.
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresUnittest_FValidPlan()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;

	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));
	const gpos::Ref<IMDType> pmdtype = mda.PtMDType<IMDTypeInt4>();

	// Create a group-by with a get child. Properties required contain one of the columns in the group by.
	// Test that the plan is valid.
	{
		gpos::Ref<CExpression> pexprGby = nullptr;
		gpos::Ref<CColRefSet> pcrs = nullptr;
		gpos::Ref<CExpression> pexprPlan = nullptr;
		gpos::Ref<CReqdPropPlan> prpp = nullptr;

		SetupPlanForFValidPlanTest(mp, &pexprGby, &pcrs, &pexprPlan, &prpp);
		gpos::Ref<CDrvdPropCtxtPlan> pdpctxtplan =
			GPOS_NEW(mp) CDrvdPropCtxtPlan(mp);

		// Test that prpp is actually satisfied.
		GPOS_ASSERT(pexprPlan->FValidPlan(prpp, pdpctxtplan));
	}
	// Create a group-by with a get child. Properties required contain one column that doesn't exist.
	// Test that the plan is NOT valid.
	{
		gpos::Ref<CExpression> pexprGby =
			PexprCreateGbyWithColumnFormat(mp, GPOS_WSZ_LIT("Test Column%d"));

		(void) pexprGby->PdpDerive();
		gpos::Ref<CColRefSet> pcrsInvalid = GPOS_NEW(mp) CColRefSet(mp);
		CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();

		// Creating the column reference with the column factory ensures that it's a brand new column.
		gpos::Ref<CColRef> pcrComputed =
			col_factory->PcrCreate(pmdtype, default_type_modifier);
		pcrsInvalid->Include(pcrComputed);

		gpos::Ref<CReqdPropPlan> prpp =
			PrppCreateRequiredProperties(mp, pcrsInvalid);
		CExpressionHandle exprhdl(mp);
		exprhdl.Attach(pexprGby);
		exprhdl.InitReqdProps(prpp);

		// Optimize the logical plan, but under default required properties, which are always satisfied.
		CEngine eng(mp);
		CAutoP<CQueryContext> pqc;
		pqc = CTestUtils::PqcGenerate(mp, pexprGby);
		eng.Init(pqc.Value(), nullptr /*search_stage_array*/);
		eng.Optimize();
		gpos::Ref<CExpression> pexprPlan = eng.PexprExtractPlan();

		gpos::Ref<CDrvdPropCtxtPlan> pdpctxtplan =
			GPOS_NEW(mp) CDrvdPropCtxtPlan(mp);

		// Test that prpp is actually unsatisfied.
		GPOS_ASSERT(!pexprPlan->FValidPlan(prpp, pdpctxtplan));
	}

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresUnittest_FValidPlan_InvalidOrder
//
//	@doc:
//		Test for CExpression::FValidPlan with incompatible order properties
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresUnittest_FValidPlan_InvalidOrder()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;

	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	gpos::Ref<CExpression> pexprGby = nullptr;
	gpos::Ref<CColRefSet> pcrs = nullptr;
	gpos::Ref<CExpression> pexprPlan = nullptr;
	gpos::Ref<CReqdPropPlan> prpp = nullptr;

	SetupPlanForFValidPlanTest(mp, &pexprGby, &pcrs, &pexprPlan, &prpp);
	gpos::Ref<CDrvdPropCtxtPlan> pdpctxtplan =
		GPOS_NEW(mp) CDrvdPropCtxtPlan(mp);

	// Create similar requirements, but
	// add an order requirement using a couple of output columns of a Get
	gpos::Ref<CExpression> pexprGet = CTestUtils::PexprLogicalGet(mp);
	gpos::Ref<CColRefSet> pcrsGet = pexprGet->DeriveOutputColumns();
	gpos::Ref<CColRefSet> pcrsGetCopy = GPOS_NEW(mp) CColRefSet(mp, *pcrsGet);

	gpos::Ref<CColRefArray> pdrgpcrGet = pcrsGetCopy->Pdrgpcr(mp);
	GPOS_ASSERT(2 <= pdrgpcrGet->Size());

	gpos::Ref<COrderSpec> pos = GPOS_NEW(mp) COrderSpec(mp);
	const gpos::Ref<IMDType> pmdtypeint4 = mda.PtMDType<IMDTypeInt4>();
	gpos::Ref<IMDId> pmdidInt4LT =
		pmdtypeint4->GetMdidForCmpType(IMDType::EcmptL);

	pos->Append(pmdidInt4LT, (*pdrgpcrGet)[1], COrderSpec::EntFirst);
	gpos::Ref<CEnfdOrder> peo =
		GPOS_NEW(mp) CEnfdOrder(pos, CEnfdOrder::EomSatisfy);

	gpos::Ref<CDistributionSpec> pds = GPOS_NEW(mp)
		CDistributionSpecSingleton(CDistributionSpecSingleton::EstMaster);
	gpos::Ref<CRewindabilitySpec> prs = GPOS_NEW(mp) CRewindabilitySpec(
		CRewindabilitySpec::ErtNone, CRewindabilitySpec::EmhtNoMotion);
	gpos::Ref<CEnfdDistribution> ped =
		GPOS_NEW(mp) CEnfdDistribution(pds, CEnfdDistribution::EdmExact);
	gpos::Ref<CEnfdRewindability> per =
		GPOS_NEW(mp) CEnfdRewindability(prs, CEnfdRewindability::ErmSatisfy);
	gpos::Ref<CPartitionPropagationSpec> pps =
		GPOS_NEW(mp) CPartitionPropagationSpec(mp);
	gpos::Ref<CEnfdPartitionPropagation> pepp = GPOS_NEW(mp)
		CEnfdPartitionPropagation(pps, CEnfdPartitionPropagation::EppmSatisfy);

	gpos::Ref<CCTEReq> pcter = GPOS_NEW(mp) CCTEReq(mp);
	gpos::Ref<CReqdPropPlan> prppIncompatibleOrder =
		GPOS_NEW(mp) CReqdPropPlan(pcrsGetCopy, peo, ped, per, pepp, pcter);

	// Test that the plan is not valid.
	GPOS_ASSERT(!pexprPlan->FValidPlan(prppIncompatibleOrder, pdpctxtplan));



	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresUnittest_FValidPlan_InvalidDistribution
//
//	@doc:
//		Test for CExpression::FValidPlan with incompatible distribution properties
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresUnittest_FValidPlan_InvalidDistribution()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;

	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	gpos::Ref<CExpression> pexprGby = nullptr;
	gpos::Ref<CColRefSet> pcrs = nullptr;
	gpos::Ref<CExpression> pexprPlan = nullptr;
	gpos::Ref<CReqdPropPlan> prpp = nullptr;

	SetupPlanForFValidPlanTest(mp, &pexprGby, &pcrs, &pexprPlan, &prpp);
	gpos::Ref<CDrvdPropCtxtPlan> pdpctxtplan =
		GPOS_NEW(mp) CDrvdPropCtxtPlan(mp);
	gpos::Ref<COrderSpec> pos = GPOS_NEW(mp) COrderSpec(mp);
	gpos::Ref<CDistributionSpec> pds = GPOS_NEW(mp) CDistributionSpecRandom();
	gpos::Ref<CRewindabilitySpec> prs = GPOS_NEW(mp) CRewindabilitySpec(
		CRewindabilitySpec::ErtNone, CRewindabilitySpec::EmhtNoMotion);
	gpos::Ref<CEnfdOrder> peo =
		GPOS_NEW(mp) CEnfdOrder(pos, CEnfdOrder::EomSatisfy);
	gpos::Ref<CEnfdDistribution> ped =
		GPOS_NEW(mp) CEnfdDistribution(pds, CEnfdDistribution::EdmExact);
	gpos::Ref<CEnfdRewindability> per =
		GPOS_NEW(mp) CEnfdRewindability(prs, CEnfdRewindability::ErmSatisfy);
	gpos::Ref<CPartitionPropagationSpec> pps =
		GPOS_NEW(mp) CPartitionPropagationSpec(mp);
	gpos::Ref<CEnfdPartitionPropagation> pepp = GPOS_NEW(mp)
		CEnfdPartitionPropagation(pps, CEnfdPartitionPropagation::EppmSatisfy);
	gpos::Ref<CCTEReq> pcter = GPOS_NEW(mp) CCTEReq(mp);
	gpos::Ref<CColRefSet> pcrsCopy = GPOS_NEW(mp) CColRefSet(mp, *pcrs);
	gpos::Ref<CReqdPropPlan> prppIncompatibleDistribution =
		GPOS_NEW(mp) CReqdPropPlan(pcrsCopy, peo, ped, per, pepp, pcter);

	// Test that the plan is not valid.
	GPOS_ASSERT(
		!pexprPlan->FValidPlan(prppIncompatibleDistribution, pdpctxtplan));



	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresUnittest_FValidPlan_InvalidRewindability
//
//	@doc:
//		Test for CExpression::FValidPlan with incompatible rewindability properties
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresUnittest_FValidPlan_InvalidRewindability()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;

	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	gpos::Ref<CExpression> pexprGby = nullptr;
	gpos::Ref<CColRefSet> pcrs = nullptr;
	gpos::Ref<CExpression> pexprPlan = nullptr;
	gpos::Ref<CReqdPropPlan> prpp = nullptr;

	SetupPlanForFValidPlanTest(mp, &pexprGby, &pcrs, &pexprPlan, &prpp);
	gpos::Ref<CDrvdPropCtxtPlan> pdpctxtplan =
		GPOS_NEW(mp) CDrvdPropCtxtPlan(mp);

	gpos::Ref<COrderSpec> pos = GPOS_NEW(mp) COrderSpec(mp);
	gpos::Ref<CDistributionSpec> pds = GPOS_NEW(mp)
		CDistributionSpecSingleton(CDistributionSpecSingleton::EstMaster);
	gpos::Ref<CRewindabilitySpec> prs = GPOS_NEW(mp) CRewindabilitySpec(
		CRewindabilitySpec::ErtRewindable, CRewindabilitySpec::EmhtNoMotion);
	gpos::Ref<CEnfdOrder> peo =
		GPOS_NEW(mp) CEnfdOrder(pos, CEnfdOrder::EomSatisfy);
	gpos::Ref<CEnfdDistribution> ped =
		GPOS_NEW(mp) CEnfdDistribution(pds, CEnfdDistribution::EdmExact);
	gpos::Ref<CEnfdRewindability> per =
		GPOS_NEW(mp) CEnfdRewindability(prs, CEnfdRewindability::ErmSatisfy);
	gpos::Ref<CPartitionPropagationSpec> pps =
		GPOS_NEW(mp) CPartitionPropagationSpec(mp);
	gpos::Ref<CEnfdPartitionPropagation> pepp = GPOS_NEW(mp)
		CEnfdPartitionPropagation(pps, CEnfdPartitionPropagation::EppmSatisfy);
	gpos::Ref<CCTEReq> pcter = GPOS_NEW(mp) CCTEReq(mp);
	gpos::Ref<CColRefSet> pcrsCopy = GPOS_NEW(mp) CColRefSet(mp, *pcrs);

	gpos::Ref<CReqdPropPlan> prppIncompatibleRewindability =
		GPOS_NEW(mp) CReqdPropPlan(pcrsCopy, peo, ped, per, pepp, pcter);
	// Test that the plan is not valid.
	GPOS_ASSERT(
		!pexprPlan->FValidPlan(prppIncompatibleRewindability, pdpctxtplan));



	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresUnittest_FValidPlan_InvalidCTEs
//
//	@doc:
//		Test for CExpression::FValidPlan with incompatible CTE properties
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresUnittest_FValidPlan_InvalidCTEs()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;

	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	gpos::Ref<CExpression> pexprGby = nullptr;
	gpos::Ref<CColRefSet> pcrs = nullptr;
	gpos::Ref<CExpression> pexprPlan = nullptr;
	gpos::Ref<CReqdPropPlan> prpp = nullptr;

	SetupPlanForFValidPlanTest(mp, &pexprGby, &pcrs, &pexprPlan, &prpp);
	gpos::Ref<CDrvdPropCtxtPlan> pdpctxtplan =
		GPOS_NEW(mp) CDrvdPropCtxtPlan(mp);

	gpos::Ref<COrderSpec> pos = GPOS_NEW(mp) COrderSpec(mp);
	gpos::Ref<CDistributionSpec> pds = GPOS_NEW(mp)
		CDistributionSpecSingleton(CDistributionSpecSingleton::EstMaster);
	gpos::Ref<CRewindabilitySpec> prs = GPOS_NEW(mp) CRewindabilitySpec(
		CRewindabilitySpec::ErtNone, CRewindabilitySpec::EmhtNoMotion);
	gpos::Ref<CEnfdOrder> peo =
		GPOS_NEW(mp) CEnfdOrder(pos, CEnfdOrder::EomSatisfy);
	gpos::Ref<CEnfdDistribution> ped =
		GPOS_NEW(mp) CEnfdDistribution(pds, CEnfdDistribution::EdmExact);
	gpos::Ref<CEnfdRewindability> per =
		GPOS_NEW(mp) CEnfdRewindability(prs, CEnfdRewindability::ErmSatisfy);
	gpos::Ref<CPartitionPropagationSpec> pps =
		GPOS_NEW(mp) CPartitionPropagationSpec(mp);
	gpos::Ref<CEnfdPartitionPropagation> pepp = GPOS_NEW(mp)
		CEnfdPartitionPropagation(pps, CEnfdPartitionPropagation::EppmSatisfy);
	gpos::Ref<CCTEReq> pcter = GPOS_NEW(mp) CCTEReq(mp);
	ULONG ulCTEId = 0;

	gpos::Ref<CExpression> pexprProducer =
		CTestUtils::PexprLogicalCTEProducerOverSelect(mp, ulCTEId);
	gpos::Ref<CDrvdPropPlan> pdpplan =
		CDrvdPropPlan::Pdpplan(pexprPlan->PdpDerive());

	pcter->Insert(ulCTEId, CCTEMap::EctProducer /*ect*/, true /*fRequired*/,
				  pdpplan);
	gpos::Ref<CColRefSet> pcrsCopy = GPOS_NEW(mp) CColRefSet(mp, *pcrs);
	gpos::Ref<CReqdPropPlan> prppIncompatibleCTE =
		GPOS_NEW(mp) CReqdPropPlan(pcrsCopy, peo, ped, per, pepp, pcter);

	// Test that the plan is not valid.
	GPOS_ASSERT(!pexprPlan->FValidPlan(prppIncompatibleCTE, pdpctxtplan));



	return GPOS_OK;
}


#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresUnittest_FValidPlanError
//
//	@doc:
//		Tests that CExpression::FValidPlan fails with an assert exception in debug mode
//		for bad input.
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresUnittest_FValidPlanError()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// setup a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;

	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));
	const gpos::Ref<IMDType> pmdtype = mda.PtMDType<IMDTypeInt4>();

	GPOS_RESULT eres = GPOS_OK;
	// Test that in debug mode GPOS_ASSERT fails for non-physical expressions.
	{
		gpos::Ref<CColRefSet> pcrsInvalid = GPOS_NEW(mp) CColRefSet(mp);
		CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();
		gpos::Ref<CColRef> pcrComputed =
			col_factory->PcrCreate(pmdtype, default_type_modifier);
		pcrsInvalid->Include(pcrComputed);

		gpos::Ref<CReqdPropPlan> prpp =
			PrppCreateRequiredProperties(mp, pcrsInvalid);
		gpos::Ref<IDatum> datum = GPOS_NEW(mp) gpnaucrates::CDatumInt8GPDB(
			CTestUtils::m_sysidDefault, 1 /*val*/, false /*is_null*/);
		gpos::Ref<CExpression> pexpr =
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarConst(mp, datum));

		gpos::Ref<CDrvdPropCtxtPlan> pdpctxtplan =
			GPOS_NEW(mp) CDrvdPropCtxtPlan(mp);
		GPOS_TRY
		{
			// FValidPlan should fail for expressions which are not physical.
			if (!pexpr->FValidPlan(prpp, pdpctxtplan))
			{
				eres = GPOS_FAILED;
			}
		}
		GPOS_CATCH_EX(ex)
		{
			if (!GPOS_MATCH_EX(ex, CException::ExmaSystem,
							   CException::ExmiAssert))
			{
				GPOS_RETHROW(ex);
			}
			else
			{
				GPOS_RESET_EX;
			}
		}
		GPOS_CATCH_END;
	}

	return eres;
}
#endif	// GPOS_DEBUG



//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresCheckCachedReqdCols
//
//	@doc:
//		Helper for checking cached required columns
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresCheckCachedReqdCols(CMemoryPool *mp,
										 gpos::Ref<CExpression> pexpr,
										 gpos::Ref<CReqdPropPlan> prppInput)
{
	if (pexpr->Pop()->FScalar())
	{
		// scalar operators have no required columns
		return GPOS_OK;
	}

	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != prppInput);

	CExpressionHandle exprhdl(mp);
	exprhdl.Attach(pexpr);

	// init required properties of expression
	exprhdl.InitReqdProps(prppInput);

	// create array of child derived properties
	gpos::Ref<CDrvdPropArray> pdrgpdp = GPOS_NEW(mp) CDrvdPropArray(mp);

	GPOS_RESULT eres = GPOS_OK;
	const ULONG arity = pexpr->Arity();
	for (ULONG ul = 0; GPOS_OK == eres && ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild = (*pexpr)[ul];
		if (pexprChild->Pop()->FScalar())
		{
			continue;
		}
		GPOS_ASSERT(nullptr != pexprChild->Prpp());

		// extract cached required columns of the n-th child
		gpos::Ref<CColRefSet> pcrsChildReqd =
			pexprChild->Prpp()->PcrsRequired();

		// compute required columns of the n-th child
		exprhdl.ComputeChildReqdCols(ul, pdrgpdp);

		// check if cached columns pointer is the same as computed columns pointer,
		// if this is not the case, then we have re-computed the same set of columns and test should fail
		if (!pcrsChildReqd->Equals(exprhdl.Prpp(ul)->PcrsRequired()))
		{
			CAutoTrace at(mp);
			at.Os() << "\nExpression: \n" << *pexprChild;
			at.Os() << "\nCached cols: " << pcrsChildReqd.get() << " : "
					<< *pcrsChildReqd;
			at.Os() << "\nComputed cols: "
					<< exprhdl.Prpp(ul)->PcrsRequired().get() << " : "
					<< *exprhdl.Prpp(ul)->PcrsRequired();

			eres = GPOS_FAILED;
			continue;
		}

		// call the function recursively for child expression
		GPOS_RESULT eres =
			EresCheckCachedReqdCols(mp, pexprChild, exprhdl.Prpp(ul));
		if (GPOS_FAILED == eres)
		{
			eres = GPOS_FAILED;
			continue;
		}

		// add plan props of current child to derived props array
		gpos::Ref<CDrvdProp> pdp = pexprChild->PdpDerive();

		pdrgpdp->Append(pdp);
	}



	return eres;
}


//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresComputeReqdCols
//
//	@doc:
//		Helper for testing required column computation
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresComputeReqdCols(const CHAR *szFilePath)
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// reset metadata cache
	CMDCache::Reset();

	// set up MD providers
	gpos::Ref<CMDProviderMemory> pmdp =
		GPOS_NEW(mp) CMDProviderMemory(mp, szFilePath);
	GPOS_CHECK_ABORT;

	GPOS_RESULT eres = GPOS_FAILED;
	{
		CAutoMDAccessor amda(mp, pmdp, CTestUtils::m_sysidDefault);
		CAutoOptCtxt aoc(mp, amda.Pmda(), nullptr,
						 /* pceeval */ CTestUtils::GetCostModel(mp));

		// read query expression
		gpos::Ref<CExpression> pexpr =
			CTestUtils::PexprReadQuery(mp, szFilePath);

		// optimize query
		CEngine eng(mp);
		CQueryContext *pqc = CTestUtils::PqcGenerate(mp, pexpr);
		eng.Init(pqc, nullptr /*search_stage_array*/);
		eng.Optimize();

		// extract plan and decorate it with required columns
		gpos::Ref<CExpression> pexprPlan = eng.PexprExtractPlan();

		// attempt computing required columns again --
		// we make sure that we reuse cached required columns at each operator
		eres = EresCheckCachedReqdCols(mp, pexprPlan, pqc->Prpp());



		GPOS_DELETE(pqc);
	}

	return eres;
}


//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresUnittest_ReqdCols
//
//	@doc:
//		Test for required columns computation
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresUnittest_ReqdCols()
{
	const CHAR *rgszTests[] = {
		"../data/dxl/tpch/q1.mdp",
		"../data/dxl/tpch/q3.mdp",
		"../data/dxl/expressiontests/WinFunc-OuterRef-Partition-Query.xml",
		"../data/dxl/expressiontests/WinFunc-OuterRef-Partition-Order-Query.xml",
	};

	GPOS_RESULT eres = GPOS_OK;
	for (ULONG ul = 0; GPOS_OK == eres && ul < GPOS_ARRAY_SIZE(rgszTests); ul++)
	{
		const CHAR *szFilePath = rgszTests[ul];
		eres = EresComputeReqdCols(szFilePath);
	}

	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::EresUnittest_InvalidSetOp
//
//	@doc:
//		Test for invalid SetOp expression,
//		SetOp is expected to have no outer references in input columns,
//		when an outer reference needs to be fed to SetOp as input, we must
//		project it first and feed the projected column into SetOp
//
//		For example, this is an invalid SetOp expression since it consumes
//		an outer reference from inner child
//
//			+--CLogicalUnion [Output: "col0" (0)], [Input: ("col0" (0)), ("col6" (6))]
//			   |--CLogicalGet ("T1"), Columns: ["col0" (0)]
//			   +--CLogicalGet ("T2"), Columns: ["col3" (3)]
//
//
//		the valid expression should looks like this:
//
//			+--CLogicalUnion [Output: "col0" (0)], [Input: ("col0" (0)), ("col7" (7))]
//			   |--CLogicalGet ("T1"), Columns: ["col0" (0)]
//			   +--CLogicalProject
//			       |--CLogicalGet ("T2"), Columns: ["col3" (3)]
//			       +--CScalarProjectList
//			       		+--CScalarProjectElement  "col7" (7)
//			       		      +--CScalarIdent "col6" (6)
//
//
//---------------------------------------------------------------------------
GPOS_RESULT
CExpressionTest::EresUnittest_InvalidSetOp()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// Setup an MD cache with a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;

	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault, pmdp);

	{
		CAutoOptCtxt aoc(mp, &mda, nullptr /* pceeval */,
						 CTestUtils::GetCostModel(mp));

		// create two different Get expressions
		CWStringConst strName1(GPOS_WSZ_LIT("T1"));
		gpos::Ref<CMDIdGPDB> pmdid1 =
			GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidRel, GPOPT_TEST_REL_OID1, 1, 1);
		gpos::Ref<CTableDescriptor> ptabdesc1 =
			CTestUtils::PtabdescCreate(mp, 3, pmdid1, CName(&strName1));
		CWStringConst strAlias1(GPOS_WSZ_LIT("T1Alias"));
		gpos::Ref<CExpression> pexprGet1 =
			CTestUtils::PexprLogicalGet(mp, ptabdesc1, &strAlias1);
		gpos::Ref<CColRefSet> pcrsOutput1 = pexprGet1->DeriveOutputColumns();

		CWStringConst strName2(GPOS_WSZ_LIT("T2"));
		gpos::Ref<CMDIdGPDB> pmdid2 =
			GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidRel, GPOPT_TEST_REL_OID2, 1, 1);
		gpos::Ref<CTableDescriptor> ptabdesc2 =
			CTestUtils::PtabdescCreate(mp, 3, pmdid2, CName(&strName2));
		CWStringConst strAlias2(GPOS_WSZ_LIT("T2Alias"));
		gpos::Ref<CExpression> pexprGet2 =
			CTestUtils::PexprLogicalGet(mp, ptabdesc2, &strAlias2);

		// create output columns of SetOp from output columns of first Get expression
		gpos::Ref<CColRefArray> pdrgpcrOutput = GPOS_NEW(mp) CColRefArray(mp);
		pdrgpcrOutput->Append(pcrsOutput1->PcrFirst());

		// create input columns of SetOp while including an outer reference in inner child
		gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput =
			GPOS_NEW(mp) CColRef2dArray(mp);

		gpos::Ref<CColRefArray> pdrgpcr1 = GPOS_NEW(mp) CColRefArray(mp);
		pdrgpcr1->Append(pcrsOutput1->PcrFirst());
		pdrgpdrgpcrInput->Append(pdrgpcr1);

		gpos::Ref<CColRefArray> pdrgpcr2 = GPOS_NEW(mp) CColRefArray(mp);
		gpos::Ref<CColRef> pcrOuterRef =
			COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
				pcrsOutput1->PcrFirst());
		pdrgpcr2->Append(pcrOuterRef);
		pdrgpdrgpcrInput->Append(pdrgpcr2);

		// create invalid SetOp expression
		gpos::Ref<CLogicalUnion> pop =
			GPOS_NEW(mp) CLogicalUnion(mp, pdrgpcrOutput, pdrgpdrgpcrInput);
		gpos::Ref<CExpression> pexprSetOp =
			GPOS_NEW(mp) CExpression(mp, pop, pexprGet1, pexprGet2);

		{
			CAutoTrace at(mp);
			at.Os() << "\nInvalid SetOp Expression: \n" << *pexprSetOp;
		}

		// deriving relational properties must fail
		(void) pexprSetOp->PdpDerive();
	}

	return GPOS_FAILED;
}

//---------------------------------------------------------------------------
//	@function:
//		CExpressionTest::PexprComplexJoinTree
//
//	@doc:
// 		Return an expression with several joins
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CExpressionTest::PexprComplexJoinTree(CMemoryPool *mp)
{
	// The plan will have this shape
	//
	//	+--CLogicalInnerJoin
	//	   |--CLogicalUnion
	//	   |  |--CLogicalLeftOuterJoin
	//	   |  |  |--CLogicalInnerJoin
	//	   |  |  |  |--CLogicalGet
	//	   |  |  |  +--CLogicalSelect
	//	   |  |  +--CLogicalInnerJoin
	//	   |  |     |--CLogicalGet
	//	   |  |     +--CLogicalDynamicGet
	//	   |  +--CLogicalLeftOuterJoin
	//	   |     |--CLogicalGet
	//	   |     +--CLogicalGet
	//	   +--CLogicalInnerJoin
	//	      |--CLogicalGet
	//	      +--CLogicalSelect

	gpos::Ref<CExpression> pexprInnerJoin1 =
		CTestUtils::PexprLogicalJoin<CLogicalInnerJoin>(
			mp, CTestUtils::PexprLogicalGet(mp),
			CTestUtils::PexprLogicalSelect(mp));
	gpos::Ref<CExpression> pexprJoinIndex =
		CTestUtils::PexprLogicalJoinWithPartitionedAndIndexedInnerChild(mp);
	gpos::Ref<CExpression> pexprLeftJoin =
		CTestUtils::PexprLogicalJoin<CLogicalLeftOuterJoin>(mp, pexprInnerJoin1,
															pexprJoinIndex);

	gpos::Ref<CExpression> pexprOuterJoin =
		CTestUtils::PexprLogicalJoin<CLogicalLeftOuterJoin>(mp);
	gpos::Ref<CExpression> pexprInnerJoin4 =
		CTestUtils::PexprLogicalJoin<CLogicalInnerJoin>(mp, pexprLeftJoin,
														pexprOuterJoin);

	gpos::Ref<CExpression> pexprInnerJoin5 =
		CTestUtils::PexprLogicalJoin<CLogicalInnerJoin>(
			mp, CTestUtils::PexprLogicalGet(mp),
			CTestUtils::PexprLogicalSelect(mp));
	gpos::Ref<CExpression> pexprTopJoin =
		CTestUtils::PexprLogicalJoin<CLogicalInnerJoin>(mp, pexprInnerJoin4,
														pexprInnerJoin5);

	return pexprTopJoin;
}

// EOF
