//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CTranslatorExprToDXL.cpp
//
//	@doc:
//		Implementation of the methods for translating Optimizer physical expression
//		trees into DXL.
//
//	@owner:
//
//
//	@test:
//
//---------------------------------------------------------------------------

#include "gpopt/translate/CTranslatorExprToDXL.h"

#include "gpos/common/CAutoTimer.h"
#include "gpos/common/CHashMap.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CCastUtils.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/cost/ICostModel.h"
#include "gpopt/exception.h"
#include "gpopt/mdcache/CMDAccessorUtils.h"
#include "gpopt/operators/CPhysicalAgg.h"
#include "gpopt/operators/CPhysicalAssert.h"
#include "gpopt/operators/CPhysicalBitmapTableScan.h"
#include "gpopt/operators/CPhysicalCTEConsumer.h"
#include "gpopt/operators/CPhysicalCTEProducer.h"
#include "gpopt/operators/CPhysicalConstTableGet.h"
#include "gpopt/operators/CPhysicalCorrelatedLeftOuterNLJoin.h"
#include "gpopt/operators/CPhysicalDML.h"
#include "gpopt/operators/CPhysicalDynamicBitmapTableScan.h"
#include "gpopt/operators/CPhysicalDynamicIndexScan.h"
#include "gpopt/operators/CPhysicalDynamicTableScan.h"
#include "gpopt/operators/CPhysicalHashAgg.h"
#include "gpopt/operators/CPhysicalHashAggDeduplicate.h"
#include "gpopt/operators/CPhysicalHashJoin.h"
#include "gpopt/operators/CPhysicalIndexOnlyScan.h"
#include "gpopt/operators/CPhysicalIndexScan.h"
#include "gpopt/operators/CPhysicalInnerIndexNLJoin.h"
#include "gpopt/operators/CPhysicalLeftOuterIndexNLJoin.h"
#include "gpopt/operators/CPhysicalLimit.h"
#include "gpopt/operators/CPhysicalMotionGather.h"
#include "gpopt/operators/CPhysicalMotionHashDistribute.h"
#include "gpopt/operators/CPhysicalMotionRandom.h"
#include "gpopt/operators/CPhysicalMotionRoutedDistribute.h"
#include "gpopt/operators/CPhysicalNLJoin.h"
#include "gpopt/operators/CPhysicalPartitionSelector.h"
#include "gpopt/operators/CPhysicalRowTrigger.h"
#include "gpopt/operators/CPhysicalScalarAgg.h"
#include "gpopt/operators/CPhysicalSequenceProject.h"
#include "gpopt/operators/CPhysicalSort.h"
#include "gpopt/operators/CPhysicalSplit.h"
#include "gpopt/operators/CPhysicalSpool.h"
#include "gpopt/operators/CPhysicalStreamAgg.h"
#include "gpopt/operators/CPhysicalStreamAggDeduplicate.h"
#include "gpopt/operators/CPhysicalTVF.h"
#include "gpopt/operators/CPhysicalTableScan.h"
#include "gpopt/operators/CPhysicalUnionAll.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarArray.h"
#include "gpopt/operators/CScalarArrayCoerceExpr.h"
#include "gpopt/operators/CScalarArrayRef.h"
#include "gpopt/operators/CScalarAssertConstraint.h"
#include "gpopt/operators/CScalarBitmapBoolOp.h"
#include "gpopt/operators/CScalarBitmapIndexProbe.h"
#include "gpopt/operators/CScalarBooleanTest.h"
#include "gpopt/operators/CScalarCaseTest.h"
#include "gpopt/operators/CScalarCast.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarCoalesce.h"
#include "gpopt/operators/CScalarCoerceToDomain.h"
#include "gpopt/operators/CScalarCoerceViaIO.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarIf.h"
#include "gpopt/operators/CScalarIsDistinctFrom.h"
#include "gpopt/operators/CScalarMinMax.h"
#include "gpopt/operators/CScalarNullIf.h"
#include "gpopt/operators/CScalarOp.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarSortGroupClause.h"
#include "gpopt/operators/CScalarSwitch.h"
#include "gpopt/translate/CTranslatorExprToDXLUtils.h"
#include "naucrates/base/CDatumBoolGPDB.h"
#include "naucrates/base/IDatumInt8.h"
#include "naucrates/dxl/operators/CDXLCtasStorageOptions.h"
#include "naucrates/dxl/operators/CDXLDatumBool.h"
#include "naucrates/dxl/operators/CDXLDirectDispatchInfo.h"
#include "naucrates/dxl/operators/CDXLPhysicalAppend.h"
#include "naucrates/dxl/operators/CDXLPhysicalAssert.h"
#include "naucrates/dxl/operators/CDXLPhysicalBitmapTableScan.h"
#include "naucrates/dxl/operators/CDXLPhysicalBroadcastMotion.h"
#include "naucrates/dxl/operators/CDXLPhysicalCTAS.h"
#include "naucrates/dxl/operators/CDXLPhysicalCTEConsumer.h"
#include "naucrates/dxl/operators/CDXLPhysicalCTEProducer.h"
#include "naucrates/dxl/operators/CDXLPhysicalExternalScan.h"
#include "naucrates/dxl/operators/CDXLPhysicalGatherMotion.h"
#include "naucrates/dxl/operators/CDXLPhysicalHashJoin.h"
#include "naucrates/dxl/operators/CDXLPhysicalIndexOnlyScan.h"
#include "naucrates/dxl/operators/CDXLPhysicalIndexScan.h"
#include "naucrates/dxl/operators/CDXLPhysicalLimit.h"
#include "naucrates/dxl/operators/CDXLPhysicalMaterialize.h"
#include "naucrates/dxl/operators/CDXLPhysicalMergeJoin.h"
#include "naucrates/dxl/operators/CDXLPhysicalNLJoin.h"
#include "naucrates/dxl/operators/CDXLPhysicalPartitionSelector.h"
#include "naucrates/dxl/operators/CDXLPhysicalRandomMotion.h"
#include "naucrates/dxl/operators/CDXLPhysicalRedistributeMotion.h"
#include "naucrates/dxl/operators/CDXLPhysicalResult.h"
#include "naucrates/dxl/operators/CDXLPhysicalRoutedDistributeMotion.h"
#include "naucrates/dxl/operators/CDXLPhysicalRowTrigger.h"
#include "naucrates/dxl/operators/CDXLPhysicalSequence.h"
#include "naucrates/dxl/operators/CDXLPhysicalSort.h"
#include "naucrates/dxl/operators/CDXLPhysicalSplit.h"
#include "naucrates/dxl/operators/CDXLPhysicalTVF.h"
#include "naucrates/dxl/operators/CDXLPhysicalTableScan.h"
#include "naucrates/dxl/operators/CDXLPhysicalWindow.h"
#include "naucrates/dxl/operators/CDXLScalarAggref.h"
#include "naucrates/dxl/operators/CDXLScalarArray.h"
#include "naucrates/dxl/operators/CDXLScalarArrayCoerceExpr.h"
#include "naucrates/dxl/operators/CDXLScalarArrayComp.h"
#include "naucrates/dxl/operators/CDXLScalarArrayRef.h"
#include "naucrates/dxl/operators/CDXLScalarAssertConstraint.h"
#include "naucrates/dxl/operators/CDXLScalarAssertConstraintList.h"
#include "naucrates/dxl/operators/CDXLScalarBitmapBoolOp.h"
#include "naucrates/dxl/operators/CDXLScalarBitmapIndexProbe.h"
#include "naucrates/dxl/operators/CDXLScalarBooleanTest.h"
#include "naucrates/dxl/operators/CDXLScalarCaseTest.h"
#include "naucrates/dxl/operators/CDXLScalarCast.h"
#include "naucrates/dxl/operators/CDXLScalarCoalesce.h"
#include "naucrates/dxl/operators/CDXLScalarCoerceToDomain.h"
#include "naucrates/dxl/operators/CDXLScalarCoerceViaIO.h"
#include "naucrates/dxl/operators/CDXLScalarComp.h"
#include "naucrates/dxl/operators/CDXLScalarDMLAction.h"
#include "naucrates/dxl/operators/CDXLScalarDistinctComp.h"
#include "naucrates/dxl/operators/CDXLScalarFuncExpr.h"
#include "naucrates/dxl/operators/CDXLScalarHashCondList.h"
#include "naucrates/dxl/operators/CDXLScalarHashExpr.h"
#include "naucrates/dxl/operators/CDXLScalarHashExprList.h"
#include "naucrates/dxl/operators/CDXLScalarIfStmt.h"
#include "naucrates/dxl/operators/CDXLScalarIndexCondList.h"
#include "naucrates/dxl/operators/CDXLScalarJoinFilter.h"
#include "naucrates/dxl/operators/CDXLScalarLimitCount.h"
#include "naucrates/dxl/operators/CDXLScalarLimitOffset.h"
#include "naucrates/dxl/operators/CDXLScalarMergeCondList.h"
#include "naucrates/dxl/operators/CDXLScalarMinMax.h"
#include "naucrates/dxl/operators/CDXLScalarNullIf.h"
#include "naucrates/dxl/operators/CDXLScalarNullTest.h"
#include "naucrates/dxl/operators/CDXLScalarOneTimeFilter.h"
#include "naucrates/dxl/operators/CDXLScalarOpExpr.h"
#include "naucrates/dxl/operators/CDXLScalarOpList.h"
#include "naucrates/dxl/operators/CDXLScalarPartBound.h"
#include "naucrates/dxl/operators/CDXLScalarPartDefault.h"
#include "naucrates/dxl/operators/CDXLScalarPartListNullTest.h"
#include "naucrates/dxl/operators/CDXLScalarProjElem.h"
#include "naucrates/dxl/operators/CDXLScalarProjList.h"
#include "naucrates/dxl/operators/CDXLScalarRecheckCondFilter.h"
#include "naucrates/dxl/operators/CDXLScalarSortCol.h"
#include "naucrates/dxl/operators/CDXLScalarSortColList.h"
#include "naucrates/dxl/operators/CDXLScalarSortGroupClause.h"
#include "naucrates/dxl/operators/CDXLScalarSwitch.h"
#include "naucrates/dxl/operators/CDXLScalarSwitchCase.h"
#include "naucrates/dxl/operators/CDXLScalarValuesList.h"
#include "naucrates/dxl/operators/CDXLScalarWindowFrameEdge.h"
#include "naucrates/dxl/operators/CDXLWindowFrame.h"
#include "naucrates/dxl/operators/CDXLWindowKey.h"
#include "naucrates/exception.h"
#include "naucrates/md/CMDRelationCtasGPDB.h"
#include "naucrates/md/IMDCast.h"
#include "naucrates/md/IMDFunction.h"
#include "naucrates/md/IMDScalarOp.h"
#include "naucrates/md/IMDTypeInt4.h"
#include "naucrates/statistics/CStatistics.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpos;
using namespace gpmd;
using namespace gpdxl;
using namespace gpopt;
using namespace gpnaucrates;

#define GPOPT_MASTER_SEGMENT_ID (-1)

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::CTranslatorExprToDXL
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CTranslatorExprToDXL::CTranslatorExprToDXL(
	CMemoryPool *mp, CMDAccessor *md_accessor,
	gpos::owner<IntPtrArray *> pdrgpiSegments, BOOL fInitColumnFactory)
	: m_mp(mp),
	  m_pmda(md_accessor),
	  m_pdpplan(nullptr),
	  m_pcf(nullptr),
	  m_pdrgpiSegments(pdrgpiSegments),
	  m_iMasterId(GPOPT_MASTER_SEGMENT_ID)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != md_accessor);
	GPOS_ASSERT_IMP(nullptr != m_pdrgpiSegments,
					(0 < m_pdrgpiSegments->Size()));

	// initialize hash map
	m_phmcrdxln = GPOS_NEW(m_mp) ColRefToDXLNodeMap(m_mp);

	m_phmcrdxlnIndexLookup = GPOS_NEW(m_mp) ColRefToDXLNodeMap(m_mp);

	if (fInitColumnFactory)
	{
		// get column factory from optimizer context object
		m_pcf = COptCtxt::PoctxtFromTLS()->Pcf();
		GPOS_ASSERT(nullptr != m_pcf);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::~CTranslatorExprToDXL
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CTranslatorExprToDXL::~CTranslatorExprToDXL()
{
	CRefCount::SafeRelease(m_pdrgpiSegments);
	m_phmcrdxln->Release();
	m_phmcrdxlnIndexLookup->Release();
	CRefCount::SafeRelease(m_pdpplan);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnTranslate
//
//	@doc:
//		Main driver
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnTranslate(CExpression *pexpr,
									 CColRefArray *colref_array,
									 gpos::pointer<CMDNameArray *> pdrgpmdname)
{
	CAutoTimer at("\n[OPT]: Expr To DXL Translation Time",
				  GPOS_FTRACE(EopttracePrintOptimizationStatistics));

	GPOS_ASSERT(nullptr == m_pdpplan);

	m_pdpplan = gpos::dyn_cast<CDrvdPropPlan>(pexpr->PdpDerive());
	m_pdpplan->AddRef();

	gpos::owner<CDistributionSpecArray *> pdrgpdsBaseTables =
		GPOS_NEW(m_mp) CDistributionSpecArray(m_mp);
	ULONG ulNonGatherMotions = 0;
	BOOL fDML = false;
	gpos::owner<CDXLNode *> dxlnode = CreateDXLNode(
		pexpr, colref_array, pdrgpdsBaseTables, &ulNonGatherMotions, &fDML,
		true /*fRemap*/, true /*fRoot*/);

	if (fDML)
	{
		pdrgpdsBaseTables->Release();
		return dxlnode;
	}

	gpos::pointer<CDXLNode *> pdxlnPrL = (*dxlnode)[0];
	GPOS_ASSERT(EdxlopScalarProjectList ==
				pdxlnPrL->GetOperator()->GetDXLOperator());

	const ULONG length = pdrgpmdname->Size();
	GPOS_ASSERT(length == colref_array->Size());
	GPOS_ASSERT(length == pdxlnPrL->Arity());
	for (ULONG ul = 0; ul < length; ul++)
	{
		// desired output column name
		CMDName *mdname =
			GPOS_NEW(m_mp) CMDName(m_mp, (*pdrgpmdname)[ul]->GetMDName());

		// get the old project element for the ColId
		gpos::pointer<CDXLNode *> pdxlnPrElOld = (*pdxlnPrL)[ul];
		gpos::pointer<CDXLScalarProjElem *> pdxlopPrElOld =
			gpos::dyn_cast<CDXLScalarProjElem>(pdxlnPrElOld->GetOperator());
		GPOS_ASSERT(1 == pdxlnPrElOld->Arity());
		CDXLNode *child_dxlnode = (*pdxlnPrElOld)[0];
		const ULONG colid = pdxlopPrElOld->Id();

		// create a new project element node with the col id and new column name
		// and add the scalar child
		gpos::owner<CDXLNode *> pdxlnPrElNew = GPOS_NEW(m_mp) CDXLNode(
			m_mp, GPOS_NEW(m_mp) CDXLScalarProjElem(m_mp, colid, mdname));
		child_dxlnode->AddRef();
		pdxlnPrElNew->AddChild(child_dxlnode);

		// replace the project element
		pdxlnPrL->ReplaceChild(ul, pdxlnPrElNew);
	}



	if (0 == ulNonGatherMotions)
	{
		CTranslatorExprToDXLUtils::SetDirectDispatchInfo(
			m_mp, m_pmda, dxlnode, pexpr, pdrgpdsBaseTables);
	}

	pdrgpdsBaseTables->Release();
	return dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::CreateDXLNode
//
//	@doc:
//		Translates an optimizer physical expression tree into DXL.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::CreateDXLNode(CExpression *pexpr,
									CColRefArray *colref_array,
									CDistributionSpecArray *pdrgpdsBaseTables,
									ULONG *pulNonGatherMotions, BOOL *pfDML,
									BOOL fRemap, BOOL fRoot)
{
	GPOS_ASSERT(nullptr != pexpr);
	ULONG ulOpId = (ULONG) pexpr->Pop()->Eopid();
	if (COperator::EopPhysicalTableScan == ulOpId ||
		COperator::EopPhysicalExternalScan == ulOpId)
	{
		gpos::owner<CDXLNode *> dxlnode = PdxlnTblScan(
			pexpr, nullptr /*pcrsOutput*/, colref_array, pdrgpdsBaseTables,
			nullptr /* pexprScalarCond */, nullptr /* cost info */);
		CTranslatorExprToDXLUtils::SetStats(m_mp, m_pmda, dxlnode,
											pexpr->Pstats(), fRoot);

		return dxlnode;
	}
	// add a result node on top to project out columns not needed any further,
	// for instance, if the grouping /order by /partition/ distribution columns
	// are no longer needed
	gpos::owner<CDXLNode *> pdxlnNew = nullptr;

	gpos::owner<CDXLNode *> dxlnode = nullptr;
	switch (pexpr->Pop()->Eopid())
	{
		case COperator::EopPhysicalFilter:
			dxlnode = CTranslatorExprToDXL::PdxlnResult(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalIndexScan:
			dxlnode = CTranslatorExprToDXL::PdxlnIndexScan(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalIndexOnlyScan:
			dxlnode = CTranslatorExprToDXL::PdxlnIndexOnlyScan(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalBitmapTableScan:
			dxlnode = CTranslatorExprToDXL::PdxlnBitmapTableScan(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalComputeScalar:
			dxlnode = CTranslatorExprToDXL::PdxlnComputeScalar(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalScalarAgg:
		case COperator::EopPhysicalHashAgg:
		case COperator::EopPhysicalStreamAgg:
			dxlnode = CTranslatorExprToDXL::PdxlnAggregate(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalHashAggDeduplicate:
		case COperator::EopPhysicalStreamAggDeduplicate:
			dxlnode = CTranslatorExprToDXL::PdxlnAggregateDedup(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalSort:
			dxlnode = CTranslatorExprToDXL::PdxlnSort(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalLimit:
			dxlnode = CTranslatorExprToDXL::PdxlnLimit(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalSequenceProject:
			dxlnode = CTranslatorExprToDXL::PdxlnWindow(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalInnerNLJoin:
		case COperator::EopPhysicalInnerIndexNLJoin:
		case COperator::EopPhysicalLeftOuterIndexNLJoin:
		case COperator::EopPhysicalLeftOuterNLJoin:
		case COperator::EopPhysicalLeftSemiNLJoin:
		case COperator::EopPhysicalLeftAntiSemiNLJoin:
		case COperator::EopPhysicalLeftAntiSemiNLJoinNotIn:
			dxlnode = CTranslatorExprToDXL::PdxlnNLJoin(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalCorrelatedInnerNLJoin:
		case COperator::EopPhysicalCorrelatedLeftOuterNLJoin:
		case COperator::EopPhysicalCorrelatedLeftSemiNLJoin:
		case COperator::EopPhysicalCorrelatedInLeftSemiNLJoin:
		case COperator::EopPhysicalCorrelatedLeftAntiSemiNLJoin:
		case COperator::EopPhysicalCorrelatedNotInLeftAntiSemiNLJoin:
			dxlnode = CTranslatorExprToDXL::PdxlnCorrelatedNLJoin(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalInnerHashJoin:
		case COperator::EopPhysicalLeftOuterHashJoin:
		case COperator::EopPhysicalLeftSemiHashJoin:
		case COperator::EopPhysicalLeftAntiSemiHashJoin:
		case COperator::EopPhysicalLeftAntiSemiHashJoinNotIn:
		case COperator::EopPhysicalRightOuterHashJoin:
			dxlnode = CTranslatorExprToDXL::PdxlnHashJoin(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalMotionGather:
		case COperator::EopPhysicalMotionBroadcast:
		case COperator::EopPhysicalMotionHashDistribute:
		case COperator::EopPhysicalMotionRoutedDistribute:
		case COperator::EopPhysicalMotionRandom:
			dxlnode = CTranslatorExprToDXL::PdxlnMotion(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalSpool:
			dxlnode = CTranslatorExprToDXL::PdxlnMaterialize(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalSequence:
			dxlnode = CTranslatorExprToDXL::PdxlnSequence(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalDynamicTableScan:
			dxlnode = CTranslatorExprToDXL::PdxlnDynamicTableScan(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalDynamicBitmapTableScan:
			dxlnode = CTranslatorExprToDXL::PdxlnDynamicBitmapTableScan(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalDynamicIndexScan:
			dxlnode = CTranslatorExprToDXL::PdxlnDynamicIndexScan(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalPartitionSelector:
			dxlnode = CTranslatorExprToDXL::PdxlnPartitionSelector(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalConstTableGet:
			dxlnode = CTranslatorExprToDXL::PdxlnResultFromConstTableGet(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalTVF:
			dxlnode = CTranslatorExprToDXL::PdxlnTVF(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalSerialUnionAll:
		case COperator::EopPhysicalParallelUnionAll:
			dxlnode = CTranslatorExprToDXL::PdxlnAppend(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalDML:
			dxlnode = CTranslatorExprToDXL::PdxlnDML(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalSplit:
			dxlnode = CTranslatorExprToDXL::PdxlnSplit(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalRowTrigger:
			dxlnode = CTranslatorExprToDXL::PdxlnRowTrigger(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalAssert:
			dxlnode = CTranslatorExprToDXL::PdxlnAssert(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalCTEProducer:
			dxlnode = CTranslatorExprToDXL::PdxlnCTEProducer(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalCTEConsumer:
			dxlnode = CTranslatorExprToDXL::PdxlnCTEConsumer(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		case COperator::EopPhysicalFullMergeJoin:
			dxlnode = CTranslatorExprToDXL::PdxlnMergeJoin(
				pexpr, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
				pfDML);
			break;
		default:
			GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp,
					   pexpr->Pop()->SzId());
			return nullptr;
	}


	if (!fRemap ||
		EdxlopPhysicalDML == dxlnode->GetOperator()->GetDXLOperator())
	{
		pdxlnNew = dxlnode;
	}
	else
	{
		gpos::owner<CColRefArray *> pdrgpcrRequired = nullptr;

		if (EdxlopPhysicalCTAS == dxlnode->GetOperator()->GetDXLOperator())
		{
			colref_array->AddRef();
			pdrgpcrRequired = colref_array;
		}
		else
		{
			pdrgpcrRequired = pexpr->Prpp()->PcrsRequired()->Pdrgpcr(m_mp);
		}
		pdxlnNew = PdxlnRemapOutputColumns(pexpr, dxlnode, pdrgpcrRequired,
										   colref_array);
		pdrgpcrRequired->Release();
	}

	if (nullptr == pdxlnNew->GetProperties()->GetDxlStatsDrvdRelation())
	{
		CTranslatorExprToDXLUtils::SetStats(m_mp, m_pmda, pdxlnNew,
											pexpr->Pstats(), fRoot);
	}

	return pdxlnNew;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScalar
//
//	@doc:
//		Translates an optimizer scalar expression tree into DXL. Any column
//		refs that are members of the input colrefset are replaced by the input
//		subplan node
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScalar(gpos::pointer<CExpression *> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	switch (pexpr->Pop()->Eopid())
	{
		case COperator::EopScalarIdent:
			return CTranslatorExprToDXL::PdxlnScId(pexpr);
		case COperator::EopScalarCmp:
			return CTranslatorExprToDXL::PdxlnScCmp(pexpr);
		case COperator::EopScalarIsDistinctFrom:
			return CTranslatorExprToDXL::PdxlnScDistinctCmp(pexpr);
		case COperator::EopScalarOp:
			return CTranslatorExprToDXL::PdxlnScOp(pexpr);
		case COperator::EopScalarBoolOp:
			return CTranslatorExprToDXL::PdxlnScBoolExpr(pexpr);
		case COperator::EopScalarConst:
			return CTranslatorExprToDXL::PdxlnScConst(pexpr);
		case COperator::EopScalarSortGroupClause:
			return CTranslatorExprToDXL::PdxlnScSortGroupClause(pexpr);
		case COperator::EopScalarFunc:
			return CTranslatorExprToDXL::PdxlnScFuncExpr(pexpr);
		case COperator::EopScalarWindowFunc:
			return CTranslatorExprToDXL::PdxlnScWindowFuncExpr(pexpr);
		case COperator::EopScalarAggFunc:
			return CTranslatorExprToDXL::PdxlnScAggref(pexpr);
		case COperator::EopScalarNullIf:
			return CTranslatorExprToDXL::PdxlnScNullIf(pexpr);
		case COperator::EopScalarNullTest:
			return CTranslatorExprToDXL::PdxlnScNullTest(pexpr);
		case COperator::EopScalarBooleanTest:
			return CTranslatorExprToDXL::PdxlnScBooleanTest(pexpr);
		case COperator::EopScalarIf:
			return CTranslatorExprToDXL::PdxlnScIfStmt(pexpr);
		case COperator::EopScalarSwitch:
			return CTranslatorExprToDXL::PdxlnScSwitch(pexpr);
		case COperator::EopScalarSwitchCase:
			return CTranslatorExprToDXL::PdxlnScSwitchCase(pexpr);
		case COperator::EopScalarCaseTest:
			return CTranslatorExprToDXL::PdxlnScCaseTest(pexpr);
		case COperator::EopScalarCoalesce:
			return CTranslatorExprToDXL::PdxlnScCoalesce(pexpr);
		case COperator::EopScalarMinMax:
			return CTranslatorExprToDXL::PdxlnScMinMax(pexpr);
		case COperator::EopScalarCast:
			return CTranslatorExprToDXL::PdxlnScCast(pexpr);
		case COperator::EopScalarCoerceToDomain:
			return CTranslatorExprToDXL::PdxlnScCoerceToDomain(pexpr);
		case COperator::EopScalarCoerceViaIO:
			return CTranslatorExprToDXL::PdxlnScCoerceViaIO(pexpr);
		case COperator::EopScalarArrayCoerceExpr:
			return CTranslatorExprToDXL::PdxlnScArrayCoerceExpr(pexpr);
		case COperator::EopScalarArray:
			return CTranslatorExprToDXL::PdxlnArray(pexpr);
		case COperator::EopScalarValuesList:
			return CTranslatorExprToDXL::PdxlnValuesList(pexpr);
		case COperator::EopScalarArrayCmp:
			return CTranslatorExprToDXL::PdxlnArrayCmp(pexpr);
		case COperator::EopScalarArrayRef:
			return CTranslatorExprToDXL::PdxlnArrayRef(pexpr);
		case COperator::EopScalarArrayRefIndexList:
			return CTranslatorExprToDXL::PdxlnArrayRefIndexList(pexpr);
		case COperator::EopScalarAssertConstraintList:
			return CTranslatorExprToDXL::PdxlnAssertPredicate(pexpr);
		case COperator::EopScalarAssertConstraint:
			return CTranslatorExprToDXL::PdxlnAssertConstraint(pexpr);
		case COperator::EopScalarDMLAction:
			return CTranslatorExprToDXL::PdxlnDMLAction(pexpr);
		case COperator::EopScalarBitmapIndexProbe:
			return CTranslatorExprToDXL::PdxlnBitmapIndexProbe(pexpr);
		case COperator::EopScalarBitmapBoolOp:
			return CTranslatorExprToDXL::PdxlnBitmapBoolOp(pexpr);
		default:
			GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp,
					   pexpr->Pop()->SzId());
			return nullptr;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnTblScan
//
//	@doc:
//		Create a DXL table scan node from an optimizer table scan node
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnTblScan(
	gpos::pointer<CExpression *> pexprTblScan,
	gpos::pointer<CColRefSet *> pcrsOutput,
	gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
	gpos::pointer<CExpression *> pexprScalar,
	gpos::owner<CDXLPhysicalProperties *> dxl_properties)
{
	GPOS_ASSERT(nullptr != pexprTblScan);

	gpos::pointer<CPhysicalTableScan *> popTblScan =
		gpos::dyn_cast<CPhysicalTableScan>(pexprTblScan->Pop());
	gpos::pointer<CColRefArray *> pdrgpcrOutput = popTblScan->PdrgpcrOutput();

	// translate table descriptor
	gpos::owner<CDXLTableDescr *> table_descr = MakeDXLTableDescr(
		popTblScan->Ptabdesc(), pdrgpcrOutput, pexprTblScan->Prpp());

	// construct plan costs, if there are not passed as a parameter
	if (nullptr == dxl_properties)
	{
		dxl_properties = GetProperties(pexprTblScan);
	}

	// construct scan operator
	gpos::owner<CDXLPhysicalTableScan *> pdxlopTS = nullptr;
	COperator::EOperatorId op_id = pexprTblScan->Pop()->Eopid();
	if (COperator::EopPhysicalTableScan == op_id)
	{
		pdxlopTS = GPOS_NEW(m_mp) CDXLPhysicalTableScan(m_mp, table_descr);
	}
	else
	{
		GPOS_ASSERT(COperator::EopPhysicalExternalScan == op_id);
		pdxlopTS = GPOS_NEW(m_mp) CDXLPhysicalExternalScan(m_mp, table_descr);
	}

	gpos::owner<CDXLNode *> pdxlnTblScan =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopTS);
	pdxlnTblScan->SetProperties(dxl_properties);

	// construct projection list
	GPOS_ASSERT(nullptr != pexprTblScan->Prpp());

	// if the output columns are passed from above then use them
	if (nullptr == pcrsOutput)
	{
		pcrsOutput = pexprTblScan->Prpp()->PcrsRequired();
	}
	gpos::owner<CDXLNode *> pdxlnPrL = PdxlnProjList(pcrsOutput, colref_array);

	gpos::owner<CDXLNode *> pdxlnCond = nullptr;
	if (nullptr != pexprScalar)
	{
		pdxlnCond = PdxlnScalar(pexprScalar);
	}

	gpos::owner<CDXLNode *> filter_dxlnode = PdxlnFilter(std::move(pdxlnCond));

	// add children in the right order
	pdxlnTblScan->AddChild(std::move(pdxlnPrL));		// project list
	pdxlnTblScan->AddChild(std::move(filter_dxlnode));	// filter

#ifdef GPOS_DEBUG
	pdxlnTblScan->GetOperator()->AssertValid(pdxlnTblScan,
											 false /* validate_children */);
#endif

	gpos::owner<CDistributionSpec *> pds =
		pexprTblScan->GetDrvdPropPlan()->Pds();
	pds->AddRef();
	pdrgpdsBaseTables->Append(std::move(pds));
	return pdxlnTblScan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnIndexScan
//
//	@doc:
//		Create a DXL index scan node from an optimizer index scan node based
//		on passed properties
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnIndexScan(
	CExpression *pexprIndexScan, gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
	ULONG *,  // pulNonGatherMotions,
	BOOL *	  // pfDML
)
{
	GPOS_ASSERT(nullptr != pexprIndexScan);
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprIndexScan);

	COptCtxt::PoctxtFromTLS()->AddDirectDispatchableFilterCandidate(
		pexprIndexScan);

	gpos::owner<CDistributionSpec *> pds =
		pexprIndexScan->GetDrvdPropPlan()->Pds();
	pds->AddRef();
	pdrgpdsBaseTables->Append(std::move(pds));
	return PdxlnIndexScan(pexprIndexScan, colref_array,
						  std::move(dxl_properties), pexprIndexScan->Prpp());
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnIndexScan
//
//	@doc:
//		Create a DXL index scan node from an optimizer index scan node
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnIndexScan(
	gpos::pointer<CExpression *> pexprIndexScan,
	gpos::pointer<CColRefArray *> colref_array,
	gpos::owner<CDXLPhysicalProperties *> dxl_properties,
	gpos::pointer<CReqdPropPlan *> prpp)
{
	GPOS_ASSERT(nullptr != pexprIndexScan);
	GPOS_ASSERT(nullptr != dxl_properties);
	GPOS_ASSERT(nullptr != prpp);

	gpos::pointer<CPhysicalIndexScan *> popIs =
		gpos::dyn_cast<CPhysicalIndexScan>(pexprIndexScan->Pop());

	gpos::pointer<CColRefArray *> pdrgpcrOutput = popIs->PdrgpcrOutput();

	// translate table descriptor
	gpos::owner<CDXLTableDescr *> table_descr = MakeDXLTableDescr(
		popIs->Ptabdesc(), pdrgpcrOutput, pexprIndexScan->Prpp());

	// create index descriptor
	gpos::pointer<CIndexDescriptor *> pindexdesc = popIs->Pindexdesc();
	CMDName *pmdnameIndex =
		GPOS_NEW(m_mp) CMDName(m_mp, pindexdesc->Name().Pstr());
	gpos::owner<IMDId *> pmdidIndex = pindexdesc->MDId();
	pmdidIndex->AddRef();
	gpos::owner<CDXLIndexDescr *> dxl_index_descr =
		GPOS_NEW(m_mp) CDXLIndexDescr(pmdidIndex, pmdnameIndex);

	// TODO: vrgahavan; we assume that the index are always forward access.
	// create the physical index scan operator
	gpos::owner<CDXLPhysicalIndexScan *> dxl_op =
		GPOS_NEW(m_mp) CDXLPhysicalIndexScan(m_mp, table_descr, dxl_index_descr,
											 EdxlisdForward);
	gpos::owner<CDXLNode *> pdxlnIndexScan =
		GPOS_NEW(m_mp) CDXLNode(m_mp, dxl_op);

	// set properties
	pdxlnIndexScan->SetProperties(dxl_properties);

	// translate project list
	gpos::pointer<CColRefSet *> pcrsOutput = prpp->PcrsRequired();
	gpos::owner<CDXLNode *> pdxlnPrL = PdxlnProjList(pcrsOutput, colref_array);

	// translate index predicates
	gpos::pointer<CExpression *> pexprCond = (*pexprIndexScan)[0];
	gpos::owner<CDXLNode *> pdxlnIndexCondList = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarIndexCondList(m_mp));

	gpos::owner<CExpressionArray *> pdrgpexprConds =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprCond);
	const ULONG length = pdrgpexprConds->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::pointer<CExpression *> pexprIndexCond = (*pdrgpexprConds)[ul];
		gpos::owner<CDXLNode *> pdxlnIndexCond = PdxlnScalar(pexprIndexCond);
		pdxlnIndexCondList->AddChild(pdxlnIndexCond);
	}
	pdrgpexprConds->Release();

	gpos::owner<CDXLNode *> pdxlnResidualCond = nullptr;
	if (2 == pexprIndexScan->Arity())
	{
		// translate residual predicates into the filter node
		gpos::pointer<CExpression *> pexprResidualCond = (*pexprIndexScan)[1];
		if (COperator::EopScalarConst != pexprResidualCond->Pop()->Eopid())
		{
			pdxlnResidualCond = PdxlnScalar(pexprResidualCond);
		}
	}

	gpos::owner<CDXLNode *> filter_dxlnode =
		PdxlnFilter(std::move(pdxlnResidualCond));

	pdxlnIndexScan->AddChild(std::move(pdxlnPrL));
	pdxlnIndexScan->AddChild(std::move(filter_dxlnode));
	pdxlnIndexScan->AddChild(std::move(pdxlnIndexCondList));

#ifdef GPOS_DEBUG
	pdxlnIndexScan->GetOperator()->AssertValid(pdxlnIndexScan,
											   false /* validate_children */);
#endif


	return pdxlnIndexScan;
}

gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnIndexOnlyScan(
	CExpression *pexprIndexOnlyScan, gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
	ULONG *,  // pulNonGatherMotions,
	BOOL *	  // pfDML
)
{
	GPOS_ASSERT(nullptr != pexprIndexOnlyScan);
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprIndexOnlyScan);

	COptCtxt::PoctxtFromTLS()->AddDirectDispatchableFilterCandidate(
		pexprIndexOnlyScan);

	gpos::owner<CDistributionSpec *> pds =
		pexprIndexOnlyScan->GetDrvdPropPlan()->Pds();
	pds->AddRef();
	pdrgpdsBaseTables->Append(std::move(pds));
	return PdxlnIndexOnlyScan(pexprIndexOnlyScan, colref_array,
							  std::move(dxl_properties),
							  pexprIndexOnlyScan->Prpp());
}

gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnIndexOnlyScan(
	gpos::pointer<CExpression *> pexprIndexOnlyScan,
	gpos::pointer<CColRefArray *> colref_array,
	gpos::owner<CDXLPhysicalProperties *> dxl_properties,
	gpos::pointer<CReqdPropPlan *> prpp)
{
	GPOS_ASSERT(nullptr != pexprIndexOnlyScan);
	GPOS_ASSERT(nullptr != dxl_properties);
	GPOS_ASSERT(nullptr != prpp);

	gpos::pointer<CPhysicalIndexOnlyScan *> popIs =
		gpos::dyn_cast<CPhysicalIndexOnlyScan>(pexprIndexOnlyScan->Pop());

	gpos::pointer<CColRefArray *> pdrgpcrOutput = popIs->PdrgpcrOutput();

	// translate table descriptor
	gpos::owner<CDXLTableDescr *> table_descr = MakeDXLTableDescr(
		popIs->Ptabdesc(), pdrgpcrOutput, pexprIndexOnlyScan->Prpp());

	// create index descriptor
	gpos::pointer<CIndexDescriptor *> pindexdesc = popIs->Pindexdesc();
	CMDName *pmdnameIndex =
		GPOS_NEW(m_mp) CMDName(m_mp, pindexdesc->Name().Pstr());
	gpos::owner<IMDId *> pmdidIndex = pindexdesc->MDId();
	pmdidIndex->AddRef();
	gpos::owner<CDXLIndexDescr *> dxl_index_descr =
		GPOS_NEW(m_mp) CDXLIndexDescr(pmdidIndex, pmdnameIndex);

	// TODO: vrgahavan; we assume that the index are always forward access.
	// create the physical index scan operator
	gpos::owner<CDXLPhysicalIndexOnlyScan *> dxl_op =
		GPOS_NEW(m_mp) CDXLPhysicalIndexOnlyScan(
			m_mp, table_descr, dxl_index_descr, EdxlisdForward);
	gpos::owner<CDXLNode *> pdxlnIndexOnlyScan =
		GPOS_NEW(m_mp) CDXLNode(m_mp, dxl_op);

	// set properties
	pdxlnIndexOnlyScan->SetProperties(dxl_properties);

	// translate project list
	gpos::pointer<CColRefSet *> pcrsOutput = prpp->PcrsRequired();
	gpos::owner<CDXLNode *> pdxlnPrL = PdxlnProjList(pcrsOutput, colref_array);

	// translate index predicates
	gpos::pointer<CExpression *> pexprCond = (*pexprIndexOnlyScan)[0];
	gpos::owner<CDXLNode *> pdxlnIndexCondList = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarIndexCondList(m_mp));

	gpos::owner<CExpressionArray *> pdrgpexprConds =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprCond);
	const ULONG length = pdrgpexprConds->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::pointer<CExpression *> pexprIndexCond = (*pdrgpexprConds)[ul];
		gpos::owner<CDXLNode *> pdxlnIndexCond = PdxlnScalar(pexprIndexCond);
		pdxlnIndexCondList->AddChild(pdxlnIndexCond);
	}
	pdrgpexprConds->Release();

	gpos::owner<CDXLNode *> pdxlnResidualCond = nullptr;
	if (2 == pexprIndexOnlyScan->Arity())
	{
		// translate residual predicates into the filter node
		gpos::pointer<CExpression *> pexprResidualCond =
			(*pexprIndexOnlyScan)[1];
		if (COperator::EopScalarConst != pexprResidualCond->Pop()->Eopid())
		{
			pdxlnResidualCond = PdxlnScalar(pexprResidualCond);
		}
	}

	gpos::owner<CDXLNode *> filter_dxlnode =
		PdxlnFilter(std::move(pdxlnResidualCond));

	pdxlnIndexOnlyScan->AddChild(std::move(pdxlnPrL));
	pdxlnIndexOnlyScan->AddChild(std::move(filter_dxlnode));
	pdxlnIndexOnlyScan->AddChild(std::move(pdxlnIndexCondList));

#ifdef GPOS_DEBUG
	pdxlnIndexOnlyScan->GetOperator()->AssertValid(
		pdxlnIndexOnlyScan, false /* validate_children */);
#endif


	return pdxlnIndexOnlyScan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnBitmapIndexProbe
//
//	@doc:
//		Create a DXL scalar bitmap index probe from an optimizer
//		scalar bitmap index probe operator.
//
//	GPDB_12_MERGE_FIXME: whenever we modify this function, we may
//	as well consider updating PdxlnBitmapIndexProbeForChildPart().
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnBitmapIndexProbe(
	gpos::pointer<CExpression *> pexprBitmapIndexProbe)
{
	GPOS_ASSERT(nullptr != pexprBitmapIndexProbe);
	gpos::pointer<CScalarBitmapIndexProbe *> pop =
		gpos::dyn_cast<CScalarBitmapIndexProbe>(pexprBitmapIndexProbe->Pop());

	// create index descriptor
	gpos::pointer<CIndexDescriptor *> pindexdesc = pop->Pindexdesc();
	CMDName *pmdnameIndex =
		GPOS_NEW(m_mp) CMDName(m_mp, pindexdesc->Name().Pstr());
	gpos::owner<IMDId *> pmdidIndex = pindexdesc->MDId();
	pmdidIndex->AddRef();

	gpos::owner<CDXLIndexDescr *> dxl_index_descr =
		GPOS_NEW(m_mp) CDXLIndexDescr(pmdidIndex, pmdnameIndex);
	gpos::owner<CDXLScalarBitmapIndexProbe *> dxl_op =
		GPOS_NEW(m_mp) CDXLScalarBitmapIndexProbe(m_mp, dxl_index_descr);
	gpos::owner<CDXLNode *> pdxlnBitmapIndexProbe =
		GPOS_NEW(m_mp) CDXLNode(m_mp, dxl_op);

	// translate index predicates
	gpos::pointer<CExpression *> pexprCond = (*pexprBitmapIndexProbe)[0];
	gpos::owner<CDXLNode *> pdxlnIndexCondList = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarIndexCondList(m_mp));
	gpos::owner<CExpressionArray *> pdrgpexprConds =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprCond);
	const ULONG length = pdrgpexprConds->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::pointer<CExpression *> pexprIndexCond = (*pdrgpexprConds)[ul];
		gpos::owner<CDXLNode *> pdxlnIndexCond = PdxlnScalar(pexprIndexCond);
		pdxlnIndexCondList->AddChild(pdxlnIndexCond);
	}
	pdrgpexprConds->Release();
	pdxlnBitmapIndexProbe->AddChild(std::move(pdxlnIndexCondList));

#ifdef GPOS_DEBUG
	pdxlnBitmapIndexProbe->GetOperator()->AssertValid(
		pdxlnBitmapIndexProbe, false /*validate_children*/);
#endif

	return pdxlnBitmapIndexProbe;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnBitmapBoolOp
//
//	@doc:
//		Create a DXL scalar bitmap boolean operator from an optimizer
//		scalar bitmap boolean operator operator.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnBitmapBoolOp(
	gpos::pointer<CExpression *> pexprBitmapBoolOp)
{
	GPOS_ASSERT(nullptr != pexprBitmapBoolOp);
	GPOS_ASSERT(2 == pexprBitmapBoolOp->Arity());

	gpos::pointer<CScalarBitmapBoolOp *> popBitmapBoolOp =
		gpos::dyn_cast<CScalarBitmapBoolOp>(pexprBitmapBoolOp->Pop());
	gpos::pointer<CExpression *> pexprLeft = (*pexprBitmapBoolOp)[0];
	gpos::pointer<CExpression *> pexprRight = (*pexprBitmapBoolOp)[1];

	gpos::owner<CDXLNode *> dxlnode_left = PdxlnScalar(pexprLeft);
	gpos::owner<CDXLNode *> dxlnode_right = PdxlnScalar(pexprRight);

	gpos::owner<IMDId *> mdid_type = popBitmapBoolOp->MdidType();
	mdid_type->AddRef();

	CDXLScalarBitmapBoolOp::EdxlBitmapBoolOp edxlbitmapop =
		CDXLScalarBitmapBoolOp::EdxlbitmapAnd;

	if (CScalarBitmapBoolOp::EbitmapboolOr == popBitmapBoolOp->Ebitmapboolop())
	{
		edxlbitmapop = CDXLScalarBitmapBoolOp::EdxlbitmapOr;
	}

	return GPOS_NEW(m_mp) CDXLNode(
		m_mp,
		GPOS_NEW(m_mp)
			CDXLScalarBitmapBoolOp(m_mp, std::move(mdid_type), edxlbitmapop),
		std::move(dxlnode_left), std::move(dxlnode_right));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnBitmapTableScan
//
//	@doc:
//		Create a DXL physical bitmap table scan from an optimizer
//		physical bitmap table scan operator.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnBitmapTableScan(
	CExpression *pexprBitmapTableScan,
	gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
	ULONG *,  // pulNonGatherMotions,
	BOOL *	  // pfDML
)
{
	return PdxlnBitmapTableScan(pexprBitmapTableScan,
								nullptr,  // pcrsOutput
								colref_array, pdrgpdsBaseTables,
								nullptr,  // pexprScalar
								nullptr	  // dxl_properties
	);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::AddBitmapFilterColumns
//
//	@doc:
//		Add used columns in the bitmap recheck and the remaining scalar filter
//		condition to the required output column
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::AddBitmapFilterColumns(
	CMemoryPool *mp, gpos::pointer<CPhysicalScan *> pop,
	gpos::pointer<CExpression *> pexprRecheckCond,
	gpos::pointer<CExpression *> pexprScalar,
	gpos::pointer<CColRefSet *>
		pcrsReqdOutput	// append the required column reference
)
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(COperator::EopPhysicalDynamicBitmapTableScan == pop->Eopid() ||
				COperator::EopPhysicalBitmapTableScan == pop->Eopid());
	GPOS_ASSERT(nullptr != pcrsReqdOutput);

	// compute what additional columns are required in the output of the (Dynamic) Bitmap Table Scan
	gpos::owner<CColRefSet *> pcrsAdditional = GPOS_NEW(mp) CColRefSet(mp);

	if (nullptr != pexprRecheckCond)
	{
		// add the columns used in the recheck condition
		pcrsAdditional->Include(pexprRecheckCond->DeriveUsedColumns());
	}

	if (nullptr != pexprScalar)
	{
		// add the columns used in the filter condition
		pcrsAdditional->Include(pexprScalar->DeriveUsedColumns());
	}

	gpos::owner<CColRefSet *> pcrsBitmap = GPOS_NEW(mp) CColRefSet(mp);
	pcrsBitmap->Include(pop->PdrgpcrOutput());

	// only keep the columns that are in the table associated with the bitmap
	pcrsAdditional->Intersection(pcrsBitmap);

	if (0 < pcrsAdditional->Size())
	{
		pcrsReqdOutput->Include(pcrsAdditional);
	}

	// clean up
	pcrsAdditional->Release();
	pcrsBitmap->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnBitmapTableScan
//
//	@doc:
//		Create a DXL physical bitmap table scan from an optimizer
//		physical bitmap table scan operator.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnBitmapTableScan(
	CExpression *pexprBitmapTableScan, gpos::pointer<CColRefSet *> pcrsOutput,
	gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
	gpos::pointer<CExpression *> pexprScalar,
	gpos::owner<CDXLPhysicalProperties *> dxl_properties)
{
	GPOS_ASSERT(nullptr != pexprBitmapTableScan);
	gpos::pointer<CPhysicalBitmapTableScan *> pop =
		gpos::dyn_cast<CPhysicalBitmapTableScan>(pexprBitmapTableScan->Pop());

	COptCtxt::PoctxtFromTLS()->AddDirectDispatchableFilterCandidate(
		pexprBitmapTableScan);

	// translate table descriptor
	gpos::owner<CDXLTableDescr *> table_descr = MakeDXLTableDescr(
		pop->Ptabdesc(), pop->PdrgpcrOutput(), pexprBitmapTableScan->Prpp());

	gpos::owner<CDXLPhysicalBitmapTableScan *> dxl_op =
		GPOS_NEW(m_mp) CDXLPhysicalBitmapTableScan(m_mp, table_descr);
	gpos::owner<CDXLNode *> pdxlnBitmapTableScan =
		GPOS_NEW(m_mp) CDXLNode(m_mp, dxl_op);

	// set properties
	// construct plan costs, if there are not passed as a parameter
	if (nullptr == dxl_properties)
	{
		dxl_properties = GetProperties(pexprBitmapTableScan);
	}
	pdxlnBitmapTableScan->SetProperties(dxl_properties);

	// build projection list
	if (nullptr == pcrsOutput)
	{
		pcrsOutput = pexprBitmapTableScan->Prpp()->PcrsRequired();
	}

	// translate scalar predicate into DXL filter only if it is not redundant
	gpos::pointer<CExpression *> pexprRecheckCond = (*pexprBitmapTableScan)[0];
	gpos::owner<CDXLNode *> pdxlnCond = nullptr;
	if (nullptr != pexprScalar && !CUtils::FScalarConstTrue(pexprScalar) &&
		!pexprScalar->Matches(pexprRecheckCond))
	{
		pdxlnCond = PdxlnScalar(pexprScalar);
	}

	gpos::owner<CDXLNode *> filter_dxlnode = PdxlnFilter(std::move(pdxlnCond));

	gpos::owner<CDXLNode *> pdxlnRecheckCond = PdxlnScalar(pexprRecheckCond);
	gpos::owner<CDXLNode *> pdxlnRecheckCondFilter = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarRecheckCondFilter(m_mp),
				 std::move(pdxlnRecheckCond));

	AddBitmapFilterColumns(m_mp, pop, pexprRecheckCond, pexprScalar,
						   pcrsOutput);

	gpos::owner<CDXLNode *> proj_list_dxlnode =
		PdxlnProjList(pcrsOutput, colref_array);

	// translate bitmap access path
	gpos::owner<CDXLNode *> pdxlnBitmapIndexPath =
		PdxlnScalar((*pexprBitmapTableScan)[1]);

	pdxlnBitmapTableScan->AddChild(std::move(proj_list_dxlnode));
	pdxlnBitmapTableScan->AddChild(std::move(filter_dxlnode));
	pdxlnBitmapTableScan->AddChild(std::move(pdxlnRecheckCondFilter));
	pdxlnBitmapTableScan->AddChild(std::move(pdxlnBitmapIndexPath));
#ifdef GPOS_DEBUG
	pdxlnBitmapTableScan->GetOperator()->AssertValid(
		pdxlnBitmapTableScan, false /*validate_children*/);
#endif

	gpos::owner<CDistributionSpec *> pds =
		pexprBitmapTableScan->GetDrvdPropPlan()->Pds();
	pds->AddRef();
	pdrgpdsBaseTables->Append(std::move(pds));

	return pdxlnBitmapTableScan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDynamicTableScan
//
//	@doc:
//		Create a DXL dynamic table scan node from an optimizer
//		dynamic table scan node.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnDynamicTableScan(
	gpos::pointer<CExpression *> pexprDTS,
	gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
	ULONG *,  // pulNonGatherMotions,
	BOOL *	  // pfDML
)
{
	gpos::pointer<CExpression *> pexprScalarCond = nullptr;
	gpos::owner<CDXLPhysicalProperties *> dxl_properties = nullptr;
	return PdxlnDynamicTableScan(pexprDTS, colref_array, pdrgpdsBaseTables,
								 pexprScalarCond, std::move(dxl_properties));
}

// Construct a dxl table descr for a child partition
gpos::owner<CTableDescriptor *>
CTranslatorExprToDXL::MakeTableDescForPart(
	gpos::pointer<const IMDRelation *> part,
	gpos::pointer<CTableDescriptor *> root_table_desc)
{
	gpos::owner<IMDId *> part_mdid = part->MDId();
	part_mdid->AddRef();

	gpos::owner<CTableDescriptor *> table_descr =
		GPOS_NEW(m_mp) CTableDescriptor(
			m_mp, part_mdid, part->Mdname().GetMDName(),
			part->ConvertHashToRandom(), part->GetRelDistribution(),
			part->RetrieveRelStorageType(),
			root_table_desc->GetExecuteAsUserId(), root_table_desc->LockMode());

	for (ULONG ul = 0; ul < part->ColumnCount(); ++ul)
	{
		gpos::pointer<const IMDColumn *> mdCol = part->GetMdCol(ul);
		if (mdCol->IsDropped())
		{
			continue;
		}
		CWStringConst strColName{m_mp,
								 mdCol->Mdname().GetMDName()->GetBuffer()};
		CName colname(m_mp, &strColName);
		gpos::owner<CColumnDescriptor *> coldesc = GPOS_NEW(m_mp)
			CColumnDescriptor(m_mp, m_pmda->RetrieveType(mdCol->MdidType()),
							  mdCol->TypeModifier(), colname, mdCol->AttrNum(),
							  mdCol->IsNullable(), mdCol->Length());
		table_descr->AddColumn(coldesc);
	}

	return table_descr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnIndexDescForPart
//
//	@doc:
//		Construct a dxl index descriptor for a child partition
//
//---------------------------------------------------------------------------
gpos::owner<CDXLIndexDescr *>
CTranslatorExprToDXL::PdxlnIndexDescForPart(
	CMemoryPool *m_mp, gpos::pointer<MdidHashSet *> child_index_mdids_set,
	gpos::pointer<const IMDRelation *> part, const CWStringConst *index_name)
{
	// iterate over each index in the child to find the matching index
	IMDId *found_index = nullptr;
	for (ULONG j = 0; j < part->IndexCount(); ++j)
	{
		IMDId *pmdidPartIndex = part->IndexMDidAt(j);
		if (child_index_mdids_set->Contains(pmdidPartIndex))
		{
			found_index = pmdidPartIndex;
			break;
		}
	}
	GPOS_ASSERT(nullptr != found_index);
	found_index->AddRef();

	// create index descriptor (this name is the parent name, but isn't displayed in the plan)
	CMDName *pmdnameIndex = GPOS_NEW(m_mp) CMDName(m_mp, index_name);
	gpos::owner<CDXLIndexDescr *> dxl_index_descr =
		GPOS_NEW(m_mp) CDXLIndexDescr(found_index, pmdnameIndex);
	return dxl_index_descr;
}

// Translate CPhysicalDynamicTableScan node. It creates a CDXLPhysicalAppend
// node over a number of CDXLPhysicalTableScan nodes - one for each unpruned
// child partition of the root partition in CPhysicalDynamicTableScan.
//
// To handle dropped and re-ordered columns, the project list and any filter
// expression from the root table are modified using the per partition mappings
// for each child CDXLPhysicalTableScan
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnDynamicTableScan(
	gpos::pointer<CExpression *> pexprDTS,
	gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
	gpos::pointer<CExpression *> pexprScalarCond,
	gpos::owner<CDXLPhysicalProperties *> dxl_properties)
{
	GPOS_ASSERT(nullptr != pexprDTS);
	GPOS_ASSERT_IFF(nullptr != pexprScalarCond, nullptr != dxl_properties);

	gpos::pointer<CPhysicalDynamicTableScan *> popDTS =
		gpos::dyn_cast<CPhysicalDynamicTableScan>(pexprDTS->Pop());

	gpos::owner<ULongPtrArray *> selector_ids =
		GPOS_NEW(m_mp) ULongPtrArray(m_mp);
	gpos::pointer<CPartitionPropagationSpec *> pps_reqd =
		pexprDTS->Prpp()->Pepp()->PppsRequired();
	if (pps_reqd->Contains(popDTS->ScanId()))
	{
		gpos::pointer<const CBitSet *> bs =
			pps_reqd->SelectorIds(popDTS->ScanId());
		CBitSetIter bsi(*bs);
		for (ULONG ul = 0; bsi.Advance(); ul++)
		{
			selector_ids->Append(GPOS_NEW(m_mp) ULONG(bsi.Bit()));
		}
	}

	// construct plan costs
	gpos::owner<CDXLPhysicalProperties *> pdxlpropDTS = GetProperties(pexprDTS);

	if (nullptr != dxl_properties)
	{
		CWStringDynamic *rows_out_str = GPOS_NEW(m_mp) CWStringDynamic(
			m_mp,
			dxl_properties->GetDXLOperatorCost()->GetRowsOutStr()->GetBuffer());
		CWStringDynamic *pstrCost = GPOS_NEW(m_mp)
			CWStringDynamic(m_mp, dxl_properties->GetDXLOperatorCost()
									  ->GetTotalCostStr()
									  ->GetBuffer());

		pdxlpropDTS->GetDXLOperatorCost()->SetRows(rows_out_str);
		pdxlpropDTS->GetDXLOperatorCost()->SetCost(pstrCost);
		dxl_properties->Release();
	}
	GPOS_ASSERT(nullptr != pexprDTS->Prpp());

	// construct projection list for top-level Append node
	gpos::pointer<CColRefSet *> pcrsOutput = pexprDTS->Prpp()->PcrsRequired();
	gpos::owner<CDXLNode *> pdxlnPrLAppend =
		PdxlnProjList(pcrsOutput, colref_array);
	gpos::owner<CDXLTableDescr *> root_dxl_table_descr = MakeDXLTableDescr(
		popDTS->Ptabdesc(), popDTS->PdrgpcrOutput(), pexprDTS->Prpp());

	// Construct the Append node - even when there is only one child partition.
	// This is done for two reasons:
	// * Dynamic partition pruning
	//   Even if one partition is present in the statically pruned plan, we could
	//   still dynamically prune it away. This needs an Append node.
	// * Col mappings issues
	//   When the first selected child partition's cols have different types/order
	//   than the root partition, we can no longer re-use the colrefs of the root
	//   partition, since colrefs are immutable. Thus, we create new colrefs for
	//   this partition. But, if there is no Append (in case of just one selected
	//   partition), then we also go through update all references above the DTS
	//   with the new colrefs. For simplicity, we decided to keep the Append
	//   around to maintain this projection (mapping) from the old root colrefs
	//   to the first selected partition colrefs.
	//
	// GPDB_12_MERGE_FIXME: An Append on a single TableScan can be removed in
	// CTranslatorDXLToPlstmt since these points do not apply there.
	gpos::owner<CDXLNode *> pdxlnAppend = GPOS_NEW(m_mp) CDXLNode(
		m_mp,
		GPOS_NEW(m_mp) CDXLPhysicalAppend(m_mp, false, false, popDTS->ScanId(),
										  root_dxl_table_descr, selector_ids));
	pdxlnAppend->SetProperties(pdxlpropDTS);
	pdxlnAppend->AddChild(pdxlnPrLAppend);
	pdxlnAppend->AddChild(PdxlnFilter(nullptr));

	gpos::pointer<IMdIdArray *> part_mdids = popDTS->GetPartitionMdids();
	for (ULONG ul = 0; ul < part_mdids->Size(); ++ul)
	{
		gpos::pointer<IMDId *> part_mdid = (*part_mdids)[ul];
		gpos::pointer<const IMDRelation *> part =
			m_pmda->RetrieveRel(part_mdid);

		gpos::owner<CTableDescriptor *> part_tabdesc =
			MakeTableDescForPart(part, popDTS->Ptabdesc());

		// Create new colrefs for the child partition. The ColRefs from root
		// DTS, which may be used in any parent node, can no longer be exported
		// by a child of the Append node. Thus it is exported by the Append
		// node itself, and new colrefs are created here.
		gpos::owner<CColRefArray *> part_colrefs =
			GPOS_NEW(m_mp) CColRefArray(m_mp);
		for (ULONG ul_col = 0; ul_col < part_tabdesc->ColumnCount(); ++ul_col)
		{
			gpos::pointer<const CColumnDescriptor *> cd =
				part_tabdesc->Pcoldesc(ul_col);
			CColRef *cr = m_pcf->PcrCreate(cd->RetrieveType(),
										   cd->TypeModifier(), cd->Name());
			part_colrefs->Append(cr);
		}

		gpos::owner<CDXLTableDescr *> dxl_table_descr =
			MakeDXLTableDescr(part_tabdesc, part_colrefs, pexprDTS->Prpp());
		part_tabdesc->Release();

		gpos::owner<CDXLNode *> dxlnode = GPOS_NEW(m_mp) CDXLNode(
			m_mp, GPOS_NEW(m_mp) CDXLPhysicalTableScan(m_mp, dxl_table_descr));

		// GPDB_12_MERGE_FIXME: Compute stats & properties per scan
		pdxlpropDTS->AddRef();
		dxlnode->SetProperties(pdxlpropDTS);

		// ColRef -> index in child table desc (per partition)
		gpos::pointer<const ColRefToUlongMap *> root_col_mapping =
			(*popDTS->GetRootColMappingPerPart())[ul];

		// construct projection list, re-ordered to match root DTS
		gpos::owner<CDXLNode *> pdxlnPrL = PdxlnProjListForChildPart(
			root_col_mapping, part_colrefs, pcrsOutput, colref_array);
		dxlnode->AddChild(pdxlnPrL);  // project list

		// construct the filter
		gpos::owner<CDXLNode *> filter_dxlnode = PdxlnFilter(
			PdxlnCondForChildPart(root_col_mapping, part_colrefs,
								  popDTS->PdrgpcrOutput(), pexprScalarCond));
		dxlnode->AddChild(filter_dxlnode);	// filter

		// add to the other scans under the created Append node
		pdxlnAppend->AddChild(dxlnode);

		// cleanup
		part_colrefs->Release();
	}

	gpos::owner<CDistributionSpec *> pds = pexprDTS->GetDrvdPropPlan()->Pds();
	pds->AddRef();
	pdrgpdsBaseTables->Append(std::move(pds));

	GPOS_ASSERT(pdxlnAppend);
	return pdxlnAppend;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDynamicBitmapTableScan
//
//	@doc:
//		Create a DXL dynamic bitmap table scan node from an optimizer
//		dynamic bitmap table scan node.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnDynamicBitmapTableScan(
	CExpression *pexprScan, gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
	ULONG *,  // pulNonGatherMotions,
	BOOL *	  // pfDML
)
{
	gpos::pointer<CExpression *> pexprScalar = nullptr;
	gpos::owner<CDXLPhysicalProperties *> dxl_properties = nullptr;
	return PdxlnDynamicBitmapTableScan(pexprScan, colref_array,
									   pdrgpdsBaseTables, pexprScalar,
									   std::move(dxl_properties));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDynamicBitmapTableScan
//
//	@doc:
//		Create a DXL dynamic bitmap table scan node from an optimizer
//		dynamic bitmap table scan node.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnDynamicBitmapTableScan(
	CExpression *pexprScan, gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
	gpos::pointer<CExpression *> pexprScalar,
	gpos::owner<CDXLPhysicalProperties *> dxl_properties)
{
	GPOS_ASSERT(nullptr != pexprScan);
	gpos::pointer<CPhysicalDynamicBitmapTableScan *> pop =
		gpos::dyn_cast<CPhysicalDynamicBitmapTableScan>(pexprScan->Pop());

	// construct projection list
	gpos::pointer<CColRefSet *> pcrsOutput = pexprScan->Prpp()->PcrsRequired();
	gpos::owner<CDXLNode *> pdxlnPrLAppend =
		PdxlnProjList(pcrsOutput, colref_array);

	gpos::owner<CDXLNode *> pdxlnResult = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLPhysicalAppend(m_mp, false, false));
	// GPDB_12_MERGE_FIXME: set plan costs
	pdxlnResult->SetProperties(dxl_properties);
	pdxlnResult->AddChild(pdxlnPrLAppend);
	pdxlnResult->AddChild(PdxlnFilter(nullptr));

	gpos::pointer<IMdIdArray *> part_mdids = pop->GetPartitionMdids();

	COptCtxt::PoctxtFromTLS()->AddDirectDispatchableFilterCandidate(pexprScan);

	for (ULONG ul = 0; ul < part_mdids->Size(); ++ul)
	{
		gpos::pointer<IMDId *> part_mdid = (*part_mdids)[ul];
		gpos::pointer<const IMDRelation *> part =
			m_pmda->RetrieveRel(part_mdid);

		// translate table descriptor
		gpos::owner<CTableDescriptor *> part_tabdesc =
			MakeTableDescForPart(part, pexprScan->DeriveTableDescriptor());

		// Create new colrefs for the child partition. The ColRefs from root
		// DTS, which may be used in any parent node, can no longer be exported
		// by a child of the Append node. Thus it is exported by the Append
		// node itself, and new colrefs are created here.
		gpos::owner<CColRefArray *> part_colrefs =
			GPOS_NEW(m_mp) CColRefArray(m_mp);
		for (ULONG ul = 0; ul < part_tabdesc->ColumnCount(); ++ul)
		{
			gpos::pointer<const CColumnDescriptor *> cd =
				part_tabdesc->Pcoldesc(ul);
			CColRef *cr = m_pcf->PcrCreate(cd->RetrieveType(),
										   cd->TypeModifier(), cd->Name());
			part_colrefs->Append(cr);
		}

		gpos::owner<CDXLTableDescr *> dxl_table_descr =
			MakeDXLTableDescr(part_tabdesc, part_colrefs, pexprScan->Prpp());
		part_tabdesc->Release();

		gpos::owner<CDXLNode *> pdxlnBitmapTableScan = GPOS_NEW(m_mp) CDXLNode(
			m_mp,
			GPOS_NEW(m_mp) CDXLPhysicalBitmapTableScan(m_mp, dxl_table_descr));

		// set properties
		// construct plan costs, if there are not passed as a parameter
		if (nullptr == dxl_properties)
		{
			dxl_properties = GetProperties(pexprScan);
		}
		dxl_properties->AddRef();
		pdxlnBitmapTableScan->SetProperties(dxl_properties);

		// build projection list
		gpos::pointer<const ColRefToUlongMap *> root_col_mapping =
			(*pop->GetRootColMappingPerPart())[ul];

		// translate scalar predicate into DXL filter only if it is not redundant
		gpos::pointer<CExpression *> pexprRecheckCond = (*pexprScan)[0];
		gpos::owner<CDXLNode *> pdxlnCond = nullptr;
		if (nullptr != pexprScalar && !CUtils::FScalarConstTrue(pexprScalar) &&
			!pexprScalar->Matches(pexprRecheckCond))
		{
			pdxlnCond =
				PdxlnCondForChildPart(root_col_mapping, part_colrefs,
									  pop->PdrgpcrOutput(), pexprScalar);
		}

		gpos::owner<CDXLNode *> filter_dxlnode = PdxlnFilter(pdxlnCond);

		gpos::owner<CDXLNode *> pdxlnRecheckCond =
			PdxlnCondForChildPart(root_col_mapping, part_colrefs,
								  pop->PdrgpcrOutput(), pexprRecheckCond);
		gpos::owner<CDXLNode *> pdxlnRecheckCondFilter = GPOS_NEW(m_mp)
			CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarRecheckCondFilter(m_mp),
					 pdxlnRecheckCond);

		AddBitmapFilterColumns(m_mp, pop, pexprRecheckCond, pexprScalar,
							   pcrsOutput);

		gpos::owner<CDXLNode *> proj_list_dxlnode = PdxlnProjListForChildPart(
			root_col_mapping, part_colrefs, pcrsOutput, colref_array);

		// translate bitmap access path
		gpos::pointer<CExpression *> pexprBitmapIndexPath = (*pexprScan)[1];

		gpos::owner<CDXLNode *> pdxlnBitmapIndexPath =
			PdxlnBitmapIndexPathForChildPart(root_col_mapping, part_colrefs,
											 pop->PdrgpcrOutput(), part,
											 pexprBitmapIndexPath);

		pdxlnBitmapTableScan->AddChild(proj_list_dxlnode);
		pdxlnBitmapTableScan->AddChild(filter_dxlnode);
		pdxlnBitmapTableScan->AddChild(pdxlnRecheckCondFilter);
		pdxlnBitmapTableScan->AddChild(pdxlnBitmapIndexPath);
#ifdef GPOS_DEBUG
		pdxlnBitmapTableScan->GetOperator()->AssertValid(
			pdxlnBitmapTableScan, false /*validate_children*/);
#endif

		pdxlnResult->AddChild(pdxlnBitmapTableScan);

		// cleanup
		part_colrefs->Release();
	}

	gpos::owner<CDistributionSpec *> pds = pexprScan->GetDrvdPropPlan()->Pds();
	pds->AddRef();
	pdrgpdsBaseTables->Append(std::move(pds));
	return pdxlnResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDynamicIndexScan
//
//	@doc:
//		Create a DXL dynamic index scan node from an optimizer
//		dynamic index scan node based on passed properties
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnDynamicIndexScan(
	gpos::pointer<CExpression *> pexprDIS,
	gpos::pointer<CColRefArray *> colref_array,
	gpos::owner<CDXLPhysicalProperties *> dxl_properties,
	gpos::pointer<CReqdPropPlan *> prpp)
{
	GPOS_ASSERT(nullptr != pexprDIS);
	GPOS_ASSERT(nullptr != dxl_properties);
	GPOS_ASSERT(nullptr != prpp);

	gpos::pointer<CPhysicalDynamicIndexScan *> popDIS =
		gpos::dyn_cast<CPhysicalDynamicIndexScan>(pexprDIS->Pop());

	// construct projection list
	gpos::pointer<CColRefSet *> pcrsOutput = prpp->PcrsRequired();
	gpos::owner<CDXLNode *> pdxlnPrLAppend =
		PdxlnProjList(pcrsOutput, colref_array);

	gpos::pointer<IMdIdArray *> part_mdids = popDIS->GetPartitionMdids();

	gpos::owner<CDXLNode *> pdxlnResult = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLPhysicalAppend(m_mp, false, false));
	// GPDB_12_MERGE_FIXME: set plan costs
	pdxlnResult->SetProperties(dxl_properties);
	pdxlnResult->AddChild(pdxlnPrLAppend);
	pdxlnResult->AddChild(PdxlnFilter(nullptr));

	// construct set of child indexes from parent list of child indexes
	gpos::pointer<IMDId *> pmdidIndex = popDIS->Pindexdesc()->MDId();
	gpos::pointer<const IMDIndex *> md_index =
		m_pmda->RetrieveIndex(pmdidIndex);
	gpos::pointer<IMdIdArray *> child_indexes = md_index->ChildIndexMdids();

	gpos::owner<MdidHashSet *> child_index_mdids_set =
		GPOS_NEW(m_mp) MdidHashSet(m_mp);
	for (ULONG ul = 0; ul < child_indexes->Size(); ul++)
	{
		(*child_indexes)[ul]->AddRef();
		child_index_mdids_set->Insert((*child_indexes)[ul]);
	}

	for (ULONG ul = 0; ul < part_mdids->Size(); ++ul)
	{
		gpos::pointer<IMDId *> part_mdid = (*part_mdids)[ul];
		gpos::pointer<const IMDRelation *> part =
			m_pmda->RetrieveRel(part_mdid);

		gpos::pointer<CPhysicalDynamicIndexScan *> popDIS =
			gpos::dyn_cast<CPhysicalDynamicIndexScan>(pexprDIS->Pop());

		// GPDB_12_MERGE_FIXME: ideally MakeTableDescForPart(part, pexprDIS->DeriveTableDescriptor());
		// should just work, at this point all properties should've been derived, but we
		// haven't derived the table descriptor.
		gpos::owner<CTableDescriptor *> part_tabdesc =
			MakeTableDescForPart(part, popDIS->Ptabdesc());

		// create new colrefs for every child partition
		gpos::owner<CColRefArray *> part_colrefs =
			GPOS_NEW(m_mp) CColRefArray(m_mp);
		for (ULONG ul = 0; ul < part_tabdesc->ColumnCount(); ++ul)
		{
			gpos::pointer<const CColumnDescriptor *> cd =
				part_tabdesc->Pcoldesc(ul);
			CColRef *cr = m_pcf->PcrCreate(cd->RetrieveType(),
										   cd->TypeModifier(), cd->Name());
			part_colrefs->Append(cr);
		}

		gpos::owner<CDXLTableDescr *> dxl_table_descr =
			MakeDXLTableDescr(part_tabdesc, part_colrefs, pexprDIS->Prpp());
		part_tabdesc->Release();

		gpos::pointer<CIndexDescriptor *> pindexdesc = popDIS->Pindexdesc();
		gpos::owner<CDXLIndexDescr *> dxl_index_descr = PdxlnIndexDescForPart(
			m_mp, child_index_mdids_set, part, pindexdesc->Name().Pstr());

		// Finally create the IndexScan DXL node
		gpos::owner<CDXLNode *> dxlnode = GPOS_NEW(m_mp) CDXLNode(
			m_mp, GPOS_NEW(m_mp) CDXLPhysicalIndexScan(
					  m_mp, dxl_table_descr, dxl_index_descr, EdxlisdForward));

		// GPDB_12_MERGE_FIXME: Compute stats & properties per scan
		dxl_properties->AddRef();
		dxlnode->SetProperties(dxl_properties);

		// construct projection list
		gpos::pointer<const ColRefToUlongMap *> root_col_mapping =
			(*popDIS->GetRootColMappingPerPart())[ul];

		gpos::owner<CDXLNode *> pdxlnPrL = PdxlnProjListForChildPart(
			root_col_mapping, part_colrefs, pcrsOutput, colref_array);
		dxlnode->AddChild(pdxlnPrL);  // project list

		gpos::owner<CDXLNode *> filter_dxlnode;
		if (2 == pexprDIS->Arity())
		{
			// translate residual predicates into the filter node
			gpos::pointer<CExpression *> pexprResidualCond = (*pexprDIS)[1];

			if (COperator::EopScalarConst != pexprResidualCond->Pop()->Eopid())
			{
				// construct the filter
				filter_dxlnode = PdxlnFilter(PdxlnCondForChildPart(
					root_col_mapping, part_colrefs, popDIS->PdrgpcrOutput(),
					pexprResidualCond));
			}
			else
			{
				filter_dxlnode = PdxlnFilter(nullptr);
			}
		}
		else
		{
			// construct an empty filter
			filter_dxlnode = PdxlnFilter(nullptr);
		}
		dxlnode->AddChild(filter_dxlnode);	// filter

		// translate index predicates
		gpos::pointer<CExpression *> pexprCond = (*pexprDIS)[0];
		gpos::owner<CDXLNode *> pdxlnIndexCondList = GPOS_NEW(m_mp)
			CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarIndexCondList(m_mp));

		gpos::owner<CExpressionArray *> pdrgpexprConds =
			CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprCond);
		const ULONG length = pdrgpexprConds->Size();
		for (ULONG ul = 0; ul < length; ul++)
		{
			gpos::pointer<CExpression *> pexprIndexCond = (*pdrgpexprConds)[ul];
			// construct the condition as a filter
			gpos::owner<CDXLNode *> pdxlnIndexCond =
				PdxlnCondForChildPart(root_col_mapping, part_colrefs,
									  popDIS->PdrgpcrOutput(), pexprIndexCond);
			pdxlnIndexCondList->AddChild(pdxlnIndexCond);
		}
		pdrgpexprConds->Release();
		dxlnode->AddChild(pdxlnIndexCondList);

		// add to the other scans under the created Append node
		pdxlnResult->AddChild(dxlnode);

		// cleanup
		part_colrefs->Release();
	}
	child_index_mdids_set->Release();

	return pdxlnResult;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDynamicIndexScan
//
//	@doc:
//		Create a DXL dynamic index scan node from an optimizer
//		dynamic index scan node.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnDynamicIndexScan(
	gpos::pointer<CExpression *> pexprDIS,
	gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions GPOS_UNUSED, BOOL *pfDML GPOS_UNUSED)
{
	GPOS_ASSERT(nullptr != pexprDIS);

	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprDIS);

	gpos::owner<CDistributionSpec *> pds = pexprDIS->GetDrvdPropPlan()->Pds();
	pds->AddRef();
	pdrgpdsBaseTables->Append(std::move(pds));
	return PdxlnDynamicIndexScan(pexprDIS, colref_array,
								 std::move(dxl_properties), pexprDIS->Prpp());
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResult
//
//	@doc:
//		Create a DXL result node over a relational expression with a DXL
//		scalar condition.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnResult(CExpression *pexprRelational,
								  CColRefArray *colref_array,
								  CDistributionSpecArray *pdrgpdsBaseTables,
								  ULONG *pulNonGatherMotions, BOOL *pfDML,
								  gpos::owner<CDXLNode *> pdxlnScalar)
{
	// extract physical properties from filter
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprRelational);

	return PdxlnResult(pexprRelational, colref_array, pdrgpdsBaseTables,
					   pulNonGatherMotions, pfDML, std::move(pdxlnScalar),
					   std::move(dxl_properties));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResult
//
//	@doc:
//		Create a DXL result node over a relational expression with a DXL
//		scalar condition using the passed DXL properties
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnResult(
	CExpression *pexprRelational, CColRefArray *colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML, gpos::owner<CDXLNode *> pdxlnScalar,
	gpos::owner<CDXLPhysicalProperties *> dxl_properties)
{
	GPOS_ASSERT(nullptr != pexprRelational);

	// translate relational child expression
	gpos::owner<CDXLNode *> pdxlnRelationalChild = CreateDXLNode(
		pexprRelational, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
		pfDML, false /*fRemap*/, false /*fRoot */);
	GPOS_ASSERT(nullptr != pexprRelational->Prpp());
	gpos::pointer<CColRefSet *> pcrsOutput =
		pexprRelational->Prpp()->PcrsRequired();

	return PdxlnAddScalarFilterOnRelationalChild(
		std::move(pdxlnRelationalChild), std::move(pdxlnScalar),
		std::move(dxl_properties), pcrsOutput, colref_array);
}

gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnAddScalarFilterOnRelationalChild(
	gpos::owner<CDXLNode *> pdxlnRelationalChild,
	gpos::owner<CDXLNode *> pdxlnScalarChild,
	gpos::owner<CDXLPhysicalProperties *> dxl_properties,
	gpos::pointer<CColRefSet *> pcrsOutput,
	gpos::pointer<CColRefArray *> pdrgpcrOrder)
{
	GPOS_ASSERT(nullptr != dxl_properties);
	// for a true condition, just translate the child
	if (CTranslatorExprToDXLUtils::FScalarConstTrue(m_pmda, pdxlnScalarChild))
	{
		pdxlnScalarChild->Release();
		dxl_properties->Release();
		return pdxlnRelationalChild;
	}
	// create a result node over outer child
	else
	{
		// wrap condition in a DXL filter node
		gpos::owner<CDXLNode *> filter_dxlnode =
			PdxlnFilter(std::move(pdxlnScalarChild));

		gpos::owner<CDXLNode *> pdxlnPrL =
			PdxlnProjList(pcrsOutput, pdrgpcrOrder);

		// create an empty one-time filter
		gpos::owner<CDXLNode *> one_time_filter = GPOS_NEW(m_mp)
			CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarOneTimeFilter(m_mp));

		return CTranslatorExprToDXLUtils::PdxlnResult(
			m_mp, std::move(dxl_properties), std::move(pdxlnPrL),
			std::move(filter_dxlnode), std::move(one_time_filter),
			std::move(pdxlnRelationalChild));
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResult
//
//	@doc:
//		Create a DXL result node from an optimizer filter node.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnResult(CExpression *pexprFilter,
								  CColRefArray *colref_array,
								  CDistributionSpecArray *pdrgpdsBaseTables,
								  ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprFilter);

	gpos::owner<CDXLNode *> pdxlnode =
		PdxlnFromFilter(pexprFilter, colref_array, pdrgpdsBaseTables,
						pulNonGatherMotions, pfDML, dxl_properties);
	dxl_properties->Release();

	return pdxlnode;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnIndexScanWithInlinedCondition
//
//	@doc:
//		Create a (dynamic) index scan node after inlining the given
//		scalar condition, if needed
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnIndexScanWithInlinedCondition(
	CExpression *pexprIndexScan, CExpression *pexprScalarCond,
	gpos::owner<CDXLPhysicalProperties *> dxl_properties,
	gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables)
{
	GPOS_ASSERT(nullptr != pexprIndexScan);
	GPOS_ASSERT(nullptr != pexprScalarCond);
	GPOS_ASSERT(pexprScalarCond->Pop()->FScalar());

	COperator::EOperatorId op_id = pexprIndexScan->Pop()->Eopid();
	GPOS_ASSERT(COperator::EopPhysicalIndexScan == op_id ||
				COperator::EopPhysicalIndexOnlyScan == op_id ||
				COperator::EopPhysicalDynamicIndexScan == op_id);

	// TODO: Index only scans work on GiST and SP-GiST only for specific operators
	// check if index is of type GiST
	BOOL isGist = false;
	if (COperator::EopPhysicalIndexScan == op_id)
	{
		gpos::pointer<CPhysicalIndexScan *> indexScan =
			gpos::dyn_cast<CPhysicalIndexScan>(pexprIndexScan->Pop());
		isGist = (indexScan->Pindexdesc()->IndexType() == IMDIndex::EmdindGist);
	}
	else if (COperator::EopPhysicalIndexOnlyScan != op_id)
	{
		gpos::pointer<CPhysicalDynamicIndexScan *> indexScan =
			gpos::dyn_cast<CPhysicalDynamicIndexScan>(pexprIndexScan->Pop());
		isGist = (indexScan->Pindexdesc()->IndexType() == IMDIndex::EmdindGist);
	}

	// inline scalar condition in index scan, if it is not the same as index lookup condition
	// Exception: most GiST indexes require a recheck condition since they are lossy: re-add the lookup
	// condition as a scalar condition. For now, all GiST indexes are treated as lossy
	CExpression *pexprIndexLookupCond = (*pexprIndexScan)[0];
	gpos::owner<CDXLNode *> pdxlnIndexScan = nullptr;
	if ((!CUtils::FScalarConstTrue(pexprScalarCond) &&
		 !pexprScalarCond->Matches(pexprIndexLookupCond)) ||
		isGist)
	{
		// combine scalar condition with existing index conditions, if any
		pexprScalarCond->AddRef();
		gpos::owner<CExpression *> pexprNewScalarCond = pexprScalarCond;
		if (2 == pexprIndexScan->Arity())
		{
			pexprNewScalarCond->Release();
			pexprNewScalarCond = CPredicateUtils::PexprConjunction(
				m_mp, (*pexprIndexScan)[1], pexprScalarCond);
		}
		pexprIndexLookupCond->AddRef();
		pexprIndexScan->Pop()->AddRef();
		gpos::owner<CExpression *> pexprNewIndexScan = GPOS_NEW(m_mp)
			CExpression(m_mp, pexprIndexScan->Pop(), pexprIndexLookupCond,
						pexprNewScalarCond);
		if (COperator::EopPhysicalIndexScan == op_id)
		{
			pdxlnIndexScan =
				PdxlnIndexScan(pexprNewIndexScan, colref_array, dxl_properties,
							   pexprIndexScan->Prpp());
		}
		else if (COperator::EopPhysicalIndexOnlyScan == op_id)
		{
			pdxlnIndexScan =
				PdxlnIndexOnlyScan(pexprNewIndexScan, colref_array,
								   dxl_properties, pexprIndexScan->Prpp());
		}
		else
		{
			pdxlnIndexScan =
				PdxlnDynamicIndexScan(pexprNewIndexScan, colref_array,
									  dxl_properties, pexprIndexScan->Prpp());
		}
		pexprNewIndexScan->Release();

		gpos::owner<CDistributionSpec *> pds =
			pexprIndexScan->GetDrvdPropPlan()->Pds();
		pds->AddRef();
		pdrgpdsBaseTables->Append(std::move(pds));

		return pdxlnIndexScan;
	}

	// index scan does not need the properties of the filter, as it does not
	// need to further inline the scalar condition
	dxl_properties->Release();
	ULONG ulNonGatherMotions = 0;
	BOOL fDML = false;
	if (COperator::EopPhysicalIndexScan == op_id)
	{
		return PdxlnIndexScan(pexprIndexScan, colref_array, pdrgpdsBaseTables,
							  &ulNonGatherMotions, &fDML);
	}
	if (COperator::EopPhysicalIndexOnlyScan == op_id)
	{
		return PdxlnIndexOnlyScan(pexprIndexScan, colref_array,
								  pdrgpdsBaseTables, &ulNonGatherMotions,
								  &fDML);
	}

	return PdxlnDynamicIndexScan(pexprIndexScan, colref_array,
								 pdrgpdsBaseTables, &ulNonGatherMotions, &fDML);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResult
//
//	@doc:
//		Create a DXL result node from an optimizer filter node.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnFromFilter(CExpression *pexprFilter,
									  CColRefArray *colref_array,
									  CDistributionSpecArray *pdrgpdsBaseTables,
									  ULONG *pulNonGatherMotions, BOOL *pfDML,
									  CDXLPhysicalProperties *dxl_properties)
{
	GPOS_ASSERT(nullptr != pexprFilter);
	GPOS_ASSERT(nullptr != dxl_properties);

	// extract components
	CExpression *pexprRelational = (*pexprFilter)[0];
	CExpression *pexprScalar = (*pexprFilter)[1];

	if (CTranslatorExprToDXLUtils::FDirectDispatchableFilter(pexprFilter))

	{
		COptCtxt::PoctxtFromTLS()->AddDirectDispatchableFilterCandidate(
			pexprFilter);
	}

	// if the filter predicate is a constant TRUE, skip to translating relational child
	if (CUtils::FScalarConstTrue(pexprScalar))
	{
		return CreateDXLNode(pexprRelational, colref_array, pdrgpdsBaseTables,
							 pulNonGatherMotions, pfDML, true /*fRemap*/,
							 false /* fRoot */);
	}

	COperator::EOperatorId eopidRelational = pexprRelational->Pop()->Eopid();
	gpos::pointer<CColRefSet *> pcrsOutput =
		pexprFilter->Prpp()->PcrsRequired();

	switch (eopidRelational)
	{
		case COperator::EopPhysicalTableScan:
		case COperator::EopPhysicalExternalScan:
		{
			// if there is a structure of the form
			// 		filter->tablescan, or filter->CTG then
			// push the scalar filter expression to the tablescan/CTG respectively
			dxl_properties->AddRef();

			// translate the table scan with the filter condition
			return PdxlnTblScan(pexprRelational, pcrsOutput,
								nullptr /* colref_array */, pdrgpdsBaseTables,
								pexprScalar, dxl_properties /* cost info */
			);
		}
		case COperator::EopPhysicalBitmapTableScan:
		{
			dxl_properties->AddRef();

			return PdxlnBitmapTableScan(
				pexprRelational, pcrsOutput, nullptr /*colref_array*/,
				pdrgpdsBaseTables, pexprScalar, dxl_properties);
		}
		case COperator::EopPhysicalDynamicTableScan:
		{
			dxl_properties->AddRef();

			// inline condition in the Dynamic Table Scan
			return PdxlnDynamicTableScan(pexprRelational, colref_array,
										 pdrgpdsBaseTables, pexprScalar,
										 dxl_properties);
		}
		case COperator::EopPhysicalIndexOnlyScan:
		case COperator::EopPhysicalIndexScan:
		case COperator::EopPhysicalDynamicIndexScan:
		{
			dxl_properties->AddRef();
			return PdxlnIndexScanWithInlinedCondition(
				pexprRelational, pexprScalar, dxl_properties, colref_array,
				pdrgpdsBaseTables);
		}
		case COperator::EopPhysicalDynamicBitmapTableScan:
		{
			dxl_properties->AddRef();

			return PdxlnDynamicBitmapTableScan(pexprRelational, colref_array,
											   pdrgpdsBaseTables, pexprScalar,
											   dxl_properties);
		}
		default:
		{
			return PdxlnResultFromFilter(pexprFilter, colref_array,
										 pdrgpdsBaseTables, pulNonGatherMotions,
										 pfDML);
		}
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResultFromFilter
//
//	@doc:
//		Create a DXL result node from an optimizer filter node.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnResultFromFilter(
	gpos::pointer<CExpression *> pexprFilter,
	gpos::pointer<CColRefArray *> colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprFilter);

	// extract components
	CExpression *pexprRelational = (*pexprFilter)[0];
	gpos::pointer<CExpression *> pexprScalar = (*pexprFilter)[1];
	gpos::pointer<CColRefSet *> pcrsOutput =
		pexprFilter->Prpp()->PcrsRequired();

	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprFilter);

	// translate relational child expression
	gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
		pexprRelational, nullptr /* colref_array */, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// translate scalar expression in to filter and one time filter dxl nodes
	gpos::pointer<CColRefSet *> relational_child_colrefset =
		pexprRelational->DeriveOutputColumns();
	// get all the scalar conditions in an array
	gpos::owner<CExpressionArray *> scalar_exprs =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprScalar);
	// array to hold scalar conditions which will qualify for filter condition
	gpos::owner<CExpressionArray *> filter_quals_exprs =
		GPOS_NEW(m_mp) CExpressionArray(m_mp);
	// array to hold scalar conditions which qualify for one time filter condition
	gpos::owner<CExpressionArray *> one_time_filter_quals_exprs =
		GPOS_NEW(m_mp) CExpressionArray(m_mp);
	for (ULONG ul = 0; ul < scalar_exprs->Size(); ul++)
	{
		CExpression *scalar_child_expr = (*scalar_exprs)[ul];
		gpos::pointer<CColRefSet *> scalar_child_colrefset =
			scalar_child_expr->DeriveUsedColumns();

		// What qualifies for a one time filter qual?
		// 1. if there is no column in the scalar child of filter expression coming from its relational
		// child
		// and
		// 2. if there is no volatile function in the scalar child
		//
		// Why quals are separated into one time filter vs filter quals?
		// one time filter quals are evaluated once for each scan, and if the filter evaluates to false,
		// the data from the nodes below is not requested, however in case of filter quals, they are
		// evaluated for each tuple coming from the nodes below. So, if the filter does not depends on the tuple
		// values coming from the nodes below, it could be a one time filter and we can save processing time on
		// each tuple and evaluating it against the filter.
		if (scalar_child_colrefset->FIntersects(relational_child_colrefset) ||
			CPredicateUtils::FContainsVolatileFunction(scalar_child_expr))
		{
			scalar_child_expr->AddRef();
			filter_quals_exprs->Append(scalar_child_expr);
		}
		else
		{
			scalar_child_expr->AddRef();
			one_time_filter_quals_exprs->Append(scalar_child_expr);
		}
	}
	scalar_exprs->Release();

	// create an emtpy filter
	gpos::owner<CDXLNode *> filter_dxlnode =
		GPOS_NEW(m_mp) CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarFilter(m_mp));
	// create an empty one-time filter
	gpos::owner<CDXLNode *> one_time_filter_dxlnode = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarOneTimeFilter(m_mp));

	if (filter_quals_exprs->Size() > 0)
	{
		// create scalar cmp expression for filter expression
		gpos::owner<CExpression *> scalar_cmp_expr =
			CPredicateUtils::PexprConjunction(m_mp, filter_quals_exprs);
		// create dxl node for the filter
		gpos::owner<CDXLNode *> scalar_cmp_dxlnode =
			PdxlnScalar(scalar_cmp_expr);
		filter_dxlnode->AddChild(scalar_cmp_dxlnode);
		scalar_cmp_expr->Release();
	}
	else
	{
		filter_quals_exprs->Release();
	}

	if (one_time_filter_quals_exprs->Size() > 0)
	{
		// create scalar cmp expression for one time filter expression
		gpos::owner<CExpression *> scalar_cmp_expr =
			CPredicateUtils::PexprConjunction(m_mp,
											  one_time_filter_quals_exprs);
		// create dxl node for one time filter
		gpos::owner<CDXLNode *> scalar_cmp_dxlnode =
			PdxlnScalar(scalar_cmp_expr);
		one_time_filter_dxlnode->AddChild(scalar_cmp_dxlnode);
		scalar_cmp_expr->Release();
	}
	else
	{
		one_time_filter_quals_exprs->Release();
	}

	GPOS_ASSERT(nullptr != pexprFilter->Prpp());

	gpos::owner<CDXLNode *> pdxlnPrL = PdxlnProjList(pcrsOutput, colref_array);

	return CTranslatorExprToDXLUtils::PdxlnResult(
		m_mp, std::move(dxl_properties), std::move(pdxlnPrL),
		std::move(filter_dxlnode), std::move(one_time_filter_dxlnode),
		std::move(child_dxlnode));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAssert
//
//	@doc:
//		Translate a physical assert expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnAssert(gpos::pointer<CExpression *> pexprAssert,
								  gpos::pointer<CColRefArray *> colref_array,
								  CDistributionSpecArray *pdrgpdsBaseTables,
								  ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprAssert);

	// extract components
	CExpression *pexprRelational = (*pexprAssert)[0];
	gpos::pointer<CExpression *> pexprScalar = (*pexprAssert)[1];
	gpos::pointer<CPhysicalAssert *> popAssert =
		gpos::dyn_cast<CPhysicalAssert>(pexprAssert->Pop());

	// extract physical properties from assert
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprAssert);

	gpos::pointer<CColRefSet *> pcrsOutput =
		pexprAssert->Prpp()->PcrsRequired();

	// translate relational child expression
	gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
		pexprRelational, nullptr /* colref_array */, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// translate scalar expression
	gpos::owner<CDXLNode *> pdxlnAssertPredicate = PdxlnScalar(pexprScalar);

	GPOS_ASSERT(nullptr != pexprAssert->Prpp());

	gpos::owner<CDXLNode *> pdxlnPrL = PdxlnProjList(pcrsOutput, colref_array);

	const CHAR *sql_state = popAssert->Pexc()->GetSQLState();
	gpos::owner<CDXLPhysicalAssert *> pdxlopAssert =
		GPOS_NEW(m_mp) CDXLPhysicalAssert(m_mp, sql_state);
	gpos::owner<CDXLNode *> pdxlnAssert = GPOS_NEW(m_mp)
		CDXLNode(m_mp, std::move(pdxlopAssert), std::move(pdxlnPrL),
				 std::move(pdxlnAssertPredicate), std::move(child_dxlnode));

	pdxlnAssert->SetProperties(std::move(dxl_properties));

	return pdxlnAssert;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnCTEProducer
//
//	@doc:
//		Translate a physical cte producer expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnCTEProducer(
	gpos::pointer<CExpression *> pexprCTEProducer,
	gpos::pointer<CColRefArray *>,	//colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprCTEProducer);

	// extract components
	CExpression *pexprRelational = (*pexprCTEProducer)[0];
	gpos::pointer<CPhysicalCTEProducer *> popCTEProducer =
		gpos::dyn_cast<CPhysicalCTEProducer>(pexprCTEProducer->Pop());

	// extract physical properties from cte producer
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprCTEProducer);

	// extract the CTE id and the array of colids
	const ULONG ulCTEId = popCTEProducer->UlCTEId();
	gpos::owner<ULongPtrArray *> colids =
		CUtils::Pdrgpul(m_mp, popCTEProducer->Pdrgpcr());

	GPOS_ASSERT(nullptr != pexprCTEProducer->Prpp());
	CColRefArray *pdrgpcrRequired = popCTEProducer->Pdrgpcr();
	gpos::owner<CColRefSet *> pcrsOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsOutput->Include(pdrgpcrRequired);

	// translate relational child expression
	gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
		pexprRelational, pdrgpcrRequired, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, true /*fRemap*/, false /*fRoot */);

	gpos::owner<CDXLNode *> pdxlnPrL =
		PdxlnProjList(pcrsOutput, pdrgpcrRequired);
	pcrsOutput->Release();

	gpos::owner<CDXLNode *> pdxlnCTEProducer = GPOS_NEW(m_mp)
		CDXLNode(m_mp,
				 GPOS_NEW(m_mp)
					 CDXLPhysicalCTEProducer(m_mp, ulCTEId, std::move(colids)),
				 std::move(pdxlnPrL), std::move(child_dxlnode));

	pdxlnCTEProducer->SetProperties(std::move(dxl_properties));

	return pdxlnCTEProducer;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnCTEConsumer
//
//	@doc:
//		Translate a physical cte consumer expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnCTEConsumer(
	gpos::pointer<CExpression *> pexprCTEConsumer,
	gpos::pointer<CColRefArray *>,			  //colref_array,
	gpos::pointer<CDistributionSpecArray *>,  // pdrgpdsBaseTables,
	ULONG *,								  // pulNonGatherMotions,
	BOOL *									  // pfDML
)
{
	GPOS_ASSERT(nullptr != pexprCTEConsumer);

	// extract components
	gpos::pointer<CPhysicalCTEConsumer *> popCTEConsumer =
		gpos::dyn_cast<CPhysicalCTEConsumer>(pexprCTEConsumer->Pop());

	// extract physical properties from cte consumer
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprCTEConsumer);

	// extract the CTE id and the array of colids
	const ULONG ulCTEId = popCTEConsumer->UlCTEId();
	gpos::pointer<CColRefArray *> colref_array = popCTEConsumer->Pdrgpcr();
	gpos::owner<ULongPtrArray *> colids = CUtils::Pdrgpul(m_mp, colref_array);

	gpos::owner<CColRefSet *> pcrsOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsOutput->Include(colref_array);

	// translate relational child expression
	gpos::owner<CDXLNode *> pdxlnPrL = PdxlnProjList(pcrsOutput, colref_array);

	gpos::owner<CDXLNode *> pdxlnCTEConsumer = GPOS_NEW(m_mp)
		CDXLNode(m_mp,
				 GPOS_NEW(m_mp)
					 CDXLPhysicalCTEConsumer(m_mp, ulCTEId, std::move(colids)),
				 std::move(pdxlnPrL));

	pcrsOutput->Release();

	pdxlnCTEConsumer->SetProperties(std::move(dxl_properties));

	return pdxlnCTEConsumer;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAppend
//
//	@doc:
//		Create a DXL Append node from an optimizer an union all node
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnAppend(
	gpos::pointer<CExpression *> pexprUnionAll,
	gpos::pointer<CColRefArray *>,	//colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprUnionAll);

	gpos::pointer<CPhysicalUnionAll *> popUnionAll =
		gpos::dyn_cast<CPhysicalUnionAll>(pexprUnionAll->Pop());
	gpos::pointer<CColRefArray *> pdrgpcrOutputAll =
		popUnionAll->PdrgpcrOutput();
	gpos::pointer<CColRefSet *> reqdCols =
		pexprUnionAll->Prpp()->PcrsRequired();

	gpos::owner<CDXLPhysicalAppend *> dxl_op =
		GPOS_NEW(m_mp) CDXLPhysicalAppend(m_mp, false, false);
	gpos::owner<CDXLNode *> pdxlnAppend = GPOS_NEW(m_mp) CDXLNode(m_mp, dxl_op);

	// compute a list of indexes of output columns that are actually required
	gpos::owner<CColRefArray *> reqd_col_array =
		GPOS_NEW(m_mp) CColRefArray(m_mp);
	ULONG num_total_cols = pdrgpcrOutputAll->Size();
	for (ULONG c = 0; c < num_total_cols; c++)
	{
		if (reqdCols->FMember((*pdrgpcrOutputAll)[c]))
		{
			reqd_col_array->Append((*pdrgpcrOutputAll)[c]);
		}
	}
	gpos::owner<ULongPtrArray *> reqd_col_positions =
		pdrgpcrOutputAll->IndexesOfSubsequence(reqd_col_array);
	gpos::owner<CColRefArray *> requiredOutput =
		pdrgpcrOutputAll->CreateReducedArray(reqd_col_positions);
	reqd_col_array->Release();

	GPOS_ASSERT(nullptr != reqd_col_positions);

	// set properties
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprUnionAll);
	pdxlnAppend->SetProperties(dxl_properties);

	// translate project list
	gpos::owner<CColRefSet *> pcrsOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsOutput->Include(reqdCols);

	// the append node does not re-order its input or output columns. The
	// re-ordering of its output columns has to be done above it (if needed)
	// via a separate result node
	gpos::owner<CDXLNode *> pdxlnPrL =
		PdxlnProjList(pcrsOutput, requiredOutput);
	pcrsOutput->Release();
	requiredOutput->Release();
	pcrsOutput = nullptr;
	requiredOutput = nullptr;

	pdxlnAppend->AddChild(pdxlnPrL);

	// scalar condition
	CDXLNode *pdxlnCond = nullptr;
	gpos::owner<CDXLNode *> filter_dxlnode = PdxlnFilter(pdxlnCond);
	pdxlnAppend->AddChild(filter_dxlnode);

	// translate children
	gpos::pointer<CColRef2dArray *> pdrgpdrgpcrInput =
		popUnionAll->PdrgpdrgpcrInput();
	GPOS_ASSERT(nullptr != pdrgpdrgpcrInput);
	const ULONG length = pexprUnionAll->Arity();
	GPOS_ASSERT(length == pdrgpdrgpcrInput->Size());
	for (ULONG ul = 0; ul < length; ul++)
	{
		// translate child
		gpos::pointer<CColRefArray *> pdrgpcrInput = (*pdrgpdrgpcrInput)[ul];
		gpos::owner<CColRefArray *> requiredInput =
			pdrgpcrInput->CreateReducedArray(reqd_col_positions);

		CExpression *pexprChild = (*pexprUnionAll)[ul];
		gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
			pexprChild, requiredInput, pdrgpdsBaseTables, pulNonGatherMotions,
			pfDML, false /*fRemap*/, false /*fRoot*/);

		// add a result node on top if necessary so the order of the input project list
		// matches the order in which the append node requires it
		gpos::owner<CDXLNode *> pdxlnChildProjected = PdxlnRemapOutputColumns(
			pexprChild, child_dxlnode,
			requiredInput /* required input columns */,
			requiredInput /* order of the input columns */
		);

		pdxlnAppend->AddChild(pdxlnChildProjected);
		requiredInput->Release();
	}
	reqd_col_positions->Release();

	return pdxlnAppend;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdrgpcrMerge
//
//	@doc:
//		Combines the ordered columns and required columns into a single list
//      with members in the ordered list inserted before the remaining columns in
//		required list. For instance, if the order list is (c, d) and
//		the required list is (a, b, c, d) then the combined list is (c, d, a, b)
//---------------------------------------------------------------------------
gpos::owner<CColRefArray *>
CTranslatorExprToDXL::PdrgpcrMerge(
	CMemoryPool *mp, gpos::pointer<CColRefArray *> pdrgpcrOrder,
	gpos::pointer<CColRefArray *> pdrgpcrRequired)
{
	gpos::owner<CColRefSet *> pcrsOutput = GPOS_NEW(mp) CColRefSet(mp);

	gpos::owner<CColRefArray *> pdrgpcrMerge = GPOS_NEW(mp) CColRefArray(mp);

	if (nullptr != pdrgpcrOrder)
	{
		const ULONG ulLenOrder = pdrgpcrOrder->Size();
		for (ULONG ul = 0; ul < ulLenOrder; ul++)
		{
			CColRef *colref = (*pdrgpcrOrder)[ul];
			pdrgpcrMerge->Append(colref);
		}
		pcrsOutput->Include(pdrgpcrMerge);
	}

	const ULONG ulLenReqd = pdrgpcrRequired->Size();
	for (ULONG ul = 0; ul < ulLenReqd; ul++)
	{
		CColRef *colref = (*pdrgpcrRequired)[ul];
		if (!pcrsOutput->FMember(colref))
		{
			pcrsOutput->Include(colref);
			pdrgpcrMerge->Append(colref);
		}
	}

	pcrsOutput->Release();

	return pdrgpcrMerge;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnRemapOutputColumns
//
//	@doc:
//		Checks if the project list of the given node matches the required
//		columns and their order. If not then a result node is created on
//		top of it
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnRemapOutputColumns(
	gpos::pointer<CExpression *> pexpr, gpos::owner<CDXLNode *> dxlnode,
	gpos::pointer<CColRefArray *> pdrgpcrRequired,
	gpos::pointer<CColRefArray *> pdrgpcrOrder)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != dxlnode);
	GPOS_ASSERT(nullptr != pdrgpcrRequired);

	// get project list
	gpos::pointer<CDXLNode *> pdxlnPrL = (*dxlnode)[0];

	gpos::owner<CColRefArray *> pdrgpcrOrderedReqdCols =
		PdrgpcrMerge(m_mp, pdrgpcrOrder, pdrgpcrRequired);

	// if the combined list is the same as proj list then no
	// further action needed. Otherwise we need result node on top
	if (CTranslatorExprToDXLUtils::FProjectListMatch(pdxlnPrL,
													 pdrgpcrOrderedReqdCols))
	{
		pdrgpcrOrderedReqdCols->Release();
		return dxlnode;
	}

	pdrgpcrOrderedReqdCols->Release();

	// output columns of new result node
	gpos::owner<CColRefSet *> pcrsOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsOutput->Include(pdrgpcrRequired);

	gpos::owner<CDXLNode *> pdxlnPrLNew =
		PdxlnProjList(pcrsOutput, pdrgpcrOrder);
	pcrsOutput->Release();

	// create a result node on top of the current dxl node with a new project list
	return PdxlnResult(GetProperties(pexpr), std::move(pdxlnPrLNew),
					   std::move(dxlnode));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnTVF
//
//	@doc:
//		Create a DXL TVF node from an optimizer TVF node
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnTVF(
	gpos::pointer<CExpression *> pexprTVF,
	gpos::pointer<CColRefArray *>,			  //colref_array,
	gpos::pointer<CDistributionSpecArray *>,  // pdrgpdsBaseTables,
	ULONG *,								  // pulNonGatherMotions,
	BOOL *									  // pfDML
)
{
	GPOS_ASSERT(nullptr != pexprTVF);

	gpos::pointer<CPhysicalTVF *> popTVF =
		gpos::dyn_cast<CPhysicalTVF>(pexprTVF->Pop());

	gpos::pointer<CColRefSet *> pcrsOutput = popTVF->DeriveOutputColumns();

	gpos::owner<IMDId *> mdid_func = popTVF->FuncMdId();
	mdid_func->AddRef();

	gpos::owner<IMDId *> mdid_return_type = popTVF->ReturnTypeMdId();
	mdid_return_type->AddRef();

	CWStringConst *pstrFunc =
		GPOS_NEW(m_mp) CWStringConst(m_mp, popTVF->Pstr()->GetBuffer());

	gpos::owner<CDXLPhysicalTVF *> dxl_op = GPOS_NEW(m_mp) CDXLPhysicalTVF(
		m_mp, std::move(mdid_func), std::move(mdid_return_type), pstrFunc);

	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprTVF);
	gpos::owner<CDXLNode *> pdxlnTVF =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(dxl_op));
	pdxlnTVF->SetProperties(std::move(dxl_properties));

	gpos::owner<CDXLNode *> pdxlnPrL =
		PdxlnProjList(pcrsOutput, nullptr /*colref_array*/);
	pdxlnTVF->AddChild(std::move(pdxlnPrL));  // project list

	TranslateScalarChildren(pexprTVF, pdxlnTVF);

	return pdxlnTVF;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResultFromConstTableGet
//
//	@doc:
//		Create a DXL result node from an optimizer const table get node
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnResultFromConstTableGet(
	gpos::pointer<CExpression *> pexprCTG,
	gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<CExpression *> pexprScalar)
{
	GPOS_ASSERT(nullptr != pexprCTG);

	gpos::pointer<CPhysicalConstTableGet *> popCTG =
		gpos::dyn_cast<CPhysicalConstTableGet>(pexprCTG->Pop());

	// construct project list from the const table get values
	gpos::pointer<CColRefArray *> pdrgpcrCTGOutput = popCTG->PdrgpcrOutput();
	gpos::pointer<IDatum2dArray *> pdrgpdrgdatum = popCTG->Pdrgpdrgpdatum();

	const ULONG ulRows = pdrgpdrgdatum->Size();
	gpos::owner<CDXLNode *> pdxlnPrL = nullptr;
	gpos::owner<CDXLNode *> one_time_filter = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarOneTimeFilter(m_mp));

	gpos::owner<IDatumArray *> pdrgpdatum = nullptr;
	if (0 == ulRows)
	{
		// no-tuples... only generate one row of NULLS and one-time "false" filter
		pdrgpdatum =
			CTranslatorExprToDXLUtils::PdrgpdatumNulls(m_mp, pdrgpcrCTGOutput);

		gpos::owner<CExpression *> pexprFalse = CUtils::PexprScalarConstBool(
			m_mp, false /*value*/, false /*is_null*/);
		gpos::owner<CDXLNode *> pdxlnFalse = PdxlnScConst(pexprFalse);
		pexprFalse->Release();

		one_time_filter->AddChild(pdxlnFalse);
	}
	else
	{
		GPOS_ASSERT(1 <= ulRows);
		pdrgpdatum = (*pdrgpdrgdatum)[0];
		pdrgpdatum->AddRef();
		gpos::owner<CDXLNode *> pdxlnCond = nullptr;
		if (nullptr != pexprScalar)
		{
			pdxlnCond = PdxlnScalar(pexprScalar);
			one_time_filter->AddChild(pdxlnCond);
		}
	}

	// if CTG has multiple rows then it has to be a valuescan of constants,
	// else, a Result node is created
	if (ulRows > 1)
	{
		GPOS_ASSERT(nullptr != pdrgpcrCTGOutput);

		gpos::pointer<CColRefSet *> pcrsOutput =
			pexprCTG->DeriveOutputColumns();
		pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrCTGOutput);

		gpos::owner<CDXLNode *> pdxlnValuesScan =
			CTranslatorExprToDXLUtils::PdxlnValuesScan(
				m_mp, GetProperties(pexprCTG), pdxlnPrL, pdrgpdrgdatum);
		one_time_filter->Release();
		pdrgpdatum->Release();

		return pdxlnValuesScan;
	}
	else
	{
		pdxlnPrL = PdxlnProjListFromConstTableGet(colref_array,
												  pdrgpcrCTGOutput, pdrgpdatum);
		pdrgpdatum->Release();
		return CTranslatorExprToDXLUtils::PdxlnResult(
			m_mp, GetProperties(pexprCTG), std::move(pdxlnPrL),
			PdxlnFilter(nullptr), std::move(one_time_filter),
			nullptr	 //child_dxlnode
		);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResultFromConstTableGet
//
//	@doc:
//		Create a DXL result node from an optimizer const table get node
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnResultFromConstTableGet(
	gpos::pointer<CExpression *> pexprCTG,
	gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<CDistributionSpecArray *>,  // pdrgpdsBaseTables,
	ULONG *,								  // pulNonGatherMotions,
	BOOL *									  // pfDML
)
{
	return PdxlnResultFromConstTableGet(pexprCTG, colref_array,
										nullptr /*pexprScalarCond*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnComputeScalar
//
//	@doc:
//		Create a DXL result node from an optimizer compute scalar expression
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnComputeScalar(
	gpos::pointer<CExpression *> pexprComputeScalar,
	gpos::pointer<CColRefArray *> colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprComputeScalar);

	// extract components
	CExpression *pexprRelational = (*pexprComputeScalar)[0];
	gpos::pointer<CExpression *> pexprProjList = (*pexprComputeScalar)[1];

	// translate relational child expression
	gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
		pexprRelational, nullptr /* colref_array */, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// compute required columns
	GPOS_ASSERT(nullptr != pexprComputeScalar->Prpp());
	gpos::pointer<CColRefSet *> pcrsOutput =
		pexprComputeScalar->Prpp()->PcrsRequired();

	// iterate the columns in the projection list, add the columns containing
	// set-returning functions to the output columns
	const ULONG ulPrLs = pexprProjList->Arity();
	for (ULONG ul = 0; ul < ulPrLs; ul++)
	{
		gpos::pointer<CExpression *> pexprPrE = (*pexprProjList)[ul];

		// for column that doesn't contain set-returning function, if it is not the
		// required column in the relational plan properties, then no need to add them
		// to the output columns
		if (pexprPrE->DeriveHasNonScalarFunction())
		{
			gpos::pointer<CScalarProjectElement *> popScPrE =
				gpos::dyn_cast<CScalarProjectElement>(pexprPrE->Pop());
			pcrsOutput->Include(popScPrE->Pcr());
		}
	}

	// translate project list expression
	gpos::owner<CDXLNode *> pdxlnPrL = nullptr;
	if (nullptr == colref_array || CUtils::FHasDuplicates(colref_array))
	{
		pdxlnPrL = PdxlnProjList(pexprProjList, pcrsOutput);
	}
	else
	{
		pdxlnPrL = PdxlnProjList(pexprProjList, pcrsOutput, colref_array);
	}

	// construct a result node
	gpos::owner<CDXLNode *> pdxlnResult =
		PdxlnResult(GetProperties(pexprComputeScalar), std::move(pdxlnPrL),
					std::move(child_dxlnode));

#ifdef GPOS_DEBUG
	(void) gpos::dyn_cast<CDXLPhysicalResult>(pdxlnResult->GetOperator())
		->AssertValid(pdxlnResult, false /* validate_children */);
#endif
	return pdxlnResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAggregate
//
//	@doc:
//		Create a DXL aggregate node from an optimizer hash agg expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnAggregate(gpos::pointer<CExpression *> pexprAgg,
									 gpos::pointer<CColRefArray *> colref_array,
									 CDistributionSpecArray *pdrgpdsBaseTables,
									 ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprAgg);
	COperator::EOperatorId op_id = pexprAgg->Pop()->Eopid();

	// extract components and construct an aggregate node
	gpos::pointer<CPhysicalAgg *> popAgg = nullptr;

	GPOS_ASSERT(COperator::EopPhysicalStreamAgg == op_id ||
				COperator::EopPhysicalHashAgg == op_id ||
				COperator::EopPhysicalScalarAgg == op_id);

	EdxlAggStrategy dxl_agg_strategy = EdxlaggstrategySentinel;

	switch (op_id)
	{
		case COperator::EopPhysicalStreamAgg:
		{
			popAgg = gpos::dyn_cast<CPhysicalStreamAgg>(pexprAgg->Pop());
			dxl_agg_strategy = EdxlaggstrategySorted;
			break;
		}
		case COperator::EopPhysicalHashAgg:
		{
			popAgg = gpos::dyn_cast<CPhysicalHashAgg>(pexprAgg->Pop());
			dxl_agg_strategy = EdxlaggstrategyHashed;
			break;
		}
		case COperator::EopPhysicalScalarAgg:
		{
			popAgg = gpos::dyn_cast<CPhysicalScalarAgg>(pexprAgg->Pop());
			dxl_agg_strategy = EdxlaggstrategyPlain;
			break;
		}
		default:
		{
			return nullptr;	 // to silence the compiler
		}
	}

	gpos::pointer<const CColRefArray *> pdrgpcrGroupingCols =
		popAgg->PdrgpcrGroupingCols();

	return PdxlnAggregate(pexprAgg, colref_array, pdrgpdsBaseTables,
						  pulNonGatherMotions, pfDML, dxl_agg_strategy,
						  pdrgpcrGroupingCols, nullptr /*pcrsKeys*/
	);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAggregateDedup
//
//	@doc:
//		Create a DXL aggregate node from an optimizer dedup agg expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnAggregateDedup(
	gpos::pointer<CExpression *> pexprAgg,
	gpos::pointer<CColRefArray *> colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprAgg);
	COperator::EOperatorId op_id = pexprAgg->Pop()->Eopid();

	GPOS_ASSERT(COperator::EopPhysicalStreamAggDeduplicate == op_id ||
				COperator::EopPhysicalHashAggDeduplicate == op_id);

	EdxlAggStrategy dxl_agg_strategy = EdxlaggstrategySentinel;
	gpos::pointer<const CColRefArray *> pdrgpcrGroupingCols = nullptr;
	gpos::owner<CColRefSet *> pcrsKeys = GPOS_NEW(m_mp) CColRefSet(m_mp);

	if (COperator::EopPhysicalStreamAggDeduplicate == op_id)
	{
		gpos::pointer<CPhysicalStreamAggDeduplicate *> popAggDedup =
			gpos::dyn_cast<CPhysicalStreamAggDeduplicate>(pexprAgg->Pop());
		pcrsKeys->Include(popAggDedup->PdrgpcrKeys());
		pdrgpcrGroupingCols = popAggDedup->PdrgpcrGroupingCols();
		dxl_agg_strategy = EdxlaggstrategySorted;
	}
	else
	{
		gpos::pointer<CPhysicalHashAggDeduplicate *> popAggDedup =
			gpos::dyn_cast<CPhysicalHashAggDeduplicate>(pexprAgg->Pop());
		pcrsKeys->Include(popAggDedup->PdrgpcrKeys());
		pdrgpcrGroupingCols = popAggDedup->PdrgpcrGroupingCols();
		dxl_agg_strategy = EdxlaggstrategyHashed;
	}

	gpos::owner<CDXLNode *> pdxlnAgg = PdxlnAggregate(
		pexprAgg, colref_array, pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
		dxl_agg_strategy, pdrgpcrGroupingCols, pcrsKeys);
	pcrsKeys->Release();

	return pdxlnAgg;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAggregate
//
//	@doc:
//		Create a DXL aggregate node from an optimizer agg expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnAggregate(
	gpos::pointer<CExpression *> pexprAgg,
	gpos::pointer<CColRefArray *> colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML, EdxlAggStrategy dxl_agg_strategy,
	gpos::pointer<const CColRefArray *> pdrgpcrGroupingCols,
	gpos::pointer<CColRefSet *> pcrsKeys)
{
	GPOS_ASSERT(nullptr != pexprAgg);
	GPOS_ASSERT(nullptr != pdrgpcrGroupingCols);
#ifdef GPOS_DEBUG
	COperator::EOperatorId op_id = pexprAgg->Pop()->Eopid();
	GPOS_ASSERT_IMP(nullptr == pcrsKeys,
					COperator::EopPhysicalStreamAgg == op_id ||
						COperator::EopPhysicalHashAgg == op_id ||
						COperator::EopPhysicalScalarAgg == op_id);
#endif	//GPOS_DEBUG

	// is it safe to stream the local hash aggregate
	BOOL stream_safe =
		CTranslatorExprToDXLUtils::FLocalHashAggStreamSafe(pexprAgg);

	CExpression *pexprChild = (*pexprAgg)[0];
	gpos::pointer<CExpression *> pexprProjList = (*pexprAgg)[1];

	// translate relational child expression
	gpos::owner<CDXLNode *> child_dxlnode =
		CreateDXLNode(pexprChild,
					  nullptr,	// colref_array,
					  pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
					  false,  // fRemap,
					  false	  // fRoot
		);

	// compute required columns
	GPOS_ASSERT(nullptr != pexprAgg->Prpp());
	gpos::pointer<CColRefSet *> pcrsRequired = pexprAgg->Prpp()->PcrsRequired();

	// translate project list expression
	gpos::owner<CDXLNode *> proj_list_dxlnode =
		PdxlnProjList(pexprProjList, pcrsRequired, colref_array);

	// create an empty filter
	gpos::owner<CDXLNode *> filter_dxlnode =
		GPOS_NEW(m_mp) CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarFilter(m_mp));

	// construct grouping columns list and check if all the grouping column are
	// already in the project list of the aggregate operator

	const ULONG num_cols = proj_list_dxlnode->Arity();
	gpos::owner<UlongToUlongMap *> phmululPL =
		GPOS_NEW(m_mp) UlongToUlongMap(m_mp);
	for (ULONG ul = 0; ul < num_cols; ul++)
	{
		gpos::pointer<CDXLNode *> pdxlnProjElem = (*proj_list_dxlnode)[ul];
		ULONG colid =
			gpos::dyn_cast<CDXLScalarProjElem>(pdxlnProjElem->GetOperator())
				->Id();

		if (nullptr == phmululPL->Find(&colid))
		{
			BOOL fRes GPOS_ASSERTS_ONLY = phmululPL->Insert(
				GPOS_NEW(m_mp) ULONG(colid), GPOS_NEW(m_mp) ULONG(colid));
			GPOS_ASSERT(fRes);
		}
	}

	gpos::owner<ULongPtrArray *> pdrgpulGroupingCols =
		GPOS_NEW(m_mp) ULongPtrArray(m_mp);

	const ULONG length = pdrgpcrGroupingCols->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		CColRef *pcrGroupingCol = (*pdrgpcrGroupingCols)[ul];

		// only add columns that are either required or in the join keys.
		// if the keys colrefset is null, then skip this check
		if (nullptr != pcrsKeys && !pcrsKeys->FMember(pcrGroupingCol) &&
			!pcrsRequired->FMember(pcrGroupingCol))
		{
			continue;
		}

		pdrgpulGroupingCols->Append(GPOS_NEW(m_mp) ULONG(pcrGroupingCol->Id()));

		ULONG colid = pcrGroupingCol->Id();
		if (nullptr == phmululPL->Find(&colid))
		{
			gpos::owner<CDXLNode *> pdxlnProjElem =
				CTranslatorExprToDXLUtils::PdxlnProjElem(m_mp, m_phmcrdxln,
														 pcrGroupingCol);
			proj_list_dxlnode->AddChild(pdxlnProjElem);
			BOOL fRes GPOS_ASSERTS_ONLY = phmululPL->Insert(
				GPOS_NEW(m_mp) ULONG(colid), GPOS_NEW(m_mp) ULONG(colid));
			GPOS_ASSERT(fRes);
		}
	}

	phmululPL->Release();

	gpos::owner<CDXLPhysicalAgg *> pdxlopAgg =
		GPOS_NEW(m_mp) CDXLPhysicalAgg(m_mp, dxl_agg_strategy, stream_safe);
	pdxlopAgg->SetGroupingCols(std::move(pdrgpulGroupingCols));

	gpos::owner<CDXLNode *> pdxlnAgg =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopAgg));
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprAgg);
	pdxlnAgg->SetProperties(std::move(dxl_properties));

	// add children
	pdxlnAgg->AddChild(std::move(proj_list_dxlnode));
	pdxlnAgg->AddChild(std::move(filter_dxlnode));
	pdxlnAgg->AddChild(std::move(child_dxlnode));

#ifdef REFCOUNT_FIXME_ASSERT_AFTER_MOVE
#ifdef GPOS_DEBUG
	pdxlopAgg->AssertValid(pdxlnAgg, false /* validate_children */);
#endif
#endif

	return pdxlnAgg;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnSort
//
//	@doc:
//		Create a DXL sort node from an optimizer physical sort expression
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnSort(gpos::pointer<CExpression *> pexprSort,
								CColRefArray *colref_array,
								CDistributionSpecArray *pdrgpdsBaseTables,
								ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprSort);

	GPOS_ASSERT(1 == pexprSort->Arity());

	// extract components
	gpos::pointer<CPhysicalSort *> popSort =
		gpos::dyn_cast<CPhysicalSort>(pexprSort->Pop());
	CExpression *pexprChild = (*pexprSort)[0];

	// translate relational child expression
	gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
		pexprChild, colref_array, pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
		false /*fRemap*/, false /*fRoot*/);

	// translate order spec
	gpos::owner<CDXLNode *> sort_col_list_dxlnode =
		GetSortColListDXL(popSort->Pos());

	// construct project list from child project list
	GPOS_ASSERT(nullptr != child_dxlnode && 1 <= child_dxlnode->Arity());
	gpos::pointer<CDXLNode *> pdxlnProjListChild = (*child_dxlnode)[0];
	gpos::owner<CDXLNode *> proj_list_dxlnode =
		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
			m_mp, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	// create an empty filter
	gpos::owner<CDXLNode *> filter_dxlnode = PdxlnFilter(nullptr);

	// construct a sort node
	gpos::owner<CDXLPhysicalSort *> pdxlopSort =
		GPOS_NEW(m_mp) CDXLPhysicalSort(m_mp, false /*discard_duplicates*/);

	// construct sort node from its components
	gpos::owner<CDXLNode *> pdxlnSort =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopSort));
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprSort);
	pdxlnSort->SetProperties(std::move(dxl_properties));

	// construct empty limit count and offset nodes
	gpos::owner<CDXLNode *> limit_count_dxlnode = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarLimitCount(m_mp));
	gpos::owner<CDXLNode *> limit_offset_dxlnode = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarLimitOffset(m_mp));

	// add children
	pdxlnSort->AddChild(std::move(proj_list_dxlnode));
	pdxlnSort->AddChild(std::move(filter_dxlnode));
	pdxlnSort->AddChild(std::move(sort_col_list_dxlnode));
	pdxlnSort->AddChild(std::move(limit_count_dxlnode));
	pdxlnSort->AddChild(std::move(limit_offset_dxlnode));
	pdxlnSort->AddChild(std::move(child_dxlnode));

#ifdef REFCOUNT_FIXME_ASSERT_AFTER_MOVE
#ifdef GPOS_DEBUG
	pdxlopSort->AssertValid(pdxlnSort, false /* validate_children */);
#endif
#endif

	return pdxlnSort;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnLimit
//
//	@doc:
//		Create a DXL limit node from an optimizer physical limit expression.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnLimit(gpos::pointer<CExpression *> pexprLimit,
								 CColRefArray *colref_array,
								 CDistributionSpecArray *pdrgpdsBaseTables,
								 ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprLimit);
	GPOS_ASSERT(3 == pexprLimit->Arity());

	// extract components
	CExpression *pexprChild = (*pexprLimit)[0];
	gpos::pointer<CExpression *> pexprOffset = (*pexprLimit)[1];
	gpos::pointer<CExpression *> pexprCount = (*pexprLimit)[2];

	// bypass translation of limit if it does not have row count and offset
	gpos::pointer<CPhysicalLimit *> popLimit =
		gpos::dyn_cast<CPhysicalLimit>(pexprLimit->Pop());
	if (!popLimit->FHasCount() && CUtils::FHasZeroOffset(pexprLimit))
	{
		return CreateDXLNode(pexprChild, colref_array, pdrgpdsBaseTables,
							 pulNonGatherMotions, pfDML, true /*fRemap*/,
							 false /*fRoot*/);
	}

	// translate relational child expression
	gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
		pexprChild, colref_array, pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
		true /*fRemap*/, false /*fRoot*/);

	// translate limit offset and count
	gpos::owner<CDXLNode *> limit_offset = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarLimitOffset(m_mp));
	limit_offset->AddChild(PdxlnScalar(pexprOffset));

	gpos::owner<CDXLNode *> limit_count = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarLimitCount(m_mp));
	limit_count->AddChild(PdxlnScalar(pexprCount));

	// construct project list from child project list
	GPOS_ASSERT(nullptr != child_dxlnode && 1 <= child_dxlnode->Arity());
	gpos::pointer<CDXLNode *> pdxlnProjListChild = (*child_dxlnode)[0];
	gpos::owner<CDXLNode *> proj_list_dxlnode =
		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
			m_mp, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	// construct a limit node
	gpos::owner<CDXLPhysicalLimit *> pdxlopLimit =
		GPOS_NEW(m_mp) CDXLPhysicalLimit(m_mp);

	// construct limit node from its components
	gpos::owner<CDXLNode *> pdxlnLimit =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopLimit));
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprLimit);
	pdxlnLimit->SetProperties(std::move(dxl_properties));

	pdxlnLimit->AddChild(std::move(proj_list_dxlnode));
	pdxlnLimit->AddChild(std::move(child_dxlnode));
	pdxlnLimit->AddChild(std::move(limit_count));
	pdxlnLimit->AddChild(std::move(limit_offset));

#ifdef REFCOUNT_FIXME_ASSERT_AFTER_MOVE
#ifdef GPOS_DEBUG
	pdxlopLimit->AssertValid(pdxlnLimit, false /* validate_children */);
#endif
#endif

	return pdxlnLimit;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::BuildSubplansForCorrelatedLOJ
//
//	@doc:
//		Helper to build subplans from correlated LOJ
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::BuildSubplansForCorrelatedLOJ(
	gpos::pointer<CExpression *> pexprCorrelatedLOJ,
	CDXLColRefArray *dxl_colref_array,
	gpos::owner<CDXLNode *> *
		ppdxlnScalar,  // output: scalar condition after replacing inner child reference with subplan
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprCorrelatedLOJ);
	GPOS_ASSERT(COperator::EopPhysicalCorrelatedLeftOuterNLJoin ==
				pexprCorrelatedLOJ->Pop()->Eopid());

	CExpression *pexprInner = (*pexprCorrelatedLOJ)[1];
	gpos::pointer<CExpression *> pexprScalar = (*pexprCorrelatedLOJ)[2];

	gpos::pointer<CColRefArray *> pdrgpcrInner =
		gpos::dyn_cast<CPhysicalNLJoin>(pexprCorrelatedLOJ->Pop())
			->PdrgPcrInner();
	GPOS_ASSERT(nullptr != pdrgpcrInner);

	EdxlSubPlanType dxl_subplan_type = Edxlsubplantype(pexprCorrelatedLOJ);

	if (EdxlSubPlanTypeScalar == dxl_subplan_type)
	{
		// for correlated left outer join for scalar subplan type, we generate a scalar subplan
		BuildScalarSubplans(pdrgpcrInner, pexprInner, dxl_colref_array,
							pdrgpdsBaseTables, pulNonGatherMotions, pfDML);

		// now translate the scalar - references to the inner child will be
		// replaced by the subplan
		*ppdxlnScalar = PdxlnScalar(pexprScalar);

		return;
	}

	GPOS_ASSERT(EdxlSubPlanTypeAny == dxl_subplan_type ||
				EdxlSubPlanTypeAll == dxl_subplan_type ||
				EdxlSubPlanTypeExists == dxl_subplan_type ||
				EdxlSubPlanTypeNotExists == dxl_subplan_type);

	// for correlated left outer join with non-scalar subplan type,
	// we need to generate quantified/exitential subplan
	if (EdxlSubPlanTypeAny == dxl_subplan_type ||
		EdxlSubPlanTypeAll == dxl_subplan_type)
	{
		(void) PdxlnQuantifiedSubplan(pdrgpcrInner, pexprCorrelatedLOJ,
									  dxl_colref_array, pdrgpdsBaseTables,
									  pulNonGatherMotions, pfDML);
	}
	else
	{
		GPOS_ASSERT(EdxlSubPlanTypeExists == dxl_subplan_type ||
					EdxlSubPlanTypeNotExists == dxl_subplan_type);
		(void) PdxlnExistentialSubplan(pdrgpcrInner, pexprCorrelatedLOJ,
									   dxl_colref_array, pdrgpdsBaseTables,
									   pulNonGatherMotions, pfDML);
	}

	gpos::owner<CExpression *> pexprTrue =
		CUtils::PexprScalarConstBool(m_mp, true /*value*/, false /*is_null*/);
	*ppdxlnScalar = PdxlnScalar(pexprTrue);
	pexprTrue->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::BuildSubplans
//
//	@doc:
//		Helper to build subplans of different types
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::BuildSubplans(
	gpos::pointer<CExpression *> pexprCorrelatedNLJoin,
	CDXLColRefArray *dxl_colref_array,
	gpos::owner<CDXLNode *> *
		ppdxlnScalar,  // output: scalar condition after replacing inner child reference with subplan
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
	GPOS_ASSERT(CUtils::FCorrelatedNLJoin(pexprCorrelatedNLJoin->Pop()));
	GPOS_ASSERT(nullptr != ppdxlnScalar);

	CExpression *pexprInner = (*pexprCorrelatedNLJoin)[1];
	gpos::pointer<CExpression *> pexprScalar = (*pexprCorrelatedNLJoin)[2];

	gpos::pointer<CColRefArray *> pdrgpcrInner =
		gpos::dyn_cast<CPhysicalNLJoin>(pexprCorrelatedNLJoin->Pop())
			->PdrgPcrInner();
	GPOS_ASSERT(nullptr != pdrgpcrInner);

	COperator::EOperatorId op_id = pexprCorrelatedNLJoin->Pop()->Eopid();
	CDXLNode *pdxlnSubPlan = nullptr;
	switch (op_id)
	{
		case COperator::EopPhysicalCorrelatedLeftOuterNLJoin:
			BuildSubplansForCorrelatedLOJ(
				pexprCorrelatedNLJoin, dxl_colref_array, ppdxlnScalar,
				pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
			return;

		case COperator::EopPhysicalCorrelatedInnerNLJoin:
			BuildScalarSubplans(pdrgpcrInner, pexprInner, dxl_colref_array,
								pdrgpdsBaseTables, pulNonGatherMotions, pfDML);

			// now translate the scalar - references to the inner child will be
			// replaced by the subplan
			*ppdxlnScalar = PdxlnScalar(pexprScalar);
			return;

		case COperator::EopPhysicalCorrelatedInLeftSemiNLJoin:
		case COperator::EopPhysicalCorrelatedNotInLeftAntiSemiNLJoin:
			pdxlnSubPlan = PdxlnQuantifiedSubplan(
				pdrgpcrInner, pexprCorrelatedNLJoin, dxl_colref_array,
				pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
			pdxlnSubPlan->AddRef();
			*ppdxlnScalar = pdxlnSubPlan;
			return;

		case COperator::EopPhysicalCorrelatedLeftSemiNLJoin:
		case COperator::EopPhysicalCorrelatedLeftAntiSemiNLJoin:
			pdxlnSubPlan = PdxlnExistentialSubplan(
				pdrgpcrInner, pexprCorrelatedNLJoin, dxl_colref_array,
				pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
			pdxlnSubPlan->AddRef();
			*ppdxlnScalar = pdxlnSubPlan;
			return;

		default:
			GPOS_ASSERT(!"Unsupported correlated join");
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnRestrictResult
//
//	@doc:
//		Helper to build a Result expression with project list
//		restricted to required column
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnRestrictResult(gpos::owner<CDXLNode *> dxlnode,
										  const CColRef *colref)
{
	gpos::owner<CDXLNode *> dxlresult = nullptr;
	gpos::owner<CColRefSet *> pcrInner = GPOS_NEW(m_mp) CColRefSet(m_mp);

	pcrInner->Include(colref);
	dxlresult = PdxlnRestrictResult(std::move(dxlnode), pcrInner);
	pcrInner->Release();

	return dxlresult;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnRestrictResult
//
//	@doc:
//		Helper to build a Result expression with project list
//		restricted to required columns
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnRestrictResult(
	gpos::owner<CDXLNode *> dxlnode, gpos::pointer<const CColRefSet *> colrefs)
{
	GPOS_ASSERT(nullptr != dxlnode);
	GPOS_ASSERT(nullptr != colrefs);

	gpos::pointer<CDXLNode *> pdxlnProjListOld = (*dxlnode)[0];
	const ULONG ulPrjElems = pdxlnProjListOld->Arity();

	if (0 == ulPrjElems)
	{
		// failed to find project elements
		dxlnode->Release();
		return nullptr;
	}

	gpos::owner<CDXLNode *> pdxlnResult = dxlnode;
	if (1 < ulPrjElems)
	{
		// restrict project list to required column
		gpos::owner<CDXLScalarProjList *> pdxlopPrL =
			GPOS_NEW(m_mp) CDXLScalarProjList(m_mp);
		gpos::owner<CDXLNode *> pdxlnProjListNew =
			GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopPrL);

		gpos::owner<IntToColRefMap *> phmicr = colrefs->Phmicr(m_mp);

		for (ULONG ul = 0; ul < ulPrjElems; ul++)
		{
			gpos::pointer<CDXLNode *> child_dxlnode = (*pdxlnProjListOld)[ul];
			gpos::pointer<CDXLScalarProjElem *> pdxlPrjElem =
				gpos::dyn_cast<CDXLScalarProjElem>(
					child_dxlnode->GetOperator());

			const INT colid = pdxlPrjElem->Id();
			CColRef *colref = phmicr->Find(&colid);
			if (colref)
			{
				// create a new project element that simply points to required column,
				// we cannot re-use child_dxlnode here since it may have a deep expression with columns inaccessible
				// above the child (inner) DXL expression
				gpos::owner<CDXLNode *> pdxlnPrEl =
					CTranslatorExprToDXLUtils::PdxlnProjElem(m_mp, m_phmcrdxln,
															 colref);
				pdxlnProjListNew->AddChild(pdxlnPrEl);
			}
		}

		phmicr->Release();

		GPOS_ASSERT(colrefs->Size() == pdxlnProjListNew->Arity());

		pdxlnResult = GPOS_NEW(m_mp)
			CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLPhysicalResult(m_mp));
		gpos::owner<CDXLPhysicalProperties *> dxl_properties =
			CTranslatorExprToDXLUtils::PdxlpropCopy(m_mp, dxlnode);
		pdxlnResult->SetProperties(dxl_properties);

		pdxlnResult->AddChild(pdxlnProjListNew);
		pdxlnResult->AddChild(PdxlnFilter(nullptr));
		pdxlnResult->AddChild(GPOS_NEW(m_mp) CDXLNode(
			m_mp, GPOS_NEW(m_mp) CDXLScalarOneTimeFilter(m_mp)));
		pdxlnResult->AddChild(dxlnode);
	}

	return pdxlnResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnQuantifiedSubplan
//
//	@doc:
//		Helper to build subplans for quantified (ANY/ALL) subqueries
//
//---------------------------------------------------------------------------
gpos::pointer<CDXLNode *>
CTranslatorExprToDXL::PdxlnQuantifiedSubplan(
	gpos::pointer<CColRefArray *> pdrgpcrInner,
	gpos::pointer<CExpression *> pexprCorrelatedNLJoin,
	CDXLColRefArray *dxl_colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
	gpos::pointer<COperator *> popCorrelatedJoin = pexprCorrelatedNLJoin->Pop();
	COperator::EOperatorId op_id = popCorrelatedJoin->Eopid();
	BOOL fCorrelatedLOJ =
		(COperator::EopPhysicalCorrelatedLeftOuterNLJoin == op_id);
	GPOS_ASSERT(COperator::EopPhysicalCorrelatedInLeftSemiNLJoin == op_id ||
				COperator::EopPhysicalCorrelatedNotInLeftAntiSemiNLJoin ==
					op_id ||
				fCorrelatedLOJ);

	EdxlSubPlanType dxl_subplan_type = Edxlsubplantype(pexprCorrelatedNLJoin);
	GPOS_ASSERT_IMP(fCorrelatedLOJ, EdxlSubPlanTypeAny == dxl_subplan_type ||
										EdxlSubPlanTypeAll == dxl_subplan_type);

	CExpression *pexprInner = (*pexprCorrelatedNLJoin)[1];
	gpos::pointer<CExpression *> pexprScalar = (*pexprCorrelatedNLJoin)[2];

	// translate inner child
	gpos::owner<CDXLNode *> pdxlnInnerChild = CreateDXLNode(
		pexprInner, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// find required column from inner child
	gpos::owner<CColRefSet *> pcrInner = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrInner->Include((*pdrgpcrInner)[0]);

	BOOL outerParam = false;
	if (fCorrelatedLOJ)
	{
		// overwrite required inner column based on scalar expression

		gpos::pointer<CColRefSet *> pcrsInner =
			pexprInner->DeriveOutputColumns();
		gpos::owner<CColRefSet *> pcrsUsed =
			GPOS_NEW(m_mp) CColRefSet(m_mp, *pexprScalar->DeriveUsedColumns());
		pcrsUsed->Intersection(pcrsInner);
		if (0 < pcrsUsed->Size())
		{
			GPOS_ASSERT(1 == pcrsUsed->Size() || 2 == pcrsUsed->Size());

			// Both sides of the SubPlan test expression can come from the
			// inner side. So we need to pass pcrsUsed instead of pcrInner into
			// PdxlnRestrictResult()
			outerParam = pcrsUsed->Size() > 1;

			pcrInner->Release();
			pcrInner = pcrsUsed;
		}
		else
		{
			pcrsUsed->Release();
		}
	}

	gpos::owner<CDXLNode *> inner_dxlnode =
		PdxlnRestrictResult(pdxlnInnerChild, pcrInner);
	pcrInner->Release();
	if (nullptr == inner_dxlnode)
	{
		GPOS_RAISE(
			gpopt::ExmaDXL, gpopt::ExmiExpr2DXLUnsupportedFeature,
			GPOS_WSZ_LIT(
				"Outer references in the project list of a correlated subquery"));
	}

	// translate test expression
	gpos::owner<CDXLNode *> dxlnode_test_expr = PdxlnScalar(pexprScalar);

	gpos::pointer<const IMDTypeBool *> pmdtypebool =
		m_pmda->PtMDType<IMDTypeBool>();
	gpos::owner<IMDId *> mdid = pmdtypebool->MDId();
	mdid->AddRef();

	// construct a subplan node, with the inner child under it
	gpos::owner<CDXLNode *> pdxlnSubPlan = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarSubPlan(
				  m_mp, std::move(mdid), dxl_colref_array, dxl_subplan_type,
				  std::move(dxlnode_test_expr), outerParam));
	pdxlnSubPlan->AddChild(std::move(inner_dxlnode));

	// add to hashmap
	BOOL fRes GPOS_ASSERTS_ONLY =
		m_phmcrdxln->Insert((*pdrgpcrInner)[0], pdxlnSubPlan);
	GPOS_ASSERT(fRes);

	return pdxlnSubPlan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnProjectBoolConst
//
//	@doc:
//		Helper to add a project of bool constant on top of given DXL node
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnProjectBoolConst(gpos::owner<CDXLNode *> dxlnode,
											BOOL value)
{
	GPOS_ASSERT(nullptr != dxlnode);

	// create a new project element with bool value
	gpos::pointer<const IMDTypeBool *> pmdtypebool =
		m_pmda->PtMDType<IMDTypeBool>();
	gpos::owner<IMDId *> mdid = pmdtypebool->MDId();
	mdid->AddRef();

	gpos::owner<CDXLDatumBool *> dxl_datum = GPOS_NEW(m_mp)
		CDXLDatumBool(m_mp, std::move(mdid), false /* is_null */, value);
	gpos::owner<CDXLScalarConstValue *> pdxlopConstValue =
		GPOS_NEW(m_mp) CDXLScalarConstValue(m_mp, std::move(dxl_datum));
	CColRef *colref = m_pcf->PcrCreate(pmdtypebool, default_type_modifier);
	gpos::owner<CDXLNode *> pdxlnPrEl = PdxlnProjElem(
		colref, GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopConstValue)));

	gpos::owner<CDXLScalarProjList *> pdxlopPrL =
		GPOS_NEW(m_mp) CDXLScalarProjList(m_mp);
	gpos::owner<CDXLNode *> proj_list_dxlnode =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopPrL));
	proj_list_dxlnode->AddChild(std::move(pdxlnPrEl));
	gpos::owner<CDXLNode *> pdxlnResult =
		GPOS_NEW(m_mp) CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLPhysicalResult(m_mp));
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		CTranslatorExprToDXLUtils::PdxlpropCopy(m_mp, dxlnode);
	pdxlnResult->SetProperties(std::move(dxl_properties));

	pdxlnResult->AddChild(std::move(proj_list_dxlnode));
	pdxlnResult->AddChild(PdxlnFilter(nullptr));
	pdxlnResult->AddChild(GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarOneTimeFilter(m_mp)));
	pdxlnResult->AddChild(std::move(dxlnode));

	return pdxlnResult;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::EdxlsubplantypeCorrelatedLOJ
//
//	@doc:
//		Helper to find subplan type from a correlated left outer
//		join expression
//
//---------------------------------------------------------------------------
EdxlSubPlanType
CTranslatorExprToDXL::EdxlsubplantypeCorrelatedLOJ(
	gpos::pointer<CExpression *> pexprCorrelatedLOJ)
{
	GPOS_ASSERT(nullptr != pexprCorrelatedLOJ);
	GPOS_ASSERT(COperator::EopPhysicalCorrelatedLeftOuterNLJoin ==
				pexprCorrelatedLOJ->Pop()->Eopid());

	COperator::EOperatorId eopidSubq =
		gpos::dyn_cast<CPhysicalCorrelatedLeftOuterNLJoin>(
			pexprCorrelatedLOJ->Pop())
			->EopidOriginSubq();
	switch (eopidSubq)
	{
		case COperator::EopScalarSubquery:
			return EdxlSubPlanTypeScalar;

		case COperator::EopScalarSubqueryAll:
			return EdxlSubPlanTypeAll;

		case COperator::EopScalarSubqueryAny:
			return EdxlSubPlanTypeAny;

		case COperator::EopScalarSubqueryExists:
			return EdxlSubPlanTypeExists;

		case COperator::EopScalarSubqueryNotExists:
			return EdxlSubPlanTypeNotExists;

		default:
			GPOS_ASSERT(
				!"Unexpected origin subquery in correlated left outer join");
	}

	return EdxlSubPlanTypeSentinel;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::Edxlsubplantype
//
//	@doc:
//		Helper to find subplan type from a correlated join expression
//
//---------------------------------------------------------------------------
EdxlSubPlanType
CTranslatorExprToDXL::Edxlsubplantype(
	gpos::pointer<CExpression *> pexprCorrelatedNLJoin)
{
	GPOS_ASSERT(nullptr != pexprCorrelatedNLJoin);
	GPOS_ASSERT(CUtils::FCorrelatedNLJoin(pexprCorrelatedNLJoin->Pop()));

	COperator::EOperatorId op_id = pexprCorrelatedNLJoin->Pop()->Eopid();
	switch (op_id)
	{
		case COperator::EopPhysicalCorrelatedLeftOuterNLJoin:
			return EdxlsubplantypeCorrelatedLOJ(pexprCorrelatedNLJoin);

		case COperator::EopPhysicalCorrelatedInnerNLJoin:
			return EdxlSubPlanTypeScalar;

		case COperator::EopPhysicalCorrelatedNotInLeftAntiSemiNLJoin:
			return EdxlSubPlanTypeAll;

		case COperator::EopPhysicalCorrelatedInLeftSemiNLJoin:
			return EdxlSubPlanTypeAny;

		case COperator::EopPhysicalCorrelatedLeftSemiNLJoin:
			return EdxlSubPlanTypeExists;

		case COperator::EopPhysicalCorrelatedLeftAntiSemiNLJoin:
			return EdxlSubPlanTypeNotExists;

		default:
			GPOS_ASSERT(!"Unexpected correlated join");
	}

	return EdxlSubPlanTypeSentinel;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnExistentialSubplan
//
//	@doc:
//		Helper to build subplans for existential subqueries
//
//---------------------------------------------------------------------------
gpos::pointer<CDXLNode *>
CTranslatorExprToDXL::PdxlnExistentialSubplan(
	gpos::pointer<CColRefArray *> pdrgpcrInner,
	gpos::pointer<CExpression *> pexprCorrelatedNLJoin,
	CDXLColRefArray *dxl_colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
#ifdef GPOS_DEBUG
	COperator::EOperatorId op_id = pexprCorrelatedNLJoin->Pop()->Eopid();
	BOOL fCorrelatedLOJ =
		(COperator::EopPhysicalCorrelatedLeftOuterNLJoin == op_id);
#endif	// GPOS_DEBUG
	GPOS_ASSERT(COperator::EopPhysicalCorrelatedLeftSemiNLJoin == op_id ||
				COperator::EopPhysicalCorrelatedLeftAntiSemiNLJoin == op_id ||
				fCorrelatedLOJ);

	EdxlSubPlanType dxl_subplan_type = Edxlsubplantype(pexprCorrelatedNLJoin);
	GPOS_ASSERT_IMP(fCorrelatedLOJ,
					EdxlSubPlanTypeExists == dxl_subplan_type ||
						EdxlSubPlanTypeNotExists == dxl_subplan_type);

	// translate inner child
	CExpression *pexprInner = (*pexprCorrelatedNLJoin)[1];

	gpos::owner<CDXLNode *> pdxlnInnerChild = CreateDXLNode(
		pexprInner, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	gpos::pointer<CDXLNode *> pdxlnInnerProjList = (*pdxlnInnerChild)[0];
	gpos::owner<CDXLNode *> inner_dxlnode = nullptr;
	if (0 == pdxlnInnerProjList->Arity())
	{
		// no requested columns from subplan, add a dummy boolean constant to project list
		inner_dxlnode = PdxlnProjectBoolConst(pdxlnInnerChild, true /*value*/);
	}
	else
	{
		// restrict requested columns to required inner column
		inner_dxlnode =
			PdxlnRestrictResult(pdxlnInnerChild, (*pdrgpcrInner)[0]);
	}

	if (nullptr == inner_dxlnode)
	{
		GPOS_RAISE(
			gpopt::ExmaDXL, gpopt::ExmiExpr2DXLUnsupportedFeature,
			GPOS_WSZ_LIT(
				"Outer references in the project list of a correlated subquery"));
	}

	gpos::pointer<const IMDTypeBool *> pmdtypebool =
		m_pmda->PtMDType<IMDTypeBool>();
	gpos::owner<IMDId *> mdid = pmdtypebool->MDId();
	mdid->AddRef();

	// construct a subplan node, with the inner child under it
	gpos::owner<CDXLNode *> pdxlnSubPlan = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarSubPlan(
						   m_mp, std::move(mdid), dxl_colref_array,
						   dxl_subplan_type, nullptr /*dxlnode_test_expr*/));
	pdxlnSubPlan->AddChild(std::move(inner_dxlnode));

	// add to hashmap
	BOOL fRes GPOS_ASSERTS_ONLY = m_phmcrdxln->Insert(
		const_cast<CColRef *>((*pdrgpcrInner)[0]), pdxlnSubPlan);
	GPOS_ASSERT(fRes);

	return pdxlnSubPlan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::BuildScalarSubplans
//
//	@doc:
//		Helper to build subplans from inner column references and store
//		generated subplans in subplan map
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::BuildScalarSubplans(
	gpos::pointer<CColRefArray *> pdrgpcrInner, CExpression *pexprInner,
	CDXLColRefArray *dxl_colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
	const ULONG size = pdrgpcrInner->Size();

	gpos::owner<CDXLNodeArray *> pdrgpdxlnInner =
		GPOS_NEW(m_mp) CDXLNodeArray(m_mp);
	for (ULONG ul = 0; ul < size; ul++)
	{
		// for each subplan, we need to re-translate inner expression
		gpos::owner<CDXLNode *> pdxlnInnerChild = CreateDXLNode(
			pexprInner, nullptr /*colref_array*/, pdrgpdsBaseTables,
			pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
		gpos::owner<CDXLNode *> inner_dxlnode =
			PdxlnRestrictResult(pdxlnInnerChild, (*pdrgpcrInner)[ul]);
		if (nullptr == inner_dxlnode)
		{
			GPOS_RAISE(
				gpopt::ExmaDXL, gpopt::ExmiExpr2DXLUnsupportedFeature,
				GPOS_WSZ_LIT(
					"Outer references in the project list of a correlated subquery"));
		}
		pdrgpdxlnInner->Append(inner_dxlnode);
	}

	for (ULONG ul = 0; ul < size; ul++)
	{
		gpos::owner<CDXLNode *> inner_dxlnode = (*pdrgpdxlnInner)[ul];
		inner_dxlnode->AddRef();
		if (0 < ul)
		{
			// if there is more than one subplan, we need to add-ref passed arrays
			dxl_colref_array->AddRef();
		}
		const CColRef *pcrInner = (*pdrgpcrInner)[ul];
		BuildDxlnSubPlan(inner_dxlnode, pcrInner, dxl_colref_array);
	}

	pdrgpdxlnInner->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PcrsOuterRefsForCorrelatedNLJoin
//
//	@doc:
//		Return outer refs in correlated join inner child
//
//---------------------------------------------------------------------------
CColRefSet *
CTranslatorExprToDXL::PcrsOuterRefsForCorrelatedNLJoin(
	gpos::pointer<CExpression *> pexpr)
{
	GPOS_ASSERT(CUtils::FCorrelatedNLJoin(pexpr->Pop()));

	gpos::pointer<CExpression *> pexprInnerChild = (*pexpr)[1];

	return pexprInnerChild->DeriveOuterReferences();
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnCorrelatedNLJoin
//
//	@doc:
//		Translate correlated NLJ expression.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnCorrelatedNLJoin(
	gpos::pointer<CExpression *> pexpr, CColRefArray *colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
	GPOS_ASSERT(CUtils::FCorrelatedNLJoin(pexpr->Pop()));

	// extract components
	CExpression *pexprOuterChild = (*pexpr)[0];
	CExpression *pexprInnerChild = (*pexpr)[1];
	gpos::pointer<CExpression *> pexprScalar = (*pexpr)[2];

	// outer references in the inner child
	gpos::owner<CDXLColRefArray *> dxl_colref_array =
		GPOS_NEW(m_mp) CDXLColRefArray(m_mp);

	gpos::pointer<CColRefSet *> outer_refs =
		PcrsOuterRefsForCorrelatedNLJoin(pexpr);
	CColRefSetIter crsi(*outer_refs);
	while (crsi.Advance())
	{
		CColRef *colref = crsi.Pcr();
		CMDName *mdname = GPOS_NEW(m_mp) CMDName(m_mp, colref->Name().Pstr());
		gpos::owner<IMDId *> mdid = colref->RetrieveType()->MDId();
		mdid->AddRef();
		gpos::owner<CDXLColRef *> dxl_colref = GPOS_NEW(m_mp)
			CDXLColRef(mdname, colref->Id(), mdid, colref->TypeModifier());
		dxl_colref_array->Append(dxl_colref);
	}

	COperator::EOperatorId op_id = pexpr->Pop()->Eopid();
	gpos::owner<CDXLNode *> pdxlnCond = nullptr;

	// Create a subplan with a Boolean from the inner child if we have a Const True as a join condition.
	// One scenario for this is when IN sublinks contain a projection from the outer table only such as:
	// select * from foo where foo.a in (select foo.b from bar);
	// If bar is a very small table, ORCA generates a CorrelatedInLeftSemiNLJoin with a Const true join filter
	// and condition foo.a = foo.b is added as a filter on the table scan of foo. If bar is a large table,
	// ORCA generates a plan with CorrelatedInnerNLJoin with a Const true join filter and a LIMIT over the
	// scan of bar. The same foo.a = foo.b condition is also added as a filter on the table scan of foo.
	if (CUtils::FScalarConstTrue(pexprScalar) &&
		(COperator::EopPhysicalCorrelatedInnerNLJoin == op_id ||
		 COperator::EopPhysicalCorrelatedInLeftSemiNLJoin == op_id))
	{
		// translate relational inner child expression
		gpos::owner<CDXLNode *> pdxlnInnerChild =
			CreateDXLNode(pexprInnerChild,
						  nullptr,	// colref_array,
						  pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
						  false,  // fRemap
						  false	  // fRoot
			);

		// if the filter predicate is a constant TRUE, create a subplan that returns
		// Boolean from the inner child, and use that as the scalar condition
		pdxlnCond =
			PdxlnBooleanScalarWithSubPlan(pdxlnInnerChild, dxl_colref_array);
	}
	else
	{
		BuildSubplans(pexpr, dxl_colref_array, &pdxlnCond, pdrgpdsBaseTables,
					  pulNonGatherMotions, pfDML);
	}

	// extract dxl properties from correlated join
	gpos::owner<CDXLPhysicalProperties *> dxl_properties = GetProperties(pexpr);
	gpos::owner<CDXLNode *> dxlnode = nullptr;

	switch (pexprOuterChild->Pop()->Eopid())
	{
		case COperator::EopPhysicalTableScan:
		{
			dxl_properties->AddRef();
			// create and return a table scan node
			dxlnode = PdxlnTblScanFromNLJoinOuter(
				pexprOuterChild, pdxlnCond, colref_array, pdrgpdsBaseTables,
				pulNonGatherMotions, dxl_properties);
			break;
		}

		case COperator::EopPhysicalFilter:
		{
			dxl_properties->AddRef();
			dxlnode = PdxlnResultFromNLJoinOuter(
				pexprOuterChild, pdxlnCond, colref_array, pdrgpdsBaseTables,
				pulNonGatherMotions, pfDML, dxl_properties);
			break;
		}

		default:
		{
			// create a result node over outer child
			dxl_properties->AddRef();
			dxlnode = PdxlnResult(pexprOuterChild, colref_array,
								  pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
								  pdxlnCond, dxl_properties);
		}
	}

	dxl_properties->Release();
	return dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::BuildDxlnSubPlan
//
//	@doc:
//		Construct a scalar dxl node with a subplan as its child. Also put this
//		subplan in the hashmap with its output column, so that anyone who
//		references that column can use the subplan
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::BuildDxlnSubPlan(gpos::owner<CDXLNode *> pdxlnRelChild,
									   const CColRef *colref,
									   CDXLColRefArray *dxl_colref_array)
{
	GPOS_ASSERT(nullptr != colref);
	gpos::owner<IMDId *> mdid = colref->RetrieveType()->MDId();
	mdid->AddRef();

	// construct a subplan node, with the inner child under it
	gpos::owner<CDXLNode *> pdxlnSubPlan = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarSubPlan(
						   m_mp, std::move(mdid), dxl_colref_array,
						   EdxlSubPlanTypeScalar, nullptr));
	pdxlnSubPlan->AddChild(std::move(pdxlnRelChild));

	// add to hashmap
	BOOL fRes GPOS_ASSERTS_ONLY = m_phmcrdxln->Insert(
		const_cast<CColRef *>(colref), std::move(pdxlnSubPlan));
	GPOS_ASSERT(fRes);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnBooleanScalarWithSubPlan
//
//	@doc:
//		Construct a boolean scalar dxl node with a subplan as its child. The
//		sublan has a boolean output column, and has	the given relational child
//		under it
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnBooleanScalarWithSubPlan(
	gpos::owner<CDXLNode *> pdxlnRelChild,
	gpos::owner<CDXLColRefArray *> dxl_colref_array)
{
	// create a new project element (const:true), and replace the first child with it
	gpos::pointer<const IMDTypeBool *> pmdtypebool =
		m_pmda->PtMDType<IMDTypeBool>();
	gpos::owner<IMDId *> mdid = pmdtypebool->MDId();
	mdid->AddRef();

	gpos::owner<CDXLDatumBool *> dxl_datum = GPOS_NEW(m_mp)
		CDXLDatumBool(m_mp, mdid, false /* is_null */, true /* value */);
	gpos::owner<CDXLScalarConstValue *> pdxlopConstValue =
		GPOS_NEW(m_mp) CDXLScalarConstValue(m_mp, std::move(dxl_datum));

	CColRef *colref = m_pcf->PcrCreate(pmdtypebool, default_type_modifier);

	gpos::owner<CDXLNode *> pdxlnPrEl = PdxlnProjElem(
		colref, GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopConstValue)));

	// create a new Result node for the created project element
	gpos::owner<CDXLNode *> pdxlnProjListNew =
		GPOS_NEW(m_mp) CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarProjList(m_mp));
	pdxlnProjListNew->AddChild(std::move(pdxlnPrEl));
	gpos::owner<CDXLNode *> pdxlnResult =
		GPOS_NEW(m_mp) CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLPhysicalResult(m_mp));
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		CTranslatorExprToDXLUtils::PdxlpropCopy(m_mp, pdxlnRelChild);
	pdxlnResult->SetProperties(std::move(dxl_properties));
	pdxlnResult->AddChild(std::move(pdxlnProjListNew));
	pdxlnResult->AddChild(PdxlnFilter(nullptr));
	pdxlnResult->AddChild(GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarOneTimeFilter(m_mp)));
	pdxlnResult->AddChild(std::move(pdxlnRelChild));

	// construct a subplan node, with the Result node under it
	mdid->AddRef();
	gpos::owner<CDXLNode *> pdxlnSubPlan = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarSubPlan(
						   m_mp, mdid, std::move(dxl_colref_array),
						   EdxlSubPlanTypeScalar, nullptr));
	pdxlnSubPlan->AddChild(std::move(pdxlnResult));

	return pdxlnSubPlan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScBoolExpr
//
//	@doc:
//		Create a DXL scalar boolean node given two DXL boolean nodes
//		and a boolean op
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScBoolExpr(EdxlBoolExprType boolexptype,
									  gpos::owner<CDXLNode *> dxlnode_left,
									  gpos::owner<CDXLNode *> dxlnode_right)
{
	gpos::owner<CDXLNode *> pdxlnBoolExpr = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarBoolExpr(m_mp, boolexptype));

	pdxlnBoolExpr->AddChild(std::move(dxlnode_left));
	pdxlnBoolExpr->AddChild(std::move(dxlnode_right));

	return pdxlnBoolExpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnTblScanFromNLJoinOuter
//
//	@doc:
//		Create a DXL table scan node from the outer child of a NLJ
//		and a DXL scalar condition. Used for translated correlated
//		subqueries.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnTblScanFromNLJoinOuter(
	gpos::pointer<CExpression *> pexprRelational,
	gpos::owner<CDXLNode *> pdxlnCond,
	gpos::pointer<CColRefArray *> colref_array,
	gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
	ULONG *,  // pulNonGatherMotions,
	CDXLPhysicalProperties *dxl_properties)
{
	// create a table scan over the input expression, without a filter
	gpos::owner<CDXLNode *> pdxlnTblScan =
		PdxlnTblScan(pexprRelational,
					 nullptr,  //pcrsOutput
					 colref_array, pdrgpdsBaseTables,
					 nullptr,  //pexprScalar
					 dxl_properties);

	if (!CTranslatorExprToDXLUtils::FScalarConstTrue(m_pmda, pdxlnCond))
	{
		// add the new filter to the table scan replacing its original
		// empty filter
		gpos::owner<CDXLNode *> filter_dxlnode = PdxlnFilter(pdxlnCond);
		pdxlnTblScan->ReplaceChild(EdxltsIndexFilter /*ulPos*/, filter_dxlnode);
	}
	else
	{
		// not used
		pdxlnCond->Release();
	}

	return pdxlnTblScan;
}

static ULONG
UlIndexFilter(Edxlopid edxlopid)
{
	switch (edxlopid)
	{
		case EdxlopPhysicalTableScan:
		case EdxlopPhysicalExternalScan:
			return EdxltsIndexFilter;
		case EdxlopPhysicalBitmapTableScan:
			return EdxlbsIndexFilter;
		case EdxlopPhysicalIndexScan:
			return EdxlisIndexFilter;
		case EdxlopPhysicalResult:
			return EdxlresultIndexFilter;
		default:
			GPOS_RTL_ASSERT(
				"Unexpected operator. Expected operators that contain a filter child");
			return gpos::ulong_max;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResultFromNLJoinOuter
//
//	@doc:
//		Create a DXL result node from the outer child of a NLJ
//		and a DXL scalar join condition. Used for translated correlated
//		subqueries.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnResultFromNLJoinOuter(
	CExpression *pexprOuterChildRelational,
	gpos::owner<CDXLNode *> pdxlnJoinCond, CColRefArray *colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML, gpos::owner<CDXLPhysicalProperties *> dxl_properties)
{
	// create a result node using the filter from the outer child of the input expression
	gpos::owner<CDXLNode *> pdxlnRelationalNew = PdxlnFromFilter(
		pexprOuterChildRelational, colref_array, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, dxl_properties);
	dxl_properties->Release();

	Edxlopid edxlopid = pdxlnRelationalNew->GetOperator()->GetDXLOperator();
	switch (edxlopid)
	{
		case EdxlopPhysicalTableScan:
		case EdxlopPhysicalExternalScan:
		case EdxlopPhysicalBitmapTableScan:
		case EdxlopPhysicalIndexScan:
		case EdxlopPhysicalResult:
		{
			// if the scalar join condition is a constant TRUE, just translate the child, no need to create an AND expression
			if (CTranslatorExprToDXLUtils::FScalarConstTrue(m_pmda,
															pdxlnJoinCond))
			{
				pdxlnJoinCond->Release();
				break;
			}

			// create new AND expression with the outer child's filter node and the join condition
			ULONG ulIndexFilter = UlIndexFilter(edxlopid);
			GPOS_ASSERT(ulIndexFilter != gpos::ulong_max);
			gpos::pointer<CDXLNode *> pdxlnChildFilter =
				(*pdxlnRelationalNew)[ulIndexFilter];
			GPOS_ASSERT(EdxlopScalarFilter ==
						pdxlnChildFilter->GetOperator()->GetDXLOperator());
			gpos::owner<CDXLNode *> newFilterPred = pdxlnJoinCond;

			if (0 < pdxlnChildFilter->Arity())
			{
				// we have both a filter condition (from the outer child) in our result node
				// and a non-trivial condition pdxlnJoinCond passed in as parameter, need to AND the two
				CDXLNode *pdxlnCondFromChildFilter = (*pdxlnChildFilter)[0];

				GPOS_ASSERT(2 > pdxlnChildFilter->Arity());
				pdxlnCondFromChildFilter->AddRef();

				newFilterPred = PdxlnScBoolExpr(
					Edxland, pdxlnCondFromChildFilter, pdxlnJoinCond);
			}

			// add the new filter to the result replacing its original
			// empty filter
			gpos::owner<CDXLNode *> new_filter_dxlnode =
				PdxlnFilter(newFilterPred);
			pdxlnRelationalNew->ReplaceChild(ulIndexFilter /*ulPos*/,
											 new_filter_dxlnode);
		}
		break;
		// In case the OuterChild is a physical sequence, it will already have the filter in the partition selector and
		// dynamic scan, thus we should not replace the filter.
		case EdxlopPhysicalSequence:
		case EdxlopPhysicalAppend:
		{
			dxl_properties->AddRef();
			GPOS_ASSERT(nullptr != pexprOuterChildRelational->Prpp());
			gpos::pointer<CColRefSet *> pcrsOutput =
				pexprOuterChildRelational->Prpp()->PcrsRequired();
			pdxlnRelationalNew = PdxlnAddScalarFilterOnRelationalChild(
				std::move(pdxlnRelationalNew), pdxlnJoinCond, dxl_properties,
				pcrsOutput, colref_array);
		}
		break;
		default:
			pdxlnJoinCond->Release();
			GPOS_RTL_ASSERT(false && "Unexpected node here");
	}

	return pdxlnRelationalNew;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::StoreIndexNLJOuterRefs
//
//	@doc:
//		Store outer references in index NLJ inner child into global map
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::StoreIndexNLJOuterRefs(gpos::pointer<CPhysical *> pop)
{
	gpos::pointer<CColRefArray *> colref_array = nullptr;

	if (COperator::EopPhysicalInnerIndexNLJoin == pop->Eopid())
	{
		colref_array =
			gpos::dyn_cast<CPhysicalInnerIndexNLJoin>(pop)->PdrgPcrOuterRefs();
	}
	else
	{
		colref_array = gpos::dyn_cast<CPhysicalLeftOuterIndexNLJoin>(pop)
						   ->PdrgPcrOuterRefs();
	}
	GPOS_ASSERT(colref_array != nullptr);

	const ULONG size = colref_array->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		CColRef *colref = (*colref_array)[ul];
		if (nullptr == m_phmcrdxlnIndexLookup->Find(colref))
		{
			gpos::owner<CDXLNode *> dxlnode =
				CTranslatorExprToDXLUtils::PdxlnIdent(
					m_mp, m_phmcrdxln, m_phmcrdxlnIndexLookup,
					m_phmcrulPartColId, colref);
#ifdef GPOS_DEBUG
			BOOL fInserted =
#endif	// GPOS_DEBUG
				m_phmcrdxlnIndexLookup->Insert(colref, dxlnode);
			GPOS_ASSERT(fInserted);
		}
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnNLJoin
//
//	@doc:
//		Create a DXL nested loop join node from an optimizer nested loop
//		join expression
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnNLJoin(gpos::pointer<CExpression *> pexprInnerNLJ,
								  gpos::pointer<CColRefArray *> colref_array,
								  CDistributionSpecArray *pdrgpdsBaseTables,
								  ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprInnerNLJ);
	GPOS_ASSERT(3 == pexprInnerNLJ->Arity());

	// extract components
	gpos::pointer<CPhysical *> pop =
		gpos::dyn_cast<CPhysical>(pexprInnerNLJ->Pop());

	CExpression *pexprOuterChild = (*pexprInnerNLJ)[0];
	CExpression *pexprInnerChild = (*pexprInnerNLJ)[1];
	gpos::pointer<CExpression *> pexprScalar = (*pexprInnerNLJ)[2];


#ifdef GPOS_DEBUG
	GPOS_ASSERT_IMP(
		COperator::EopPhysicalInnerIndexNLJoin != pop->Eopid() &&
			COperator::EopPhysicalLeftOuterIndexNLJoin != pop->Eopid(),
		pexprInnerChild->DeriveOuterReferences()->IsDisjoint(
			pexprOuterChild->DeriveOutputColumns()) &&
			"detected outer references in NL inner child");
#endif	// GPOS_DEBUG

	EdxlJoinType join_type = EdxljtSentinel;
	BOOL is_index_nlj = false;
	gpos::pointer<CColRefArray *> outer_refs = nullptr;
	switch (pop->Eopid())
	{
		case COperator::EopPhysicalInnerNLJoin:
			join_type = EdxljtInner;
			break;

		case COperator::EopPhysicalInnerIndexNLJoin:
			join_type = EdxljtInner;
			is_index_nlj = true;
			StoreIndexNLJOuterRefs(pop);
			outer_refs = gpos::dyn_cast<CPhysicalInnerIndexNLJoin>(pop)
							 ->PdrgPcrOuterRefs();
			break;

		case COperator::EopPhysicalLeftOuterIndexNLJoin:
			join_type = EdxljtLeft;
			is_index_nlj = true;
			StoreIndexNLJOuterRefs(pop);
			outer_refs = gpos::dyn_cast<CPhysicalLeftOuterIndexNLJoin>(pop)
							 ->PdrgPcrOuterRefs();
			break;

		case COperator::EopPhysicalLeftOuterNLJoin:
			join_type = EdxljtLeft;
			break;

		case COperator::EopPhysicalLeftSemiNLJoin:
			join_type = EdxljtIn;
			break;

		case COperator::EopPhysicalLeftAntiSemiNLJoin:
			join_type = EdxljtLeftAntiSemijoin;
			break;

		case COperator::EopPhysicalLeftAntiSemiNLJoinNotIn:
			join_type = EdxljtLeftAntiSemijoinNotIn;
			break;

		default:
			GPOS_ASSERT(!"Invalid join type");
	}

	// translate relational child expressions
	gpos::owner<CDXLNode *> pdxlnOuterChild = CreateDXLNode(
		pexprOuterChild, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	gpos::owner<CDXLNode *> pdxlnInnerChild = CreateDXLNode(
		pexprInnerChild, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	gpos::owner<CDXLNode *> pdxlnCond = PdxlnScalar(pexprScalar);

	gpos::owner<CDXLNode *> dxlnode_join_filter = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarJoinFilter(m_mp));
	if (nullptr != pdxlnCond)
	{
		dxlnode_join_filter->AddChild(pdxlnCond);
	}

	BOOL nest_params_exists = false;
	gpos::owner<CDXLColRefArray *> col_refs = nullptr;
	if (is_index_nlj && GPOS_FTRACE(EopttraceIndexedNLJOuterRefAsParams))
	{
		nest_params_exists = true;
		col_refs = GPOS_NEW(m_mp) CDXLColRefArray(m_mp);
		for (ULONG ul = 0; ul < outer_refs->Size(); ul++)
		{
			CColRef *col_ref = (*outer_refs)[ul];
			CMDName *md_name =
				GPOS_NEW(m_mp) CMDName(m_mp, col_ref->Name().Pstr());
			gpos::owner<IMDId *> mdid = col_ref->RetrieveType()->MDId();
			mdid->AddRef();
			gpos::owner<CDXLColRef *> colref_dxl = GPOS_NEW(m_mp) CDXLColRef(
				md_name, col_ref->Id(), mdid, col_ref->TypeModifier());
			col_refs->Append(colref_dxl);
		}
	}

	// construct a join node
	gpos::owner<CDXLPhysicalNLJoin *> pdxlopNLJ = GPOS_NEW(m_mp)
		CDXLPhysicalNLJoin(m_mp, join_type, is_index_nlj, nest_params_exists);
	pdxlopNLJ->SetNestLoopParamsColRefs(std::move(col_refs));

	// construct projection list
	// compute required columns
	GPOS_ASSERT(nullptr != pexprInnerNLJ->Prpp());
	gpos::pointer<CColRefSet *> pcrsOutput =
		pexprInnerNLJ->Prpp()->PcrsRequired();

	gpos::owner<CDXLNode *> proj_list_dxlnode =
		PdxlnProjList(pcrsOutput, colref_array);

	gpos::owner<CDXLNode *> pdxlnNLJ =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopNLJ));
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprInnerNLJ);
	pdxlnNLJ->SetProperties(std::move(dxl_properties));

	// construct an empty plan filter
	gpos::owner<CDXLNode *> filter_dxlnode = PdxlnFilter(nullptr);

	// add children
	pdxlnNLJ->AddChild(std::move(proj_list_dxlnode));
	pdxlnNLJ->AddChild(std::move(filter_dxlnode));
	pdxlnNLJ->AddChild(std::move(dxlnode_join_filter));
	pdxlnNLJ->AddChild(std::move(pdxlnOuterChild));
	pdxlnNLJ->AddChild(std::move(pdxlnInnerChild));

#ifdef REFCOUNT_FIXME_ASSERT_AFTER_MOVE
#ifdef GPOS_DEBUG
	pdxlopNLJ->AssertValid(pdxlnNLJ, false /* validate_children */);
#endif
#endif

	return pdxlnNLJ;
}

gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnMergeJoin(gpos::pointer<CExpression *> pexprMJ,
									 gpos::pointer<CColRefArray *> colref_array,
									 CDistributionSpecArray *pdrgpdsBaseTables,
									 ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprMJ);
	GPOS_ASSERT(3 == pexprMJ->Arity());

	// extract components
	gpos::pointer<CPhysical *> pop = gpos::dyn_cast<CPhysical>(pexprMJ->Pop());

	CExpression *pexprOuterChild = (*pexprMJ)[0];
	CExpression *pexprInnerChild = (*pexprMJ)[1];
	gpos::pointer<CExpression *> pexprScalar = (*pexprMJ)[2];

	EdxlJoinType join_type = EdxljtSentinel;
	switch (pop->Eopid())
	{
		case COperator::EopPhysicalFullMergeJoin:
			join_type = EdxljtFull;
			break;

		default:
			GPOS_ASSERT(!"Invalid join type");
	}

	// translate relational child expressions
	gpos::owner<CDXLNode *> pdxlnOuterChild = CreateDXLNode(
		pexprOuterChild, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	gpos::owner<CDXLNode *> pdxlnInnerChild = CreateDXLNode(
		pexprInnerChild, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	gpos::owner<CDXLNode *> dxlnode_merge_conds = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarMergeCondList(m_mp));

	gpos::owner<CExpressionArray *> pdrgpexprPredicates =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprScalar);
	const ULONG length = pdrgpexprPredicates->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::owner<CExpression *> pexprPred = (*pdrgpexprPredicates)[ul];
		// At this point, they all better be merge joinable
		GPOS_ASSERT(CPhysicalJoin::FMergeJoinCompatible(
			pexprPred, pexprOuterChild, pexprInnerChild));
		CExpression *pexprPredOuter = (*pexprPred)[0];
		CExpression *pexprPredInner = (*pexprPred)[1];

		// align extracted columns with outer and inner children of the join
		gpos::pointer<CColRefSet *> pcrsOuterChild =
			pexprOuterChild->DeriveOutputColumns();
		gpos::pointer<CColRefSet *> pcrsPredInner =
			pexprPredInner->DeriveUsedColumns();
#ifdef GPOS_DEBUG
		gpos::pointer<CColRefSet *> pcrsInnerChild =
			pexprInnerChild->DeriveOutputColumns();
		CColRefSet *pcrsPredOuter = pexprPredOuter->DeriveUsedColumns();
#endif

		if (pcrsOuterChild->ContainsAll(pcrsPredInner))
		{
			GPOS_ASSERT(pcrsInnerChild->ContainsAll(pcrsPredOuter));
			std::swap(pexprPredOuter, pexprPredInner);
#ifdef GPOS_DEBUG
			std::swap(pcrsPredOuter, pcrsPredInner);
#endif

			pexprPredOuter->AddRef();
			pexprPredInner->AddRef();
			pexprPred =
				CUtils::PexprScalarEqCmp(m_mp, pexprPredOuter, pexprPredInner);
		}
		else
		{
			pexprPred->AddRef();
		}

		GPOS_ASSERT(pcrsOuterChild->ContainsAll(pcrsPredOuter) &&
					pcrsInnerChild->ContainsAll(pcrsPredInner) &&
					"merge join keys are not aligned with children");

		dxlnode_merge_conds->AddChild(PdxlnScalar(pexprPred));
		pexprPred->Release();
	}
	pdrgpexprPredicates->Release();

	// construct a join node
	gpos::owner<CDXLPhysicalMergeJoin *> pdxlopMJ = GPOS_NEW(m_mp)
		CDXLPhysicalMergeJoin(m_mp, join_type, false /* is_unique_outer */);

	// construct projection list
	// compute required columns
	GPOS_ASSERT(nullptr != pexprMJ->Prpp());
	gpos::pointer<CColRefSet *> pcrsOutput = pexprMJ->Prpp()->PcrsRequired();

	gpos::owner<CDXLNode *> proj_list_dxlnode =
		PdxlnProjList(pcrsOutput, colref_array);

	gpos::owner<CDXLNode *> pdxlnMJ =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopMJ));
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprMJ);
	pdxlnMJ->SetProperties(std::move(dxl_properties));

	// construct an empty plan filter and join filter
	gpos::owner<CDXLNode *> filter_dxlnode = PdxlnFilter(nullptr);
	gpos::owner<CDXLNode *> dxlnode_join_filter = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarJoinFilter(m_mp));

	// add children
	pdxlnMJ->AddChild(std::move(proj_list_dxlnode));
	pdxlnMJ->AddChild(std::move(filter_dxlnode));
	pdxlnMJ->AddChild(std::move(dxlnode_join_filter));
	pdxlnMJ->AddChild(std::move(dxlnode_merge_conds));
	pdxlnMJ->AddChild(std::move(pdxlnOuterChild));
	pdxlnMJ->AddChild(std::move(pdxlnInnerChild));

#ifdef GPOS_DEBUG
	pdxlnMJ->AssertValid(false /* validate_children */);
#endif

	return pdxlnMJ;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::EdxljtHashJoin
//
//	@doc:
//		Return hash join type
//---------------------------------------------------------------------------
EdxlJoinType
CTranslatorExprToDXL::EdxljtHashJoin(gpos::pointer<CPhysicalHashJoin *> popHJ)
{
	GPOS_ASSERT(CUtils::FHashJoin(popHJ));

	switch (popHJ->Eopid())
	{
		case COperator::EopPhysicalInnerHashJoin:
			return EdxljtInner;

		case COperator::EopPhysicalLeftOuterHashJoin:
			return EdxljtLeft;

		case COperator::EopPhysicalRightOuterHashJoin:
			return EdxljtRight;

		case COperator::EopPhysicalLeftSemiHashJoin:
			return EdxljtIn;

		case COperator::EopPhysicalLeftAntiSemiHashJoin:
			return EdxljtLeftAntiSemijoin;

		case COperator::EopPhysicalLeftAntiSemiHashJoinNotIn:
			return EdxljtLeftAntiSemijoinNotIn;

		default:
			GPOS_ASSERT(!"Invalid join type");
			return EdxljtSentinel;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnHashJoin
//
//	@doc:
//		Create a DXL hash join node from an optimizer hash join expression.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnHashJoin(gpos::pointer<CExpression *> pexprHJ,
									gpos::pointer<CColRefArray *> colref_array,
									CDistributionSpecArray *pdrgpdsBaseTables,
									ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprHJ);

	GPOS_ASSERT(3 == pexprHJ->Arity());

	// extract components
	gpos::pointer<CPhysicalHashJoin *> popHJ =
		gpos::dyn_cast<CPhysicalHashJoin>(pexprHJ->Pop());
	CExpression *pexprOuterChild = (*pexprHJ)[0];
	CExpression *pexprInnerChild = (*pexprHJ)[1];
	gpos::pointer<CExpression *> pexprScalar = (*pexprHJ)[2];

	EdxlJoinType join_type = EdxljtHashJoin(popHJ);
	GPOS_ASSERT(popHJ->PdrgpexprOuterKeys()->Size() ==
				popHJ->PdrgpexprInnerKeys()->Size());

	// translate relational child expression
	gpos::owner<CDXLNode *> pdxlnOuterChild = CreateDXLNode(
		pexprOuterChild, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	gpos::owner<CDXLNode *> pdxlnInnerChild = CreateDXLNode(
		pexprInnerChild, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// construct hash condition
	gpos::owner<CDXLNode *> pdxlnHashCondList = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarHashCondList(m_mp));

#ifdef GPOS_DEBUG
	ULONG ulHashJoinPreds = 0;
#endif

	gpos::owner<CExpressionArray *> pdrgpexprPredicates =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprScalar);
	gpos::owner<CExpressionArray *> pdrgpexprRemainingPredicates =
		GPOS_NEW(m_mp) CExpressionArray(m_mp);
	const ULONG size = pdrgpexprPredicates->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		gpos::owner<CExpression *> pexprPred = (*pdrgpexprPredicates)[ul];
		if (CPhysicalJoin::FHashJoinCompatible(pexprPred, pexprOuterChild,
											   pexprInnerChild))
		{
			CExpression *pexprPredOuter;
			CExpression *pexprPredInner;
			IMDId *mdid_scop;
			CPhysicalJoin::AlignJoinKeyOuterInner(
				pexprPred, pexprOuterChild, pexprInnerChild, &pexprPredOuter,
				&pexprPredInner, &mdid_scop);

			pexprPredOuter->AddRef();
			pexprPredInner->AddRef();
			// create hash join predicate based on conjunct type
			if (CPredicateUtils::IsEqualityOp(pexprPred))
			{
				pexprPred = CUtils::PexprScalarCmp(m_mp, pexprPredOuter,
												   pexprPredInner, mdid_scop);
			}
			else
			{
				GPOS_ASSERT(CPredicateUtils::FINDF(pexprPred));
				pexprPred = CUtils::PexprINDF(m_mp, pexprPredOuter,
											  pexprPredInner, mdid_scop);
			}

			gpos::owner<CDXLNode *> pdxlnPred = PdxlnScalar(pexprPred);
			pdxlnHashCondList->AddChild(pdxlnPred);
			pexprPred->Release();
#ifdef GPOS_DEBUG
			ulHashJoinPreds++;
#endif	// GPOS_DEBUG
		}
		else
		{
			pexprPred->AddRef();
			pdrgpexprRemainingPredicates->Append(pexprPred);
		}
	}
	GPOS_ASSERT(popHJ->PdrgpexprOuterKeys()->Size() == ulHashJoinPreds);

	gpos::owner<CDXLNode *> dxlnode_join_filter = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarJoinFilter(m_mp));
	if (0 < pdrgpexprRemainingPredicates->Size())
	{
		gpos::owner<CExpression *> pexprJoinCond =
			CPredicateUtils::PexprConjunction(m_mp,
											  pdrgpexprRemainingPredicates);
		gpos::owner<CDXLNode *> pdxlnJoinCond = PdxlnScalar(pexprJoinCond);
		dxlnode_join_filter->AddChild(pdxlnJoinCond);
		pexprJoinCond->Release();
	}
	else
	{
		pdrgpexprRemainingPredicates->Release();
	}

	// construct a hash join node
	gpos::owner<CDXLPhysicalHashJoin *> pdxlopHJ =
		GPOS_NEW(m_mp) CDXLPhysicalHashJoin(m_mp, join_type);

	// construct projection list from required columns
	GPOS_ASSERT(nullptr != pexprHJ->Prpp());
	gpos::pointer<CColRefSet *> pcrsOutput = pexprHJ->Prpp()->PcrsRequired();
	gpos::owner<CDXLNode *> proj_list_dxlnode =
		PdxlnProjList(pcrsOutput, colref_array);

	gpos::owner<CDXLNode *> pdxlnHJ =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopHJ));
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprHJ);
	pdxlnHJ->SetProperties(std::move(dxl_properties));

	// construct an empty plan filter
	gpos::owner<CDXLNode *> filter_dxlnode = PdxlnFilter(nullptr);

	// add children
	pdxlnHJ->AddChild(std::move(proj_list_dxlnode));
	pdxlnHJ->AddChild(std::move(filter_dxlnode));
	pdxlnHJ->AddChild(std::move(dxlnode_join_filter));
	pdxlnHJ->AddChild(std::move(pdxlnHashCondList));
	pdxlnHJ->AddChild(std::move(pdxlnOuterChild));
	pdxlnHJ->AddChild(std::move(pdxlnInnerChild));

	// cleanup
	pdrgpexprPredicates->Release();

#ifdef REFCOUNT_FIXME_ASSERT_AFTER_MOVE
#ifdef GPOS_DEBUG
	pdxlopHJ->AssertValid(pdxlnHJ, false /* validate_children */);
#endif
#endif

	return pdxlnHJ;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnMotion
//
//	@doc:
//		Create a DXL motion node from an optimizer motion expression
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnMotion(gpos::pointer<CExpression *> pexprMotion,
								  CColRefArray *colref_array,
								  CDistributionSpecArray *pdrgpdsBaseTables,
								  ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprMotion);
	GPOS_ASSERT(1 == pexprMotion->Arity());

	// extract components
	CExpression *pexprChild = (*pexprMotion)[0];

	// translate relational child expression
	gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
		pexprChild, colref_array, pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
		true /*fRemap*/, false /*fRoot*/);

	// construct a motion node
	gpos::owner<CDXLPhysicalMotion *> motion = nullptr;
	BOOL fDuplicateHazardMotion = CUtils::FDuplicateHazardMotion(pexprMotion);
	switch (pexprMotion->Pop()->Eopid())
	{
		case COperator::EopPhysicalMotionGather:
			motion = GPOS_NEW(m_mp) CDXLPhysicalGatherMotion(m_mp);
			break;

		case COperator::EopPhysicalMotionBroadcast:
			motion = GPOS_NEW(m_mp) CDXLPhysicalBroadcastMotion(m_mp);
			break;

		case COperator::EopPhysicalMotionHashDistribute:
			// If child is tainted-replicated, then we cannot use a result hash
			// filter node because the values on each segment are not guaranteed
			// to be identical.
			//
			// Example of tainted-replication: a scan of a replicated table that
			// contains the values (10), (20) along with a project of nextval()
			// can produce the following tuples:
			//
			//       rep-val  nextval
			// Seg1:      10       1
			//            20       2
			//
			// Seg2:      10       3
			//            20       4
			//
			// Seg3:      10       5
			//            20       6
			//
			// If nextval is used in a hash filter we could get missing or
			// duplicate results because we're not guaranteed that 1, 3, and 5
			// (coresponding to value 10) will produce same hashed segment
			// value. Same for 2, 4, and 6 (corresponding to value 20).
			//
			// CDXLPhysicalRedistributeMotion::is_duplicate_sensitive is set to
			// decide whether the motion should be translated into result hash
			// filter node or redistribute motion.
			motion = GPOS_NEW(m_mp)
				CDXLPhysicalRedistributeMotion(m_mp, fDuplicateHazardMotion);
			break;

		case COperator::EopPhysicalMotionRandom:
			motion = GPOS_NEW(m_mp)
				CDXLPhysicalRandomMotion(m_mp, fDuplicateHazardMotion);
			break;

		case COperator::EopPhysicalMotionRoutedDistribute:
		{
			gpos::pointer<CPhysicalMotionRoutedDistribute *> popMotion =
				gpos::dyn_cast<CPhysicalMotionRoutedDistribute>(
					pexprMotion->Pop());
			CColRef *pcrSegmentId =
				dynamic_cast<const CDistributionSpecRouted *>(popMotion->Pds())
					->Pcr();

			motion = GPOS_NEW(m_mp)
				CDXLPhysicalRoutedDistributeMotion(m_mp, pcrSegmentId->Id());
			break;
		}
		default:
			GPOS_ASSERT(!"Unrecognized motion type");
	}

	if (COperator::EopPhysicalMotionGather != pexprMotion->Pop()->Eopid())
	{
		(*pulNonGatherMotions)++;
	}

	GPOS_ASSERT(nullptr != motion);

	// construct project list from child project list
	GPOS_ASSERT(nullptr != child_dxlnode && 1 <= child_dxlnode->Arity());
	gpos::pointer<CDXLNode *> pdxlnProjListChild = (*child_dxlnode)[0];

	gpos::owner<CDXLNode *> proj_list_dxlnode =
		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
			m_mp, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	// set input and output segment information
	motion->SetSegmentInfo(GetInputSegIdsArray(pexprMotion),
						   GetOutputSegIdsArray(pexprMotion));

	gpos::owner<CDXLNode *> pdxlnMotion = GPOS_NEW(m_mp) CDXLNode(m_mp, motion);
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprMotion);
	pdxlnMotion->SetProperties(dxl_properties);

	// construct an empty filter node
	gpos::owner<CDXLNode *> filter_dxlnode = PdxlnFilter(nullptr /*pdxlnCond*/);

	// construct sort column list
	gpos::owner<CDXLNode *> sort_col_list_dxlnode =
		GetSortColListDXL(pexprMotion);

	// add children
	pdxlnMotion->AddChild(proj_list_dxlnode);
	pdxlnMotion->AddChild(filter_dxlnode);
	pdxlnMotion->AddChild(sort_col_list_dxlnode);

	if (COperator::EopPhysicalMotionHashDistribute ==
		pexprMotion->Pop()->Eopid())
	{
		// construct a hash expr list node
		gpos::pointer<CPhysicalMotionHashDistribute *> popHashDistribute =
			gpos::dyn_cast<CPhysicalMotionHashDistribute>(pexprMotion->Pop());
		gpos::pointer<CDistributionSpecHashed *> pdsHashed =
			gpos::dyn_cast<CDistributionSpecHashed>(popHashDistribute->Pds());
		gpos::owner<CDXLNode *> hash_expr_list =
			PdxlnHashExprList(pdsHashed->Pdrgpexpr(), pdsHashed->Opfamilies());
		pdxlnMotion->AddChild(hash_expr_list);
	}

	pdxlnMotion->AddChild(std::move(child_dxlnode));

#ifdef GPOS_DEBUG
	motion->AssertValid(pdxlnMotion, false /* validate_children */);
#endif

	return pdxlnMotion;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnMaterialize
//
//	@doc:
//		Create a DXL materialize node from an optimizer spool expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnMaterialize(
	gpos::pointer<CExpression *> pexprSpool, CColRefArray *colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprSpool);

	GPOS_ASSERT(1 == pexprSpool->Arity());

	// extract components
	CExpression *pexprChild = (*pexprSpool)[0];

	// translate relational child expression
	gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
		pexprChild, colref_array, pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
		false /*fRemap*/, false /*fRoot*/);

	gpos::pointer<CPhysicalSpool *> spool =
		gpos::dyn_cast<CPhysicalSpool>(pexprSpool->Pop());

	// construct a materialize node
	gpos::owner<CDXLPhysicalMaterialize *> pdxlopMat =
		GPOS_NEW(m_mp) CDXLPhysicalMaterialize(m_mp, spool->FEager());

	// construct project list from child project list
	GPOS_ASSERT(nullptr != child_dxlnode && 1 <= child_dxlnode->Arity());
	gpos::pointer<CDXLNode *> pdxlnProjListChild = (*child_dxlnode)[0];
	gpos::owner<CDXLNode *> proj_list_dxlnode =
		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
			m_mp, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	gpos::owner<CDXLNode *> pdxlnMaterialize =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopMat));
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprSpool);
	pdxlnMaterialize->SetProperties(std::move(dxl_properties));

	// construct an empty filter node
	gpos::owner<CDXLNode *> filter_dxlnode =
		PdxlnFilter(nullptr /* pdxlnCond */);

	// add children
	pdxlnMaterialize->AddChild(std::move(proj_list_dxlnode));
	pdxlnMaterialize->AddChild(std::move(filter_dxlnode));
	pdxlnMaterialize->AddChild(std::move(child_dxlnode));

#ifdef REFCOUNT_FIXME_ASSERT_AFTER_MOVE
#ifdef GPOS_DEBUG
	pdxlopMat->AssertValid(pdxlnMaterialize, false /* validate_children */);
#endif
#endif

	return pdxlnMaterialize;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnSequence
//
//	@doc:
//		Create a DXL sequence node from an optimizer sequence expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnSequence(gpos::pointer<CExpression *> pexprSequence,
									CColRefArray *colref_array,
									CDistributionSpecArray *pdrgpdsBaseTables,
									ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprSequence);

	const ULONG arity = pexprSequence->Arity();
	GPOS_ASSERT(0 < arity);

	// construct sequence node
	gpos::owner<CDXLPhysicalSequence *> pdxlopSequence =
		GPOS_NEW(m_mp) CDXLPhysicalSequence(m_mp);
	gpos::owner<CDXLNode *> pdxlnSequence =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopSequence);
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprSequence);
	pdxlnSequence->SetProperties(dxl_properties);

	// translate children
	gpos::owner<CDXLNodeArray *> pdrgpdxlnChildren =
		GPOS_NEW(m_mp) CDXLNodeArray(m_mp);

	for (ULONG ul = 0; ul < arity; ul++)
	{
		CExpression *pexprChild = (*pexprSequence)[ul];

		CColRefArray *pdrgpcrChildOutput = nullptr;
		if (ul == arity - 1)
		{
			// impose output columns on last child
			pdrgpcrChildOutput = colref_array;
		}

		gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
			pexprChild, pdrgpcrChildOutput, pdrgpdsBaseTables,
			pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
		pdrgpdxlnChildren->Append(child_dxlnode);
	}

	// construct project list from the project list of the last child
	gpos::pointer<CDXLNode *> pdxlnLastChild = (*pdrgpdxlnChildren)[arity - 1];
	gpos::pointer<CDXLNode *> pdxlnProjListChild = (*pdxlnLastChild)[0];

	gpos::owner<CDXLNode *> proj_list_dxlnode =
		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
			m_mp, m_pcf, m_phmcrdxln, pdxlnProjListChild);
	pdxlnSequence->AddChild(proj_list_dxlnode);

	// add children
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::owner<CDXLNode *> pdxlnChid = (*pdrgpdxlnChildren)[ul];
		pdxlnChid->AddRef();
		pdxlnSequence->AddChild(pdxlnChid);
	}

	pdrgpdxlnChildren->Release();

#ifdef GPOS_DEBUG
	pdxlopSequence->AssertValid(pdxlnSequence, false /* validate_children */);
#endif

	return pdxlnSequence;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnPartitionSelector
//
//	@doc:
//		Translate a partition selector into DXL
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnPartitionSelector(
	gpos::pointer<CExpression *> pexpr, CColRefArray *colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
	gpos::pointer<CPhysicalPartitionSelector *> popSelector =
		gpos::dyn_cast<CPhysicalPartitionSelector>(pexpr->Pop());

	CExpression *pexprChild = (*pexpr)[0];

	// translate child
	gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
		pexprChild, colref_array, pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
		false /*fRemap*/, false /*fRoot*/);

	gpos::pointer<CDXLNode *> pdxlnPrLChild = (*child_dxlnode)[0];
	gpos::owner<CDXLNode *> pdxlnPrL =
		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
			m_mp, m_pcf, m_phmcrdxln, pdxlnPrLChild);
	const ULONG scanid = popSelector->ScanId();

	gpos::pointer<CBitSet *> bs =
		COptCtxt::PoctxtFromTLS()->GetPartitionsForScanId(scanid);
	GPOS_ASSERT(nullptr != bs);
	gpos::owner<ULongPtrArray *> parts = GPOS_NEW(m_mp) ULongPtrArray(m_mp);
	CBitSetIter bsi(*bs);
	for (ULONG ul = 0; bsi.Advance(); ul++)
	{
		parts->Append(GPOS_NEW(m_mp) ULONG(bsi.Bit()));
	}

	popSelector->MDId()->AddRef();
	gpos::owner<CDXLNode *> pdxlnSelector = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLPhysicalPartitionSelector(
						   m_mp, popSelector->MDId(), popSelector->SelectorId(),
						   popSelector->ScanId(), std::move(parts)));

	gpos::owner<CDXLNode *> pdxlnFilter =
		PdxlnScalar(popSelector->FilterExpr());
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprChild);

	pdxlnSelector->SetProperties(std::move(dxl_properties));
	pdxlnSelector->AddChild(std::move(pdxlnPrL));
	pdxlnSelector->AddChild(std::move(pdxlnFilter));
	pdxlnSelector->AddChild(std::move(child_dxlnode));

	return pdxlnSelector;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDML
//
//	@doc:
//		Translate a DML operator
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnDML(gpos::pointer<CExpression *> pexpr,
							   gpos::pointer<CColRefArray *>,  // colref_array
							   CDistributionSpecArray *pdrgpdsBaseTables,
							   ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(1 == pexpr->Arity());

	ULONG action_colid = 0;
	ULONG oid_colid = 0;
	ULONG ctid_colid = 0;
	ULONG segid_colid = 0;

	// extract components
	gpos::pointer<CPhysicalDML *> popDML =
		gpos::dyn_cast<CPhysicalDML>(pexpr->Pop());
	*pfDML = false;
	if (IMDId::EmdidGPDBCtas == popDML->Ptabdesc()->MDId()->MdidType())
	{
		return PdxlnCTAS(pexpr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
	}

	EdxlDmlType dxl_dml_type = Edxldmloptype(popDML->Edmlop());

	CExpression *pexprChild = (*pexpr)[0];
	gpos::pointer<CTableDescriptor *> ptabdesc = popDML->Ptabdesc();
	CColRefArray *pdrgpcrSource = popDML->PdrgpcrSource();

	CColRef *pcrAction = popDML->PcrAction();
	GPOS_ASSERT(nullptr != pcrAction);
	action_colid = pcrAction->Id();

	CColRef *pcrOid = popDML->PcrTableOid();
	if (pcrOid != nullptr)
	{
		oid_colid = pcrOid->Id();
	}

	CColRef *pcrCtid = popDML->PcrCtid();
	CColRef *pcrSegmentId = popDML->PcrSegmentId();
	if (nullptr != pcrCtid)
	{
		GPOS_ASSERT(nullptr != pcrSegmentId);
		ctid_colid = pcrCtid->Id();
		segid_colid = pcrSegmentId->Id();
	}

	CColRef *pcrTupleOid = popDML->PcrTupleOid();
	ULONG tuple_oid = 0;
	BOOL preserve_oids = false;
	if (nullptr != pcrTupleOid)
	{
		preserve_oids = true;
		tuple_oid = pcrTupleOid->Id();
	}

	gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
		pexprChild, pdrgpcrSource, pdrgpdsBaseTables, pulNonGatherMotions,
		pfDML, false /*fRemap*/, false /*fRoot*/);

	gpos::owner<CDXLTableDescr *> table_descr = MakeDXLTableDescr(
		ptabdesc, nullptr /*pdrgpcrOutput*/, nullptr /*requiredProperties*/);
	gpos::owner<ULongPtrArray *> pdrgpul = CUtils::Pdrgpul(m_mp, pdrgpcrSource);

	gpos::owner<CDXLDirectDispatchInfo *> dxl_direct_dispatch_info =
		GetDXLDirectDispatchInfo(pexpr);
	gpos::owner<CDXLPhysicalDML *> pdxlopDML = GPOS_NEW(m_mp) CDXLPhysicalDML(
		m_mp, dxl_dml_type, std::move(table_descr), std::move(pdrgpul),
		action_colid, oid_colid, ctid_colid, segid_colid, preserve_oids,
		tuple_oid, std::move(dxl_direct_dispatch_info),
		popDML->IsInputSortReq());

	// project list
	gpos::pointer<CColRefSet *> pcrsOutput = pexpr->Prpp()->PcrsRequired();
	gpos::owner<CDXLNode *> pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrSource);

	gpos::owner<CDXLNode *> pdxlnDML =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopDML));
	gpos::owner<CDXLPhysicalProperties *> dxl_properties = GetProperties(pexpr);
	pdxlnDML->SetProperties(std::move(dxl_properties));

	pdxlnDML->AddChild(std::move(pdxlnPrL));
	pdxlnDML->AddChild(std::move(child_dxlnode));

#ifdef GPOS_DEBUG
	pdxlnDML->GetOperator()->AssertValid(pdxlnDML,
										 false /* validate_children */);
#endif
	*pfDML = true;

	return pdxlnDML;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnCTAS
//
//	@doc:
//		Translate a CTAS expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnCTAS(gpos::pointer<CExpression *> pexpr,
								CDistributionSpecArray *pdrgpdsBaseTables,
								ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(1 == pexpr->Arity());

	gpos::pointer<CPhysicalDML *> popDML =
		gpos::dyn_cast<CPhysicalDML>(pexpr->Pop());
	GPOS_ASSERT(CLogicalDML::EdmlInsert == popDML->Edmlop());

	CExpression *pexprChild = (*pexpr)[0];
	gpos::pointer<CTableDescriptor *> ptabdesc = popDML->Ptabdesc();
	CColRefArray *pdrgpcrSource = popDML->PdrgpcrSource();
	gpos::pointer<CMDRelationCtasGPDB *> pmdrel =
		const_cast<CMDRelationCtasGPDB *>(gpos::cast<CMDRelationCtasGPDB>(
			m_pmda->RetrieveRel(ptabdesc->MDId())));

	gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
		pexprChild, pdrgpcrSource, pdrgpdsBaseTables, pulNonGatherMotions,
		pfDML, true /*fRemap*/, true /*fRoot*/);

	gpos::owner<ULongPtrArray *> pdrgpul = CUtils::Pdrgpul(m_mp, pdrgpcrSource);

	pmdrel->GetDxlCtasStorageOption()->AddRef();

	const ULONG ulColumns = ptabdesc->ColumnCount();

	IntPtrArray *vartypemod_array = pmdrel->GetVarTypeModArray();
	GPOS_ASSERT(ulColumns == vartypemod_array->Size());

	// translate col descriptors
	gpos::owner<CDXLColDescrArray *> dxl_col_descr_array =
		GPOS_NEW(m_mp) CDXLColDescrArray(m_mp);
	for (ULONG ul = 0; ul < ulColumns; ul++)
	{
		gpos::pointer<const CColumnDescriptor *> pcd = ptabdesc->Pcoldesc(ul);

		CMDName *pmdnameCol = GPOS_NEW(m_mp) CMDName(m_mp, pcd->Name().Pstr());
		CColRef *colref = m_pcf->PcrCreate(pcd->RetrieveType(),
										   pcd->TypeModifier(), pcd->Name());

		// use the col ref id for the corresponding output output column as
		// colid for the dxl column
		gpos::owner<CMDIdGPDB *> pmdidColType =
			gpos::dyn_cast<CMDIdGPDB>(colref->RetrieveType()->MDId());
		pmdidColType->AddRef();

		gpos::owner<CDXLColDescr *> pdxlcd = GPOS_NEW(m_mp) CDXLColDescr(
			pmdnameCol, colref->Id(), pcd->AttrNum(), pmdidColType,
			colref->TypeModifier(), false /* fdropped */, pcd->Width());

		dxl_col_descr_array->Append(pdxlcd);
	}

	gpos::owner<ULongPtrArray *> pdrgpulDistr = nullptr;
	if (IMDRelation::EreldistrHash == pmdrel->GetRelDistribution())
	{
		pdrgpulDistr = GPOS_NEW(m_mp) ULongPtrArray(m_mp);
		const ULONG ulDistrCols = pmdrel->DistrColumnCount();
		for (ULONG ul = 0; ul < ulDistrCols; ul++)
		{
			gpos::pointer<const IMDColumn *> pmdcol = pmdrel->GetDistrColAt(ul);
			INT attno = pmdcol->AttrNum();
			GPOS_ASSERT(0 < attno);
			pdrgpulDistr->Append(GPOS_NEW(m_mp) ULONG(attno - 1));
		}
	}

	CMDName *mdname_schema = nullptr;
	if (nullptr != pmdrel->GetMdNameSchema())
	{
		mdname_schema = GPOS_NEW(m_mp)
			CMDName(m_mp, pmdrel->GetMdNameSchema()->GetMDName());
	}

	gpos::owner<IMdIdArray *> distr_opclasses = pmdrel->GetDistrOpClasses();
	distr_opclasses->AddRef();

	vartypemod_array->AddRef();
	gpos::owner<CDXLPhysicalCTAS *> pdxlopCTAS =
		GPOS_NEW(m_mp) CDXLPhysicalCTAS(
			m_mp, mdname_schema,
			GPOS_NEW(m_mp) CMDName(m_mp, pmdrel->Mdname().GetMDName()),
			std::move(dxl_col_descr_array), pmdrel->GetDxlCtasStorageOption(),
			pmdrel->GetRelDistribution(), std::move(pdrgpulDistr),
			pmdrel->GetDistrOpClasses(), pmdrel->IsTemporary(),
			pmdrel->HasOids(), pmdrel->RetrieveRelStorageType(),
			std::move(pdrgpul), vartypemod_array);

	gpos::owner<CDXLNode *> pdxlnCTAS =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopCTAS));
	gpos::owner<CDXLPhysicalProperties *> dxl_properties = GetProperties(pexpr);
	pdxlnCTAS->SetProperties(std::move(dxl_properties));

	gpos::pointer<CColRefSet *> pcrsOutput = pexpr->Prpp()->PcrsRequired();
	gpos::owner<CDXLNode *> pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrSource);

	pdxlnCTAS->AddChild(std::move(pdxlnPrL));
	pdxlnCTAS->AddChild(std::move(child_dxlnode));

#ifdef GPOS_DEBUG
	pdxlnCTAS->GetOperator()->AssertValid(pdxlnCTAS,
										  false /* validate_children */);
#endif
	return pdxlnCTAS;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::GetDXLDirectDispatchInfo
//
//	@doc:
//		Return the direct dispatch info spec for the possible values of the distribution
//		key in a DML insert statement. Returns NULL if values are not constant.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLDirectDispatchInfo *>
CTranslatorExprToDXL::GetDXLDirectDispatchInfo(
	gpos::pointer<CExpression *> pexprDML)
{
	GPOS_ASSERT(nullptr != pexprDML);

	gpos::pointer<CPhysicalDML *> popDML =
		gpos::dyn_cast<CPhysicalDML>(pexprDML->Pop());
	gpos::pointer<CTableDescriptor *> ptabdesc = popDML->Ptabdesc();
	gpos::pointer<const CColumnDescriptorArray *> pdrgpcoldescDist =
		ptabdesc->PdrgpcoldescDist();

	if (CLogicalDML::EdmlInsert != popDML->Edmlop() ||
		IMDRelation::EreldistrHash != ptabdesc->GetRelDistribution() ||
		1 < pdrgpcoldescDist->Size())
	{
		// directed dispatch only supported for insert statements on hash-distributed tables
		// with a single distribution column
		return nullptr;
	}


	GPOS_ASSERT(1 == pdrgpcoldescDist->Size());
	gpos::pointer<CColumnDescriptor *> pcoldesc = (*pdrgpcoldescDist)[0];
	ULONG ulPos =
		gpopt::CTableDescriptor::UlPos(pcoldesc, ptabdesc->Pdrgpcoldesc());
	GPOS_ASSERT(ulPos < ptabdesc->Pdrgpcoldesc()->Size() && "Column not found");

	CColRef *pcrDistrCol = (*popDML->PdrgpcrSource())[ulPos];
	gpos::pointer<CPropConstraint *> ppc =
		(*pexprDML)[0]->DerivePropertyConstraint();

	if (nullptr == ppc->Pcnstr())
	{
		return nullptr;
	}

	gpos::owner<CConstraint *> pcnstrDistrCol =
		ppc->Pcnstr()->Pcnstr(m_mp, pcrDistrCol);
	if (!CPredicateUtils::FConstColumn(pcnstrDistrCol, pcrDistrCol))
	{
		CRefCount::SafeRelease(std::move(pcnstrDistrCol));
		return nullptr;
	}

	GPOS_ASSERT(CConstraint::EctInterval == pcnstrDistrCol->Ect());

	gpos::pointer<CConstraintInterval *> pci =
		gpos::dyn_cast<CConstraintInterval>(pcnstrDistrCol);
	GPOS_ASSERT(1 >= pci->Pdrgprng()->Size());

	gpos::owner<CDXLDatumArray *> pdrgpdxldatum =
		GPOS_NEW(m_mp) CDXLDatumArray(m_mp);
	gpos::owner<CDXLDatum *> dxl_datum = nullptr;

	if (1 == pci->Pdrgprng()->Size())
	{
		gpos::pointer<const CRange *> prng = (*pci->Pdrgprng())[0];
		dxl_datum = CTranslatorExprToDXLUtils::GetDatumVal(m_mp, m_pmda,
														   prng->PdatumLeft());
	}
	else
	{
		GPOS_ASSERT(pci->FIncludesNull());
		dxl_datum = pcrDistrCol->RetrieveType()->GetDXLDatumNull(m_mp);
	}

	pdrgpdxldatum->Append(std::move(dxl_datum));

	pcnstrDistrCol->Release();

	gpos::owner<CDXLDatum2dArray *> pdrgpdrgpdxldatum =
		GPOS_NEW(m_mp) CDXLDatum2dArray(m_mp);
	pdrgpdrgpdxldatum->Append(std::move(pdrgpdxldatum));
	return GPOS_NEW(m_mp)
		CDXLDirectDispatchInfo(std::move(pdrgpdrgpdxldatum), false);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnSplit
//
//	@doc:
//		Translate a split operator
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnSplit(
	gpos::pointer<CExpression *> pexpr,
	gpos::pointer<CColRefArray *>,	// colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(2 == pexpr->Arity());

	ULONG action_colid = 0;
	ULONG ctid_colid = 0;
	ULONG segid_colid = 0;
	ULONG tuple_oid = 0;

	// extract components
	gpos::pointer<CPhysicalSplit *> popSplit =
		gpos::dyn_cast<CPhysicalSplit>(pexpr->Pop());

	CExpression *pexprChild = (*pexpr)[0];
	gpos::pointer<CExpression *> pexprProjList = (*pexpr)[1];

	CColRef *pcrAction = popSplit->PcrAction();
	GPOS_ASSERT(nullptr != pcrAction);
	action_colid = pcrAction->Id();

	CColRef *pcrCtid = popSplit->PcrCtid();
	GPOS_ASSERT(nullptr != pcrCtid);
	ctid_colid = pcrCtid->Id();

	CColRef *pcrSegmentId = popSplit->PcrSegmentId();
	GPOS_ASSERT(nullptr != pcrSegmentId);
	segid_colid = pcrSegmentId->Id();

	CColRef *pcrTupleOid = popSplit->PcrTupleOid();
	BOOL preserve_oids = false;
	if (nullptr != pcrTupleOid)
	{
		preserve_oids = true;
		tuple_oid = pcrTupleOid->Id();
	}

	gpos::pointer<CColRefArray *> pdrgpcrDelete = popSplit->PdrgpcrDelete();
	gpos::owner<ULongPtrArray *> delete_colid_array =
		CUtils::Pdrgpul(m_mp, pdrgpcrDelete);

	gpos::pointer<CColRefArray *> pdrgpcrInsert = popSplit->PdrgpcrInsert();
	gpos::owner<ULongPtrArray *> insert_colid_array =
		CUtils::Pdrgpul(m_mp, pdrgpcrInsert);

	gpos::owner<CColRefSet *> pcrsRequired = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsRequired->Include(pdrgpcrInsert);
	pcrsRequired->Include(pdrgpcrDelete);
	gpos::owner<CColRefArray *> pdrgpcrRequired = pcrsRequired->Pdrgpcr(m_mp);

	gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
		pexprChild, pdrgpcrRequired, pdrgpdsBaseTables, pulNonGatherMotions,
		pfDML, true /*fRemap*/, false /*fRoot*/);
	pdrgpcrRequired->Release();
	pcrsRequired->Release();

	gpos::owner<CDXLPhysicalSplit *> pdxlopSplit = GPOS_NEW(m_mp)
		CDXLPhysicalSplit(m_mp, std::move(delete_colid_array),
						  std::move(insert_colid_array), action_colid,
						  ctid_colid, segid_colid, preserve_oids, tuple_oid);

	// project list
	gpos::pointer<CColRefSet *> pcrsOutput = pexpr->Prpp()->PcrsRequired();
	gpos::owner<CDXLNode *> pdxlnPrL =
		PdxlnProjList(pexprProjList, pcrsOutput, pdrgpcrDelete);

	gpos::owner<CDXLNode *> pdxlnSplit =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopSplit));
	gpos::owner<CDXLPhysicalProperties *> dxl_properties = GetProperties(pexpr);
	pdxlnSplit->SetProperties(std::move(dxl_properties));

	pdxlnSplit->AddChild(std::move(pdxlnPrL));
	pdxlnSplit->AddChild(std::move(child_dxlnode));

#ifdef GPOS_DEBUG
	pdxlnSplit->GetOperator()->AssertValid(pdxlnSplit,
										   false /* validate_children */);
#endif
	return pdxlnSplit;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnRowTrigger
//
//	@doc:
//		Translate a row trigger operator
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnRowTrigger(
	gpos::pointer<CExpression *> pexpr,
	gpos::pointer<CColRefArray *>,	// colref_array,
	CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
	BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(1 == pexpr->Arity());

	// extract components
	gpos::pointer<CPhysicalRowTrigger *> popRowTrigger =
		gpos::dyn_cast<CPhysicalRowTrigger>(pexpr->Pop());

	CExpression *pexprChild = (*pexpr)[0];

	gpos::owner<IMDId *> rel_mdid = popRowTrigger->GetRelMdId();
	rel_mdid->AddRef();

	INT type = popRowTrigger->GetType();

	gpos::owner<CColRefSet *> pcrsRequired = GPOS_NEW(m_mp) CColRefSet(m_mp);
	gpos::owner<ULongPtrArray *> colids_old = nullptr;
	gpos::owner<ULongPtrArray *> colids_new = nullptr;

	gpos::pointer<CColRefArray *> pdrgpcrOld = popRowTrigger->PdrgpcrOld();
	if (nullptr != pdrgpcrOld)
	{
		colids_old = CUtils::Pdrgpul(m_mp, pdrgpcrOld);
		pcrsRequired->Include(pdrgpcrOld);
	}

	gpos::pointer<CColRefArray *> pdrgpcrNew = popRowTrigger->PdrgpcrNew();
	if (nullptr != pdrgpcrNew)
	{
		colids_new = CUtils::Pdrgpul(m_mp, pdrgpcrNew);
		pcrsRequired->Include(pdrgpcrNew);
	}

	gpos::owner<CColRefArray *> pdrgpcrRequired = pcrsRequired->Pdrgpcr(m_mp);
	gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
		pexprChild, pdrgpcrRequired, pdrgpdsBaseTables, pulNonGatherMotions,
		pfDML, false /*fRemap*/, false /*fRoot*/);
	pdrgpcrRequired->Release();
	pcrsRequired->Release();

	gpos::owner<CDXLPhysicalRowTrigger *> pdxlopRowTrigger = GPOS_NEW(m_mp)
		CDXLPhysicalRowTrigger(m_mp, rel_mdid, type, colids_old, colids_new);

	// project list
	gpos::pointer<CColRefSet *> pcrsOutput = pexpr->Prpp()->PcrsRequired();
	gpos::owner<CDXLNode *> pdxlnPrL = nullptr;
	if (nullptr != pdrgpcrNew)
	{
		pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrNew);
	}
	else
	{
		pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrOld);
	}

	gpos::owner<CDXLNode *> pdxlnRowTrigger =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopRowTrigger));
	gpos::owner<CDXLPhysicalProperties *> dxl_properties = GetProperties(pexpr);
	pdxlnRowTrigger->SetProperties(std::move(dxl_properties));

	pdxlnRowTrigger->AddChild(std::move(pdxlnPrL));
	pdxlnRowTrigger->AddChild(std::move(child_dxlnode));

#ifdef GPOS_DEBUG
	pdxlnRowTrigger->GetOperator()->AssertValid(pdxlnRowTrigger,
												false /* validate_children */);
#endif
	return pdxlnRowTrigger;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::Edxldmloptype
//
//	@doc:
//		Return the EdxlDmlType for a given DML op type
//
//---------------------------------------------------------------------------
EdxlDmlType
CTranslatorExprToDXL::Edxldmloptype(const CLogicalDML::EDMLOperator edmlop)
{
	switch (edmlop)
	{
		case CLogicalDML::EdmlInsert:
			return Edxldmlinsert;

		case CLogicalDML::EdmlDelete:
			return Edxldmldelete;

		case CLogicalDML::EdmlUpdate:
			return Edxldmlupdate;

		default:
			GPOS_ASSERT(!"Unrecognized DML operation");
			return EdxldmlSentinel;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScCmp
//
//	@doc:
//		Create a DXL scalar comparison node from an optimizer scalar comparison
//		expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScCmp(gpos::pointer<CExpression *> pexprScCmp)
{
	GPOS_ASSERT(nullptr != pexprScCmp);

	// extract components
	gpos::pointer<CExpression *> pexprLeft = (*pexprScCmp)[0];
	gpos::pointer<CExpression *> pexprRight = (*pexprScCmp)[1];

	// translate children expression
	gpos::owner<CDXLNode *> dxlnode_left = PdxlnScalar(pexprLeft);
	gpos::owner<CDXLNode *> dxlnode_right = PdxlnScalar(pexprRight);

	gpos::pointer<CScalarCmp *> popScCmp =
		gpos::dyn_cast<CScalarCmp>(pexprScCmp->Pop());

	GPOS_ASSERT(nullptr != popScCmp);
	GPOS_ASSERT(nullptr != popScCmp->Pstr());
	GPOS_ASSERT(nullptr != popScCmp->Pstr()->GetBuffer());

	// construct a scalar comparison node
	gpos::owner<IMDId *> mdid = popScCmp->MdIdOp();
	mdid->AddRef();

	CWStringConst *str_name =
		GPOS_NEW(m_mp) CWStringConst(m_mp, popScCmp->Pstr()->GetBuffer());

	gpos::owner<CDXLNode *> pdxlnCmp = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarComp(m_mp, std::move(mdid), str_name));

	// add children
	pdxlnCmp->AddChild(std::move(dxlnode_left));
	pdxlnCmp->AddChild(std::move(dxlnode_right));

#ifdef GPOS_DEBUG
	pdxlnCmp->GetOperator()->AssertValid(pdxlnCmp,
										 false /* validate_children */);
#endif

	return pdxlnCmp;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScDistinctCmp
//
//	@doc:
//		Create a DXL scalar distinct comparison node from an optimizer scalar
//		is distinct from expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScDistinctCmp(
	gpos::pointer<CExpression *> pexprScDist)
{
	GPOS_ASSERT(nullptr != pexprScDist);

	// extract components
	gpos::pointer<CExpression *> pexprLeft = (*pexprScDist)[0];
	gpos::pointer<CExpression *> pexprRight = (*pexprScDist)[1];

	// translate children expression
	gpos::owner<CDXLNode *> dxlnode_left = PdxlnScalar(pexprLeft);
	gpos::owner<CDXLNode *> dxlnode_right = PdxlnScalar(pexprRight);

	gpos::pointer<CScalarIsDistinctFrom *> popScIDF =
		gpos::dyn_cast<CScalarIsDistinctFrom>(pexprScDist->Pop());

	// construct a scalar distinct comparison node
	gpos::owner<IMDId *> mdid = popScIDF->MdIdOp();
	mdid->AddRef();

	gpos::owner<CDXLNode *> pdxlnDistCmp = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarDistinctComp(m_mp, std::move(mdid)));

	// add children
	pdxlnDistCmp->AddChild(std::move(dxlnode_left));
	pdxlnDistCmp->AddChild(std::move(dxlnode_right));

#ifdef GPOS_DEBUG
	pdxlnDistCmp->GetOperator()->AssertValid(pdxlnDistCmp,
											 false /* validate_children */);
#endif

	return pdxlnDistCmp;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScOp
//
//	@doc:
//		Create a DXL scalar op expr node from an optimizer scalar op expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScOp(gpos::pointer<CExpression *> pexprOp)
{
	GPOS_ASSERT(nullptr != pexprOp &&
				((1 == pexprOp->Arity()) || (2 == pexprOp->Arity())));
	gpos::pointer<CScalarOp *> pscop =
		gpos::dyn_cast<CScalarOp>(pexprOp->Pop());

	// construct a scalar opexpr node
	CWStringConst *str_name =
		GPOS_NEW(m_mp) CWStringConst(m_mp, pscop->Pstr()->GetBuffer());

	gpos::owner<IMDId *> mdid_op = pscop->MdIdOp();
	mdid_op->AddRef();

	IMDId *return_type_mdid = pscop->GetReturnTypeMdId();
	if (nullptr != return_type_mdid)
	{
		return_type_mdid->AddRef();
	}

	gpos::owner<CDXLNode *> pdxlnOpExpr = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarOpExpr(m_mp, std::move(mdid_op),
											  return_type_mdid, str_name));

	TranslateScalarChildren(pexprOp, pdxlnOpExpr);

#ifdef GPOS_DEBUG
	pdxlnOpExpr->GetOperator()->AssertValid(pdxlnOpExpr,
											false /* validate_children */);
#endif

	return pdxlnOpExpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScBoolExpr
//
//	@doc:
//		Create a DXL scalar bool expression node from an optimizer scalar log op
//		expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScBoolExpr(
	gpos::pointer<CExpression *> pexprScBoolOp)
{
	GPOS_ASSERT(nullptr != pexprScBoolOp);
	gpos::pointer<CScalarBoolOp *> popScBoolOp =
		gpos::dyn_cast<CScalarBoolOp>(pexprScBoolOp->Pop());
	EdxlBoolExprType edxlbooltype = Edxlbooltype(popScBoolOp->Eboolop());

#ifdef GPOS_DEBUG
	if (CScalarBoolOp::EboolopNot == popScBoolOp->Eboolop())
	{
		GPOS_ASSERT(1 == pexprScBoolOp->Arity());
	}
	else
	{
		GPOS_ASSERT(2 <= pexprScBoolOp->Arity());
	}
#endif	// GPOS_DEBUG

	gpos::owner<CDXLNode *> pdxlnBoolExpr = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarBoolExpr(m_mp, edxlbooltype));

	TranslateScalarChildren(pexprScBoolOp, pdxlnBoolExpr);

#ifdef GPOS_DEBUG
	pdxlnBoolExpr->GetOperator()->AssertValid(pdxlnBoolExpr,
											  false /* validate_children */);
#endif

	return pdxlnBoolExpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::Edxlbooltype
//
//	@doc:
//		Return the EdxlBoolExprType for a given scalar logical op type
//
//---------------------------------------------------------------------------
EdxlBoolExprType
CTranslatorExprToDXL::Edxlbooltype(const CScalarBoolOp::EBoolOperator eboolop)
{
	switch (eboolop)
	{
		case CScalarBoolOp::EboolopNot:
			return Edxlnot;

		case CScalarBoolOp::EboolopAnd:
			return Edxland;

		case CScalarBoolOp::EboolopOr:
			return Edxlor;

		default:
			GPOS_ASSERT(!"Unrecognized boolean expression type");
			return EdxlBoolExprTypeSentinel;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScId
//
//	@doc:
//		Create a DXL scalar identifier node from an optimizer scalar id expr.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScId(gpos::pointer<CExpression *> pexprIdent)
{
	GPOS_ASSERT(nullptr != pexprIdent);

	gpos::pointer<CScalarIdent *> popScId =
		gpos::dyn_cast<CScalarIdent>(pexprIdent->Pop());
	CColRef *colref = const_cast<CColRef *>(popScId->Pcr());

	return CTranslatorExprToDXLUtils::PdxlnIdent(
		m_mp, m_phmcrdxln, m_phmcrdxlnIndexLookup, m_phmcrulPartColId, colref);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScFuncExpr
//
//	@doc:
//		Create a DXL scalar func expr node from an optimizer scalar func expr.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScFuncExpr(gpos::pointer<CExpression *> pexprFunc)
{
	GPOS_ASSERT(nullptr != pexprFunc);

	gpos::pointer<CScalarFunc *> popScFunc =
		gpos::dyn_cast<CScalarFunc>(pexprFunc->Pop());

	gpos::owner<IMDId *> mdid_func = popScFunc->FuncMdId();
	mdid_func->AddRef();

	gpos::owner<IMDId *> mdid_return_type = popScFunc->MdidType();
	mdid_return_type->AddRef();

	gpos::pointer<const IMDFunction *> pmdfunc =
		m_pmda->RetrieveFunc(mdid_func);

	gpos::owner<CDXLNode *> pdxlnFuncExpr = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarFuncExpr(
				  m_mp, std::move(mdid_func), std::move(mdid_return_type),
				  popScFunc->TypeModifier(), pmdfunc->ReturnsSet()));

	// translate children
	TranslateScalarChildren(pexprFunc, pdxlnFuncExpr);

	return pdxlnFuncExpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScWindowFuncExpr
//
//	@doc:
//		Create a DXL scalar window ref node from an optimizer scalar window
//		function expr
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScWindowFuncExpr(
	gpos::pointer<CExpression *> pexprWindowFunc)
{
	GPOS_ASSERT(nullptr != pexprWindowFunc);

	gpos::pointer<CScalarWindowFunc *> popScWindowFunc =
		gpos::dyn_cast<CScalarWindowFunc>(pexprWindowFunc->Pop());

	gpos::owner<IMDId *> mdid_func = popScWindowFunc->FuncMdId();
	mdid_func->AddRef();

	gpos::owner<IMDId *> mdid_return_type = popScWindowFunc->MdidType();
	mdid_return_type->AddRef();

	EdxlWinStage dxl_win_stage = Ews(popScWindowFunc->Ews());
	gpos::owner<CDXLScalarWindowRef *> pdxlopWindowref =
		GPOS_NEW(m_mp) CDXLScalarWindowRef(
			m_mp, std::move(mdid_func), std::move(mdid_return_type),
			popScWindowFunc->IsDistinct(), popScWindowFunc->IsStarArg(),
			popScWindowFunc->IsSimpleAgg(), dxl_win_stage,
			0 /* ulWinspecPosition */
		);

	gpos::owner<CDXLNode *> pdxlnWindowRef =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopWindowref));

	// translate children
	TranslateScalarChildren(pexprWindowFunc, pdxlnWindowRef);

	return pdxlnWindowRef;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::Ews
//
//	@doc:
//		Get the DXL representation of the window stage
//
//---------------------------------------------------------------------------
EdxlWinStage
CTranslatorExprToDXL::Ews(CScalarWindowFunc::EWinStage ews)
{
	ULONG window_frame_boundary_to_frame_boundary_mapping[][2] = {
		{EdxlwinstageImmediate, CScalarWindowFunc::EwsImmediate},
		{EdxlwinstagePreliminary, CScalarWindowFunc::EwsPreliminary},
		{EdxlwinstageRowKey, CScalarWindowFunc::EwsRowKey}};
#ifdef GPOS_DEBUG
	const ULONG arity =
		GPOS_ARRAY_SIZE(window_frame_boundary_to_frame_boundary_mapping);
	GPOS_ASSERT(arity > (ULONG) ews);
#endif
	ULONG *pulElem =
		window_frame_boundary_to_frame_boundary_mapping[(ULONG) ews];
	EdxlWinStage edxlws = (EdxlWinStage) pulElem[0];

	return edxlws;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScAggref
//
//	@doc:
//		Create a DXL scalar aggref node from an optimizer scalar agg func expr.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScAggref(gpos::pointer<CExpression *> pexprAggFunc)
{
	GPOS_ASSERT(nullptr != pexprAggFunc);

	gpos::pointer<CScalarAggFunc *> popScAggFunc =
		gpos::dyn_cast<CScalarAggFunc>(pexprAggFunc->Pop());
	gpos::owner<IMDId *> pmdidAggFunc = popScAggFunc->MDId();
	pmdidAggFunc->AddRef();

	IMDId *resolved_rettype = nullptr;
	if (popScAggFunc->FHasAmbiguousReturnType())
	{
		// Agg has an ambiguous return type, use the resolved type instead
		resolved_rettype = popScAggFunc->MdidType();
		resolved_rettype->AddRef();
	}

	EdxlAggrefStage edxlaggrefstage = EdxlaggstageNormal;

	if (popScAggFunc->FGlobal() && popScAggFunc->FSplit())
	{
		edxlaggrefstage = EdxlaggstageFinal;
	}
	else if (EaggfuncstageIntermediate == popScAggFunc->Eaggfuncstage())
	{
		edxlaggrefstage = EdxlaggstageIntermediate;
	}
	else if (!popScAggFunc->FGlobal())
	{
		edxlaggrefstage = EdxlaggstagePartial;
	}

	EdxlAggrefKind edxlaggrefkind = EdxlaggkindNormal;
	switch (popScAggFunc->AggKind())
	{
		case EaggfunckindNormal:
		{
			edxlaggrefkind = EdxlaggkindNormal;
			break;
		}
		case EaggfunckindOrderedSet:
		{
			edxlaggrefkind = EdxlaggkindOrderedSet;
			break;
		}
		case EaggfunckindHypothetical:
		{
			edxlaggrefkind = EdxlaggkindHypothetical;
			break;
		}
	}

	gpos::owner<ULongPtrArray *> argtypes = popScAggFunc->GetArgTypes();
	argtypes->AddRef();

	gpos::owner<CDXLScalarAggref *> pdxlopAggRef = GPOS_NEW(m_mp)
		CDXLScalarAggref(m_mp, std::move(pmdidAggFunc), resolved_rettype,
						 popScAggFunc->IsDistinct(), edxlaggrefstage,
						 edxlaggrefkind, std::move(argtypes));

	gpos::owner<CDXLNode *> pdxlnAggref =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopAggRef));

	pdxlnAggref->AddChild(
		PdxlnValuesList((*pexprAggFunc)[EdxlscalaraggrefIndexArgs]));

	pdxlnAggref->AddChild(
		PdxlnValuesList((*pexprAggFunc)[EdxlscalaraggrefIndexDirectArgs]));

	pdxlnAggref->AddChild(
		PdxlnValuesList((*pexprAggFunc)[EdxlscalaraggrefIndexAggOrder]));

	pdxlnAggref->AddChild(
		PdxlnValuesList((*pexprAggFunc)[EdxlscalaraggrefIndexAggDistinct]));

	return pdxlnAggref;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScIfStmt
//
//	@doc:
//		Create a DXL scalar if node from an optimizer scalar if expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScIfStmt(gpos::pointer<CExpression *> pexprIfStmt)
{
	GPOS_ASSERT(nullptr != pexprIfStmt);

	GPOS_ASSERT(3 == pexprIfStmt->Arity());

	gpos::pointer<CScalarIf *> popScIf =
		gpos::dyn_cast<CScalarIf>(pexprIfStmt->Pop());

	gpos::owner<IMDId *> mdid_type = popScIf->MdidType();
	mdid_type->AddRef();

	gpos::owner<CDXLNode *> pdxlnIfStmt = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarIfStmt(m_mp, std::move(mdid_type)));
	TranslateScalarChildren(pexprIfStmt, pdxlnIfStmt);

	return pdxlnIfStmt;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScSwitch
//
//	@doc:
//		Create a DXL scalar switch node from an optimizer scalar switch expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScSwitch(gpos::pointer<CExpression *> pexprSwitch)
{
	GPOS_ASSERT(nullptr != pexprSwitch);
	GPOS_ASSERT(1 < pexprSwitch->Arity());
	gpos::pointer<CScalarSwitch *> pop =
		gpos::dyn_cast<CScalarSwitch>(pexprSwitch->Pop());

	gpos::owner<IMDId *> mdid_type = pop->MdidType();
	mdid_type->AddRef();

	gpos::owner<CDXLNode *> dxlnode = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarSwitch(m_mp, std::move(mdid_type)));
	TranslateScalarChildren(pexprSwitch, dxlnode);

	return dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScSwitchCase
//
//	@doc:
//		Create a DXL scalar switch case node from an optimizer scalar switch
//		case expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScSwitchCase(
	gpos::pointer<CExpression *> pexprSwitchCase)
{
	GPOS_ASSERT(nullptr != pexprSwitchCase);
	GPOS_ASSERT(2 == pexprSwitchCase->Arity());

	gpos::owner<CDXLNode *> dxlnode = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarSwitchCase(m_mp));
	TranslateScalarChildren(pexprSwitchCase, dxlnode);

	return dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScNullIf
//
//	@doc:
//		Create a DXL scalar nullif node from an optimizer scalar
//		nullif expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScNullIf(gpos::pointer<CExpression *> pexprScNullIf)
{
	GPOS_ASSERT(nullptr != pexprScNullIf);

	gpos::pointer<CScalarNullIf *> pop =
		gpos::dyn_cast<CScalarNullIf>(pexprScNullIf->Pop());

	gpos::owner<IMDId *> mdid = pop->MdIdOp();
	mdid->AddRef();

	gpos::owner<IMDId *> mdid_type = pop->MdidType();
	mdid_type->AddRef();

	gpos::owner<CDXLScalarNullIf *> dxl_op = GPOS_NEW(m_mp)
		CDXLScalarNullIf(m_mp, std::move(mdid), std::move(mdid_type));
	gpos::owner<CDXLNode *> dxlnode =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(dxl_op));
	TranslateScalarChildren(pexprScNullIf, dxlnode);

	return dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScCaseTest
//
//	@doc:
//		Create a DXL scalar case test node from an optimizer scalar case test
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScCaseTest(
	gpos::pointer<CExpression *> pexprScCaseTest)
{
	GPOS_ASSERT(nullptr != pexprScCaseTest);
	gpos::pointer<CScalarCaseTest *> pop =
		gpos::dyn_cast<CScalarCaseTest>(pexprScCaseTest->Pop());

	gpos::owner<IMDId *> mdid_type = pop->MdidType();
	mdid_type->AddRef();

	gpos::owner<CDXLScalarCaseTest *> dxl_op =
		GPOS_NEW(m_mp) CDXLScalarCaseTest(m_mp, std::move(mdid_type));

	return GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(dxl_op));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScNullTest
//
//	@doc:
//		Create a DXL scalar null test node from an optimizer scalar null test expr.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScNullTest(
	gpos::pointer<CExpression *> pexprNullTest)
{
	GPOS_ASSERT(nullptr != pexprNullTest);

	gpos::owner<CDXLNode *> pdxlnNullTest = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarNullTest(m_mp, true /* is_null */));

	// translate child
	GPOS_ASSERT(1 == pexprNullTest->Arity());

	gpos::pointer<CExpression *> pexprChild = (*pexprNullTest)[0];
	gpos::owner<CDXLNode *> child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnNullTest->AddChild(std::move(child_dxlnode));

	return pdxlnNullTest;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScBooleanTest
//
//	@doc:
//		Create a DXL scalar null test node from an optimizer scalar null test expr.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScBooleanTest(
	gpos::pointer<CExpression *> pexprScBooleanTest)
{
	GPOS_ASSERT(nullptr != pexprScBooleanTest);
	GPOS_ASSERT(1 == pexprScBooleanTest->Arity());

	const ULONG rgulBoolTestMapping[][2] = {
		{CScalarBooleanTest::EbtIsTrue, EdxlbooleantestIsTrue},
		{CScalarBooleanTest::EbtIsNotTrue, EdxlbooleantestIsNotTrue},
		{CScalarBooleanTest::EbtIsFalse, EdxlbooleantestIsFalse},
		{CScalarBooleanTest::EbtIsNotFalse, EdxlbooleantestIsNotFalse},
		{CScalarBooleanTest::EbtIsUnknown, EdxlbooleantestIsUnknown},
		{CScalarBooleanTest::EbtIsNotUnknown, EdxlbooleantestIsNotUnknown},
	};

	gpos::pointer<CScalarBooleanTest *> popBoolTest =
		gpos::dyn_cast<CScalarBooleanTest>(pexprScBooleanTest->Pop());
	EdxlBooleanTestType edxlbooltest =
		(EdxlBooleanTestType) (rgulBoolTestMapping[popBoolTest->Ebt()][1]);
	gpos::owner<CDXLNode *> pdxlnScBooleanTest = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarBooleanTest(m_mp, edxlbooltest));

	// translate child
	gpos::pointer<CExpression *> pexprChild = (*pexprScBooleanTest)[0];
	gpos::owner<CDXLNode *> child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnScBooleanTest->AddChild(std::move(child_dxlnode));

	return pdxlnScBooleanTest;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScCoalesce
//
//	@doc:
//		Create a DXL scalar coalesce node from an optimizer scalar coalesce expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScCoalesce(
	gpos::pointer<CExpression *> pexprCoalesce)
{
	GPOS_ASSERT(nullptr != pexprCoalesce);
	GPOS_ASSERT(0 < pexprCoalesce->Arity());
	gpos::pointer<CScalarCoalesce *> popScCoalesce =
		gpos::dyn_cast<CScalarCoalesce>(pexprCoalesce->Pop());

	gpos::owner<IMDId *> mdid_type = popScCoalesce->MdidType();
	mdid_type->AddRef();

	gpos::owner<CDXLNode *> dxlnode = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarCoalesce(m_mp, std::move(mdid_type)));
	TranslateScalarChildren(pexprCoalesce, dxlnode);

	return dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScMinMax
//
//	@doc:
//		Create a DXL scalar MinMax node from an optimizer scalar MinMax expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScMinMax(gpos::pointer<CExpression *> pexprMinMax)
{
	GPOS_ASSERT(nullptr != pexprMinMax);
	GPOS_ASSERT(0 < pexprMinMax->Arity());
	gpos::pointer<CScalarMinMax *> popScMinMax =
		gpos::dyn_cast<CScalarMinMax>(pexprMinMax->Pop());

	CScalarMinMax::EScalarMinMaxType esmmt = popScMinMax->Esmmt();
	GPOS_ASSERT(CScalarMinMax::EsmmtMin == esmmt ||
				CScalarMinMax::EsmmtMax == esmmt);

	CDXLScalarMinMax::EdxlMinMaxType min_max_type = CDXLScalarMinMax::EmmtMin;
	if (CScalarMinMax::EsmmtMax == esmmt)
	{
		min_max_type = CDXLScalarMinMax::EmmtMax;
	}

	gpos::owner<IMDId *> mdid_type = popScMinMax->MdidType();
	mdid_type->AddRef();

	gpos::owner<CDXLNode *> dxlnode = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarMinMax(
						   m_mp, std::move(mdid_type), min_max_type));
	TranslateScalarChildren(pexprMinMax, dxlnode);

	return dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::TranslateScalarChildren
//
//	@doc:
//		Translate expression children and add them as children of the DXL node
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::TranslateScalarChildren(
	gpos::pointer<CExpression *> pexpr, gpos::pointer<CDXLNode *> dxlnode)
{
	const ULONG arity = pexpr->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::pointer<CExpression *> pexprChild = (*pexpr)[ul];
		gpos::owner<CDXLNode *> child_dxlnode = PdxlnScalar(pexprChild);
		dxlnode->AddChild(child_dxlnode);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScCast
//
//	@doc:
//		Create a DXL scalar relabel type node from an
//		optimizer scalar relabel type expr.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScCast(gpos::pointer<CExpression *> pexprCast)
{
	GPOS_ASSERT(nullptr != pexprCast);
	gpos::pointer<CScalarCast *> popScCast =
		gpos::dyn_cast<CScalarCast>(pexprCast->Pop());

	gpos::owner<IMDId *> mdid = popScCast->MdidType();
	mdid->AddRef();

	gpos::owner<IMDId *> mdid_func = popScCast->FuncMdId();
	mdid_func->AddRef();

	gpos::owner<CDXLNode *> pdxlnCast = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarCast(m_mp, std::move(mdid),
													 std::move(mdid_func)));

	// translate child
	GPOS_ASSERT(1 == pexprCast->Arity());
	gpos::pointer<CExpression *> pexprChild = (*pexprCast)[0];
	gpos::owner<CDXLNode *> child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnCast->AddChild(std::move(child_dxlnode));

	return pdxlnCast;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScCoerceToDomain
//
//	@doc:
//		Create a DXL scalar coerce node from an optimizer scalar coerce expr.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScCoerceToDomain(
	gpos::pointer<CExpression *> pexprCoerce)
{
	GPOS_ASSERT(nullptr != pexprCoerce);
	gpos::pointer<CScalarCoerceToDomain *> popScCoerce =
		gpos::dyn_cast<CScalarCoerceToDomain>(pexprCoerce->Pop());

	gpos::owner<IMDId *> mdid = popScCoerce->MdidType();
	mdid->AddRef();


	gpos::owner<CDXLNode *> pdxlnCoerce = GPOS_NEW(m_mp) CDXLNode(
		m_mp,
		GPOS_NEW(m_mp) CDXLScalarCoerceToDomain(
			m_mp, std::move(mdid), popScCoerce->TypeModifier(),
			(EdxlCoercionForm) popScCoerce
				->Ecf(),  // map Coercion Form directly based on position in enum
			popScCoerce->Location()));

	// translate child
	GPOS_ASSERT(1 == pexprCoerce->Arity());
	gpos::pointer<CExpression *> pexprChild = (*pexprCoerce)[0];
	gpos::owner<CDXLNode *> child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnCoerce->AddChild(std::move(child_dxlnode));

	return pdxlnCoerce;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScCoerceViaIO
//
//	@doc:
//		Create a DXL scalar coerce node from an optimizer scalar coerce expr.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScCoerceViaIO(
	gpos::pointer<CExpression *> pexprCoerce)
{
	GPOS_ASSERT(nullptr != pexprCoerce);
	gpos::pointer<CScalarCoerceViaIO *> popScCerce =
		gpos::dyn_cast<CScalarCoerceViaIO>(pexprCoerce->Pop());

	gpos::owner<IMDId *> mdid = popScCerce->MdidType();
	mdid->AddRef();


	gpos::owner<CDXLNode *> pdxlnCoerce = GPOS_NEW(m_mp) CDXLNode(
		m_mp,
		GPOS_NEW(m_mp) CDXLScalarCoerceViaIO(
			m_mp, std::move(mdid), popScCerce->TypeModifier(),
			(EdxlCoercionForm) popScCerce
				->Ecf(),  // map Coercion Form directly based on position in enum
			popScCerce->Location()));

	// translate child
	GPOS_ASSERT(1 == pexprCoerce->Arity());
	gpos::pointer<CExpression *> pexprChild = (*pexprCoerce)[0];
	gpos::owner<CDXLNode *> child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnCoerce->AddChild(std::move(child_dxlnode));

	return pdxlnCoerce;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScArrayCoerceExpr
//
//	@doc:
//		Create a DXL node from an optimizer scalar array coerce expr.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScArrayCoerceExpr(
	gpos::pointer<CExpression *> pexprArrayCoerceExpr)
{
	GPOS_ASSERT(nullptr != pexprArrayCoerceExpr);
	gpos::pointer<CScalarArrayCoerceExpr *> popScArrayCoerceExpr =
		gpos::dyn_cast<CScalarArrayCoerceExpr>(pexprArrayCoerceExpr->Pop());

	gpos::owner<IMDId *> pmdidElemFunc =
		popScArrayCoerceExpr->PmdidElementFunc();
	pmdidElemFunc->AddRef();
	gpos::owner<IMDId *> mdid = popScArrayCoerceExpr->MdidType();
	mdid->AddRef();

	gpos::owner<CDXLNode *> pdxlnArrayCoerceExpr = GPOS_NEW(m_mp) CDXLNode(
		m_mp,
		GPOS_NEW(m_mp) CDXLScalarArrayCoerceExpr(
			m_mp, std::move(pmdidElemFunc), std::move(mdid),
			popScArrayCoerceExpr->TypeModifier(),
			popScArrayCoerceExpr->IsExplicit(),
			(EdxlCoercionForm) popScArrayCoerceExpr
				->Ecf(),  // map Coercion Form directly based on position in enum
			popScArrayCoerceExpr->Location()));

	// translate child
	GPOS_ASSERT(1 == pexprArrayCoerceExpr->Arity());
	gpos::pointer<CExpression *> pexprChild = (*pexprArrayCoerceExpr)[0];
	gpos::owner<CDXLNode *> child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnArrayCoerceExpr->AddChild(std::move(child_dxlnode));

	return pdxlnArrayCoerceExpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::GetWindowFrame
//
//	@doc:
//		Translate a window frame
//
//---------------------------------------------------------------------------
gpos::owner<CDXLWindowFrame *>
CTranslatorExprToDXL::GetWindowFrame(gpos::pointer<CWindowFrame *> pwf)
{
	GPOS_ASSERT(nullptr != pwf);

	if (CWindowFrame::IsEmpty(pwf))
	{
		// an empty frame is translated as 'no frame'
		return nullptr;
	}

	// mappings for frame info in expression and dxl worlds
	const ULONG rgulSpecMapping[][2] = {{CWindowFrame::EfsRows, EdxlfsRow},
										{CWindowFrame::EfsRange, EdxlfsRange}};

	const ULONG rgulBoundaryMapping[][2] = {
		{CWindowFrame::EfbUnboundedPreceding, EdxlfbUnboundedPreceding},
		{CWindowFrame::EfbBoundedPreceding, EdxlfbBoundedPreceding},
		{CWindowFrame::EfbCurrentRow, EdxlfbCurrentRow},
		{CWindowFrame::EfbUnboundedFollowing, EdxlfbUnboundedFollowing},
		{CWindowFrame::EfbBoundedFollowing, EdxlfbBoundedFollowing},
		{CWindowFrame::EfbDelayedBoundedPreceding,
		 EdxlfbDelayedBoundedPreceding},
		{CWindowFrame::EfbDelayedBoundedFollowing,
		 EdxlfbDelayedBoundedFollowing}};

	const ULONG rgulExclusionStrategyMapping[][2] = {
		{CWindowFrame::EfesNone, EdxlfesNone},
		{CWindowFrame::EfesNulls, EdxlfesNulls},
		{CWindowFrame::EfesCurrentRow, EdxlfesCurrentRow},
		{CWindowFrame::EfseMatchingOthers, EdxlfesGroup},
		{CWindowFrame::EfesTies, EdxlfesTies}};

	EdxlFrameSpec edxlfs = (EdxlFrameSpec) (rgulSpecMapping[pwf->Efs()][1]);
	EdxlFrameBoundary edxlfbLeading =
		(EdxlFrameBoundary) (rgulBoundaryMapping[pwf->EfbLeading()][1]);
	EdxlFrameBoundary edxlfbTrailing =
		(EdxlFrameBoundary) (rgulBoundaryMapping[pwf->EfbTrailing()][1]);
	EdxlFrameExclusionStrategy frame_exc_strategy =
		(EdxlFrameExclusionStrategy) (rgulExclusionStrategyMapping[pwf->Efes()]
																  [1]);

	// translate scalar expressions representing leading and trailing frame edges
	gpos::owner<CDXLNode *> pdxlnLeading = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarWindowFrameEdge(
						   m_mp, true /* fLeading */, edxlfbLeading));
	if (nullptr != pwf->PexprLeading())
	{
		pdxlnLeading->AddChild(PdxlnScalar(pwf->PexprLeading()));
	}

	gpos::owner<CDXLNode *> pdxlnTrailing = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarWindowFrameEdge(
						   m_mp, false /* fLeading */, edxlfbTrailing));
	if (nullptr != pwf->PexprTrailing())
	{
		pdxlnTrailing->AddChild(PdxlnScalar(pwf->PexprTrailing()));
	}

	return GPOS_NEW(m_mp)
		CDXLWindowFrame(edxlfs, frame_exc_strategy, std::move(pdxlnLeading),
						std::move(pdxlnTrailing));
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnWindow
//
//	@doc:
//		Create a DXL window node from physical sequence project expression.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnWindow(gpos::pointer<CExpression *> pexprSeqPrj,
								  gpos::pointer<CColRefArray *> colref_array,
								  CDistributionSpecArray *pdrgpdsBaseTables,
								  ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprSeqPrj);

	gpos::pointer<CPhysicalSequenceProject *> popSeqPrj =
		gpos::dyn_cast<CPhysicalSequenceProject>(pexprSeqPrj->Pop());
	gpos::pointer<CDistributionSpec *> pds = popSeqPrj->Pds();
	gpos::owner<ULongPtrArray *> colids = GPOS_NEW(m_mp) ULongPtrArray(m_mp);
	gpos::pointer<CExpressionArray *> pdrgpexprPartCol = nullptr;
	if (CDistributionSpec::EdtHashed == pds->Edt())
	{
		gpos::pointer<CDistributionSpecHashed *> pdshashed =
			gpos::dyn_cast<CDistributionSpecHashed>(pds);
		pdrgpexprPartCol = pdshashed->Pdrgpexpr();
		const ULONG size = pdrgpexprPartCol->Size();
		for (ULONG ul = 0; ul < size; ul++)
		{
			gpos::pointer<CExpression *> pexpr = (*pdrgpexprPartCol)[ul];
			gpos::pointer<CScalarIdent *> popScId =
				gpos::dyn_cast<CScalarIdent>(pexpr->Pop());
			colids->Append(GPOS_NEW(m_mp) ULONG(popScId->Pcr()->Id()));
		}
	}

	// translate order specification and window frames into window keys
	gpos::owner<CDXLWindowKeyArray *> pdrgpdxlwk =
		GPOS_NEW(m_mp) CDXLWindowKeyArray(m_mp);
	gpos::pointer<COrderSpecArray *> pdrgpos = popSeqPrj->Pdrgpos();
	GPOS_ASSERT(nullptr != pdrgpos);
	const ULONG ulOsSize = pdrgpos->Size();
	for (ULONG ul = 0; ul < ulOsSize; ul++)
	{
		gpos::owner<CDXLWindowKey *> pdxlwk = GPOS_NEW(m_mp) CDXLWindowKey();
		gpos::owner<CDXLNode *> sort_col_list_dxlnode =
			GetSortColListDXL((*popSeqPrj->Pdrgpos())[ul]);
		pdxlwk->SetSortColList(sort_col_list_dxlnode);
		pdrgpdxlwk->Append(pdxlwk);
	}

	const ULONG ulFrames = popSeqPrj->Pdrgpwf()->Size();
	for (ULONG ul = 0; ul < ulFrames; ul++)
	{
		gpos::owner<CDXLWindowFrame *> window_frame =
			GetWindowFrame((*popSeqPrj->Pdrgpwf())[ul]);
		if (nullptr != window_frame)
		{
			GPOS_ASSERT(ul <= ulOsSize);
			gpos::pointer<CDXLWindowKey *> pdxlwk = (*pdrgpdxlwk)[ul];
			pdxlwk->SetWindowFrame(window_frame);
		}
	}

	// extract physical properties
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GetProperties(pexprSeqPrj);

	// translate relational child
	gpos::owner<CDXLNode *> child_dxlnode = CreateDXLNode(
		(*pexprSeqPrj)[0], nullptr /* colref_array */, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	GPOS_ASSERT(nullptr != pexprSeqPrj->Prpp());
	gpos::owner<CColRefSet *> pcrsOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsOutput->Include(pexprSeqPrj->Prpp()->PcrsRequired());
	if (nullptr != pdrgpexprPartCol)
	{
		gpos::owner<CColRefSet *> pcrs =
			CUtils::PcrsExtractColumns(m_mp, pdrgpexprPartCol);
		pcrsOutput->Include(pcrs);
		pcrs->Release();
	}
	for (ULONG ul = 0; ul < ulOsSize; ul++)
	{
		gpos::pointer<COrderSpec *> pos = (*popSeqPrj->Pdrgpos())[ul];
		if (!pos->IsEmpty())
		{
			const CColRef *colref = pos->Pcr(ul);
			pcrsOutput->Include(colref);
		}
	}

	// translate project list expression
	gpos::owner<CDXLNode *> pdxlnPrL =
		PdxlnProjList((*pexprSeqPrj)[1], pcrsOutput, colref_array);

	// create an empty one-time filter
	gpos::owner<CDXLNode *> filter_dxlnode =
		PdxlnFilter(nullptr /* pdxlnCond */);

	// construct a Window node
	gpos::owner<CDXLPhysicalWindow *> pdxlopWindow = GPOS_NEW(m_mp)
		CDXLPhysicalWindow(m_mp, std::move(colids), std::move(pdrgpdxlwk));
	gpos::owner<CDXLNode *> pdxlnWindow =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopWindow));
	pdxlnWindow->SetProperties(std::move(dxl_properties));

	// add children
	pdxlnWindow->AddChild(std::move(pdxlnPrL));
	pdxlnWindow->AddChild(std::move(filter_dxlnode));
	pdxlnWindow->AddChild(std::move(child_dxlnode));

#ifdef REFCOUNT_FIXME_ASSERT_AFTER_MOVE
#ifdef GPOS_DEBUG
	pdxlopWindow->AssertValid(pdxlnWindow, false /* validate_children */);
#endif
#endif

	pcrsOutput->Release();

	return pdxlnWindow;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnArray
//
//	@doc:
//		Create a DXL array node from an optimizer array expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnArray(gpos::pointer<CExpression *> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	gpos::pointer<CScalarArray *> pop =
		gpos::dyn_cast<CScalarArray>(pexpr->Pop());

	gpos::owner<IMDId *> elem_type_mdid = pop->PmdidElem();
	elem_type_mdid->AddRef();

	gpos::owner<IMDId *> array_type_mdid = pop->PmdidArray();
	array_type_mdid->AddRef();

	gpos::owner<CDXLNode *> pdxlnArray = GPOS_NEW(m_mp) CDXLNode(
		m_mp,
		GPOS_NEW(m_mp) CDXLScalarArray(m_mp, elem_type_mdid, array_type_mdid,
									   pop->FMultiDimensional()));

	const ULONG arity = CUtils::UlScalarArrayArity(pexpr);

	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::owner<CExpression *> pexprChild =
			CUtils::PScalarArrayExprChildAt(m_mp, pexpr, ul);
		gpos::owner<CDXLNode *> child_dxlnode = PdxlnScalar(pexprChild);
		pdxlnArray->AddChild(child_dxlnode);
		pexprChild->Release();
	}

	return pdxlnArray;
}
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnValuesList(gpos::pointer<CExpression *> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	gpos::owner<CDXLNode *> pdxlnValuesList = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarValuesList(m_mp));

	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		gpos::pointer<CExpression *> pexprChild = (*pexpr)[ul];
		gpos::owner<CDXLNode *> child_dxlnode = PdxlnScalar(pexprChild);
		pdxlnValuesList->AddChild(child_dxlnode);
	}

	return pdxlnValuesList;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnArrayRef
//
//	@doc:
//		Create a DXL arrayref node from an optimizer arrayref expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnArrayRef(gpos::pointer<CExpression *> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	gpos::pointer<CScalarArrayRef *> pop =
		gpos::dyn_cast<CScalarArrayRef>(pexpr->Pop());

	gpos::owner<IMDId *> elem_type_mdid = pop->PmdidElem();
	elem_type_mdid->AddRef();

	gpos::owner<IMDId *> array_type_mdid = pop->PmdidArray();
	array_type_mdid->AddRef();

	gpos::owner<IMDId *> return_type_mdid = pop->MdidType();
	return_type_mdid->AddRef();

	gpos::owner<CDXLNode *> pdxlnArrayref = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarArrayRef(
				  m_mp, std::move(elem_type_mdid), pop->TypeModifier(),
				  std::move(array_type_mdid), std::move(return_type_mdid)));

	TranslateScalarChildren(pexpr, pdxlnArrayref);

	return pdxlnArrayref;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnArrayRefIndexList
//
//	@doc:
//		Create a DXL arrayref index list from an optimizer arrayref index list
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnArrayRefIndexList(gpos::pointer<CExpression *> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	gpos::pointer<CScalarArrayRefIndexList *> pop =
		gpos::dyn_cast<CScalarArrayRefIndexList>(pexpr->Pop());

	gpos::owner<CDXLNode *> pdxlnIndexlist = GPOS_NEW(m_mp) CDXLNode(
		m_mp,
		GPOS_NEW(m_mp) CDXLScalarArrayRefIndexList(m_mp, Eilb(pop->Eilt())));

	TranslateScalarChildren(pexpr, pdxlnIndexlist);

	return pdxlnIndexlist;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAssertPredicate
//
//	@doc:
//		Create a DXL assert predicate from an optimizer assert predicate expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnAssertPredicate(gpos::pointer<CExpression *> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	gpos::owner<CDXLNode *> pdxlnAssertConstraintList = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarAssertConstraintList(m_mp));
	TranslateScalarChildren(pexpr, pdxlnAssertConstraintList);
	return pdxlnAssertConstraintList;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAssertConstraint
//
//	@doc:
//		Create a DXL assert constraint from an optimizer assert constraint expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnAssertConstraint(gpos::pointer<CExpression *> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	gpos::pointer<CScalarAssertConstraint *> popAssertConstraint =
		gpos::dyn_cast<CScalarAssertConstraint>(pexpr->Pop());
	CWStringDynamic *pstrErrorMsg = GPOS_NEW(m_mp)
		CWStringDynamic(m_mp, popAssertConstraint->PstrErrorMsg()->GetBuffer());

	gpos::owner<CDXLNode *> pdxlnAssertConstraint = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarAssertConstraint(m_mp, pstrErrorMsg));
	TranslateScalarChildren(pexpr, pdxlnAssertConstraint);
	return pdxlnAssertConstraint;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::Eilb
//
//	@doc:
// 		Translate the arrayref index list bound
//
//---------------------------------------------------------------------------
CDXLScalarArrayRefIndexList::EIndexListBound
CTranslatorExprToDXL::Eilb(const CScalarArrayRefIndexList::EIndexListType eilt)
{
	switch (eilt)
	{
		case CScalarArrayRefIndexList::EiltLower:
			return CDXLScalarArrayRefIndexList::EilbLower;

		case CScalarArrayRefIndexList::EiltUpper:
			return CDXLScalarArrayRefIndexList::EilbUpper;

		default:
			GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp,
					   GPOS_WSZ_LIT("Invalid arrayref index bound"));
			return CDXLScalarArrayRefIndexList::EilbSentinel;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnArrayCmp
//
//	@doc:
//		Create a DXL array compare node from an optimizer array expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnArrayCmp(gpos::pointer<CExpression *> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	gpos::pointer<CScalarArrayCmp *> pop =
		gpos::dyn_cast<CScalarArrayCmp>(pexpr->Pop());

	gpos::owner<IMDId *> mdid_op = pop->MdIdOp();
	mdid_op->AddRef();

	const CWStringConst *str_opname = pop->Pstr();

	CScalarArrayCmp::EArrCmpType earrcmpt = pop->Earrcmpt();
	GPOS_ASSERT(CScalarArrayCmp::EarrcmpSentinel > earrcmpt);
	EdxlArrayCompType edxlarrcmpt = Edxlarraycomptypeall;
	if (CScalarArrayCmp::EarrcmpAny == earrcmpt)
	{
		edxlarrcmpt = Edxlarraycomptypeany;
	}

	gpos::owner<CDXLNode *> pdxlnArrayCmp = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarArrayComp(
				  m_mp, std::move(mdid_op),
				  GPOS_NEW(m_mp) CWStringConst(m_mp, str_opname->GetBuffer()),
				  edxlarrcmpt));

	TranslateScalarChildren(pexpr, pdxlnArrayCmp);

	return pdxlnArrayCmp;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDMLAction
//
//	@doc:
//		Create a DXL DML action node from an optimizer action expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnDMLAction(gpos::pointer<CExpression *>
#ifdef GPOS_DEBUG
										 pexpr
#endif	// GPOS_DEBUG
)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(COperator::EopScalarDMLAction == pexpr->Pop()->Eopid());

	return GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarDMLAction(m_mp));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScConst
//
//	@doc:
//		Create a DXL scalar constant node from an optimizer scalar const expr.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScConst(gpos::pointer<CExpression *> pexprScConst)
{
	GPOS_ASSERT(nullptr != pexprScConst);

	gpos::pointer<CScalarConst *> popScConst =
		gpos::dyn_cast<CScalarConst>(pexprScConst->Pop());

	gpos::pointer<IDatum *> datum = popScConst->GetDatum();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	gpos::pointer<const IMDType *> pmdtype =
		md_accessor->RetrieveType(datum->MDId());

	gpos::owner<CDXLNode *> dxlnode =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pmdtype->GetDXLOpScConst(m_mp, datum));

	return dxlnode;
}

gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnScSortGroupClause(
	gpos::pointer<CExpression *> pexprScSortGroupClause)
{
	GPOS_ASSERT(nullptr != pexprScSortGroupClause);

	gpos::pointer<CScalarSortGroupClause *> pop =
		gpos::dyn_cast<CScalarSortGroupClause>(pexprScSortGroupClause->Pop());

	gpos::owner<CDXLNode *> dxlnode = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarSortGroupClause(
						   m_mp, pop->Index(), pop->EqOp(), pop->SortOp(),
						   pop->NullsFirst(), pop->IsHashable()));

	return dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnFilter
//
//	@doc:
//		Create a DXL filter node containing the given scalar node as a child.
//		If the scalar node is NULL, a filter node with no children is returned
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnFilter(gpos::owner<CDXLNode *> pdxlnCond)
{
	gpos::owner<CDXLNode *> filter_dxlnode =
		GPOS_NEW(m_mp) CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarFilter(m_mp));
	if (nullptr != pdxlnCond)
	{
		filter_dxlnode->AddChild(pdxlnCond);
	}

	return filter_dxlnode;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::MakeDXLTableDescr
//
//	@doc:
//		Create a DXL table descriptor from the corresponding optimizer structure
//
//---------------------------------------------------------------------------
gpos::owner<CDXLTableDescr *>
CTranslatorExprToDXL::MakeDXLTableDescr(
	gpos::pointer<const CTableDescriptor *> ptabdesc,
	gpos::pointer<const CColRefArray *> pdrgpcrOutput,
	gpos::pointer<const CReqdPropPlan *> reqd_prop_plan GPOS_ASSERTS_ONLY)
{
	GPOS_ASSERT(nullptr != ptabdesc);
	GPOS_ASSERT_IMP(nullptr != pdrgpcrOutput,
					ptabdesc->ColumnCount() == pdrgpcrOutput->Size());

	// get tbl name
	CMDName *pmdnameTbl = GPOS_NEW(m_mp) CMDName(m_mp, ptabdesc->Name().Pstr());

	gpos::owner<CMDIdGPDB *> mdid = gpos::dyn_cast<CMDIdGPDB>(ptabdesc->MDId());
	mdid->AddRef();

	gpos::owner<CDXLTableDescr *> table_descr = GPOS_NEW(m_mp)
		CDXLTableDescr(m_mp, mdid, pmdnameTbl, ptabdesc->GetExecuteAsUserId(),
					   ptabdesc->LockMode());

	const ULONG ulColumns = ptabdesc->ColumnCount();
	// translate col descriptors
	for (ULONG ul = 0; ul < ulColumns; ul++)
	{
		gpos::pointer<const CColumnDescriptor *> pcd = ptabdesc->Pcoldesc(ul);

		GPOS_ASSERT(nullptr != pcd);

		// output col ref for the current col descrs
		CColRef *colref = nullptr;
		if (nullptr != pdrgpcrOutput)
		{
			colref = (*pdrgpcrOutput)[ul];
			if (colref->GetUsage() != CColRef::EUsed)
			{
#ifdef GPOS_DEBUG
				if (nullptr != reqd_prop_plan &&
					nullptr != reqd_prop_plan->PcrsRequired())
				{
					// ensure that any col removed is not a part of the plan's required cols
					GPOS_ASSERT(
						!reqd_prop_plan->PcrsRequired()->FMember(colref));
				}
#endif
				continue;
			}
		}
		else
		{
			colref = m_pcf->PcrCreate(pcd->RetrieveType(), pcd->TypeModifier(),
									  pcd->Name());
		}

		CMDName *pmdnameCol = GPOS_NEW(m_mp) CMDName(m_mp, pcd->Name().Pstr());

		// use the col ref id for the corresponding output column as
		// colid for the dxl column
		gpos::owner<CMDIdGPDB *> pmdidColType =
			gpos::dyn_cast<CMDIdGPDB>(colref->RetrieveType()->MDId());
		pmdidColType->AddRef();

		gpos::owner<CDXLColDescr *> pdxlcd = GPOS_NEW(m_mp) CDXLColDescr(
			pmdnameCol, colref->Id(), pcd->AttrNum(), pmdidColType,
			colref->TypeModifier(), false /* fdropped */, pcd->Width());

		table_descr->AddColumnDescr(pdxlcd);
	}

	return table_descr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::GetProperties
//
//	@doc:
//		Construct a DXL physical properties container with operator costs for
//		the given expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLPhysicalProperties *>
CTranslatorExprToDXL::GetProperties(gpos::pointer<const CExpression *> pexpr)
{
	// extract out rows from statistics object
	CWStringDynamic *rows_out_str = GPOS_NEW(m_mp) CWStringDynamic(m_mp);
	gpos::pointer<const IStatistics *> stats = pexpr->Pstats();
	CDouble rows = CStatistics::DefaultRelationRows;

	// stats may not be present in artificially generated physical expression trees.
	// fill in default statistics
	if (nullptr != stats)
	{
		rows = stats->Rows();
	}

	if (CDistributionSpec::EdtStrictReplicated ==
			pexpr->GetDrvdPropPlan()->Pds()->Edt() ||
		CDistributionSpec::EdtTaintedReplicated ==
			pexpr->GetDrvdPropPlan()->Pds()->Edt())
	{
		// if distribution is replicated, multiply number of rows by number of segments
		ULONG ulSegments = COptCtxt::PoctxtFromTLS()->GetCostModel()->UlHosts();
		rows = rows * ulSegments;
	}

	rows_out_str->AppendFormat(GPOS_WSZ_LIT("%f"), rows.Get());

	// extract our width from statistics object
	CDouble width = CStatistics::DefaultColumnWidth;
	gpos::pointer<CReqdPropPlan *> prpp = pexpr->Prpp();
	gpos::pointer<CColRefSet *> pcrs = prpp->PcrsRequired();
	gpos::owner<ULongPtrArray *> colids = GPOS_NEW(m_mp) ULongPtrArray(m_mp);
	pcrs->ExtractColIds(m_mp, colids);
	CWStringDynamic *width_str = GPOS_NEW(m_mp) CWStringDynamic(m_mp);

	if (nullptr != stats)
	{
		width = stats->Width(colids);
	}
	colids->Release();
	width_str->AppendFormat(GPOS_WSZ_LIT("%lld"), (LINT) width.Get());

	// get the cost from expression node
	CWStringDynamic str(m_mp);
	COstreamString oss(&str);
	oss << pexpr->Cost();

	CWStringDynamic *pstrStartupcost =
		GPOS_NEW(m_mp) CWStringDynamic(m_mp, GPOS_WSZ_LIT("0"));
	CWStringDynamic *pstrTotalcost =
		GPOS_NEW(m_mp) CWStringDynamic(m_mp, str.GetBuffer());

	gpos::owner<CDXLOperatorCost *> cost = GPOS_NEW(m_mp) CDXLOperatorCost(
		pstrStartupcost, pstrTotalcost, rows_out_str, width_str);
	gpos::owner<CDXLPhysicalProperties *> dxl_properties =
		GPOS_NEW(m_mp) CDXLPhysicalProperties(std::move(cost));

	return dxl_properties;
}


gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnProjListForChildPart(
	gpos::pointer<const ColRefToUlongMap *> root_col_mapping,
	gpos::pointer<const CColRefArray *> part_colrefs,
	gpos::pointer<const CColRefSet *> reqd_colrefs,
	gpos::pointer<const CColRefArray *> colref_array)
{
	gpos::owner<CColRefArray *> mapped_colrefs =
		GPOS_NEW(m_mp) CColRefArray(m_mp);
	gpos::owner<CColRefSet *> pcrs = GPOS_NEW(m_mp) CColRefSet(m_mp);
	// project columns in order if explicitly asked
	if (nullptr != colref_array)
	{
		for (ULONG i = 0; i < colref_array->Size(); ++i)
		{
			CColRef *cr = (*colref_array)[i];
			ULONG *idx = root_col_mapping->Find(cr);
			GPOS_ASSERT(nullptr != idx);
			CColRef *mapped_cr = (*part_colrefs)[*idx];
			mapped_colrefs->Append(mapped_cr);
			pcrs->Include(mapped_cr);
		}
	}

	CColRefSetIter crsi(*reqd_colrefs);
	while (crsi.Advance())
	{
		CColRef *cr = crsi.Pcr();
		ULONG *idx = root_col_mapping->Find(cr);
		GPOS_ASSERT(nullptr != idx);
		CColRef *mapped_cr = (*part_colrefs)[*idx];
		if (!pcrs->FMember(mapped_cr))
		{
			mapped_colrefs->Append(mapped_cr);
		}
	}

	gpos::owner<CColRefSet *> empty_set = GPOS_NEW(m_mp) CColRefSet(m_mp);
	gpos::owner<CDXLNode *> pdxlnPrL = PdxlnProjList(empty_set, mapped_colrefs);
	empty_set->Release();
	mapped_colrefs->Release();
	pcrs->Release();
	return pdxlnPrL;
}

// Translate a filter expr on the root for a child partition using:
//   root_col_mapping - root col to part col mapping
//   part_colrefs - (new) colrefs of the child partition
//   root_colrefs - (original) root DTS colrefs
//   pred - filter predicate to translate
//
// The method first creates a temporary mapping from root colrefs to (new) child
// partition colref ids, which is used when translating via PdxlnScalar().
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnCondForChildPart(
	gpos::pointer<const ColRefToUlongMap *> root_col_mapping,
	gpos::pointer<const CColRefArray *> part_colrefs,
	gpos::pointer<const CColRefArray *> root_colrefs,
	gpos::pointer<CExpression *> pred)
{
	GPOS_ASSERT(part_colrefs->Size() == root_colrefs->Size());

	// Set up a temporary mapping from root colrefs to partition colref ids
	m_phmcrulPartColId = GPOS_NEW(m_mp) ColRefToUlongMap(m_mp);

	for (ULONG i = 0; i < root_colrefs->Size(); ++i)
	{
		CColRef *cr = (*root_colrefs)[i];
		ULONG *idx = root_col_mapping->Find(cr);
		GPOS_ASSERT(nullptr != idx);

		CColRef *mapped_cr = (*part_colrefs)[*idx];
		m_phmcrulPartColId->Insert(cr, GPOS_NEW(m_mp) ULONG(mapped_cr->Id()));
	}

	gpos::owner<CDXLNode *> pdxlnCond = nullptr;
	if (nullptr != pred)
	{
		pdxlnCond = PdxlnScalar(pred);
	}

	// clean up the temporary mapping
	m_phmcrulPartColId->Release();
	m_phmcrulPartColId = nullptr;

	return pdxlnCond;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnBitmapIndexPathForChildPart
//
//	@doc:
//		Construct a DXL BitmapTableScan's bitmap index path child
//		DLX node for a child partition.
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnBitmapIndexPathForChildPart(
	gpos::pointer<const ColRefToUlongMap *> root_col_mapping,
	gpos::pointer<const CColRefArray *> part_colrefs,
	gpos::pointer<const CColRefArray *> root_colrefs,
	gpos::pointer<const IMDRelation *> part,
	gpos::pointer<CExpression *> pexprBitmapIndexPath)
{
	GPOS_CHECK_STACK_SIZE;

	switch (pexprBitmapIndexPath->Pop()->Eopid())
	{
		case COperator::EopScalarBitmapIndexProbe:
			return PdxlnBitmapIndexProbeForChildPart(
				root_col_mapping, part_colrefs, root_colrefs, part,
				pexprBitmapIndexPath);
		case COperator::EopScalarBitmapBoolOp:
		{
			GPOS_ASSERT(nullptr != pexprBitmapIndexPath);
			GPOS_ASSERT(2 == pexprBitmapIndexPath->Arity());

			gpos::pointer<CScalarBitmapBoolOp *> popBitmapBoolOp =
				gpos::dyn_cast<CScalarBitmapBoolOp>(
					pexprBitmapIndexPath->Pop());
			gpos::pointer<CExpression *> pexprLeft = (*pexprBitmapIndexPath)[0];
			gpos::pointer<CExpression *> pexprRight =
				(*pexprBitmapIndexPath)[1];

			gpos::owner<CDXLNode *> dxlnode_left =
				PdxlnBitmapIndexPathForChildPart(root_col_mapping, part_colrefs,
												 root_colrefs, part, pexprLeft);
			gpos::owner<CDXLNode *> dxlnode_right =
				PdxlnBitmapIndexPathForChildPart(root_col_mapping, part_colrefs,
												 root_colrefs, part,
												 pexprRight);

			gpos::owner<IMDId *> mdid_type = popBitmapBoolOp->MdidType();
			mdid_type->AddRef();

			CDXLScalarBitmapBoolOp::EdxlBitmapBoolOp edxlbitmapop =
				CDXLScalarBitmapBoolOp::EdxlbitmapAnd;

			if (CScalarBitmapBoolOp::EbitmapboolOr ==
				popBitmapBoolOp->Ebitmapboolop())
			{
				edxlbitmapop = CDXLScalarBitmapBoolOp::EdxlbitmapOr;
			}

			return GPOS_NEW(m_mp)
				CDXLNode(m_mp,
						 GPOS_NEW(m_mp) CDXLScalarBitmapBoolOp(
							 m_mp, std::move(mdid_type), edxlbitmapop),
						 std::move(dxlnode_left), std::move(dxlnode_right));
		}
		default:
			GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp,
					   pexprBitmapIndexPath->Pop()->SzId());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnBitmapIndexProbeForChildPart
//
//	@doc:
//		Create a DXL scalar bitmap index probe from an optimizer
//		scalar bitmap index probe operator for a child partition.
//
//		GPDB_12_MERGE_FIXME: this function should always keep parity
//		with PdxlnBitmapIndexProbe(). We did not extract the duplicate
//		code into a common function because the handlings for child
//		partition are also all over this function.
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnBitmapIndexProbeForChildPart(
	gpos::pointer<const ColRefToUlongMap *> root_col_mapping,
	gpos::pointer<const CColRefArray *> part_colrefs,
	gpos::pointer<const CColRefArray *> root_colrefs,
	gpos::pointer<const IMDRelation *> part,
	gpos::pointer<CExpression *> pexprBitmapIndexProbe)
{
	GPOS_ASSERT(nullptr != pexprBitmapIndexProbe);
	gpos::pointer<CScalarBitmapIndexProbe *> pop =
		gpos::dyn_cast<CScalarBitmapIndexProbe>(pexprBitmapIndexProbe->Pop());

	// create index descriptor
	gpos::pointer<CIndexDescriptor *> pindexdesc = pop->Pindexdesc();
	gpos::pointer<IMDId *> pmdidIndex = pindexdesc->MDId();

	// construct set of child indexes from parent list of child indexes
	gpos::pointer<const IMDIndex *> md_index =
		m_pmda->RetrieveIndex(pmdidIndex);
	gpos::pointer<IMdIdArray *> child_indexes = md_index->ChildIndexMdids();

	gpos::owner<MdidHashSet *> child_index_mdids_set =
		GPOS_NEW(m_mp) MdidHashSet(m_mp);
	for (ULONG ul = 0; ul < child_indexes->Size(); ul++)
	{
		(*child_indexes)[ul]->AddRef();
		child_index_mdids_set->Insert((*child_indexes)[ul]);
	}

	gpos::owner<CDXLIndexDescr *> dxl_index_descr = PdxlnIndexDescForPart(
		m_mp, child_index_mdids_set, part, pindexdesc->Name().Pstr());

	child_index_mdids_set->Release();

	gpos::owner<CDXLScalarBitmapIndexProbe *> dxl_op =
		GPOS_NEW(m_mp) CDXLScalarBitmapIndexProbe(m_mp, dxl_index_descr);
	gpos::owner<CDXLNode *> pdxlnBitmapIndexProbe =
		GPOS_NEW(m_mp) CDXLNode(m_mp, dxl_op);

	// translate index predicates
	gpos::pointer<CExpression *> pexprCond = (*pexprBitmapIndexProbe)[0];
	gpos::owner<CDXLNode *> pdxlnIndexCondList = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarIndexCondList(m_mp));
	gpos::owner<CExpressionArray *> pdrgpexprConds =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprCond);
	const ULONG length = pdrgpexprConds->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::pointer<CExpression *> pexprIndexCond = (*pdrgpexprConds)[ul];
		gpos::owner<CDXLNode *> pdxlnIndexCond = PdxlnCondForChildPart(
			root_col_mapping, part_colrefs, root_colrefs, pexprIndexCond);
		pdxlnIndexCondList->AddChild(pdxlnIndexCond);
	}
	pdrgpexprConds->Release();
	pdxlnBitmapIndexProbe->AddChild(std::move(pdxlnIndexCondList));

#ifdef GPOS_DEBUG
	pdxlnBitmapIndexProbe->GetOperator()->AssertValid(
		pdxlnBitmapIndexProbe, false /*validate_children*/);
#endif

	return pdxlnBitmapIndexProbe;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnProjList
//
//	@doc:
//		Translate the set of output col refs into a dxl project list.
//		If the given array of columns is not NULL, it specifies the order of the
//		columns in the project list, otherwise any order is good
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnProjList(
	gpos::pointer<const CColRefSet *> pcrsOutput,
	gpos::pointer<CColRefArray *> colref_array)
{
	GPOS_ASSERT(nullptr != pcrsOutput);

	gpos::owner<CDXLScalarProjList *> pdxlopPrL =
		GPOS_NEW(m_mp) CDXLScalarProjList(m_mp);
	gpos::owner<CDXLNode *> pdxlnPrL = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopPrL);

	if (nullptr != colref_array)
	{
		gpos::owner<CColRefSet *> pcrs = GPOS_NEW(m_mp) CColRefSet(m_mp);

		for (ULONG ul = 0; ul < colref_array->Size(); ul++)
		{
			CColRef *colref = (*colref_array)[ul];

			gpos::owner<CDXLNode *> pdxlnPrEl =
				CTranslatorExprToDXLUtils::PdxlnProjElem(m_mp, m_phmcrdxln,
														 colref);
			pdxlnPrL->AddChild(pdxlnPrEl);
			pcrs->Include(colref);
		}

		// add the remaining required columns
		CColRefSetIter crsi(*pcrsOutput);
		while (crsi.Advance())
		{
			CColRef *colref = crsi.Pcr();

			if (!pcrs->FMember(colref))
			{
				gpos::owner<CDXLNode *> pdxlnPrEl =
					CTranslatorExprToDXLUtils::PdxlnProjElem(m_mp, m_phmcrdxln,
															 colref);
				pdxlnPrL->AddChild(pdxlnPrEl);
				pcrs->Include(colref);
			}
		}
		pcrs->Release();
	}
	else
	{
		// no order specified
		CColRefSetIter crsi(*pcrsOutput);
		while (crsi.Advance())
		{
			CColRef *colref = crsi.Pcr();
			gpos::owner<CDXLNode *> pdxlnPrEl =
				CTranslatorExprToDXLUtils::PdxlnProjElem(m_mp, m_phmcrdxln,
														 colref);
			pdxlnPrL->AddChild(pdxlnPrEl);
		}
	}

	return pdxlnPrL;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnProjList
//
//	@doc:
//		 Translate a project list expression into DXL project list node
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnProjList(
	gpos::pointer<const CExpression *> pexprProjList,
	gpos::pointer<const CColRefSet *> pcrsRequired,
	gpos::pointer<CColRefArray *> colref_array)
{
	if (nullptr == colref_array)
	{
		// no order specified
		return PdxlnProjList(pexprProjList, pcrsRequired);
	}

	// translate computed column expressions into DXL and index them on their col ids
	gpos::owner<
		CHashMap<ULONG, CDXLNode, gpos::HashValue<ULONG>, gpos::Equals<ULONG>,
				 CleanupDelete<ULONG>, CleanupRelease<CDXLNode> > *>
		phmComputedColumns =
			GPOS_NEW(m_mp) CHashMap<ULONG, CDXLNode, gpos::HashValue<ULONG>,
									gpos::Equals<ULONG>, CleanupDelete<ULONG>,
									CleanupRelease<CDXLNode> >(m_mp);

	for (ULONG ul = 0; nullptr != pexprProjList && ul < pexprProjList->Arity();
		 ul++)
	{
		gpos::pointer<CExpression *> pexprProjElem = (*pexprProjList)[ul];

		// translate proj elem
		gpos::owner<CDXLNode *> pdxlnProjElem = PdxlnProjElem(pexprProjElem);

		gpos::pointer<const CScalarProjectElement *> popScPrEl =
			gpos::dyn_cast<CScalarProjectElement>(pexprProjElem->Pop());

		ULONG *pulKey = GPOS_NEW(m_mp) ULONG(popScPrEl->Pcr()->Id());
		BOOL fInserted GPOS_ASSERTS_ONLY =
			phmComputedColumns->Insert(pulKey, pdxlnProjElem);

		GPOS_ASSERT(fInserted);
	}

	// add required columns to the project list
	gpos::owner<CColRefArray *> pdrgpcrCopy = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrCopy->AppendArray(colref_array);
	gpos::owner<CColRefSet *> pcrsOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsOutput->Include(colref_array);
	CColRefSetIter crsi(*pcrsRequired);
	while (crsi.Advance())
	{
		CColRef *colref = crsi.Pcr();
		if (!pcrsOutput->FMember(colref))
		{
			pdrgpcrCopy->Append(colref);
		}
	}

	// translate project list according to the specified order
	gpos::owner<CDXLScalarProjList *> pdxlopPrL =
		GPOS_NEW(m_mp) CDXLScalarProjList(m_mp);
	gpos::owner<CDXLNode *> proj_list_dxlnode =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopPrL);

	const ULONG num_cols = pdrgpcrCopy->Size();
	for (ULONG ul = 0; ul < num_cols; ul++)
	{
		CColRef *colref = (*pdrgpcrCopy)[ul];
		ULONG ulKey = colref->Id();
		gpos::owner<CDXLNode *> pdxlnProjElem =
			phmComputedColumns->Find(&ulKey);

		if (nullptr == pdxlnProjElem)
		{
			// not a computed column
			pdxlnProjElem = CTranslatorExprToDXLUtils::PdxlnProjElem(
				m_mp, m_phmcrdxln, colref);
		}
		else
		{
			pdxlnProjElem->AddRef();
		}

		proj_list_dxlnode->AddChild(pdxlnProjElem);
	}

	// cleanup
	pdrgpcrCopy->Release();
	pcrsOutput->Release();
	phmComputedColumns->Release();

	return proj_list_dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnProjList
//
//	@doc:
//		 Translate a project list expression into DXL project list node
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnProjList(
	gpos::pointer<const CExpression *> pexprProjList,
	gpos::pointer<const CColRefSet *> pcrsRequired)
{
	gpos::owner<CDXLScalarProjList *> pdxlopPrL =
		GPOS_NEW(m_mp) CDXLScalarProjList(m_mp);
	gpos::owner<CDXLNode *> proj_list_dxlnode =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopPrL);

	// create a copy of the required output columns
	gpos::owner<CColRefSet *> pcrsOutput =
		GPOS_NEW(m_mp) CColRefSet(m_mp, *pcrsRequired);

	if (nullptr != pexprProjList)
	{
		// translate defined columns from project list
		for (ULONG ul = 0; ul < pexprProjList->Arity(); ul++)
		{
			gpos::pointer<CExpression *> pexprProjElem = (*pexprProjList)[ul];

			// translate proj elem
			gpos::owner<CDXLNode *> pdxlnProjElem =
				PdxlnProjElem(pexprProjElem);
			proj_list_dxlnode->AddChild(pdxlnProjElem);

			// exclude proj elem col ref from the output column set as it has been
			// processed already
			gpos::pointer<const CScalarProjectElement *> popScPrEl =
				gpos::dyn_cast<CScalarProjectElement>(pexprProjElem->Pop());
			pcrsOutput->Exclude(popScPrEl->Pcr());
		}
	}

	// translate columns which remained after processing the project list: those
	// are columns passed from the level below
	CColRefSetIter crsi(*pcrsOutput);
	while (crsi.Advance())
	{
		CColRef *colref = crsi.Pcr();
		gpos::owner<CDXLNode *> pdxlnPrEl =
			CTranslatorExprToDXLUtils::PdxlnProjElem(m_mp, m_phmcrdxln, colref);
		proj_list_dxlnode->AddChild(pdxlnPrEl);
	}

	// cleanup
	pcrsOutput->Release();

	return proj_list_dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnProjListFromConstTableGet
//
//	@doc:
//		Construct a project list node by creating references to the columns
//		of the given project list of the child node
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnProjListFromConstTableGet(
	gpos::pointer<CColRefArray *> pdrgpcrReqOutput,
	gpos::pointer<CColRefArray *> pdrgpcrCTGOutput,
	gpos::pointer<IDatumArray *> pdrgpdatumValues)
{
	GPOS_ASSERT(nullptr != pdrgpcrCTGOutput);
	GPOS_ASSERT(nullptr != pdrgpdatumValues);
	GPOS_ASSERT(pdrgpcrCTGOutput->Size() == pdrgpdatumValues->Size());

	gpos::owner<CDXLNode *> proj_list_dxlnode = nullptr;
	gpos::owner<CColRefSet *> pcrsOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsOutput->Include(pdrgpcrCTGOutput);

	if (nullptr != pdrgpcrReqOutput)
	{
		const ULONG arity = pdrgpcrReqOutput->Size();
		gpos::owner<IDatumArray *> pdrgpdatumOrdered =
			GPOS_NEW(m_mp) IDatumArray(m_mp);

		for (ULONG ul = 0; ul < arity; ul++)
		{
			CColRef *colref = (*pdrgpcrReqOutput)[ul];
			ULONG ulPos = UlPosInArray(colref, pdrgpcrCTGOutput);
			GPOS_ASSERT(ulPos < pdrgpcrCTGOutput->Size());
			gpos::owner<IDatum *> datum = (*pdrgpdatumValues)[ulPos];
			datum->AddRef();
			pdrgpdatumOrdered->Append(datum);
			pcrsOutput->Exclude(colref);
		}

		proj_list_dxlnode = PdxlnProjListFromConstTableGet(
			nullptr, pdrgpcrReqOutput, pdrgpdatumOrdered);
		pdrgpdatumOrdered->Release();
	}
	else
	{
		proj_list_dxlnode = GPOS_NEW(m_mp)
			CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarProjList(m_mp));
	}

	// construct project elements for columns which remained after processing the required list
	CColRefSetIter crsi(*pcrsOutput);
	while (crsi.Advance())
	{
		CColRef *colref = crsi.Pcr();
		ULONG ulPos = UlPosInArray(colref, pdrgpcrCTGOutput);
		GPOS_ASSERT(ulPos < pdrgpcrCTGOutput->Size());
		gpos::pointer<IDatum *> datum = (*pdrgpdatumValues)[ulPos];
		gpos::owner<CDXLScalarConstValue *> pdxlopConstValue =
			colref->RetrieveType()->GetDXLOpScConst(m_mp, datum);
		gpos::owner<CDXLNode *> pdxlnPrEl = PdxlnProjElem(
			colref, GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopConstValue));
		proj_list_dxlnode->AddChild(pdxlnPrEl);
	}

	// cleanup
	pcrsOutput->Release();

	return proj_list_dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnProjElem
//
//	@doc:
//		 Create a project elem from a given col ref and a value
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnProjElem(const CColRef *colref,
									gpos::owner<CDXLNode *> pdxlnValue)
{
	GPOS_ASSERT(nullptr != colref);

	CMDName *mdname = GPOS_NEW(m_mp) CMDName(m_mp, colref->Name().Pstr());
	gpos::owner<CDXLNode *> pdxlnPrEl = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarProjElem(m_mp, colref->Id(), mdname));

	// attach scalar id expression to proj elem
	pdxlnPrEl->AddChild(std::move(pdxlnValue));

	return pdxlnPrEl;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnProjElem
//
//	@doc:
//		 Create a project elem from a given col ref
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnProjElem(
	gpos::pointer<const CExpression *> pexprProjElem)
{
	GPOS_ASSERT(nullptr != pexprProjElem && 1 == pexprProjElem->Arity());

	gpos::pointer<CScalarProjectElement *> popScPrEl =
		gpos::dyn_cast<CScalarProjectElement>(pexprProjElem->Pop());

	CColRef *colref = popScPrEl->Pcr();

	CMDName *mdname = GPOS_NEW(m_mp) CMDName(m_mp, colref->Name().Pstr());
	gpos::owner<CDXLNode *> pdxlnPrEl = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarProjElem(m_mp, colref->Id(), mdname));

	gpos::pointer<CExpression *> pexprChild = (*pexprProjElem)[0];
	gpos::owner<CDXLNode *> child_dxlnode = PdxlnScalar(pexprChild);

	pdxlnPrEl->AddChild(std::move(child_dxlnode));

	return pdxlnPrEl;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::GetSortColListDXL
//
//	@doc:
//		 Create a dxl sort column list node from a given order spec
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::GetSortColListDXL(gpos::pointer<const COrderSpec *> pos)
{
	GPOS_ASSERT(nullptr != pos);

	gpos::owner<CDXLNode *> sort_col_list_dxlnode = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarSortColList(m_mp));

	for (ULONG ul = 0; ul < pos->UlSortColumns(); ul++)
	{
		// get sort column components
		gpos::owner<IMDId *> sort_op_id = pos->GetMdIdSortOp(ul);
		sort_op_id->AddRef();

		const CColRef *colref = pos->Pcr(ul);

		COrderSpec::ENullTreatment ent = pos->Ent(ul);
		GPOS_ASSERT(COrderSpec::EntFirst == ent || COrderSpec::EntLast == ent ||
					COrderSpec::EntAuto == ent);

		// get sort operator name
		gpos::pointer<const IMDScalarOp *> md_scalar_op =
			m_pmda->RetrieveScOp(sort_op_id);

		CWStringConst *sort_op_name = GPOS_NEW(m_mp) CWStringConst(
			m_mp, md_scalar_op->Mdname().GetMDName()->GetBuffer());

		BOOL fSortNullsFirst = false;
		if (COrderSpec::EntFirst == ent)
		{
			fSortNullsFirst = true;
		}

		gpos::owner<CDXLScalarSortCol *> pdxlopSortCol =
			GPOS_NEW(m_mp) CDXLScalarSortCol(m_mp, colref->Id(), sort_op_id,
											 sort_op_name, fSortNullsFirst);

		gpos::owner<CDXLNode *> pdxlnSortCol =
			GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopSortCol);
		sort_col_list_dxlnode->AddChild(pdxlnSortCol);
	}

	return sort_col_list_dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnHashExprList
//
//	@doc:
//		 Create a dxl hash expr list node from a given array of column references
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnHashExprList(
	gpos::pointer<const CExpressionArray *> pdrgpexpr,
	gpos::pointer<const IMdIdArray *> opfamilies)
{
	GPOS_ASSERT(nullptr != pdrgpexpr);
	GPOS_ASSERT_IMP(GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution),
					nullptr != opfamilies);

	gpos::owner<CDXLNode *> hash_expr_list = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarHashExprList(m_mp));

	for (ULONG ul = 0; ul < pdrgpexpr->Size(); ul++)
	{
		gpos::pointer<CExpression *> pexpr = (*pdrgpexpr)[ul];

		IMDId *opfamily = nullptr;
		if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution))
		{
			opfamily = (*opfamilies)[ul];
			opfamily->AddRef();
		}

		// constrct a hash expr node for the col ref
		gpos::owner<CDXLNode *> pdxlnHashExpr = GPOS_NEW(m_mp)
			CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarHashExpr(m_mp, opfamily));

		pdxlnHashExpr->AddChild(PdxlnScalar(pexpr));

		hash_expr_list->AddChild(pdxlnHashExpr);
	}

	return hash_expr_list;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::GetSortColListDXL
//
//	@doc:
//		 Create a dxl sort column list node for a given motion operator
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::GetSortColListDXL(
	gpos::pointer<CExpression *> pexprMotion)
{
	gpos::owner<CDXLNode *> sort_col_list_dxlnode = nullptr;
	if (COperator::EopPhysicalMotionGather == pexprMotion->Pop()->Eopid())
	{
		// construct a sorting column list node
		gpos::pointer<CPhysicalMotionGather *> popGather =
			gpos::dyn_cast<CPhysicalMotionGather>(pexprMotion->Pop());
		sort_col_list_dxlnode = GetSortColListDXL(popGather->Pos());
	}
	else
	{
		sort_col_list_dxlnode = GPOS_NEW(m_mp)
			CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarSortColList(m_mp));
	}

	return sort_col_list_dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::GetOutputSegIdsArray
//
//	@doc:
//		Construct an array with output segment indices for the given Motion
//		expression.
//
//---------------------------------------------------------------------------
gpos::owner<IntPtrArray *>
CTranslatorExprToDXL::GetOutputSegIdsArray(
	gpos::pointer<CExpression *> pexprMotion)
{
	gpos::owner<IntPtrArray *> pdrgpi = nullptr;

	gpos::pointer<COperator *> pop = pexprMotion->Pop();

	switch (pop->Eopid())
	{
		case COperator::EopPhysicalMotionGather:
		{
			gpos::pointer<CPhysicalMotionGather *> popGather =
				gpos::dyn_cast<CPhysicalMotionGather>(pop);

			pdrgpi = GPOS_NEW(m_mp) IntPtrArray(m_mp);
			INT iSegmentId = m_iMasterId;

			if (CDistributionSpecSingleton::EstSegment == popGather->Est())
			{
				// gather to first segment
				iSegmentId = *((*m_pdrgpiSegments)[0]);
			}
			pdrgpi->Append(GPOS_NEW(m_mp) INT(iSegmentId));
			break;
		}
		case COperator::EopPhysicalMotionBroadcast:
		case COperator::EopPhysicalMotionHashDistribute:
		case COperator::EopPhysicalMotionRoutedDistribute:
		case COperator::EopPhysicalMotionRandom:
		{
			m_pdrgpiSegments->AddRef();
			pdrgpi = m_pdrgpiSegments;
			break;
		}
		default:
			GPOS_ASSERT(!"Unrecognized motion operator");
	}

	GPOS_ASSERT(nullptr != pdrgpi);

	return pdrgpi;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::GetInputSegIdsArray
//
//	@doc:
//		Construct an array with input segment indices for the given Motion
//		expression.
//
//---------------------------------------------------------------------------
gpos::owner<IntPtrArray *>
CTranslatorExprToDXL::GetInputSegIdsArray(
	gpos::pointer<CExpression *> pexprMotion)
{
	GPOS_ASSERT(1 == pexprMotion->Arity());

	// derive the distribution of child expression
	gpos::pointer<CExpression *> pexprChild = (*pexprMotion)[0];
	gpos::pointer<CDrvdPropPlan *> pdpplan =
		gpos::dyn_cast<CDrvdPropPlan>(pexprChild->PdpDerive());
	gpos::pointer<CDistributionSpec *> pds = pdpplan->Pds();

	if (CDistributionSpec::EdtSingleton == pds->Edt() ||
		CDistributionSpec::EdtStrictSingleton == pds->Edt())
	{
		gpos::owner<IntPtrArray *> pdrgpi = GPOS_NEW(m_mp) IntPtrArray(m_mp);
		INT iSegmentId = m_iMasterId;
		gpos::pointer<CDistributionSpecSingleton *> pdss =
			gpos::dyn_cast<CDistributionSpecSingleton>(pds);
		if (!pdss->FOnMaster())
		{
			// non-master singleton is currently fixed to the first segment
			iSegmentId = *((*m_pdrgpiSegments)[0]);
		}
		pdrgpi->Append(GPOS_NEW(m_mp) INT(iSegmentId));
		return pdrgpi;
	}

	if (CUtils::FDuplicateHazardMotion(pexprMotion) ||
		CDistributionSpec::EdtTaintedReplicated == pds->Edt())
	{
		// if Motion is duplicate-hazard, we have to read from one input segment
		// to avoid generating duplicate values
		//
		// Additionally, if the child distribution is tainted-replicated, the
		// motion (which cannot be a result hash filter node) will read the
		// input from one segment in order to ensure that data is consistent
		// after bring read from operator delivering tainted replication.
		gpos::owner<IntPtrArray *> pdrgpi = GPOS_NEW(m_mp) IntPtrArray(m_mp);
		INT iSegmentId = *((*m_pdrgpiSegments)[0]);
		pdrgpi->Append(GPOS_NEW(m_mp) INT(iSegmentId));
		return pdrgpi;
	}

	m_pdrgpiSegments->AddRef();
	return m_pdrgpiSegments;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::UlPosInArray
//
//	@doc:
//		Find position of colref in the array
//
//---------------------------------------------------------------------------
ULONG
CTranslatorExprToDXL::UlPosInArray(
	const CColRef *colref, gpos::pointer<const CColRefArray *> colref_array)
{
	GPOS_ASSERT(nullptr != colref_array);
	GPOS_ASSERT(nullptr != colref);

	const ULONG size = colref_array->Size();

	for (ULONG ul = 0; ul < size; ul++)
	{
		if (colref == (*colref_array)[ul])
		{
			return ul;
		}
	}

	// not found
	return size;
}

// A wrapper around CTranslatorExprToDXLUtils::PdxlnResult to check if the project list imposes a motion hazard,
// eventually leading to a deadlock. If yes, add a Materialize on the Result child to break the deadlock cycle
gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnResult(
	gpos::owner<CDXLPhysicalProperties *> dxl_properties,
	gpos::owner<CDXLNode *> pdxlnPrL, gpos::owner<CDXLNode *> child_dxlnode)
{
	gpos::owner<CDXLNode *> pdxlnMaterialize = nullptr;
	gpos::owner<CDXLNode *> pdxlnScalarOneTimeFilter = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarOneTimeFilter(m_mp));

	// If the result project list contains a subplan with a Broadcast motion,
	// along with other projections from the result's child node with a motion as well,
	// it may result in a deadlock. In such cases, add a materialize node.
	if (FNeedsMaterializeUnderResult(pdxlnPrL, child_dxlnode))
	{
		pdxlnMaterialize = PdxlnMaterialize(child_dxlnode);
	}

	return CTranslatorExprToDXLUtils::PdxlnResult(
		m_mp, std::move(dxl_properties), std::move(pdxlnPrL),
		PdxlnFilter(nullptr), std::move(pdxlnScalarOneTimeFilter),
		pdxlnMaterialize ? pdxlnMaterialize : child_dxlnode);
}

gpos::owner<CDXLNode *>
CTranslatorExprToDXL::PdxlnMaterialize(
	gpos::owner<CDXLNode *> dxlnode	 // node that needs to be materialized
)
{
	GPOS_ASSERT(nullptr != dxlnode);
	GPOS_ASSERT(nullptr != dxlnode->GetProperties());

	gpos::owner<CDXLPhysicalMaterialize *> pdxlopMaterialize =
		GPOS_NEW(m_mp) CDXLPhysicalMaterialize(m_mp, true /* fEager */);
	gpos::owner<CDXLNode *> pdxlnMaterialize =
		GPOS_NEW(m_mp) CDXLNode(m_mp, std::move(pdxlopMaterialize));
	gpos::owner<CDXLPhysicalProperties *> pdxlpropChild =
		gpos::dyn_cast<CDXLPhysicalProperties>(dxlnode->GetProperties());
	pdxlpropChild->AddRef();
	pdxlnMaterialize->SetProperties(std::move(pdxlpropChild));

	// construct an empty filter node
	gpos::owner<CDXLNode *> filter_dxlnode =
		PdxlnFilter(nullptr /* pdxlnCond */);

	gpos::pointer<CDXLNode *> pdxlnProjListChild = (*dxlnode)[0];
	gpos::owner<CDXLNode *> proj_list_dxlnode =
		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
			m_mp, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	// add children
	pdxlnMaterialize->AddChild(std::move(proj_list_dxlnode));
	pdxlnMaterialize->AddChild(std::move(filter_dxlnode));
	pdxlnMaterialize->AddChild(std::move(dxlnode));
	return pdxlnMaterialize;
}

BOOL
CTranslatorExprToDXL::FNeedsMaterializeUnderResult(
	gpos::pointer<CDXLNode *> proj_list_dxlnode,
	gpos::pointer<CDXLNode *> child_dxlnode)
{
	if (GPOS_FTRACE(EopttraceMotionHazardHandling))
	{
		// If motion hazard handling is enabled then optimization framework has
		// already handled this, hence no materialize is needed in this case.
		return false;
	}

	BOOL fMotionHazard = false;

	// if there is no subplan with a broadcast motion in the project list,
	// then don't bother checking for motion hazard
	BOOL fPrjListContainsSubplan =
		CTranslatorExprToDXLUtils::FProjListContainsSubplanWithBroadCast(
			proj_list_dxlnode);

	if (fPrjListContainsSubplan)
	{
		gpos::owner<CBitSet *> pbsScIdentColIds = GPOS_NEW(m_mp) CBitSet(m_mp);

		// recurse into project elements to extract out columns ids of scalar idents
		CTranslatorExprToDXLUtils::ExtractIdentColIds(proj_list_dxlnode,
													  pbsScIdentColIds);

		// result node will impose motion hazard only if it projects a Subplan
		// and an Ident produced by a tree that contains a motion
		if (pbsScIdentColIds->Size() > 0)
		{
			// motions which can impose a hazard
			gpdxl::Edxlopid rgeopid[] = {
				EdxlopPhysicalMotionBroadcast,
				EdxlopPhysicalMotionRedistribute,
				EdxlopPhysicalMotionRandom,
			};

			fMotionHazard = CTranslatorExprToDXLUtils::FMotionHazard(
				m_mp, child_dxlnode, rgeopid, GPOS_ARRAY_SIZE(rgeopid),
				pbsScIdentColIds);
		}
		pbsScIdentColIds->Release();
	}
	return fMotionHazard;
}

// EOF
