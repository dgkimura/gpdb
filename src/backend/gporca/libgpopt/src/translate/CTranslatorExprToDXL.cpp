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

#include "gpopt/base/CCastUtils.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/CDrvdPropCtxt.h"
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
#include "gpopt/operators/CPhysicalDynamicForeignScan.h"
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
#include "naucrates/dxl/operators/CDXLPhysicalDynamicBitmapTableScan.h"
#include "naucrates/dxl/operators/CDXLPhysicalDynamicForeignScan.h"
#include "naucrates/dxl/operators/CDXLPhysicalDynamicIndexScan.h"
#include "naucrates/dxl/operators/CDXLPhysicalDynamicTableScan.h"
#include "naucrates/dxl/operators/CDXLPhysicalForeignScan.h"
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
	gpos::Ref<IntPtrArray> pdrgpiSegments, BOOL fInitColumnFactory)
	: m_mp(mp),
	  m_pmda(md_accessor),
	  m_pdpplan(nullptr),
	  m_pcf(nullptr),
	  m_pdrgpiSegments(pdrgpiSegments),
	  m_iMasterId(GPOPT_MASTER_SEGMENT_ID)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != md_accessor);
	GPOS_ASSERT_IMP(nullptr != pdrgpiSegments, (0 < pdrgpiSegments->Size()));

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
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnTranslate
//
//	@doc:
//		Main driver
//
//---------------------------------------------------------------------------
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnTranslate(gpos::Ref<CExpression> pexpr,
									 gpos::Ref<CColRefArray> colref_array,
									 gpos::Ref<CMDNameArray> pdrgpmdname)
{
	CAutoTimer at("\n[OPT]: Expr To DXL Translation Time",
				  GPOS_FTRACE(EopttracePrintOptimizationStatistics));

	GPOS_ASSERT(nullptr == m_pdpplan);

	m_pdpplan = CDrvdPropPlan::Pdpplan(pexpr->PdpDerive());


	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables =
		GPOS_NEW(m_mp) CDistributionSpecArray(m_mp);
	ULONG ulNonGatherMotions = 0;
	BOOL fDML = false;
	gpos::Ref<CDXLNode> dxlnode = CreateDXLNode(
		pexpr, colref_array, pdrgpdsBaseTables, &ulNonGatherMotions, &fDML,
		true /*fRemap*/, true /*fRoot*/);

	if (fDML)
	{
		return dxlnode;
	}

	gpos::Ref<CDXLNode> pdxlnPrL = (*dxlnode)[0];
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
		gpos::Ref<CDXLNode> pdxlnPrElOld = (*pdxlnPrL)[ul];
		gpos::Ref<CDXLScalarProjElem> pdxlopPrElOld =
			CDXLScalarProjElem::Cast(pdxlnPrElOld->GetOperator());
		GPOS_ASSERT(1 == pdxlnPrElOld->Arity());
		gpos::Ref<CDXLNode> child_dxlnode = (*pdxlnPrElOld)[0];
		const ULONG colid = pdxlopPrElOld->Id();

		// create a new project element node with the col id and new column name
		// and add the scalar child
		gpos::Ref<CDXLNode> pdxlnPrElNew = GPOS_NEW(m_mp) CDXLNode(
			m_mp, GPOS_NEW(m_mp) CDXLScalarProjElem(m_mp, colid, mdname));

		pdxlnPrElNew->AddChild(child_dxlnode);

		// replace the project element
		pdxlnPrL->ReplaceChild(ul, pdxlnPrElNew);
	}



	if (0 == ulNonGatherMotions)
	{
		CTranslatorExprToDXLUtils::SetDirectDispatchInfo(
			m_mp, m_pmda, dxlnode, pexpr, pdrgpdsBaseTables);
	}


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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::CreateDXLNode(
	gpos::Ref<CExpression> pexpr, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML, BOOL fRemap, BOOL fRoot)
{
	GPOS_ASSERT(nullptr != pexpr);
	ULONG ulOpId = (ULONG) pexpr->Pop()->Eopid();
	if (COperator::EopPhysicalTableScan == ulOpId ||
		COperator::EopPhysicalForeignScan == ulOpId)
	{
		gpos::Ref<CDXLNode> dxlnode = PdxlnTblScan(
			pexpr, nullptr /*pcrsOutput*/, colref_array, pdrgpdsBaseTables,
			nullptr /* pexprScalarCond */, nullptr /* cost info */);
		CTranslatorExprToDXLUtils::SetStats(m_mp, m_pmda, dxlnode,
											pexpr->Pstats(), fRoot);

		return dxlnode;
	}
	// add a result node on top to project out columns not needed any further,
	// for instance, if the grouping /order by /partition/ distribution columns
	// are no longer needed
	gpos::Ref<CDXLNode> pdxlnNew = nullptr;

	gpos::Ref<CDXLNode> dxlnode = nullptr;
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
		case COperator::EopPhysicalDynamicForeignScan:
			dxlnode = CTranslatorExprToDXL::PdxlnDynamicForeignScan(
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
		gpos::Ref<CColRefArray> pdrgpcrRequired = nullptr;

		if (EdxlopPhysicalCTAS == dxlnode->GetOperator()->GetDXLOperator())
		{
			pdrgpcrRequired = colref_array;
		}
		else
		{
			pdrgpcrRequired = pexpr->Prpp()->PcrsRequired()->Pdrgpcr(m_mp);
		}
		pdxlnNew = PdxlnRemapOutputColumns(pexpr, dxlnode, pdrgpcrRequired,
										   colref_array);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScalar(gpos::Ref<CExpression> pexpr)
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnTblScan(
	gpos::Ref<CExpression> pexprTblScan, gpos::Ref<CColRefSet> pcrsOutput,
	gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	gpos::Ref<CExpression> pexprScalar,
	gpos::Ref<CDXLPhysicalProperties> dxl_properties)
{
	GPOS_ASSERT(nullptr != pexprTblScan);

	gpos::Ref<CPhysicalTableScan> popTblScan =
		CPhysicalTableScan::PopConvert(pexprTblScan->Pop());
	gpos::Ref<CColRefArray> pdrgpcrOutput = popTblScan->PdrgpcrOutput();

	// translate table descriptor
	gpos::Ref<CDXLTableDescr> table_descr = MakeDXLTableDescr(
		popTblScan->Ptabdesc(), pdrgpcrOutput, pexprTblScan->Prpp());

	// construct plan costs, if there are not passed as a parameter
	if (nullptr == dxl_properties)
	{
		dxl_properties = GetProperties(pexprTblScan);
	}

	// construct scan operator
	gpos::Ref<CDXLPhysicalTableScan> pdxlopTS = nullptr;
	COperator::EOperatorId op_id = pexprTblScan->Pop()->Eopid();
	if (COperator::EopPhysicalTableScan == op_id)
	{
		pdxlopTS = GPOS_NEW(m_mp) CDXLPhysicalTableScan(m_mp, table_descr);
	}
	else
	{
		GPOS_ASSERT(COperator::EopPhysicalForeignScan == op_id);
		pdxlopTS = GPOS_NEW(m_mp) CDXLPhysicalForeignScan(m_mp, table_descr);
	}

	gpos::Ref<CDXLNode> pdxlnTblScan = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopTS);
	pdxlnTblScan->SetProperties(dxl_properties);

	// construct projection list
	GPOS_ASSERT(nullptr != pexprTblScan->Prpp());

	// if the output columns are passed from above then use them
	if (nullptr == pcrsOutput)
	{
		pcrsOutput = pexprTblScan->Prpp()->PcrsRequired();
	}
	gpos::Ref<CDXLNode> pdxlnPrL = PdxlnProjList(pcrsOutput, colref_array);

	gpos::Ref<CDXLNode> pdxlnCond = nullptr;
	if (nullptr != pexprScalar)
	{
		pdxlnCond = PdxlnScalar(pexprScalar);
	}

	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(pdxlnCond);

	// add children in the right order
	pdxlnTblScan->AddChild(pdxlnPrL);		 // project list
	pdxlnTblScan->AddChild(filter_dxlnode);	 // filter

#ifdef GPOS_DEBUG
	pdxlnTblScan->GetOperator()->AssertValid(pdxlnTblScan,
											 false /* validate_children */);
#endif

	gpos::Ref<CDistributionSpec> pds = pexprTblScan->GetDrvdPropPlan()->Pds();

	pdrgpdsBaseTables->Append(pds);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnIndexScan(
	gpos::Ref<CExpression> pexprIndexScan, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *,  // pulNonGatherMotions,
	BOOL *	  // pfDML
)
{
	GPOS_ASSERT(nullptr != pexprIndexScan);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprIndexScan);

	COptCtxt::PoctxtFromTLS()->AddDirectDispatchableFilterCandidate(
		pexprIndexScan);

	gpos::Ref<CDistributionSpec> pds = pexprIndexScan->GetDrvdPropPlan()->Pds();

	pdrgpdsBaseTables->Append(pds);
	return PdxlnIndexScan(pexprIndexScan, colref_array, dxl_properties,
						  pexprIndexScan->Prpp());
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnIndexScan
//
//	@doc:
//		Create a DXL index scan node from an optimizer index scan node
//
//---------------------------------------------------------------------------
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnIndexScan(
	gpos::Ref<CExpression> pexprIndexScan, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDXLPhysicalProperties> dxl_properties,
	gpos::Ref<CReqdPropPlan> prpp)
{
	GPOS_ASSERT(nullptr != pexprIndexScan);
	GPOS_ASSERT(nullptr != dxl_properties);
	GPOS_ASSERT(nullptr != prpp);

	gpos::Ref<CPhysicalIndexScan> popIs =
		CPhysicalIndexScan::PopConvert(pexprIndexScan->Pop());

	gpos::Ref<CColRefArray> pdrgpcrOutput = popIs->PdrgpcrOutput();

	// translate table descriptor
	gpos::Ref<CDXLTableDescr> table_descr = MakeDXLTableDescr(
		popIs->Ptabdesc(), pdrgpcrOutput, pexprIndexScan->Prpp());

	// create index descriptor
	gpos::Ref<CIndexDescriptor> pindexdesc = popIs->Pindexdesc();
	CMDName *pmdnameIndex =
		GPOS_NEW(m_mp) CMDName(m_mp, pindexdesc->Name().Pstr());
	gpos::Ref<IMDId> pmdidIndex = pindexdesc->MDId();

	gpos::Ref<CDXLIndexDescr> dxl_index_descr =
		GPOS_NEW(m_mp) CDXLIndexDescr(pmdidIndex, pmdnameIndex);

	// TODO: vrgahavan; we assume that the index are always forward access.
	// create the physical index scan operator
	gpos::Ref<CDXLPhysicalIndexScan> dxl_op =
		GPOS_NEW(m_mp) CDXLPhysicalIndexScan(m_mp, table_descr, dxl_index_descr,
											 EdxlisdForward);
	gpos::Ref<CDXLNode> pdxlnIndexScan = GPOS_NEW(m_mp) CDXLNode(m_mp, dxl_op);

	// set properties
	pdxlnIndexScan->SetProperties(dxl_properties);

	// translate project list
	gpos::Ref<CColRefSet> pcrsOutput = prpp->PcrsRequired();
	gpos::Ref<CDXLNode> pdxlnPrL = PdxlnProjList(pcrsOutput, colref_array);

	// translate index predicates
	gpos::Ref<CExpression> pexprCond = (*pexprIndexScan)[0];
	gpos::Ref<CDXLNode> pdxlnIndexCondList = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarIndexCondList(m_mp));

	gpos::Ref<CExpressionArray> pdrgpexprConds =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprCond);
	const ULONG length = pdrgpexprConds->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::Ref<CExpression> pexprIndexCond = (*pdrgpexprConds)[ul];
		gpos::Ref<CDXLNode> pdxlnIndexCond = PdxlnScalar(pexprIndexCond);
		pdxlnIndexCondList->AddChild(pdxlnIndexCond);
	}


	gpos::Ref<CDXLNode> pdxlnResidualCond = nullptr;
	if (2 == pexprIndexScan->Arity())
	{
		// translate residual predicates into the filter node
		gpos::Ref<CExpression> pexprResidualCond = (*pexprIndexScan)[1];
		if (COperator::EopScalarConst != pexprResidualCond->Pop()->Eopid())
		{
			pdxlnResidualCond = PdxlnScalar(pexprResidualCond);
		}
	}

	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(pdxlnResidualCond);

	pdxlnIndexScan->AddChild(pdxlnPrL);
	pdxlnIndexScan->AddChild(filter_dxlnode);
	pdxlnIndexScan->AddChild(pdxlnIndexCondList);

#ifdef GPOS_DEBUG
	pdxlnIndexScan->GetOperator()->AssertValid(pdxlnIndexScan,
											   false /* validate_children */);
#endif


	return pdxlnIndexScan;
}

gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnIndexOnlyScan(
	gpos::Ref<CExpression> pexprIndexOnlyScan,
	gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *,  // pulNonGatherMotions,
	BOOL *	  // pfDML
)
{
	GPOS_ASSERT(nullptr != pexprIndexOnlyScan);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprIndexOnlyScan);

	COptCtxt::PoctxtFromTLS()->AddDirectDispatchableFilterCandidate(
		pexprIndexOnlyScan);

	gpos::Ref<CDistributionSpec> pds =
		pexprIndexOnlyScan->GetDrvdPropPlan()->Pds();

	pdrgpdsBaseTables->Append(pds);
	return PdxlnIndexOnlyScan(pexprIndexOnlyScan, colref_array, dxl_properties,
							  pexprIndexOnlyScan->Prpp());
}

gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnIndexOnlyScan(
	gpos::Ref<CExpression> pexprIndexOnlyScan,
	gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDXLPhysicalProperties> dxl_properties,
	gpos::Ref<CReqdPropPlan> prpp)
{
	GPOS_ASSERT(nullptr != pexprIndexOnlyScan);
	GPOS_ASSERT(nullptr != dxl_properties);
	GPOS_ASSERT(nullptr != prpp);

	gpos::Ref<CPhysicalIndexOnlyScan> popIs =
		CPhysicalIndexOnlyScan::PopConvert(pexprIndexOnlyScan->Pop());

	gpos::Ref<CColRefArray> pdrgpcrOutput = popIs->PdrgpcrOutput();

	// translate table descriptor
	gpos::Ref<CDXLTableDescr> table_descr = MakeDXLTableDescr(
		popIs->Ptabdesc(), pdrgpcrOutput, pexprIndexOnlyScan->Prpp());

	// create index descriptor
	gpos::Ref<CIndexDescriptor> pindexdesc = popIs->Pindexdesc();
	CMDName *pmdnameIndex =
		GPOS_NEW(m_mp) CMDName(m_mp, pindexdesc->Name().Pstr());
	gpos::Ref<IMDId> pmdidIndex = pindexdesc->MDId();

	gpos::Ref<CDXLIndexDescr> dxl_index_descr =
		GPOS_NEW(m_mp) CDXLIndexDescr(pmdidIndex, pmdnameIndex);

	// TODO: vrgahavan; we assume that the index are always forward access.
	// create the physical index scan operator
	gpos::Ref<CDXLPhysicalIndexOnlyScan> dxl_op =
		GPOS_NEW(m_mp) CDXLPhysicalIndexOnlyScan(
			m_mp, table_descr, dxl_index_descr, EdxlisdForward);
	gpos::Ref<CDXLNode> pdxlnIndexOnlyScan =
		GPOS_NEW(m_mp) CDXLNode(m_mp, dxl_op);

	// set properties
	pdxlnIndexOnlyScan->SetProperties(dxl_properties);

	// translate project list
	gpos::Ref<CColRefSet> pcrsOutput = prpp->PcrsRequired();
	gpos::Ref<CDXLNode> pdxlnPrL = PdxlnProjList(pcrsOutput, colref_array);

	// translate index predicates
	gpos::Ref<CExpression> pexprCond = (*pexprIndexOnlyScan)[0];
	gpos::Ref<CDXLNode> pdxlnIndexCondList = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarIndexCondList(m_mp));

	gpos::Ref<CExpressionArray> pdrgpexprConds =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprCond);
	const ULONG length = pdrgpexprConds->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::Ref<CExpression> pexprIndexCond = (*pdrgpexprConds)[ul];
		gpos::Ref<CDXLNode> pdxlnIndexCond = PdxlnScalar(pexprIndexCond);
		pdxlnIndexCondList->AddChild(pdxlnIndexCond);
	}


	gpos::Ref<CDXLNode> pdxlnResidualCond = nullptr;
	if (2 == pexprIndexOnlyScan->Arity())
	{
		// translate residual predicates into the filter node
		gpos::Ref<CExpression> pexprResidualCond = (*pexprIndexOnlyScan)[1];
		if (COperator::EopScalarConst != pexprResidualCond->Pop()->Eopid())
		{
			pdxlnResidualCond = PdxlnScalar(pexprResidualCond);
		}
	}

	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(pdxlnResidualCond);

	pdxlnIndexOnlyScan->AddChild(pdxlnPrL);
	pdxlnIndexOnlyScan->AddChild(filter_dxlnode);
	pdxlnIndexOnlyScan->AddChild(pdxlnIndexCondList);

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
//---------------------------------------------------------------------------
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnBitmapIndexProbe(
	gpos::Ref<CExpression> pexprBitmapIndexProbe)
{
	GPOS_ASSERT(nullptr != pexprBitmapIndexProbe);
	gpos::Ref<CScalarBitmapIndexProbe> pop =
		CScalarBitmapIndexProbe::PopConvert(pexprBitmapIndexProbe->Pop());

	// create index descriptor
	gpos::Ref<CIndexDescriptor> pindexdesc = pop->Pindexdesc();
	CMDName *pmdnameIndex =
		GPOS_NEW(m_mp) CMDName(m_mp, pindexdesc->Name().Pstr());
	gpos::Ref<IMDId> pmdidIndex = pindexdesc->MDId();


	gpos::Ref<CDXLIndexDescr> dxl_index_descr =
		GPOS_NEW(m_mp) CDXLIndexDescr(pmdidIndex, pmdnameIndex);
	gpos::Ref<CDXLScalarBitmapIndexProbe> dxl_op =
		GPOS_NEW(m_mp) CDXLScalarBitmapIndexProbe(m_mp, dxl_index_descr);
	gpos::Ref<CDXLNode> pdxlnBitmapIndexProbe =
		GPOS_NEW(m_mp) CDXLNode(m_mp, dxl_op);

	// translate index predicates
	gpos::Ref<CExpression> pexprCond = (*pexprBitmapIndexProbe)[0];
	gpos::Ref<CDXLNode> pdxlnIndexCondList = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarIndexCondList(m_mp));
	gpos::Ref<CExpressionArray> pdrgpexprConds =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprCond);
	const ULONG length = pdrgpexprConds->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::Ref<CExpression> pexprIndexCond = (*pdrgpexprConds)[ul];
		gpos::Ref<CDXLNode> pdxlnIndexCond = PdxlnScalar(pexprIndexCond);
		pdxlnIndexCondList->AddChild(pdxlnIndexCond);
	}

	pdxlnBitmapIndexProbe->AddChild(pdxlnIndexCondList);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnBitmapBoolOp(
	gpos::Ref<CExpression> pexprBitmapBoolOp)
{
	GPOS_ASSERT(nullptr != pexprBitmapBoolOp);
	GPOS_ASSERT(2 == pexprBitmapBoolOp->Arity());

	gpos::Ref<CScalarBitmapBoolOp> popBitmapBoolOp =
		CScalarBitmapBoolOp::PopConvert(pexprBitmapBoolOp->Pop());
	gpos::Ref<CExpression> pexprLeft = (*pexprBitmapBoolOp)[0];
	gpos::Ref<CExpression> pexprRight = (*pexprBitmapBoolOp)[1];

	gpos::Ref<CDXLNode> dxlnode_left = PdxlnScalar(pexprLeft);
	gpos::Ref<CDXLNode> dxlnode_right = PdxlnScalar(pexprRight);

	gpos::Ref<IMDId> mdid_type = popBitmapBoolOp->MdidType();


	CDXLScalarBitmapBoolOp::EdxlBitmapBoolOp edxlbitmapop =
		CDXLScalarBitmapBoolOp::EdxlbitmapAnd;

	if (CScalarBitmapBoolOp::EbitmapboolOr == popBitmapBoolOp->Ebitmapboolop())
	{
		edxlbitmapop = CDXLScalarBitmapBoolOp::EdxlbitmapOr;
	}

	return GPOS_NEW(m_mp) CDXLNode(
		m_mp,
		GPOS_NEW(m_mp) CDXLScalarBitmapBoolOp(m_mp, mdid_type, edxlbitmapop),
		dxlnode_left, dxlnode_right);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnBitmapTableScan(
	gpos::Ref<CExpression> pexprBitmapTableScan,
	gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
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
	CMemoryPool *mp, gpos::Ref<CPhysicalScan> pop,
	gpos::Ref<CExpression> pexprRecheckCond, gpos::Ref<CExpression> pexprScalar,
	gpos::Ref<CColRefSet>
		pcrsReqdOutput	// append the required column reference
)
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(COperator::EopPhysicalDynamicBitmapTableScan == pop->Eopid() ||
				COperator::EopPhysicalBitmapTableScan == pop->Eopid());
	GPOS_ASSERT(nullptr != pcrsReqdOutput);

	// compute what additional columns are required in the output of the (Dynamic) Bitmap Table Scan
	gpos::Ref<CColRefSet> pcrsAdditional = GPOS_NEW(mp) CColRefSet(mp);

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

	gpos::Ref<CColRefSet> pcrsBitmap = GPOS_NEW(mp) CColRefSet(mp);
	pcrsBitmap->Include(pop->PdrgpcrOutput());

	// only keep the columns that are in the table associated with the bitmap
	pcrsAdditional->Intersection(pcrsBitmap);

	if (0 < pcrsAdditional->Size())
	{
		pcrsReqdOutput->Include(pcrsAdditional);
	}

	// clean up
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnBitmapTableScan(
	gpos::Ref<CExpression> pexprBitmapTableScan,
	gpos::Ref<CColRefSet> pcrsOutput, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	gpos::Ref<CExpression> pexprScalar,
	gpos::Ref<CDXLPhysicalProperties> dxl_properties)
{
	GPOS_ASSERT(nullptr != pexprBitmapTableScan);
	gpos::Ref<CPhysicalBitmapTableScan> pop =
		CPhysicalBitmapTableScan::PopConvert(pexprBitmapTableScan->Pop());

	COptCtxt::PoctxtFromTLS()->AddDirectDispatchableFilterCandidate(
		pexprBitmapTableScan);

	// translate table descriptor
	gpos::Ref<CDXLTableDescr> table_descr = MakeDXLTableDescr(
		pop->Ptabdesc(), pop->PdrgpcrOutput(), pexprBitmapTableScan->Prpp());

	gpos::Ref<CDXLPhysicalBitmapTableScan> dxl_op =
		GPOS_NEW(m_mp) CDXLPhysicalBitmapTableScan(m_mp, table_descr);
	gpos::Ref<CDXLNode> pdxlnBitmapTableScan =
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
	gpos::Ref<CExpression> pexprRecheckCond = (*pexprBitmapTableScan)[0];
	gpos::Ref<CDXLNode> pdxlnCond = nullptr;
	if (nullptr != pexprScalar && !CUtils::FScalarConstTrue(pexprScalar) &&
		!pexprScalar->Matches(pexprRecheckCond))
	{
		pdxlnCond = PdxlnScalar(pexprScalar);
	}

	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(pdxlnCond);

	gpos::Ref<CDXLNode> pdxlnRecheckCond = PdxlnScalar(pexprRecheckCond);
	gpos::Ref<CDXLNode> pdxlnRecheckCondFilter = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarRecheckCondFilter(m_mp),
				 pdxlnRecheckCond);

	AddBitmapFilterColumns(m_mp, pop, pexprRecheckCond, pexprScalar,
						   pcrsOutput);

	gpos::Ref<CDXLNode> proj_list_dxlnode =
		PdxlnProjList(pcrsOutput, colref_array);

	// translate bitmap access path
	gpos::Ref<CDXLNode> pdxlnBitmapIndexPath =
		PdxlnScalar((*pexprBitmapTableScan)[1]);

	pdxlnBitmapTableScan->AddChild(proj_list_dxlnode);
	pdxlnBitmapTableScan->AddChild(filter_dxlnode);
	pdxlnBitmapTableScan->AddChild(pdxlnRecheckCondFilter);
	pdxlnBitmapTableScan->AddChild(pdxlnBitmapIndexPath);
#ifdef GPOS_DEBUG
	pdxlnBitmapTableScan->GetOperator()->AssertValid(
		pdxlnBitmapTableScan, false /*validate_children*/);
#endif

	gpos::Ref<CDistributionSpec> pds =
		pexprBitmapTableScan->GetDrvdPropPlan()->Pds();

	pdrgpdsBaseTables->Append(pds);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnDynamicTableScan(
	gpos::Ref<CExpression> pexprDTS, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *,  // pulNonGatherMotions,
	BOOL *	  // pfDML
)
{
	gpos::Ref<CExpression> pexprScalarCond = nullptr;
	gpos::Ref<CDXLPhysicalProperties> dxl_properties = nullptr;
	return PdxlnDynamicTableScan(pexprDTS, colref_array, pdrgpdsBaseTables,
								 pexprScalarCond, dxl_properties);
}

// Construct a dxl table descr for a child partition
gpos::Ref<CTableDescriptor>
CTranslatorExprToDXL::MakeTableDescForPart(
	const gpos::Ref<IMDRelation> part,
	gpos::Ref<CTableDescriptor> root_table_desc)
{
	gpos::Ref<IMDId> part_mdid = part->MDId();


	gpos::Ref<CTableDescriptor> table_descr = GPOS_NEW(m_mp) CTableDescriptor(
		m_mp, part_mdid, part->Mdname().GetMDName(),
		part->ConvertHashToRandom(), part->GetRelDistribution(),
		part->RetrieveRelStorageType(), root_table_desc->GetExecuteAsUserId(),
		root_table_desc->LockMode(),
		root_table_desc->GetAssignedQueryIdForTargetRel());

	for (ULONG ul = 0; ul < part->ColumnCount(); ++ul)
	{
		const gpos::Ref<IMDColumn> mdCol = part->GetMdCol(ul);
		if (mdCol->IsDropped())
		{
			continue;
		}
		CWStringConst strColName{m_mp,
								 mdCol->Mdname().GetMDName()->GetBuffer()};
		CName colname(m_mp, &strColName);
		gpos::Ref<CColumnDescriptor> coldesc = GPOS_NEW(m_mp)
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
gpos::Ref<CDXLIndexDescr>
CTranslatorExprToDXL::PdxlnIndexDescForPart(CMemoryPool *m_mp,
											MdidHashSet *child_index_mdids_set,
											const gpos::Ref<IMDRelation> part,
											const CWStringConst *index_name)
{
	// iterate over each index in the child to find the matching index
	gpos::Ref<IMDId> found_index = nullptr;
	for (ULONG j = 0; j < part->IndexCount(); ++j)
	{
		gpos::Ref<IMDId> pmdidPartIndex = part->IndexMDidAt(j);
		if (child_index_mdids_set->Contains(pmdidPartIndex))
		{
			found_index = pmdidPartIndex;
			break;
		}
	}
	GPOS_ASSERT(nullptr != found_index);


	// create index descriptor (this name is the parent name, but isn't displayed in the plan)
	CMDName *pmdnameIndex = GPOS_NEW(m_mp) CMDName(m_mp, index_name);
	gpos::Ref<CDXLIndexDescr> dxl_index_descr =
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnDynamicTableScan(
	gpos::Ref<CExpression> pexprDTS, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	gpos::Ref<CExpression> pexprScalarCond,
	gpos::Ref<CDXLPhysicalProperties> dxl_properties)
{
	GPOS_ASSERT(nullptr != pexprDTS);
	GPOS_ASSERT_IFF(nullptr != pexprScalarCond, nullptr != dxl_properties);

	gpos::Ref<CPhysicalDynamicTableScan> popDTS =
		CPhysicalDynamicTableScan::PopConvert(pexprDTS->Pop());
	gpos::Ref<CColRefArray> pdrgpcrOutput = popDTS->PdrgpcrOutput();

	// translate table descriptor
	gpos::Ref<CDXLTableDescr> table_descr =
		MakeDXLTableDescr(popDTS->Ptabdesc(), pdrgpcrOutput, pexprDTS->Prpp());

	// construct plan costs
	gpos::Ref<CDXLPhysicalProperties> pdxlpropDTS = GetProperties(pexprDTS);

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
	}

	// construct dynamic table scan operator
	gpos::Ref<IMdIdArray> part_mdids = popDTS->GetPartitionMdids();


	gpos::Ref<ULongPtrArray> selector_ids = GPOS_NEW(m_mp) ULongPtrArray(m_mp);
	gpos::Ref<CPartitionPropagationSpec> pps_reqd =
		pexprDTS->Prpp()->Pepp()->PppsRequired();
	if (pps_reqd->Contains(popDTS->ScanId()))
	{
		const gpos::Ref<CBitSet> bs = pps_reqd->SelectorIds(popDTS->ScanId());
		CBitSetIter bsi(*bs);
		while (bsi.Advance())
		{
			selector_ids->Append(GPOS_NEW(m_mp) ULONG(bsi.Bit()));
		}
	}


	gpos::Ref<CDXLPhysicalDynamicTableScan> pdxlopDTS =
		GPOS_NEW(m_mp) CDXLPhysicalDynamicTableScan(m_mp, table_descr,
													part_mdids, selector_ids);

	gpos::Ref<CDXLNode> pdxlnDTS = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopDTS);
	pdxlnDTS->SetProperties(pdxlpropDTS);

	gpos::Ref<CDXLNode> pdxlnCond = nullptr;

	if (nullptr != pexprScalarCond)
	{
		pdxlnCond = PdxlnScalar(pexprScalarCond);
	}

	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(pdxlnCond);

	// construct projection list
	GPOS_ASSERT(nullptr != pexprDTS->Prpp());

	gpos::Ref<CColRefSet> pcrsOutput = pexprDTS->Prpp()->PcrsRequired();
	pdxlnDTS->AddChild(PdxlnProjList(pcrsOutput, colref_array));
	pdxlnDTS->AddChild(filter_dxlnode);

#ifdef GPOS_DEBUG
	pdxlnDTS->GetOperator()->AssertValid(pdxlnDTS, false);
#endif

	gpos::Ref<CDistributionSpec> pds = pexprDTS->GetDrvdPropPlan()->Pds();

	pdrgpdsBaseTables->Append(pds);

	return pdxlnDTS;
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnDynamicBitmapTableScan(
	gpos::Ref<CExpression> pexprScan, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *,  // pulNonGatherMotions,
	BOOL *	  // pfDML
)
{
	gpos::Ref<CExpression> pexprScalar = nullptr;
	gpos::Ref<CDXLPhysicalProperties> dxl_properties = nullptr;
	return PdxlnDynamicBitmapTableScan(pexprScan, colref_array,
									   pdrgpdsBaseTables, pexprScalar,
									   dxl_properties);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnDynamicBitmapTableScan(
	gpos::Ref<CExpression> pexprScan, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	gpos::Ref<CExpression> pexprScalar,
	gpos::Ref<CDXLPhysicalProperties> dxl_properties)
{
	GPOS_ASSERT(nullptr != pexprScan);

	gpos::Ref<CPhysicalDynamicBitmapTableScan> pop =
		CPhysicalDynamicBitmapTableScan::PopConvert(pexprScan->Pop());
	gpos::Ref<CColRefArray> pdrgpcrOutput = pop->PdrgpcrOutput();

	gpos::Ref<CDXLTableDescr> table_descr =
		MakeDXLTableDescr(pop->Ptabdesc(), pdrgpcrOutput, pexprScan->Prpp());

	// construct dynamic table scan operator
	gpos::Ref<IMdIdArray> part_mdids = pop->GetPartitionMdids();


	gpos::Ref<ULongPtrArray> selector_ids = GPOS_NEW(m_mp) ULongPtrArray(m_mp);
	gpos::Ref<CPartitionPropagationSpec> pps_reqd =
		pexprScan->Prpp()->Pepp()->PppsRequired();
	if (pps_reqd->Contains(pop->ScanId()))
	{
		const gpos::Ref<CBitSet> bs = pps_reqd->SelectorIds(pop->ScanId());
		CBitSetIter bsi(*bs);
		while (bsi.Advance())
		{
			selector_ids->Append(GPOS_NEW(m_mp) ULONG(bsi.Bit()));
		}
	}

	gpos::Ref<CDXLPhysicalDynamicBitmapTableScan> pdxlopScan =
		GPOS_NEW(m_mp) CDXLPhysicalDynamicBitmapTableScan(
			m_mp, table_descr, part_mdids, selector_ids);

	gpos::Ref<CDXLNode> pdxlnScan = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopScan);

	// construct plan costs
	if (nullptr == dxl_properties)
	{
		dxl_properties = GetProperties(pexprScan);
	}
	pdxlnScan->SetProperties(dxl_properties);

	// translate predicates into DXL filter
	gpos::Ref<CDXLNode> pdxlnCond = nullptr;
	if (nullptr != pexprScalar)
	{
		pdxlnCond = PdxlnScalar(pexprScalar);
	}
	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(pdxlnCond);

	gpos::Ref<CExpression> pexprRecheckCond = (*pexprScan)[0];
	gpos::Ref<CDXLNode> pdxlnRecheckCond = PdxlnScalar(pexprRecheckCond);
	gpos::Ref<CDXLNode> pdxlnRecheckCondFilter = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarRecheckCondFilter(m_mp));
	pdxlnRecheckCondFilter->AddChild(pdxlnRecheckCond);

	// translate bitmap access path
	gpos::Ref<CDXLNode> pdxlnBitmapAccessPath = PdxlnScalar((*pexprScan)[1]);

	// build projection list
	gpos::Ref<CColRefSet> pcrsOutput = pexprScan->Prpp()->PcrsRequired();
	gpos::Ref<CDXLNode> proj_list_dxlnode =
		PdxlnProjList(pcrsOutput, colref_array);

	pdxlnScan->AddChild(proj_list_dxlnode);
	pdxlnScan->AddChild(filter_dxlnode);
	pdxlnScan->AddChild(pdxlnRecheckCondFilter);
	pdxlnScan->AddChild(pdxlnBitmapAccessPath);

#ifdef GPOS_DEBUG
	pdxlnScan->GetOperator()->AssertValid(pdxlnScan,
										  false /* validate_children */);
#endif

	gpos::Ref<CDistributionSpec> pds = pexprScan->GetDrvdPropPlan()->Pds();

	pdrgpdsBaseTables->Append(pds);

	return pdxlnScan;
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnDynamicIndexScan(
	gpos::Ref<CExpression> pexprDIS, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDXLPhysicalProperties> dxl_properties,
	gpos::Ref<CReqdPropPlan> prpp)
{
	GPOS_ASSERT(nullptr != pexprDIS);
	GPOS_ASSERT(nullptr != dxl_properties);
	GPOS_ASSERT(nullptr != prpp);

	gpos::Ref<CPhysicalDynamicIndexScan> popDIS =
		CPhysicalDynamicIndexScan::PopConvert(pexprDIS->Pop());
	gpos::Ref<CColRefArray> pdrgpcrOutput = popDIS->PdrgpcrOutput();

	// translate table descriptor
	gpos::Ref<CDXLTableDescr> table_descr =
		MakeDXLTableDescr(popDIS->Ptabdesc(), pdrgpcrOutput, pexprDIS->Prpp());

	// create index descriptor
	gpos::Ref<CIndexDescriptor> pindexdesc = popDIS->Pindexdesc();
	CMDName *pmdnameIndex =
		GPOS_NEW(m_mp) CMDName(m_mp, pindexdesc->Name().Pstr());
	gpos::Ref<IMDId> pmdidIndex = pindexdesc->MDId();

	gpos::Ref<CDXLIndexDescr> dxl_index_descr =
		GPOS_NEW(m_mp) CDXLIndexDescr(pmdidIndex, pmdnameIndex);


	// construct dynamic table scan operator
	gpos::Ref<IMdIdArray> part_mdids = popDIS->GetPartitionMdids();


	gpos::Ref<ULongPtrArray> selector_ids = GPOS_NEW(m_mp) ULongPtrArray(m_mp);
	gpos::Ref<CPartitionPropagationSpec> pps_reqd =
		prpp->Pepp()->PppsRequired();
	if (pps_reqd->Contains(popDIS->ScanId()))
	{
		const gpos::Ref<CBitSet> bs = pps_reqd->SelectorIds(popDIS->ScanId());
		CBitSetIter bsi(*bs);
		while (bsi.Advance())
		{
			selector_ids->Append(GPOS_NEW(m_mp) ULONG(bsi.Bit()));
		}
	}

	// TODO: we assume that the index are always forward access.

	gpos::Ref<CDXLNode> pdxlnDIS = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLPhysicalDynamicIndexScan(
						   m_mp, table_descr, dxl_index_descr, EdxlisdForward,
						   part_mdids, selector_ids));

	// set plan costs
	pdxlnDIS->SetProperties(dxl_properties);

	// construct projection list
	gpos::Ref<CColRefSet> pcrsOutput = prpp->PcrsRequired();
	gpos::Ref<CDXLNode> pdxlnPrL = PdxlnProjList(pcrsOutput, colref_array);

	// translate index predicates
	gpos::Ref<CExpression> pexprCond = (*pexprDIS)[0];
	gpos::Ref<CDXLNode> pdxlnIndexCondList = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarIndexCondList(m_mp));

	gpos::Ref<CExpressionArray> pdrgpexprConds =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprCond);
	const ULONG length = pdrgpexprConds->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::Ref<CExpression> pexprIndexCond = (*pdrgpexprConds)[ul];
		gpos::Ref<CDXLNode> pdxlnIndexCond = PdxlnScalar(pexprIndexCond);
		pdxlnIndexCondList->AddChild(pdxlnIndexCond);
	}


	gpos::Ref<CDXLNode> pdxlnResidualCond = nullptr;
	if (2 == pexprDIS->Arity())
	{
		// translate residual predicates into the filter node
		gpos::Ref<CExpression> pexprResidualCond = (*pexprDIS)[1];
		if (COperator::EopScalarConst != pexprResidualCond->Pop()->Eopid())
		{
			pdxlnResidualCond = PdxlnScalar(pexprResidualCond);
		}
	}

	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(pdxlnResidualCond);

	pdxlnDIS->AddChild(pdxlnPrL);
	pdxlnDIS->AddChild(filter_dxlnode);
	pdxlnDIS->AddChild(pdxlnIndexCondList);

#ifdef GPOS_DEBUG
	pdxlnDIS->GetOperator()->AssertValid(pdxlnDIS,
										 false /* validate_children */);
#endif

	return pdxlnDIS;
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnDynamicIndexScan(
	gpos::Ref<CExpression> pexprDIS, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions GPOS_UNUSED, BOOL *pfDML GPOS_UNUSED)
{
	GPOS_ASSERT(nullptr != pexprDIS);

	gpos::Ref<CDXLPhysicalProperties> dxl_properties = GetProperties(pexprDIS);

	gpos::Ref<CDistributionSpec> pds = pexprDIS->GetDrvdPropPlan()->Pds();

	pdrgpdsBaseTables->Append(pds);
	return PdxlnDynamicIndexScan(pexprDIS, colref_array, dxl_properties,
								 pexprDIS->Prpp());
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDynamicForeignScan
//
//	@doc:
//		Create a DXL dynamic foreign table scan node from an optimizer
//		dynamic foreign table scan node.
//
//---------------------------------------------------------------------------
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnDynamicForeignScan(
	gpos::Ref<CExpression> pexprDFS, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *,  // pulNonGatherMotions,
	BOOL *	  // pfDML
)
{
	gpos::Ref<CExpression> pexprScalarCond = nullptr;
	gpos::Ref<CDXLPhysicalProperties> dxl_properties = nullptr;
	return PdxlnDynamicForeignScan(pexprDFS, colref_array, pdrgpdsBaseTables,
								   pexprScalarCond, dxl_properties);
}

// Translate CPhysicalDynamicForeignScan node.
// This creates a dynamic foreign scan node similar to creating a dynamic seq scan.
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnDynamicForeignScan(
	gpos::Ref<CExpression> pexprDFS, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	gpos::Ref<CExpression> pexprScalarCond,
	gpos::Ref<CDXLPhysicalProperties> dxl_properties)
{
	GPOS_ASSERT(nullptr != pexprDFS);
	GPOS_ASSERT_IFF(nullptr != pexprScalarCond, nullptr != dxl_properties);

	gpos::Ref<CPhysicalDynamicForeignScan> popDFS =
		CPhysicalDynamicForeignScan::PopConvert(pexprDFS->Pop());
	gpos::Ref<CColRefArray> pdrgpcrOutput = popDFS->PdrgpcrOutput();

	// translate table descriptor
	gpos::Ref<CDXLTableDescr> table_descr =
		MakeDXLTableDescr(popDFS->Ptabdesc(), pdrgpcrOutput, pexprDFS->Prpp());

	// construct plan costs
	gpos::Ref<CDXLPhysicalProperties> pdxlpropDFS = GetProperties(pexprDFS);

	if (nullptr != dxl_properties)
	{
		CWStringDynamic *rows_out_str = GPOS_NEW(m_mp) CWStringDynamic(
			m_mp,
			dxl_properties->GetDXLOperatorCost()->GetRowsOutStr()->GetBuffer());
		CWStringDynamic *pstrCost = GPOS_NEW(m_mp)
			CWStringDynamic(m_mp, dxl_properties->GetDXLOperatorCost()
									  ->GetTotalCostStr()
									  ->GetBuffer());

		pdxlpropDFS->GetDXLOperatorCost()->SetRows(rows_out_str);
		pdxlpropDFS->GetDXLOperatorCost()->SetCost(pstrCost);
	}

	gpos::Ref<IMdIdArray> part_mdids = popDFS->GetPartitionMdids();


	// populate selector ids for dynamic partition elimination
	gpos::Ref<ULongPtrArray> selector_ids = GPOS_NEW(m_mp) ULongPtrArray(m_mp);
	gpos::Ref<CPartitionPropagationSpec> pps_reqd =
		pexprDFS->Prpp()->Pepp()->PppsRequired();
	if (pps_reqd->Contains(popDFS->ScanId()))
	{
		const gpos::Ref<CBitSet> bs = pps_reqd->SelectorIds(popDFS->ScanId());
		CBitSetIter bsi(*bs);
		for (ULONG ul = 0; bsi.Advance(); ul++)
		{
			selector_ids->Append(GPOS_NEW(m_mp) ULONG(bsi.Bit()));
		}
	}


	gpos::Ref<CDXLPhysicalDynamicForeignScan> pdxlopDFS = GPOS_NEW(m_mp)
		CDXLPhysicalDynamicForeignScan(m_mp, table_descr, part_mdids,
									   selector_ids,
									   popDFS->GetForeignServerOid());

	gpos::Ref<CDXLNode> pdxlnDFS = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopDFS);
	pdxlnDFS->SetProperties(pdxlpropDFS);

	gpos::Ref<CDXLNode> pdxlnCond = nullptr;

	if (nullptr != pexprScalarCond)
	{
		pdxlnCond = PdxlnScalar(pexprScalarCond);
	}

	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(pdxlnCond);

	// construct projection list
	GPOS_ASSERT(nullptr != pexprDFS->Prpp());

	gpos::Ref<CColRefSet> pcrsOutput = pexprDFS->Prpp()->PcrsRequired();
	pdxlnDFS->AddChild(PdxlnProjList(pcrsOutput, colref_array));
	pdxlnDFS->AddChild(filter_dxlnode);

#ifdef GPOS_DEBUG
	pdxlnDFS->GetOperator()->AssertValid(pdxlnDFS, false);
#endif

	gpos::Ref<CDistributionSpec> pds = pexprDFS->GetDrvdPropPlan()->Pds();

	pdrgpdsBaseTables->Append(pds);

	return pdxlnDFS;
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnResult(
	gpos::Ref<CExpression> pexprRelational,
	gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML, gpos::Ref<CDXLNode> pdxlnScalar)
{
	// extract physical properties from filter
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprRelational);

	return PdxlnResult(pexprRelational, colref_array, pdrgpdsBaseTables,
					   pulNonGatherMotions, pfDML, pdxlnScalar, dxl_properties);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnResult(
	gpos::Ref<CExpression> pexprRelational,
	gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML, gpos::Ref<CDXLNode> pdxlnScalar,
	gpos::Ref<CDXLPhysicalProperties> dxl_properties)
{
	GPOS_ASSERT(nullptr != pexprRelational);

	// translate relational child expression
	gpos::Ref<CDXLNode> pdxlnRelationalChild = CreateDXLNode(
		pexprRelational, colref_array, pdrgpdsBaseTables, pulNonGatherMotions,
		pfDML, false /*fRemap*/, false /*fRoot */);
	GPOS_ASSERT(nullptr != pexprRelational->Prpp());
	gpos::Ref<CColRefSet> pcrsOutput = pexprRelational->Prpp()->PcrsRequired();

	return PdxlnAddScalarFilterOnRelationalChild(pdxlnRelationalChild,
												 pdxlnScalar, dxl_properties,
												 pcrsOutput, colref_array);
}

gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnAddScalarFilterOnRelationalChild(
	gpos::Ref<CDXLNode> pdxlnRelationalChild,
	gpos::Ref<CDXLNode> pdxlnScalarChild,
	gpos::Ref<CDXLPhysicalProperties> dxl_properties,
	gpos::Ref<CColRefSet> pcrsOutput, gpos::Ref<CColRefArray> pdrgpcrOrder)
{
	GPOS_ASSERT(nullptr != dxl_properties);
	// for a true condition, just translate the child
	if (CTranslatorExprToDXLUtils::FScalarConstTrue(m_pmda, pdxlnScalarChild))
	{
		return pdxlnRelationalChild;
	}
	// create a result node over outer child
	else
	{
		// wrap condition in a DXL filter node
		gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(pdxlnScalarChild);

		gpos::Ref<CDXLNode> pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrOrder);

		// create an empty one-time filter
		gpos::Ref<CDXLNode> one_time_filter = GPOS_NEW(m_mp)
			CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarOneTimeFilter(m_mp));

		return CTranslatorExprToDXLUtils::PdxlnResult(
			m_mp, dxl_properties, pdxlnPrL, filter_dxlnode, one_time_filter,
			pdxlnRelationalChild);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnResult(
	gpos::Ref<CExpression> pexprFilter, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprFilter);

	gpos::Ref<CDXLNode> pdxlnode =
		PdxlnFromFilter(pexprFilter, colref_array, pdrgpdsBaseTables,
						pulNonGatherMotions, pfDML, dxl_properties);


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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnIndexScanWithInlinedCondition(
	gpos::Ref<CExpression> pexprIndexScan,
	gpos::Ref<CExpression> pexprScalarCond,
	gpos::Ref<CDXLPhysicalProperties> dxl_properties,
	gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables)
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
		gpos::Ref<CPhysicalIndexScan> indexScan =
			CPhysicalIndexScan::PopConvert(pexprIndexScan->Pop());
		isGist = (indexScan->Pindexdesc()->IndexType() == IMDIndex::EmdindGist);
	}
	else if (COperator::EopPhysicalIndexOnlyScan != op_id)
	{
		gpos::Ref<CPhysicalDynamicIndexScan> indexScan =
			CPhysicalDynamicIndexScan::PopConvert(pexprIndexScan->Pop());
		isGist = (indexScan->Pindexdesc()->IndexType() == IMDIndex::EmdindGist);
	}

	// inline scalar condition in index scan, if it is not the same as index lookup condition
	// Exception: most GiST indexes require a recheck condition since they are lossy: re-add the lookup
	// condition as a scalar condition. For now, all GiST indexes are treated as lossy
	gpos::Ref<CExpression> pexprIndexLookupCond = (*pexprIndexScan)[0];
	gpos::Ref<CDXLNode> pdxlnIndexScan = nullptr;
	if ((!CUtils::FScalarConstTrue(pexprScalarCond) &&
		 !pexprScalarCond->Matches(pexprIndexLookupCond)) ||
		isGist)
	{
		// combine scalar condition with existing index conditions, if any

		gpos::Ref<CExpression> pexprNewScalarCond = pexprScalarCond;
		if (2 == pexprIndexScan->Arity())
		{
			pexprNewScalarCond = CPredicateUtils::PexprConjunction(
				m_mp, (*pexprIndexScan)[1], pexprScalarCond);
		}


		gpos::Ref<CExpression> pexprNewIndexScan = GPOS_NEW(m_mp)
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


		gpos::Ref<CDistributionSpec> pds =
			pexprIndexScan->GetDrvdPropPlan()->Pds();

		pdrgpdsBaseTables->Append(pds);

		return pdxlnIndexScan;
	}

	// index scan does not need the properties of the filter, as it does not
	// need to further inline the scalar condition

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnFromFilter(
	gpos::Ref<CExpression> pexprFilter, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML,
	gpos::Ref<CDXLPhysicalProperties> dxl_properties)
{
	GPOS_ASSERT(nullptr != pexprFilter);
	GPOS_ASSERT(nullptr != dxl_properties);

	// extract components
	gpos::Ref<CExpression> pexprRelational = (*pexprFilter)[0];
	gpos::Ref<CExpression> pexprScalar = (*pexprFilter)[1];

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
	gpos::Ref<CColRefSet> pcrsOutput = pexprFilter->Prpp()->PcrsRequired();

	switch (eopidRelational)
	{
		case COperator::EopPhysicalTableScan:
		case COperator::EopPhysicalForeignScan:
		{
			// if there is a structure of the form
			// 		filter->tablescan, or filter->CTG then
			// push the scalar filter expression to the tablescan/CTG respectively


			// translate the table scan with the filter condition
			return PdxlnTblScan(pexprRelational, pcrsOutput,
								nullptr /* colref_array */, pdrgpdsBaseTables,
								pexprScalar, dxl_properties /* cost info */
			);
		}
		case COperator::EopPhysicalBitmapTableScan:
		{
			return PdxlnBitmapTableScan(
				pexprRelational, pcrsOutput, nullptr /*colref_array*/,
				pdrgpdsBaseTables, pexprScalar, dxl_properties);
		}
		case COperator::EopPhysicalDynamicTableScan:
		{
			// inline condition in the Dynamic Table Scan
			return PdxlnDynamicTableScan(pexprRelational, colref_array,
										 pdrgpdsBaseTables, pexprScalar,
										 dxl_properties);
		}
		case COperator::EopPhysicalDynamicForeignScan:
		{
			// inline condition in the Dynamic Foreign Scan Scan
			return PdxlnDynamicForeignScan(pexprRelational, colref_array,
										   pdrgpdsBaseTables, pexprScalar,
										   dxl_properties);
		}
		case COperator::EopPhysicalIndexOnlyScan:
		case COperator::EopPhysicalIndexScan:
		case COperator::EopPhysicalDynamicIndexScan:
		{
			return PdxlnIndexScanWithInlinedCondition(
				pexprRelational, pexprScalar, dxl_properties, colref_array,
				pdrgpdsBaseTables);
		}
		case COperator::EopPhysicalDynamicBitmapTableScan:
		{
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnResultFromFilter(
	gpos::Ref<CExpression> pexprFilter, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprFilter);

	// extract components
	gpos::Ref<CExpression> pexprRelational = (*pexprFilter)[0];
	gpos::Ref<CExpression> pexprScalar = (*pexprFilter)[1];
	gpos::Ref<CColRefSet> pcrsOutput = pexprFilter->Prpp()->PcrsRequired();

	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprFilter);

	// translate relational child expression
	gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
		pexprRelational, nullptr /* colref_array */, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// translate scalar expression in to filter and one time filter dxl nodes
	gpos::Ref<CColRefSet> relational_child_colrefset =
		pexprRelational->DeriveOutputColumns();
	// get all the scalar conditions in an array
	gpos::Ref<CExpressionArray> scalar_exprs =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprScalar);
	// array to hold scalar conditions which will qualify for filter condition
	gpos::Ref<CExpressionArray> filter_quals_exprs =
		GPOS_NEW(m_mp) CExpressionArray(m_mp);
	// array to hold scalar conditions which qualify for one time filter condition
	gpos::Ref<CExpressionArray> one_time_filter_quals_exprs =
		GPOS_NEW(m_mp) CExpressionArray(m_mp);
	for (ULONG ul = 0; ul < scalar_exprs->Size(); ul++)
	{
		gpos::Ref<CExpression> scalar_child_expr = (*scalar_exprs)[ul];
		gpos::Ref<CColRefSet> scalar_child_colrefset =
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
			filter_quals_exprs->Append(scalar_child_expr);
		}
		else
		{
			one_time_filter_quals_exprs->Append(scalar_child_expr);
		}
	}


	// create an emtpy filter
	gpos::Ref<CDXLNode> filter_dxlnode =
		GPOS_NEW(m_mp) CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarFilter(m_mp));
	// create an empty one-time filter
	gpos::Ref<CDXLNode> one_time_filter_dxlnode = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarOneTimeFilter(m_mp));

	if (filter_quals_exprs->Size() > 0)
	{
		// create scalar cmp expression for filter expression
		gpos::Ref<CExpression> scalar_cmp_expr =
			CPredicateUtils::PexprConjunction(m_mp, filter_quals_exprs);
		// create dxl node for the filter
		gpos::Ref<CDXLNode> scalar_cmp_dxlnode = PdxlnScalar(scalar_cmp_expr);
		filter_dxlnode->AddChild(scalar_cmp_dxlnode);
	}
	else
	{
	}

	if (one_time_filter_quals_exprs->Size() > 0)
	{
		// create scalar cmp expression for one time filter expression
		gpos::Ref<CExpression> scalar_cmp_expr =
			CPredicateUtils::PexprConjunction(m_mp,
											  one_time_filter_quals_exprs);
		// create dxl node for one time filter
		gpos::Ref<CDXLNode> scalar_cmp_dxlnode = PdxlnScalar(scalar_cmp_expr);
		one_time_filter_dxlnode->AddChild(scalar_cmp_dxlnode);
	}
	else
	{
	}

	GPOS_ASSERT(nullptr != pexprFilter->Prpp());

	gpos::Ref<CDXLNode> pdxlnPrL = PdxlnProjList(pcrsOutput, colref_array);

	return CTranslatorExprToDXLUtils::PdxlnResult(
		m_mp, dxl_properties, pdxlnPrL, filter_dxlnode, one_time_filter_dxlnode,
		child_dxlnode);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAssert
//
//	@doc:
//		Translate a physical assert expression
//
//---------------------------------------------------------------------------
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnAssert(
	gpos::Ref<CExpression> pexprAssert, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprAssert);

	// extract components
	gpos::Ref<CExpression> pexprRelational = (*pexprAssert)[0];
	gpos::Ref<CExpression> pexprScalar = (*pexprAssert)[1];
	gpos::Ref<CPhysicalAssert> popAssert =
		CPhysicalAssert::PopConvert(pexprAssert->Pop());

	// extract physical properties from assert
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprAssert);

	gpos::Ref<CColRefSet> pcrsOutput = pexprAssert->Prpp()->PcrsRequired();

	// translate relational child expression
	gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
		pexprRelational, nullptr /* colref_array */, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// translate scalar expression
	gpos::Ref<CDXLNode> pdxlnAssertPredicate = PdxlnScalar(pexprScalar);

	GPOS_ASSERT(nullptr != pexprAssert->Prpp());

	gpos::Ref<CDXLNode> pdxlnPrL = PdxlnProjList(pcrsOutput, colref_array);

	const CHAR *sql_state = popAssert->Pexc()->GetSQLState();
	gpos::Ref<CDXLPhysicalAssert> pdxlopAssert =
		GPOS_NEW(m_mp) CDXLPhysicalAssert(m_mp, sql_state);
	gpos::Ref<CDXLNode> pdxlnAssert = GPOS_NEW(m_mp) CDXLNode(
		m_mp, pdxlopAssert, pdxlnPrL, pdxlnAssertPredicate, child_dxlnode);

	pdxlnAssert->SetProperties(dxl_properties);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnCTEProducer(
	gpos::Ref<CExpression> pexprCTEProducer,
	gpos::Ref<CColRefArray>,  //colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprCTEProducer);

	// extract components
	gpos::Ref<CExpression> pexprRelational = (*pexprCTEProducer)[0];
	gpos::Ref<CPhysicalCTEProducer> popCTEProducer =
		CPhysicalCTEProducer::PopConvert(pexprCTEProducer->Pop());

	// extract physical properties from cte producer
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprCTEProducer);

	// extract the CTE id and the array of colids
	const ULONG ulCTEId = popCTEProducer->UlCTEId();
	gpos::Ref<ULongPtrArray> colids =
		CUtils::Pdrgpul(m_mp, popCTEProducer->Pdrgpcr());

	GPOS_ASSERT(nullptr != pexprCTEProducer->Prpp());
	gpos::Ref<CColRefArray> pdrgpcrRequired = popCTEProducer->Pdrgpcr();
	gpos::Ref<CColRefSet> pcrsOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsOutput->Include(pdrgpcrRequired);

	// translate relational child expression
	gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
		pexprRelational, pdrgpcrRequired, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, true /*fRemap*/, false /*fRoot */);

	gpos::Ref<CDXLNode> pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrRequired);


	gpos::Ref<CDXLNode> pdxlnCTEProducer = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLPhysicalCTEProducer(m_mp, ulCTEId, colids),
		pdxlnPrL, child_dxlnode);

	pdxlnCTEProducer->SetProperties(dxl_properties);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnCTEConsumer(
	gpos::Ref<CExpression> pexprCTEConsumer,
	gpos::Ref<CColRefArray>,			//colref_array,
	gpos::Ref<CDistributionSpecArray>,	// pdrgpdsBaseTables,
	ULONG *,							// pulNonGatherMotions,
	BOOL *								// pfDML
)
{
	GPOS_ASSERT(nullptr != pexprCTEConsumer);

	// extract components
	gpos::Ref<CPhysicalCTEConsumer> popCTEConsumer =
		CPhysicalCTEConsumer::PopConvert(pexprCTEConsumer->Pop());

	// extract physical properties from cte consumer
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprCTEConsumer);

	// extract the CTE id and the array of colids
	const ULONG ulCTEId = popCTEConsumer->UlCTEId();
	gpos::Ref<CColRefArray> colref_array = popCTEConsumer->Pdrgpcr();
	gpos::Ref<ULongPtrArray> colids = CUtils::Pdrgpul(m_mp, colref_array);

	gpos::Ref<CColRefSet> pcrsOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsOutput->Include(colref_array);

	// translate relational child expression
	gpos::Ref<CDXLNode> pdxlnPrL = PdxlnProjList(pcrsOutput, colref_array);

	gpos::Ref<CDXLNode> pdxlnCTEConsumer = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLPhysicalCTEConsumer(m_mp, ulCTEId, colids),
		pdxlnPrL);



	pdxlnCTEConsumer->SetProperties(dxl_properties);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnAppend(
	gpos::Ref<CExpression> pexprUnionAll,
	gpos::Ref<CColRefArray>,  //colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprUnionAll);

	gpos::Ref<CPhysicalUnionAll> popUnionAll =
		CPhysicalUnionAll::PopConvert(pexprUnionAll->Pop());
	gpos::Ref<CColRefArray> pdrgpcrOutputAll = popUnionAll->PdrgpcrOutput();
	gpos::Ref<CColRefSet> reqdCols = pexprUnionAll->Prpp()->PcrsRequired();

	gpos::Ref<CDXLPhysicalAppend> dxl_op =
		GPOS_NEW(m_mp) CDXLPhysicalAppend(m_mp, false, false);
	gpos::Ref<CDXLNode> pdxlnAppend = GPOS_NEW(m_mp) CDXLNode(m_mp, dxl_op);

	// compute a list of indexes of output columns that are actually required
	gpos::Ref<CColRefArray> reqd_col_array = GPOS_NEW(m_mp) CColRefArray(m_mp);
	ULONG num_total_cols = pdrgpcrOutputAll->Size();
	for (ULONG c = 0; c < num_total_cols; c++)
	{
		if (reqdCols->FMember((*pdrgpcrOutputAll)[c]))
		{
			reqd_col_array->Append((*pdrgpcrOutputAll)[c]);
		}
	}
	gpos::Ref<ULongVector> reqd_col_positions =
		pdrgpcrOutputAll->IndexesOfSubsequence(reqd_col_array);
	gpos::Ref<CColRefArray> requiredOutput =
		pdrgpcrOutputAll->CreateReducedArray(reqd_col_positions);


	GPOS_ASSERT(nullptr != reqd_col_positions);

	// set properties
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprUnionAll);
	pdxlnAppend->SetProperties(dxl_properties);

	// translate project list
	gpos::Ref<CColRefSet> pcrsOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsOutput->Include(reqdCols);

	// the append node does not re-order its input or output columns. The
	// re-ordering of its output columns has to be done above it (if needed)
	// via a separate result node
	gpos::Ref<CDXLNode> pdxlnPrL = PdxlnProjList(pcrsOutput, requiredOutput);


	pcrsOutput = nullptr;
	requiredOutput = nullptr;

	pdxlnAppend->AddChild(pdxlnPrL);

	// scalar condition
	gpos::Ref<CDXLNode> pdxlnCond = nullptr;
	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(pdxlnCond);
	pdxlnAppend->AddChild(filter_dxlnode);

	// translate children
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput =
		popUnionAll->PdrgpdrgpcrInput();
	GPOS_ASSERT(nullptr != pdrgpdrgpcrInput);
	const ULONG length = pexprUnionAll->Arity();
	GPOS_ASSERT(length == pdrgpdrgpcrInput->Size());
	for (ULONG ul = 0; ul < length; ul++)
	{
		// translate child
		gpos::Ref<CColRefArray> pdrgpcrInput = (*pdrgpdrgpcrInput)[ul];
		gpos::Ref<CColRefArray> requiredInput =
			pdrgpcrInput->CreateReducedArray(reqd_col_positions);

		gpos::Ref<CExpression> pexprChild = (*pexprUnionAll)[ul];
		gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
			pexprChild, requiredInput, pdrgpdsBaseTables, pulNonGatherMotions,
			pfDML, false /*fRemap*/, false /*fRoot*/);

		// add a result node on top if necessary so the order of the input project list
		// matches the order in which the append node requires it
		gpos::Ref<CDXLNode> pdxlnChildProjected = PdxlnRemapOutputColumns(
			pexprChild, child_dxlnode,
			requiredInput /* required input columns */,
			requiredInput /* order of the input columns */
		);

		pdxlnAppend->AddChild(pdxlnChildProjected);
	}


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
gpos::Ref<CColRefArray>
CTranslatorExprToDXL::PdrgpcrMerge(CMemoryPool *mp,
								   gpos::Ref<CColRefArray> pdrgpcrOrder,
								   gpos::Ref<CColRefArray> pdrgpcrRequired)
{
	gpos::Ref<CColRefSet> pcrsOutput = GPOS_NEW(mp) CColRefSet(mp);

	gpos::Ref<CColRefArray> pdrgpcrMerge = GPOS_NEW(mp) CColRefArray(mp);

	if (nullptr != pdrgpcrOrder)
	{
		const ULONG ulLenOrder = pdrgpcrOrder->Size();
		for (ULONG ul = 0; ul < ulLenOrder; ul++)
		{
			gpos::Ref<CColRef> colref = (*pdrgpcrOrder)[ul];
			pdrgpcrMerge->Append(colref);
		}
		pcrsOutput->Include(pdrgpcrMerge);
	}

	const ULONG ulLenReqd = pdrgpcrRequired->Size();
	for (ULONG ul = 0; ul < ulLenReqd; ul++)
	{
		gpos::Ref<CColRef> colref = (*pdrgpcrRequired)[ul];
		if (!pcrsOutput->FMember(colref))
		{
			pcrsOutput->Include(colref);
			pdrgpcrMerge->Append(colref);
		}
	}



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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnRemapOutputColumns(
	gpos::Ref<CExpression> pexpr, gpos::Ref<CDXLNode> dxlnode,
	gpos::Ref<CColRefArray> pdrgpcrRequired,
	gpos::Ref<CColRefArray> pdrgpcrOrder)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != dxlnode);
	GPOS_ASSERT(nullptr != pdrgpcrRequired);

	// get project list
	gpos::Ref<CDXLNode> pdxlnPrL = (*dxlnode)[0];

	gpos::Ref<CColRefArray> pdrgpcrOrderedReqdCols =
		PdrgpcrMerge(m_mp, pdrgpcrOrder, pdrgpcrRequired);

	// if the combined list is the same as proj list then no
	// further action needed. Otherwise we need result node on top
	if (CTranslatorExprToDXLUtils::FProjectListMatch(pdxlnPrL,
													 pdrgpcrOrderedReqdCols))
	{
		return dxlnode;
	}



	// output columns of new result node
	gpos::Ref<CColRefSet> pcrsOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsOutput->Include(pdrgpcrRequired);

	gpos::Ref<CDXLNode> pdxlnPrLNew = PdxlnProjList(pcrsOutput, pdrgpcrOrder);


	// create a result node on top of the current dxl node with a new project list
	return PdxlnResult(GetProperties(pexpr), pdxlnPrLNew, dxlnode);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnTVF
//
//	@doc:
//		Create a DXL TVF node from an optimizer TVF node
//---------------------------------------------------------------------------
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnTVF(
	gpos::Ref<CExpression> pexprTVF,
	gpos::Ref<CColRefArray>,			//colref_array,
	gpos::Ref<CDistributionSpecArray>,	// pdrgpdsBaseTables,
	ULONG *,							// pulNonGatherMotions,
	BOOL *								// pfDML
)
{
	GPOS_ASSERT(nullptr != pexprTVF);

	gpos::Ref<CPhysicalTVF> popTVF = CPhysicalTVF::PopConvert(pexprTVF->Pop());

	gpos::Ref<CColRefSet> pcrsOutput = popTVF->DeriveOutputColumns();

	gpos::Ref<IMDId> mdid_func = popTVF->FuncMdId();


	gpos::Ref<IMDId> mdid_return_type = popTVF->ReturnTypeMdId();


	CWStringConst *pstrFunc =
		GPOS_NEW(m_mp) CWStringConst(m_mp, popTVF->Pstr()->GetBuffer());

	gpos::Ref<CDXLPhysicalTVF> dxl_op = GPOS_NEW(m_mp)
		CDXLPhysicalTVF(m_mp, mdid_func, mdid_return_type, pstrFunc);

	gpos::Ref<CDXLPhysicalProperties> dxl_properties = GetProperties(pexprTVF);
	gpos::Ref<CDXLNode> pdxlnTVF = GPOS_NEW(m_mp) CDXLNode(m_mp, dxl_op);
	pdxlnTVF->SetProperties(dxl_properties);

	gpos::Ref<CDXLNode> pdxlnPrL =
		PdxlnProjList(pcrsOutput, nullptr /*colref_array*/);
	pdxlnTVF->AddChild(pdxlnPrL);  // project list

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnResultFromConstTableGet(
	gpos::Ref<CExpression> pexprCTG, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CExpression> pexprScalar)
{
	GPOS_ASSERT(nullptr != pexprCTG);

	gpos::Ref<CPhysicalConstTableGet> popCTG =
		CPhysicalConstTableGet::PopConvert(pexprCTG->Pop());

	// construct project list from the const table get values
	gpos::Ref<CColRefArray> pdrgpcrCTGOutput = popCTG->PdrgpcrOutput();
	gpos::Ref<IDatum2dArray> pdrgpdrgdatum = popCTG->Pdrgpdrgpdatum();

	const ULONG ulRows = pdrgpdrgdatum->Size();
	gpos::Ref<CDXLNode> pdxlnPrL = nullptr;
	gpos::Ref<CDXLNode> one_time_filter = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarOneTimeFilter(m_mp));

	gpos::Ref<IDatumArray> pdrgpdatum = nullptr;
	if (0 == ulRows)
	{
		// no-tuples... only generate one row of NULLS and one-time "false" filter
		pdrgpdatum =
			CTranslatorExprToDXLUtils::PdrgpdatumNulls(m_mp, pdrgpcrCTGOutput);

		gpos::Ref<CExpression> pexprFalse = CUtils::PexprScalarConstBool(
			m_mp, false /*value*/, false /*is_null*/);
		gpos::Ref<CDXLNode> pdxlnFalse = PdxlnScConst(pexprFalse);


		one_time_filter->AddChild(pdxlnFalse);
	}
	else
	{
		GPOS_ASSERT(1 <= ulRows);
		pdrgpdatum = (*pdrgpdrgdatum)[0];

		gpos::Ref<CDXLNode> pdxlnCond = nullptr;
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

		gpos::Ref<CColRefSet> pcrsOutput = pexprCTG->DeriveOutputColumns();
		pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrCTGOutput);

		gpos::Ref<CDXLNode> pdxlnValuesScan =
			CTranslatorExprToDXLUtils::PdxlnValuesScan(
				m_mp, GetProperties(pexprCTG), pdxlnPrL, pdrgpdrgdatum);



		return pdxlnValuesScan;
	}
	else
	{
		pdxlnPrL = PdxlnProjListFromConstTableGet(colref_array,
												  pdrgpcrCTGOutput, pdrgpdatum);

		return CTranslatorExprToDXLUtils::PdxlnResult(
			m_mp, GetProperties(pexprCTG), pdxlnPrL, PdxlnFilter(nullptr),
			one_time_filter,
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnResultFromConstTableGet(
	gpos::Ref<CExpression> pexprCTG, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray>,	// pdrgpdsBaseTables,
	ULONG *,							// pulNonGatherMotions,
	BOOL *								// pfDML
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnComputeScalar(
	gpos::Ref<CExpression> pexprComputeScalar,
	gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprComputeScalar);

	// extract components
	gpos::Ref<CExpression> pexprRelational = (*pexprComputeScalar)[0];
	gpos::Ref<CExpression> pexprProjList = (*pexprComputeScalar)[1];

	// translate relational child expression
	gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
		pexprRelational, nullptr /* colref_array */, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// compute required columns
	GPOS_ASSERT(nullptr != pexprComputeScalar->Prpp());
	gpos::Ref<CColRefSet> pcrsOutput =
		pexprComputeScalar->Prpp()->PcrsRequired();

	// iterate the columns in the projection list, add the columns containing
	// set-returning functions to the output columns
	const ULONG ulPrLs = pexprProjList->Arity();
	for (ULONG ul = 0; ul < ulPrLs; ul++)
	{
		gpos::Ref<CExpression> pexprPrE = (*pexprProjList)[ul];

		// for column that doesn't contain set-returning function, if it is not the
		// required column in the relational plan properties, then no need to add them
		// to the output columns
		if (pexprPrE->DeriveHasNonScalarFunction())
		{
			gpos::Ref<CScalarProjectElement> popScPrE =
				CScalarProjectElement::PopConvert(pexprPrE->Pop());
			pcrsOutput->Include(popScPrE->Pcr());
		}
	}

	// translate project list expression
	gpos::Ref<CDXLNode> pdxlnPrL = nullptr;
	if (nullptr == colref_array || CUtils::FHasDuplicates(colref_array))
	{
		pdxlnPrL = PdxlnProjList(pexprProjList, pcrsOutput);
	}
	else
	{
		pdxlnPrL = PdxlnProjList(pexprProjList, pcrsOutput, colref_array);
	}

	// construct a result node
	gpos::Ref<CDXLNode> pdxlnResult =
		PdxlnResult(GetProperties(pexprComputeScalar), pdxlnPrL, child_dxlnode);

#ifdef GPOS_DEBUG
	(void) CDXLPhysicalResult::Cast(pdxlnResult->GetOperator())
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnAggregate(
	gpos::Ref<CExpression> pexprAgg, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprAgg);
	COperator::EOperatorId op_id = pexprAgg->Pop()->Eopid();

	// extract components and construct an aggregate node
	gpos::Ref<CPhysicalAgg> popAgg = nullptr;

	GPOS_ASSERT(COperator::EopPhysicalStreamAgg == op_id ||
				COperator::EopPhysicalHashAgg == op_id ||
				COperator::EopPhysicalScalarAgg == op_id);

	EdxlAggStrategy dxl_agg_strategy = EdxlaggstrategySentinel;

	switch (op_id)
	{
		case COperator::EopPhysicalStreamAgg:
		{
			popAgg = CPhysicalStreamAgg::PopConvert(pexprAgg->Pop());
			dxl_agg_strategy = EdxlaggstrategySorted;
			break;
		}
		case COperator::EopPhysicalHashAgg:
		{
			popAgg = CPhysicalHashAgg::PopConvert(pexprAgg->Pop());
			dxl_agg_strategy = EdxlaggstrategyHashed;
			break;
		}
		case COperator::EopPhysicalScalarAgg:
		{
			popAgg = CPhysicalScalarAgg::PopConvert(pexprAgg->Pop());
			dxl_agg_strategy = EdxlaggstrategyPlain;
			break;
		}
		default:
		{
			return nullptr;	 // to silence the compiler
		}
	}

	const gpos::Ref<CColRefArray> pdrgpcrGroupingCols =
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnAggregateDedup(
	gpos::Ref<CExpression> pexprAgg, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprAgg);
	COperator::EOperatorId op_id = pexprAgg->Pop()->Eopid();

	GPOS_ASSERT(COperator::EopPhysicalStreamAggDeduplicate == op_id ||
				COperator::EopPhysicalHashAggDeduplicate == op_id);

	EdxlAggStrategy dxl_agg_strategy = EdxlaggstrategySentinel;
	gpos::Ref<CColRefArray> pdrgpcrGroupingCols = nullptr;
	gpos::Ref<CColRefSet> pcrsKeys = GPOS_NEW(m_mp) CColRefSet(m_mp);

	if (COperator::EopPhysicalStreamAggDeduplicate == op_id)
	{
		gpos::Ref<CPhysicalStreamAggDeduplicate> popAggDedup =
			CPhysicalStreamAggDeduplicate::PopConvert(pexprAgg->Pop());
		pcrsKeys->Include(popAggDedup->PdrgpcrKeys());
		pdrgpcrGroupingCols = popAggDedup->PdrgpcrGroupingCols();
		dxl_agg_strategy = EdxlaggstrategySorted;
	}
	else
	{
		gpos::Ref<CPhysicalHashAggDeduplicate> popAggDedup =
			CPhysicalHashAggDeduplicate::PopConvert(pexprAgg->Pop());
		pcrsKeys->Include(popAggDedup->PdrgpcrKeys());
		pdrgpcrGroupingCols = popAggDedup->PdrgpcrGroupingCols();
		dxl_agg_strategy = EdxlaggstrategyHashed;
	}

	gpos::Ref<CDXLNode> pdxlnAgg = PdxlnAggregate(
		pexprAgg, colref_array, pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
		dxl_agg_strategy, pdrgpcrGroupingCols, pcrsKeys);


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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnAggregate(
	gpos::Ref<CExpression> pexprAgg, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML, EdxlAggStrategy dxl_agg_strategy,
	const gpos::Ref<CColRefArray> pdrgpcrGroupingCols,
	gpos::Ref<CColRefSet> pcrsKeys)
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

	gpos::Ref<CExpression> pexprChild = (*pexprAgg)[0];
	gpos::Ref<CExpression> pexprProjList = (*pexprAgg)[1];

	// translate relational child expression
	gpos::Ref<CDXLNode> child_dxlnode =
		CreateDXLNode(pexprChild,
					  nullptr,	// colref_array,
					  pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
					  false,  // fRemap,
					  false	  // fRoot
		);

	// compute required columns
	GPOS_ASSERT(nullptr != pexprAgg->Prpp());
	gpos::Ref<CColRefSet> pcrsRequired = pexprAgg->Prpp()->PcrsRequired();

	// translate project list expression
	gpos::Ref<CDXLNode> proj_list_dxlnode =
		PdxlnProjList(pexprProjList, pcrsRequired, colref_array);

	// create an empty filter
	gpos::Ref<CDXLNode> filter_dxlnode =
		GPOS_NEW(m_mp) CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarFilter(m_mp));

	// construct grouping columns list and check if all the grouping column are
	// already in the project list of the aggregate operator

	const ULONG num_cols = proj_list_dxlnode->Arity();
	gpos::Ref<UlongToUlongMap> phmululPL = GPOS_NEW(m_mp) UlongToUlongMap(m_mp);
	for (ULONG ul = 0; ul < num_cols; ul++)
	{
		gpos::Ref<CDXLNode> pdxlnProjElem = (*proj_list_dxlnode)[ul];
		ULONG colid =
			CDXLScalarProjElem::Cast(pdxlnProjElem->GetOperator())->Id();

		if (nullptr == phmululPL->Find(&colid))
		{
			BOOL fRes GPOS_ASSERTS_ONLY = phmululPL->Insert(
				GPOS_NEW(m_mp) ULONG(colid), GPOS_NEW(m_mp) ULONG(colid));
			GPOS_ASSERT(fRes);
		}
	}

	gpos::Ref<ULongPtrArray> pdrgpulGroupingCols =
		GPOS_NEW(m_mp) ULongPtrArray(m_mp);

	const ULONG length = pdrgpcrGroupingCols->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::Ref<CColRef> pcrGroupingCol = (*pdrgpcrGroupingCols)[ul];

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
			gpos::Ref<CDXLNode> pdxlnProjElem =
				CTranslatorExprToDXLUtils::PdxlnProjElem(m_mp, m_phmcrdxln,
														 pcrGroupingCol);
			proj_list_dxlnode->AddChild(pdxlnProjElem);
			BOOL fRes GPOS_ASSERTS_ONLY = phmululPL->Insert(
				GPOS_NEW(m_mp) ULONG(colid), GPOS_NEW(m_mp) ULONG(colid));
			GPOS_ASSERT(fRes);
		}
	}



	gpos::Ref<CDXLPhysicalAgg> pdxlopAgg =
		GPOS_NEW(m_mp) CDXLPhysicalAgg(m_mp, dxl_agg_strategy, stream_safe);
	pdxlopAgg->SetGroupingCols(pdrgpulGroupingCols);

	gpos::Ref<CDXLNode> pdxlnAgg = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopAgg);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties = GetProperties(pexprAgg);
	pdxlnAgg->SetProperties(dxl_properties);

	// add children
	pdxlnAgg->AddChild(proj_list_dxlnode);
	pdxlnAgg->AddChild(filter_dxlnode);
	pdxlnAgg->AddChild(child_dxlnode);

#ifdef GPOS_DEBUG
	pdxlopAgg->AssertValid(pdxlnAgg, false /* validate_children */);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnSort(
	gpos::Ref<CExpression> pexprSort, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprSort);

	GPOS_ASSERT(1 == pexprSort->Arity());

	// extract components
	gpos::Ref<CPhysicalSort> popSort =
		CPhysicalSort::PopConvert(pexprSort->Pop());
	gpos::Ref<CExpression> pexprChild = (*pexprSort)[0];

	// translate relational child expression
	gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
		pexprChild, colref_array, pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
		false /*fRemap*/, false /*fRoot*/);

	// translate order spec
	gpos::Ref<CDXLNode> sort_col_list_dxlnode =
		GetSortColListDXL(popSort->Pos());

	// construct project list from child project list
	GPOS_ASSERT(nullptr != child_dxlnode && 1 <= child_dxlnode->Arity());
	gpos::Ref<CDXLNode> pdxlnProjListChild = (*child_dxlnode)[0];
	gpos::Ref<CDXLNode> proj_list_dxlnode =
		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
			m_mp, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	// create an empty filter
	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(nullptr);

	// construct a sort node
	gpos::Ref<CDXLPhysicalSort> pdxlopSort =
		GPOS_NEW(m_mp) CDXLPhysicalSort(m_mp, false /*discard_duplicates*/);

	// construct sort node from its components
	gpos::Ref<CDXLNode> pdxlnSort = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopSort);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties = GetProperties(pexprSort);
	pdxlnSort->SetProperties(dxl_properties);

	// construct empty limit count and offset nodes
	gpos::Ref<CDXLNode> limit_count_dxlnode = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarLimitCount(m_mp));
	gpos::Ref<CDXLNode> limit_offset_dxlnode = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarLimitOffset(m_mp));

	// add children
	pdxlnSort->AddChild(proj_list_dxlnode);
	pdxlnSort->AddChild(filter_dxlnode);
	pdxlnSort->AddChild(sort_col_list_dxlnode);
	pdxlnSort->AddChild(limit_count_dxlnode);
	pdxlnSort->AddChild(limit_offset_dxlnode);
	pdxlnSort->AddChild(child_dxlnode);

#ifdef GPOS_DEBUG
	pdxlopSort->AssertValid(pdxlnSort, false /* validate_children */);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnLimit(
	gpos::Ref<CExpression> pexprLimit, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprLimit);
	GPOS_ASSERT(3 == pexprLimit->Arity());

	// extract components
	gpos::Ref<CExpression> pexprChild = (*pexprLimit)[0];
	gpos::Ref<CExpression> pexprOffset = (*pexprLimit)[1];
	gpos::Ref<CExpression> pexprCount = (*pexprLimit)[2];

	// bypass translation of limit if it does not have row count and offset
	gpos::Ref<CPhysicalLimit> popLimit =
		CPhysicalLimit::PopConvert(pexprLimit->Pop());
	if (!popLimit->FHasCount() && CUtils::FHasZeroOffset(pexprLimit))
	{
		return CreateDXLNode(pexprChild, colref_array, pdrgpdsBaseTables,
							 pulNonGatherMotions, pfDML, true /*fRemap*/,
							 false /*fRoot*/);
	}

	// translate relational child expression
	gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
		pexprChild, colref_array, pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
		true /*fRemap*/, false /*fRoot*/);

	// translate limit offset and count
	gpos::Ref<CDXLNode> limit_offset = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarLimitOffset(m_mp));
	limit_offset->AddChild(PdxlnScalar(pexprOffset));

	gpos::Ref<CDXLNode> limit_count = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarLimitCount(m_mp));
	limit_count->AddChild(PdxlnScalar(pexprCount));

	// construct project list from child project list
	GPOS_ASSERT(nullptr != child_dxlnode && 1 <= child_dxlnode->Arity());
	gpos::Ref<CDXLNode> pdxlnProjListChild = (*child_dxlnode)[0];
	gpos::Ref<CDXLNode> proj_list_dxlnode =
		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
			m_mp, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	// construct a limit node
	gpos::Ref<CDXLPhysicalLimit> pdxlopLimit =
		GPOS_NEW(m_mp) CDXLPhysicalLimit(m_mp);

	// construct limit node from its components
	gpos::Ref<CDXLNode> pdxlnLimit = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopLimit);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprLimit);
	pdxlnLimit->SetProperties(dxl_properties);

	pdxlnLimit->AddChild(proj_list_dxlnode);
	pdxlnLimit->AddChild(child_dxlnode);
	pdxlnLimit->AddChild(limit_count);
	pdxlnLimit->AddChild(limit_offset);

#ifdef GPOS_DEBUG
	pdxlopLimit->AssertValid(pdxlnLimit, false /* validate_children */);
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
	gpos::Ref<CExpression> pexprCorrelatedLOJ,
	CDXLColRefArray *dxl_colref_array,
	gpos::Ref<CDXLNode> *
		ppdxlnScalar,  // output: scalar condition after replacing inner child reference with subplan
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprCorrelatedLOJ);
	GPOS_ASSERT(COperator::EopPhysicalCorrelatedLeftOuterNLJoin ==
				pexprCorrelatedLOJ->Pop()->Eopid());

	gpos::Ref<CExpression> pexprInner = (*pexprCorrelatedLOJ)[1];
	gpos::Ref<CExpression> pexprScalar = (*pexprCorrelatedLOJ)[2];

	gpos::Ref<CColRefArray> pdrgpcrInner =
		CPhysicalNLJoin::PopConvert(pexprCorrelatedLOJ->Pop())->PdrgPcrInner();
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

	gpos::Ref<CExpression> pexprTrue =
		CUtils::PexprScalarConstBool(m_mp, true /*value*/, false /*is_null*/);
	*ppdxlnScalar = PdxlnScalar(pexprTrue);
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
	gpos::Ref<CExpression> pexprCorrelatedNLJoin,
	CDXLColRefArray *dxl_colref_array,
	gpos::Ref<CDXLNode> *
		ppdxlnScalar,  // output: scalar condition after replacing inner child reference with subplan
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(CUtils::FCorrelatedNLJoin(pexprCorrelatedNLJoin->Pop()));
	GPOS_ASSERT(nullptr != ppdxlnScalar);

	gpos::Ref<CExpression> pexprInner = (*pexprCorrelatedNLJoin)[1];
	gpos::Ref<CExpression> pexprScalar = (*pexprCorrelatedNLJoin)[2];

	gpos::Ref<CColRefArray> pdrgpcrInner =
		CPhysicalNLJoin::PopConvert(pexprCorrelatedNLJoin->Pop())
			->PdrgPcrInner();
	GPOS_ASSERT(nullptr != pdrgpcrInner);

	COperator::EOperatorId op_id = pexprCorrelatedNLJoin->Pop()->Eopid();
	gpos::Ref<CDXLNode> pdxlnSubPlan = nullptr;
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

			*ppdxlnScalar = pdxlnSubPlan;
			return;

		case COperator::EopPhysicalCorrelatedLeftSemiNLJoin:
		case COperator::EopPhysicalCorrelatedLeftAntiSemiNLJoin:
			pdxlnSubPlan = PdxlnExistentialSubplan(
				pdrgpcrInner, pexprCorrelatedNLJoin, dxl_colref_array,
				pdrgpdsBaseTables, pulNonGatherMotions, pfDML);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnRestrictResult(gpos::Ref<CDXLNode> dxlnode,
										  const gpos::Ref<CColRef> colref)
{
	gpos::Ref<CDXLNode> dxlresult = nullptr;
	gpos::Ref<CColRefSet> pcrInner = GPOS_NEW(m_mp) CColRefSet(m_mp);

	pcrInner->Include(colref);
	dxlresult = PdxlnRestrictResult(dxlnode, pcrInner);


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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnRestrictResult(gpos::Ref<CDXLNode> dxlnode,
										  const gpos::Ref<CColRefSet> colrefs)
{
	GPOS_ASSERT(nullptr != dxlnode);
	GPOS_ASSERT(nullptr != colrefs);

	gpos::Ref<CDXLNode> pdxlnProjListOld = (*dxlnode)[0];
	const ULONG ulPrjElems = pdxlnProjListOld->Arity();

	if (0 == ulPrjElems)
	{
		// failed to find project elements

		return nullptr;
	}

	gpos::Ref<CDXLNode> pdxlnResult = dxlnode;
	if (1 < ulPrjElems)
	{
		// restrict project list to required column
		gpos::Ref<CDXLScalarProjList> pdxlopPrL =
			GPOS_NEW(m_mp) CDXLScalarProjList(m_mp);
		gpos::Ref<CDXLNode> pdxlnProjListNew =
			GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopPrL);

		gpos::Ref<IntToColRefMap> phmicr = colrefs->Phmicr(m_mp);

		for (ULONG ul = 0; ul < ulPrjElems; ul++)
		{
			gpos::Ref<CDXLNode> child_dxlnode = (*pdxlnProjListOld)[ul];
			gpos::Ref<CDXLScalarProjElem> pdxlPrjElem =
				CDXLScalarProjElem::Cast(child_dxlnode->GetOperator());

			const INT colid = pdxlPrjElem->Id();
			gpos::Ref<CColRef> colref = phmicr->Find(colid);
			if (colref)
			{
				// create a new project element that simply points to required column,
				// we cannot re-use child_dxlnode here since it may have a deep expression with columns inaccessible
				// above the child (inner) DXL expression
				gpos::Ref<CDXLNode> pdxlnPrEl =
					CTranslatorExprToDXLUtils::PdxlnProjElem(m_mp, m_phmcrdxln,
															 colref);
				pdxlnProjListNew->AddChild(pdxlnPrEl);
			}
		}



		GPOS_ASSERT(colrefs->Size() == pdxlnProjListNew->Arity());

		pdxlnResult = GPOS_NEW(m_mp)
			CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLPhysicalResult(m_mp));
		gpos::Ref<CDXLPhysicalProperties> dxl_properties =
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnQuantifiedSubplan(
	gpos::Ref<CColRefArray> pdrgpcrInner,
	gpos::Ref<CExpression> pexprCorrelatedNLJoin,
	CDXLColRefArray *dxl_colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	gpos::Ref<COperator> popCorrelatedJoin = pexprCorrelatedNLJoin->Pop();
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

	gpos::Ref<CExpression> pexprInner = (*pexprCorrelatedNLJoin)[1];
	gpos::Ref<CExpression> pexprScalar = (*pexprCorrelatedNLJoin)[2];

	// translate inner child
	gpos::Ref<CDXLNode> pdxlnInnerChild = CreateDXLNode(
		pexprInner, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// find required column from inner child
	gpos::Ref<CColRefSet> pcrInner = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrInner->Include((*pdrgpcrInner)[0]);

	BOOL outerParam = false;
	if (fCorrelatedLOJ)
	{
		// overwrite required inner column based on scalar expression

		gpos::Ref<CColRefSet> pcrsInner = pexprInner->DeriveOutputColumns();
		gpos::Ref<CColRefSet> pcrsUsed =
			GPOS_NEW(m_mp) CColRefSet(m_mp, *pexprScalar->DeriveUsedColumns());
		pcrsUsed->Intersection(pcrsInner);
		if (0 < pcrsUsed->Size())
		{
			GPOS_ASSERT(1 == pcrsUsed->Size() || 2 == pcrsUsed->Size());

			// Both sides of the SubPlan test expression can come from the
			// inner side. So we need to pass pcrsUsed instead of pcrInner into
			// PdxlnRestrictResult()
			outerParam = pcrsUsed->Size() > 1;


			pcrInner = pcrsUsed;
		}
		else
		{
		}
	}

	gpos::Ref<CDXLNode> inner_dxlnode =
		PdxlnRestrictResult(pdxlnInnerChild, pcrInner);

	if (nullptr == inner_dxlnode)
	{
		GPOS_RAISE(
			gpopt::ExmaDXL, gpopt::ExmiExpr2DXLUnsupportedFeature,
			GPOS_WSZ_LIT(
				"Outer references in the project list of a correlated subquery"));
	}

	// translate test expression
	gpos::Ref<CDXLNode> dxlnode_test_expr = PdxlnScalar(pexprScalar);

	const gpos::Ref<IMDTypeBool> pmdtypebool = m_pmda->PtMDType<IMDTypeBool>();
	gpos::Ref<IMDId> mdid = pmdtypebool->MDId();


	// construct a subplan node, with the inner child under it
	gpos::Ref<CDXLNode> pdxlnSubPlan = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarSubPlan(
						   m_mp, mdid, dxl_colref_array, dxl_subplan_type,
						   dxlnode_test_expr, outerParam));
	pdxlnSubPlan->AddChild(inner_dxlnode);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnProjectBoolConst(gpos::Ref<CDXLNode> dxlnode,
											BOOL value)
{
	GPOS_ASSERT(nullptr != dxlnode);

	// create a new project element with bool value
	const gpos::Ref<IMDTypeBool> pmdtypebool = m_pmda->PtMDType<IMDTypeBool>();
	gpos::Ref<IMDId> mdid = pmdtypebool->MDId();


	gpos::Ref<CDXLDatumBool> dxl_datum =
		GPOS_NEW(m_mp) CDXLDatumBool(m_mp, mdid, false /* is_null */, value);
	gpos::Ref<CDXLScalarConstValue> pdxlopConstValue =
		GPOS_NEW(m_mp) CDXLScalarConstValue(m_mp, dxl_datum);
	gpos::Ref<CColRef> colref =
		m_pcf->PcrCreate(pmdtypebool, default_type_modifier);
	gpos::Ref<CDXLNode> pdxlnPrEl =
		PdxlnProjElem(colref, GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopConstValue));

	gpos::Ref<CDXLScalarProjList> pdxlopPrL =
		GPOS_NEW(m_mp) CDXLScalarProjList(m_mp);
	gpos::Ref<CDXLNode> proj_list_dxlnode =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopPrL);
	proj_list_dxlnode->AddChild(pdxlnPrEl);
	gpos::Ref<CDXLNode> pdxlnResult =
		GPOS_NEW(m_mp) CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLPhysicalResult(m_mp));
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		CTranslatorExprToDXLUtils::PdxlpropCopy(m_mp, dxlnode);
	pdxlnResult->SetProperties(dxl_properties);

	pdxlnResult->AddChild(proj_list_dxlnode);
	pdxlnResult->AddChild(PdxlnFilter(nullptr));
	pdxlnResult->AddChild(GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarOneTimeFilter(m_mp)));
	pdxlnResult->AddChild(dxlnode);

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
	gpos::Ref<CExpression> pexprCorrelatedLOJ)
{
	GPOS_ASSERT(nullptr != pexprCorrelatedLOJ);
	GPOS_ASSERT(COperator::EopPhysicalCorrelatedLeftOuterNLJoin ==
				pexprCorrelatedLOJ->Pop()->Eopid());

	COperator::EOperatorId eopidSubq =
		CPhysicalCorrelatedLeftOuterNLJoin::PopConvert(
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
	gpos::Ref<CExpression> pexprCorrelatedNLJoin)
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnExistentialSubplan(
	gpos::Ref<CColRefArray> pdrgpcrInner,
	gpos::Ref<CExpression> pexprCorrelatedNLJoin,
	CDXLColRefArray *dxl_colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
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
	gpos::Ref<CExpression> pexprInner = (*pexprCorrelatedNLJoin)[1];

	gpos::Ref<CDXLNode> pdxlnInnerChild = CreateDXLNode(
		pexprInner, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	gpos::Ref<CDXLNode> pdxlnInnerProjList = (*pdxlnInnerChild)[0];
	gpos::Ref<CDXLNode> inner_dxlnode = nullptr;
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

	const gpos::Ref<IMDTypeBool> pmdtypebool = m_pmda->PtMDType<IMDTypeBool>();
	gpos::Ref<IMDId> mdid = pmdtypebool->MDId();


	// construct a subplan node, with the inner child under it
	gpos::Ref<CDXLNode> pdxlnSubPlan = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarSubPlan(
						   m_mp, mdid, dxl_colref_array, dxl_subplan_type,
						   nullptr /*dxlnode_test_expr*/));
	pdxlnSubPlan->AddChild(inner_dxlnode);

	// add to hashmap
	BOOL fRes GPOS_ASSERTS_ONLY =
		m_phmcrdxln->Insert((*pdrgpcrInner)[0], pdxlnSubPlan);
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
	gpos::Ref<CColRefArray> pdrgpcrInner, gpos::Ref<CExpression> pexprInner,
	CDXLColRefArray *dxl_colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	const ULONG size = pdrgpcrInner->Size();

	gpos::Ref<CDXLNodeArray> pdrgpdxlnInner =
		GPOS_NEW(m_mp) CDXLNodeArray(m_mp);
	for (ULONG ul = 0; ul < size; ul++)
	{
		// for each subplan, we need to re-translate inner expression
		gpos::Ref<CDXLNode> pdxlnInnerChild = CreateDXLNode(
			pexprInner, nullptr /*colref_array*/, pdrgpdsBaseTables,
			pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
		gpos::Ref<CDXLNode> inner_dxlnode =
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
		gpos::Ref<CDXLNode> inner_dxlnode = (*pdrgpdxlnInner)[ul];

		if (0 < ul)
		{
			// if there is more than one subplan, we need to add-ref passed arrays
		}
		const gpos::Ref<CColRef> pcrInner = (*pdrgpcrInner)[ul];
		BuildDxlnSubPlan(inner_dxlnode, pcrInner, dxl_colref_array);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PcrsOuterRefsForCorrelatedNLJoin
//
//	@doc:
//		Return outer refs in correlated join inner child
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CTranslatorExprToDXL::PcrsOuterRefsForCorrelatedNLJoin(
	gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(CUtils::FCorrelatedNLJoin(pexpr->Pop()));

	gpos::Ref<CExpression> pexprInnerChild = (*pexpr)[1];

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnCorrelatedNLJoin(
	gpos::Ref<CExpression> pexpr, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(CUtils::FCorrelatedNLJoin(pexpr->Pop()));

	// extract components
	gpos::Ref<CExpression> pexprOuterChild = (*pexpr)[0];
	gpos::Ref<CExpression> pexprInnerChild = (*pexpr)[1];
	gpos::Ref<CExpression> pexprScalar = (*pexpr)[2];

	// outer references in the inner child
	CDXLColRefArray *dxl_colref_array = GPOS_NEW(m_mp) CDXLColRefArray(m_mp);

	gpos::Ref<CColRefSet> outer_refs = PcrsOuterRefsForCorrelatedNLJoin(pexpr);
	CColRefSetIter crsi(*outer_refs);
	while (crsi.Advance())
	{
		gpos::Ref<CColRef> colref = crsi.Pcr();
		CMDName *mdname = GPOS_NEW(m_mp) CMDName(m_mp, colref->Name().Pstr());
		gpos::Ref<IMDId> mdid = colref->RetrieveType()->MDId();

		gpos::Ref<CDXLColRef> dxl_colref = GPOS_NEW(m_mp)
			CDXLColRef(mdname, colref->Id(), mdid, colref->TypeModifier());
		dxl_colref_array->Append(dxl_colref);
	}

	COperator::EOperatorId op_id = pexpr->Pop()->Eopid();
	gpos::Ref<CDXLNode> pdxlnCond = nullptr;

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
		gpos::Ref<CDXLNode> pdxlnInnerChild =
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
	gpos::Ref<CDXLPhysicalProperties> dxl_properties = GetProperties(pexpr);
	gpos::Ref<CDXLNode> dxlnode = nullptr;

	switch (pexprOuterChild->Pop()->Eopid())
	{
		case COperator::EopPhysicalTableScan:
		{
			// create and return a table scan node
			dxlnode = PdxlnTblScanFromNLJoinOuter(
				pexprOuterChild, pdxlnCond, colref_array, pdrgpdsBaseTables,
				pulNonGatherMotions, dxl_properties);
			break;
		}

		case COperator::EopPhysicalFilter:
		{
			dxlnode = PdxlnResultFromNLJoinOuter(
				pexprOuterChild, pdxlnCond, colref_array, pdrgpdsBaseTables,
				pulNonGatherMotions, pfDML, dxl_properties);
			break;
		}

		default:
		{
			// create a result node over outer child

			dxlnode = PdxlnResult(pexprOuterChild, colref_array,
								  pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
								  pdxlnCond, dxl_properties);
		}
	}


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
CTranslatorExprToDXL::BuildDxlnSubPlan(gpos::Ref<CDXLNode> pdxlnRelChild,
									   const gpos::Ref<CColRef> colref,
									   CDXLColRefArray *dxl_colref_array)
{
	GPOS_ASSERT(nullptr != colref);
	gpos::Ref<IMDId> mdid = colref->RetrieveType()->MDId();


	// construct a subplan node, with the inner child under it
	gpos::Ref<CDXLNode> pdxlnSubPlan = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarSubPlan(m_mp, mdid, dxl_colref_array,
											   EdxlSubPlanTypeScalar, nullptr));
	pdxlnSubPlan->AddChild(pdxlnRelChild);

	// add to hashmap
	BOOL fRes GPOS_ASSERTS_ONLY = m_phmcrdxln->Insert(colref, pdxlnSubPlan);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnBooleanScalarWithSubPlan(
	gpos::Ref<CDXLNode> pdxlnRelChild, CDXLColRefArray *dxl_colref_array)
{
	// create a new project element (const:true), and replace the first child with it
	const gpos::Ref<IMDTypeBool> pmdtypebool = m_pmda->PtMDType<IMDTypeBool>();
	gpos::Ref<IMDId> mdid = pmdtypebool->MDId();


	gpos::Ref<CDXLDatumBool> dxl_datum = GPOS_NEW(m_mp)
		CDXLDatumBool(m_mp, mdid, false /* is_null */, true /* value */);
	gpos::Ref<CDXLScalarConstValue> pdxlopConstValue =
		GPOS_NEW(m_mp) CDXLScalarConstValue(m_mp, dxl_datum);

	gpos::Ref<CColRef> colref =
		m_pcf->PcrCreate(pmdtypebool, default_type_modifier);

	gpos::Ref<CDXLNode> pdxlnPrEl =
		PdxlnProjElem(colref, GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopConstValue));

	// create a new Result node for the created project element
	gpos::Ref<CDXLNode> pdxlnProjListNew =
		GPOS_NEW(m_mp) CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarProjList(m_mp));
	pdxlnProjListNew->AddChild(pdxlnPrEl);
	gpos::Ref<CDXLNode> pdxlnResult =
		GPOS_NEW(m_mp) CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLPhysicalResult(m_mp));
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		CTranslatorExprToDXLUtils::PdxlpropCopy(m_mp, pdxlnRelChild);
	pdxlnResult->SetProperties(dxl_properties);
	pdxlnResult->AddChild(pdxlnProjListNew);
	pdxlnResult->AddChild(PdxlnFilter(nullptr));
	pdxlnResult->AddChild(GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarOneTimeFilter(m_mp)));
	pdxlnResult->AddChild(pdxlnRelChild);

	// construct a subplan node, with the Result node under it

	gpos::Ref<CDXLNode> pdxlnSubPlan = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarSubPlan(m_mp, mdid, dxl_colref_array,
											   EdxlSubPlanTypeScalar, nullptr));
	pdxlnSubPlan->AddChild(pdxlnResult);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScBoolExpr(EdxlBoolExprType boolexptype,
									  gpos::Ref<CDXLNode> dxlnode_left,
									  gpos::Ref<CDXLNode> dxlnode_right)
{
	gpos::Ref<CDXLNode> pdxlnBoolExpr = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarBoolExpr(m_mp, boolexptype));

	pdxlnBoolExpr->AddChild(dxlnode_left);
	pdxlnBoolExpr->AddChild(dxlnode_right);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnTblScanFromNLJoinOuter(
	gpos::Ref<CExpression> pexprRelational, gpos::Ref<CDXLNode> pdxlnCond,
	gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *,  // pulNonGatherMotions,
	gpos::Ref<CDXLPhysicalProperties> dxl_properties)
{
	// create a table scan over the input expression, without a filter
	gpos::Ref<CDXLNode> pdxlnTblScan =
		PdxlnTblScan(pexprRelational,
					 nullptr,  //pcrsOutput
					 colref_array, pdrgpdsBaseTables,
					 nullptr,  //pexprScalar
					 dxl_properties);

	if (!CTranslatorExprToDXLUtils::FScalarConstTrue(m_pmda, pdxlnCond))
	{
		// add the new filter to the table scan replacing its original
		// empty filter
		gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(pdxlnCond);
		pdxlnTblScan->ReplaceChild(EdxltsIndexFilter /*ulPos*/, filter_dxlnode);
	}
	else
	{
		// not used
	}

	return pdxlnTblScan;
}

static ULONG
UlIndexFilter(Edxlopid edxlopid)
{
	switch (edxlopid)
	{
		case EdxlopPhysicalTableScan:
		case EdxlopPhysicalForeignScan:
			return EdxltsIndexFilter;
		case EdxlopPhysicalDynamicForeignScan:
			return EdxldfsIndexFilter;
		case EdxlopPhysicalBitmapTableScan:
		case EdxlopPhysicalDynamicBitmapTableScan:
			return EdxlbsIndexFilter;
		case EdxlopPhysicalDynamicTableScan:
			return EdxldtsIndexFilter;
		case EdxlopPhysicalIndexScan:
		case EdxlopPhysicalDynamicIndexScan:
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnResultFromNLJoinOuter(
	gpos::Ref<CExpression> pexprOuterChildRelational,
	gpos::Ref<CDXLNode> pdxlnJoinCond, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML,
	gpos::Ref<CDXLPhysicalProperties> dxl_properties)
{
	// create a result node using the filter from the outer child of the input expression
	gpos::Ref<CDXLNode> pdxlnRelationalNew = PdxlnFromFilter(
		pexprOuterChildRelational, colref_array, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, dxl_properties);


	Edxlopid edxlopid = pdxlnRelationalNew->GetOperator()->GetDXLOperator();
	switch (edxlopid)
	{
		case EdxlopPhysicalTableScan:
		case EdxlopPhysicalForeignScan:
		case EdxlopPhysicalBitmapTableScan:
		case EdxlopPhysicalDynamicTableScan:
		case EdxlopPhysicalIndexScan:
		case EdxlopPhysicalDynamicIndexScan:
		case EdxlopPhysicalDynamicBitmapTableScan:
		case EdxlopPhysicalResult:
		{
			// if the scalar join condition is a constant TRUE, just translate the child, no need to create an AND expression
			if (CTranslatorExprToDXLUtils::FScalarConstTrue(m_pmda,
															pdxlnJoinCond))
			{
				break;
			}

			// create new AND expression with the outer child's filter node and the join condition
			ULONG ulIndexFilter = UlIndexFilter(edxlopid);
			GPOS_ASSERT(ulIndexFilter != gpos::ulong_max);
			gpos::Ref<CDXLNode> pdxlnChildFilter =
				(*pdxlnRelationalNew)[ulIndexFilter];
			GPOS_ASSERT(EdxlopScalarFilter ==
						pdxlnChildFilter->GetOperator()->GetDXLOperator());
			gpos::Ref<CDXLNode> newFilterPred = pdxlnJoinCond;

			if (0 < pdxlnChildFilter->Arity())
			{
				// we have both a filter condition (from the outer child) in our result node
				// and a non-trivial condition pdxlnJoinCond passed in as parameter, need to AND the two
				gpos::Ref<CDXLNode> pdxlnCondFromChildFilter =
					(*pdxlnChildFilter)[0];

				GPOS_ASSERT(2 > pdxlnChildFilter->Arity());


				newFilterPred = PdxlnScBoolExpr(
					Edxland, pdxlnCondFromChildFilter, pdxlnJoinCond);
			}

			// add the new filter to the result replacing its original
			// empty filter
			gpos::Ref<CDXLNode> new_filter_dxlnode = PdxlnFilter(newFilterPred);
			pdxlnRelationalNew->ReplaceChild(ulIndexFilter /*ulPos*/,
											 new_filter_dxlnode);
		}
		break;
		// In case the OuterChild is a physical sequence, it will already have the filter in the partition selector and
		// dynamic scan, thus we should not replace the filter.
		case EdxlopPhysicalSequence:
		case EdxlopPhysicalAppend:
		{
			GPOS_ASSERT(nullptr != pexprOuterChildRelational->Prpp());
			gpos::Ref<CColRefSet> pcrsOutput =
				pexprOuterChildRelational->Prpp()->PcrsRequired();
			pdxlnRelationalNew = PdxlnAddScalarFilterOnRelationalChild(
				pdxlnRelationalNew, pdxlnJoinCond, dxl_properties, pcrsOutput,
				colref_array);
		}
		break;
		default:

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
CTranslatorExprToDXL::StoreIndexNLJOuterRefs(gpos::Ref<CPhysical> pop)
{
	gpos::Ref<CColRefArray> colref_array = nullptr;

	if (COperator::EopPhysicalInnerIndexNLJoin == pop->Eopid())
	{
		colref_array =
			CPhysicalInnerIndexNLJoin::PopConvert(pop)->PdrgPcrOuterRefs();
	}
	else
	{
		colref_array =
			CPhysicalLeftOuterIndexNLJoin::PopConvert(pop)->PdrgPcrOuterRefs();
	}
	GPOS_ASSERT(colref_array != nullptr);

	const ULONG size = colref_array->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		gpos::Ref<CColRef> colref = (*colref_array)[ul];
		if (nullptr == m_phmcrdxlnIndexLookup->Find(colref))
		{
			gpos::Ref<CDXLNode> dxlnode = CTranslatorExprToDXLUtils::PdxlnIdent(
				m_mp, m_phmcrdxln, m_phmcrdxlnIndexLookup, m_phmcrulPartColId,
				colref);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnNLJoin(
	gpos::Ref<CExpression> pexprInnerNLJ, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprInnerNLJ);
	GPOS_ASSERT(3 == pexprInnerNLJ->Arity());

	// extract components
	gpos::Ref<CPhysical> pop = CPhysical::PopConvert(pexprInnerNLJ->Pop());

	gpos::Ref<CExpression> pexprOuterChild = (*pexprInnerNLJ)[0];
	gpos::Ref<CExpression> pexprInnerChild = (*pexprInnerNLJ)[1];
	gpos::Ref<CExpression> pexprScalar = (*pexprInnerNLJ)[2];


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
	gpos::Ref<CColRefArray> outer_refs = nullptr;
	switch (pop->Eopid())
	{
		case COperator::EopPhysicalInnerNLJoin:
			join_type = EdxljtInner;
			break;

		case COperator::EopPhysicalInnerIndexNLJoin:
			join_type = EdxljtInner;
			is_index_nlj = true;
			StoreIndexNLJOuterRefs(pop);
			outer_refs =
				CPhysicalInnerIndexNLJoin::PopConvert(pop)->PdrgPcrOuterRefs();
			break;

		case COperator::EopPhysicalLeftOuterIndexNLJoin:
			join_type = EdxljtLeft;
			is_index_nlj = true;
			StoreIndexNLJOuterRefs(pop);
			outer_refs = CPhysicalLeftOuterIndexNLJoin::PopConvert(pop)
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
	gpos::Ref<CDXLNode> pdxlnOuterChild = CreateDXLNode(
		pexprOuterChild, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	gpos::Ref<CDXLNode> pdxlnInnerChild = CreateDXLNode(
		pexprInnerChild, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	gpos::Ref<CDXLNode> pdxlnCond = PdxlnScalar(pexprScalar);

	gpos::Ref<CDXLNode> dxlnode_join_filter = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarJoinFilter(m_mp));
	if (nullptr != pdxlnCond)
	{
		dxlnode_join_filter->AddChild(pdxlnCond);
	}

	BOOL nest_params_exists = false;
	CDXLColRefArray *col_refs = nullptr;
	if (is_index_nlj && GPOS_FTRACE(EopttraceIndexedNLJOuterRefAsParams))
	{
		nest_params_exists = true;
		col_refs = GPOS_NEW(m_mp) CDXLColRefArray(m_mp);
		for (ULONG ul = 0; ul < outer_refs->Size(); ul++)
		{
			gpos::Ref<CColRef> col_ref = (*outer_refs)[ul];
			CMDName *md_name =
				GPOS_NEW(m_mp) CMDName(m_mp, col_ref->Name().Pstr());
			gpos::Ref<IMDId> mdid = col_ref->RetrieveType()->MDId();

			gpos::Ref<CDXLColRef> colref_dxl = GPOS_NEW(m_mp) CDXLColRef(
				md_name, col_ref->Id(), mdid, col_ref->TypeModifier());
			col_refs->Append(colref_dxl);
		}
	}

	// construct a join node
	gpos::Ref<CDXLPhysicalNLJoin> pdxlopNLJ = GPOS_NEW(m_mp)
		CDXLPhysicalNLJoin(m_mp, join_type, is_index_nlj, nest_params_exists);
	pdxlopNLJ->SetNestLoopParamsColRefs(col_refs);

	// construct projection list
	// compute required columns
	GPOS_ASSERT(nullptr != pexprInnerNLJ->Prpp());
	gpos::Ref<CColRefSet> pcrsOutput = pexprInnerNLJ->Prpp()->PcrsRequired();

	gpos::Ref<CDXLNode> proj_list_dxlnode =
		PdxlnProjList(pcrsOutput, colref_array);

	gpos::Ref<CDXLNode> pdxlnNLJ = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopNLJ);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprInnerNLJ);
	pdxlnNLJ->SetProperties(dxl_properties);

	// construct an empty plan filter
	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(nullptr);

	// add children
	pdxlnNLJ->AddChild(proj_list_dxlnode);
	pdxlnNLJ->AddChild(filter_dxlnode);
	pdxlnNLJ->AddChild(dxlnode_join_filter);
	pdxlnNLJ->AddChild(pdxlnOuterChild);
	pdxlnNLJ->AddChild(pdxlnInnerChild);

#ifdef GPOS_DEBUG
	pdxlopNLJ->AssertValid(pdxlnNLJ, false /* validate_children */);
#endif

	return pdxlnNLJ;
}

gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnMergeJoin(
	gpos::Ref<CExpression> pexprMJ, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprMJ);
	GPOS_ASSERT(3 == pexprMJ->Arity());

	// extract components
	gpos::Ref<CPhysical> pop = CPhysical::PopConvert(pexprMJ->Pop());

	gpos::Ref<CExpression> pexprOuterChild = (*pexprMJ)[0];
	gpos::Ref<CExpression> pexprInnerChild = (*pexprMJ)[1];
	gpos::Ref<CExpression> pexprScalar = (*pexprMJ)[2];

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
	gpos::Ref<CDXLNode> pdxlnOuterChild = CreateDXLNode(
		pexprOuterChild, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	gpos::Ref<CDXLNode> pdxlnInnerChild = CreateDXLNode(
		pexprInnerChild, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	gpos::Ref<CDXLNode> dxlnode_merge_conds = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarMergeCondList(m_mp));

	gpos::Ref<CExpressionArray> pdrgpexprPredicates =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprScalar);
	const ULONG length = pdrgpexprPredicates->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::Ref<CExpression> pexprPred = (*pdrgpexprPredicates)[ul];
		// At this point, they all better be merge joinable
		GPOS_ASSERT(CPhysicalJoin::FMergeJoinCompatible(
			pexprPred, pexprOuterChild, pexprInnerChild));
		gpos::Ref<CExpression> pexprPredOuter = (*pexprPred)[0];
		gpos::Ref<CExpression> pexprPredInner = (*pexprPred)[1];

		// align extracted columns with outer and inner children of the join
		gpos::Ref<CColRefSet> pcrsOuterChild =
			pexprOuterChild->DeriveOutputColumns();
		gpos::Ref<CColRefSet> pcrsPredInner =
			pexprPredInner->DeriveUsedColumns();
#ifdef GPOS_DEBUG
		gpos::Ref<CColRefSet> pcrsInnerChild =
			pexprInnerChild->DeriveOutputColumns();
		gpos::Ref<CColRefSet> pcrsPredOuter =
			pexprPredOuter->DeriveUsedColumns();
#endif

		if (pcrsOuterChild->ContainsAll(pcrsPredInner))
		{
			GPOS_ASSERT(pcrsInnerChild->ContainsAll(pcrsPredOuter));
			std::swap(pexprPredOuter, pexprPredInner);
#ifdef GPOS_DEBUG
			std::swap(pcrsPredOuter, pcrsPredInner);
#endif



			pexprPred =
				CUtils::PexprScalarEqCmp(m_mp, pexprPredOuter, pexprPredInner);
		}
		else
		{
		}

		GPOS_ASSERT(pcrsOuterChild->ContainsAll(pcrsPredOuter) &&
					pcrsInnerChild->ContainsAll(pcrsPredInner) &&
					"merge join keys are not aligned with children");

		dxlnode_merge_conds->AddChild(PdxlnScalar(pexprPred));
	}


	// construct a join node
	gpos::Ref<CDXLPhysicalMergeJoin> pdxlopMJ = GPOS_NEW(m_mp)
		CDXLPhysicalMergeJoin(m_mp, join_type, false /* is_unique_outer */);

	// construct projection list
	// compute required columns
	GPOS_ASSERT(nullptr != pexprMJ->Prpp());
	gpos::Ref<CColRefSet> pcrsOutput = pexprMJ->Prpp()->PcrsRequired();

	gpos::Ref<CDXLNode> proj_list_dxlnode =
		PdxlnProjList(pcrsOutput, colref_array);

	gpos::Ref<CDXLNode> pdxlnMJ = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopMJ);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties = GetProperties(pexprMJ);
	pdxlnMJ->SetProperties(dxl_properties);

	// construct an empty plan filter and join filter
	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(nullptr);
	gpos::Ref<CDXLNode> dxlnode_join_filter = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarJoinFilter(m_mp));

	// add children
	pdxlnMJ->AddChild(proj_list_dxlnode);
	pdxlnMJ->AddChild(filter_dxlnode);
	pdxlnMJ->AddChild(dxlnode_join_filter);
	pdxlnMJ->AddChild(dxlnode_merge_conds);
	pdxlnMJ->AddChild(pdxlnOuterChild);
	pdxlnMJ->AddChild(pdxlnInnerChild);

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
CTranslatorExprToDXL::EdxljtHashJoin(gpos::Ref<CPhysicalHashJoin> popHJ)
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnHashJoin(
	gpos::Ref<CExpression> pexprHJ, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprHJ);

	GPOS_ASSERT(3 == pexprHJ->Arity());

	// extract components
	gpos::Ref<CPhysicalHashJoin> popHJ =
		CPhysicalHashJoin::PopConvert(pexprHJ->Pop());
	gpos::Ref<CExpression> pexprOuterChild = (*pexprHJ)[0];
	gpos::Ref<CExpression> pexprInnerChild = (*pexprHJ)[1];
	gpos::Ref<CExpression> pexprScalar = (*pexprHJ)[2];

	EdxlJoinType join_type = EdxljtHashJoin(popHJ);
	GPOS_ASSERT(popHJ->PdrgpexprOuterKeys()->Size() ==
				popHJ->PdrgpexprInnerKeys()->Size());

	// translate relational child expression
	gpos::Ref<CDXLNode> pdxlnOuterChild = CreateDXLNode(
		pexprOuterChild, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	gpos::Ref<CDXLNode> pdxlnInnerChild = CreateDXLNode(
		pexprInnerChild, nullptr /*colref_array*/, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// construct hash condition
	gpos::Ref<CDXLNode> pdxlnHashCondList = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarHashCondList(m_mp));

#ifdef GPOS_DEBUG
	ULONG ulHashJoinPreds = 0;
#endif

	gpos::Ref<CExpressionArray> pdrgpexprPredicates =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprScalar);
	gpos::Ref<CExpressionArray> pdrgpexprRemainingPredicates =
		GPOS_NEW(m_mp) CExpressionArray(m_mp);
	const ULONG size = pdrgpexprPredicates->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		gpos::Ref<CExpression> pexprPred = (*pdrgpexprPredicates)[ul];
		if (CPhysicalJoin::FHashJoinCompatible(pexprPred, pexprOuterChild,
											   pexprInnerChild))
		{
			gpos::Ref<CExpression> pexprPredOuter;
			gpos::Ref<CExpression> pexprPredInner;
			gpos::Ref<IMDId> mdid_scop;
			CPhysicalJoin::AlignJoinKeyOuterInner(
				pexprPred, pexprOuterChild, pexprInnerChild, &pexprPredOuter,
				&pexprPredInner, &mdid_scop);



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

			gpos::Ref<CDXLNode> pdxlnPred = PdxlnScalar(pexprPred);
			pdxlnHashCondList->AddChild(pdxlnPred);

#ifdef GPOS_DEBUG
			ulHashJoinPreds++;
#endif	// GPOS_DEBUG
		}
		else
		{
			pdrgpexprRemainingPredicates->Append(pexprPred);
		}
	}
	GPOS_ASSERT(popHJ->PdrgpexprOuterKeys()->Size() == ulHashJoinPreds);

	gpos::Ref<CDXLNode> dxlnode_join_filter = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarJoinFilter(m_mp));
	if (0 < pdrgpexprRemainingPredicates->Size())
	{
		gpos::Ref<CExpression> pexprJoinCond =
			CPredicateUtils::PexprConjunction(m_mp,
											  pdrgpexprRemainingPredicates);
		gpos::Ref<CDXLNode> pdxlnJoinCond = PdxlnScalar(pexprJoinCond);
		dxlnode_join_filter->AddChild(pdxlnJoinCond);
	}
	else
	{
	}

	// construct a hash join node
	gpos::Ref<CDXLPhysicalHashJoin> pdxlopHJ =
		GPOS_NEW(m_mp) CDXLPhysicalHashJoin(m_mp, join_type);

	// construct projection list from required columns
	GPOS_ASSERT(nullptr != pexprHJ->Prpp());
	gpos::Ref<CColRefSet> pcrsOutput = pexprHJ->Prpp()->PcrsRequired();
	gpos::Ref<CDXLNode> proj_list_dxlnode =
		PdxlnProjList(pcrsOutput, colref_array);

	gpos::Ref<CDXLNode> pdxlnHJ = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopHJ);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties = GetProperties(pexprHJ);
	pdxlnHJ->SetProperties(dxl_properties);

	// construct an empty plan filter
	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(nullptr);

	// add children
	pdxlnHJ->AddChild(proj_list_dxlnode);
	pdxlnHJ->AddChild(filter_dxlnode);
	pdxlnHJ->AddChild(dxlnode_join_filter);
	pdxlnHJ->AddChild(pdxlnHashCondList);
	pdxlnHJ->AddChild(pdxlnOuterChild);
	pdxlnHJ->AddChild(pdxlnInnerChild);

	// cleanup


#ifdef GPOS_DEBUG
	pdxlopHJ->AssertValid(pdxlnHJ, false /* validate_children */);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnMotion(
	gpos::Ref<CExpression> pexprMotion, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprMotion);
	GPOS_ASSERT(1 == pexprMotion->Arity());

	// extract components
	gpos::Ref<CExpression> pexprChild = (*pexprMotion)[0];

	// translate relational child expression
	gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
		pexprChild, colref_array, pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
		true /*fRemap*/, false /*fRoot*/);

	// construct a motion node
	gpos::Ref<CDXLPhysicalMotion> motion = nullptr;
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
			gpos::Ref<CPhysicalMotionRoutedDistribute> popMotion =
				CPhysicalMotionRoutedDistribute::PopConvert(pexprMotion->Pop());
			gpos::Ref<CColRef> pcrSegmentId =
				gpos::dyn_cast<CDistributionSpecRouted>(popMotion->Pds())
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
	gpos::Ref<CDXLNode> pdxlnProjListChild = (*child_dxlnode)[0];

	gpos::Ref<CDXLNode> proj_list_dxlnode =
		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
			m_mp, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	// set input and output segment information
	motion->SetSegmentInfo(GetInputSegIdsArray(pexprMotion),
						   GetOutputSegIdsArray(pexprMotion));

	gpos::Ref<CDXLNode> pdxlnMotion = GPOS_NEW(m_mp) CDXLNode(m_mp, motion);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprMotion);
	pdxlnMotion->SetProperties(dxl_properties);

	// construct an empty filter node
	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(nullptr /*pdxlnCond*/);

	// construct sort column list
	gpos::Ref<CDXLNode> sort_col_list_dxlnode = GetSortColListDXL(pexprMotion);

	// add children
	pdxlnMotion->AddChild(proj_list_dxlnode);
	pdxlnMotion->AddChild(filter_dxlnode);
	pdxlnMotion->AddChild(sort_col_list_dxlnode);

	if (COperator::EopPhysicalMotionHashDistribute ==
		pexprMotion->Pop()->Eopid())
	{
		// construct a hash expr list node
		gpos::Ref<CPhysicalMotionHashDistribute> popHashDistribute =
			CPhysicalMotionHashDistribute::PopConvert(pexprMotion->Pop());
		gpos::Ref<CDistributionSpecHashed> pdsHashed =
			CDistributionSpecHashed::PdsConvert(popHashDistribute->Pds());
		gpos::Ref<CDXLNode> hash_expr_list =
			PdxlnHashExprList(pdsHashed->Pdrgpexpr(), pdsHashed->Opfamilies());
		pdxlnMotion->AddChild(hash_expr_list);
	}

	pdxlnMotion->AddChild(child_dxlnode);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnMaterialize(
	gpos::Ref<CExpression> pexprSpool, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprSpool);

	GPOS_ASSERT(1 == pexprSpool->Arity());

	// extract components
	gpos::Ref<CExpression> pexprChild = (*pexprSpool)[0];

	// translate relational child expression
	gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
		pexprChild, colref_array, pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
		false /*fRemap*/, false /*fRoot*/);

	gpos::Ref<CPhysicalSpool> spool =
		CPhysicalSpool::PopConvert(pexprSpool->Pop());

	// construct a materialize node
	gpos::Ref<CDXLPhysicalMaterialize> pdxlopMat =
		GPOS_NEW(m_mp) CDXLPhysicalMaterialize(m_mp, spool->FEager());

	// construct project list from child project list
	GPOS_ASSERT(nullptr != child_dxlnode && 1 <= child_dxlnode->Arity());
	gpos::Ref<CDXLNode> pdxlnProjListChild = (*child_dxlnode)[0];
	gpos::Ref<CDXLNode> proj_list_dxlnode =
		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
			m_mp, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	gpos::Ref<CDXLNode> pdxlnMaterialize =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopMat);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprSpool);
	pdxlnMaterialize->SetProperties(dxl_properties);

	// construct an empty filter node
	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(nullptr /* pdxlnCond */);

	// add children
	pdxlnMaterialize->AddChild(proj_list_dxlnode);
	pdxlnMaterialize->AddChild(filter_dxlnode);
	pdxlnMaterialize->AddChild(child_dxlnode);

#ifdef GPOS_DEBUG
	pdxlopMat->AssertValid(pdxlnMaterialize, false /* validate_children */);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnSequence(
	gpos::Ref<CExpression> pexprSequence, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprSequence);

	const ULONG arity = pexprSequence->Arity();
	GPOS_ASSERT(0 < arity);

	// construct sequence node
	gpos::Ref<CDXLPhysicalSequence> pdxlopSequence =
		GPOS_NEW(m_mp) CDXLPhysicalSequence(m_mp);
	gpos::Ref<CDXLNode> pdxlnSequence =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopSequence);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprSequence);
	pdxlnSequence->SetProperties(dxl_properties);

	// translate children
	gpos::Ref<CDXLNodeArray> pdrgpdxlnChildren =
		GPOS_NEW(m_mp) CDXLNodeArray(m_mp);

	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild = (*pexprSequence)[ul];

		gpos::Ref<CColRefArray> pdrgpcrChildOutput = nullptr;
		if (ul == arity - 1)
		{
			// impose output columns on last child
			pdrgpcrChildOutput = colref_array;
		}

		gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
			pexprChild, pdrgpcrChildOutput, pdrgpdsBaseTables,
			pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
		pdrgpdxlnChildren->Append(child_dxlnode);
	}

	// construct project list from the project list of the last child
	gpos::Ref<CDXLNode> pdxlnLastChild = (*pdrgpdxlnChildren)[arity - 1];
	gpos::Ref<CDXLNode> pdxlnProjListChild = (*pdxlnLastChild)[0];

	gpos::Ref<CDXLNode> proj_list_dxlnode =
		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
			m_mp, m_pcf, m_phmcrdxln, pdxlnProjListChild);
	pdxlnSequence->AddChild(proj_list_dxlnode);

	// add children
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CDXLNode> pdxlnChid = (*pdrgpdxlnChildren)[ul];

		pdxlnSequence->AddChild(pdxlnChid);
	}



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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnPartitionSelector(
	gpos::Ref<CExpression> pexpr, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	gpos::Ref<CPhysicalPartitionSelector> popSelector =
		CPhysicalPartitionSelector::PopConvert(pexpr->Pop());

	gpos::Ref<CExpression> pexprChild = (*pexpr)[0];

	// translate child
	gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
		pexprChild, colref_array, pdrgpdsBaseTables, pulNonGatherMotions, pfDML,
		false /*fRemap*/, false /*fRoot*/);

	gpos::Ref<CDXLNode> pdxlnPrLChild = (*child_dxlnode)[0];
	gpos::Ref<CDXLNode> pdxlnPrL =
		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
			m_mp, m_pcf, m_phmcrdxln, pdxlnPrLChild);
	const ULONG scanid = popSelector->ScanId();

	gpos::Ref<CBitSet> bs =
		COptCtxt::PoctxtFromTLS()->GetPartitionsForScanId(scanid);
	GPOS_ASSERT(nullptr != bs);
	gpos::Ref<ULongPtrArray> parts = GPOS_NEW(m_mp) ULongPtrArray(m_mp);
	CBitSetIter bsi(*bs);
	while (bsi.Advance())
	{
		parts->Append(GPOS_NEW(m_mp) ULONG(bsi.Bit()));
	}


	gpos::Ref<CDXLNode> pdxlnSelector = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLPhysicalPartitionSelector(
						   m_mp, popSelector->MDId(), popSelector->SelectorId(),
						   popSelector->ScanId(), parts));

	gpos::Ref<CDXLNode> pdxlnFilter = PdxlnScalar(popSelector->FilterExpr());
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprChild);

	pdxlnSelector->SetProperties(dxl_properties);
	pdxlnSelector->AddChild(pdxlnPrL);
	pdxlnSelector->AddChild(pdxlnFilter);
	pdxlnSelector->AddChild(child_dxlnode);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnDML(
	gpos::Ref<CExpression> pexpr,
	gpos::Ref<CColRefArray>,  // colref_array
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(1 == pexpr->Arity());

	ULONG action_colid = 0;
	ULONG ctid_colid = 0;
	ULONG segid_colid = 0;

	// extract components
	gpos::Ref<CPhysicalDML> popDML = CPhysicalDML::PopConvert(pexpr->Pop());
	*pfDML = false;
	if (IMDId::EmdidGPDBCtas == popDML->Ptabdesc()->MDId()->MdidType())
	{
		return PdxlnCTAS(pexpr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
	}

	EdxlDmlType dxl_dml_type = Edxldmloptype(popDML->Edmlop());

	gpos::Ref<CExpression> pexprChild = (*pexpr)[0];
	gpos::Ref<CTableDescriptor> ptabdesc = popDML->Ptabdesc();
	gpos::Ref<CColRefArray> pdrgpcrSource = popDML->PdrgpcrSource();

	gpos::Ref<CColRef> pcrAction = popDML->PcrAction();
	GPOS_ASSERT(nullptr != pcrAction);
	action_colid = pcrAction->Id();

	gpos::Ref<CColRef> pcrCtid = popDML->PcrCtid();
	gpos::Ref<CColRef> pcrSegmentId = popDML->PcrSegmentId();
	if (nullptr != pcrCtid)
	{
		GPOS_ASSERT(nullptr != pcrSegmentId);
		ctid_colid = pcrCtid->Id();
		segid_colid = pcrSegmentId->Id();
	}

	gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
		pexprChild, pdrgpcrSource, pdrgpdsBaseTables, pulNonGatherMotions,
		pfDML, false /*fRemap*/, false /*fRoot*/);

	gpos::Ref<CDXLTableDescr> table_descr = MakeDXLTableDescr(
		ptabdesc, nullptr /*pdrgpcrOutput*/, nullptr /*requiredProperties*/);
	gpos::Ref<ULongPtrArray> pdrgpul = CUtils::Pdrgpul(m_mp, pdrgpcrSource);

	gpos::Ref<CDXLDirectDispatchInfo> dxl_direct_dispatch_info =
		GetDXLDirectDispatchInfo(pexpr);
	gpos::Ref<CDXLPhysicalDML> pdxlopDML = GPOS_NEW(m_mp) CDXLPhysicalDML(
		m_mp, dxl_dml_type, table_descr, pdrgpul, action_colid, ctid_colid,
		segid_colid, dxl_direct_dispatch_info, popDML->FSplit());

	// project list
	gpos::Ref<CColRefSet> pcrsOutput = pexpr->Prpp()->PcrsRequired();
	gpos::Ref<CDXLNode> pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrSource);

	gpos::Ref<CDXLNode> pdxlnDML = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopDML);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties = GetProperties(pexpr);
	pdxlnDML->SetProperties(dxl_properties);

	pdxlnDML->AddChild(pdxlnPrL);
	pdxlnDML->AddChild(child_dxlnode);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnCTAS(
	gpos::Ref<CExpression> pexpr,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(1 == pexpr->Arity());

	gpos::Ref<CPhysicalDML> popDML = CPhysicalDML::PopConvert(pexpr->Pop());
	GPOS_ASSERT(CLogicalDML::EdmlInsert == popDML->Edmlop());

	gpos::Ref<CExpression> pexprChild = (*pexpr)[0];
	gpos::Ref<CTableDescriptor> ptabdesc = popDML->Ptabdesc();
	gpos::Ref<CColRefArray> pdrgpcrSource = popDML->PdrgpcrSource();
	gpos::Ref<CMDRelationCtasGPDB> pmdrel = gpos::dyn_cast<CMDRelationCtasGPDB>(
		m_pmda->RetrieveRel(ptabdesc->MDId()));

	gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
		pexprChild, pdrgpcrSource, pdrgpdsBaseTables, pulNonGatherMotions,
		pfDML, true /*fRemap*/, true /*fRoot*/);

	gpos::Ref<ULongPtrArray> pdrgpul = CUtils::Pdrgpul(m_mp, pdrgpcrSource);



	const ULONG ulColumns = ptabdesc->ColumnCount();

	gpos::Ref<IntPtrArray> vartypemod_array = pmdrel->GetVarTypeModArray();
	GPOS_ASSERT(ulColumns == vartypemod_array->Size());

	// translate col descriptors
	gpos::Ref<CDXLColDescrArray> dxl_col_descr_array =
		GPOS_NEW(m_mp) CDXLColDescrArray(m_mp);
	for (ULONG ul = 0; ul < ulColumns; ul++)
	{
		const gpos::Ref<CColumnDescriptor> pcd = ptabdesc->Pcoldesc(ul);

		CMDName *pmdnameCol = GPOS_NEW(m_mp) CMDName(m_mp, pcd->Name().Pstr());
		gpos::Ref<CColRef> colref = m_pcf->PcrCreate(
			pcd->RetrieveType(), pcd->TypeModifier(), pcd->Name());

		// use the col ref id for the corresponding output output column as
		// colid for the dxl column
		gpos::Ref<CMDIdGPDB> pmdidColType =
			CMDIdGPDB::CastMdid(colref->RetrieveType()->MDId());


		gpos::Ref<CDXLColDescr> pdxlcd = GPOS_NEW(m_mp) CDXLColDescr(
			pmdnameCol, colref->Id(), pcd->AttrNum(), pmdidColType,
			colref->TypeModifier(), false /* fdropped */, pcd->Width());

		dxl_col_descr_array->Append(pdxlcd);
	}

	gpos::Ref<ULongPtrArray> pdrgpulDistr = nullptr;
	if (IMDRelation::EreldistrHash == pmdrel->GetRelDistribution())
	{
		pdrgpulDistr = GPOS_NEW(m_mp) ULongPtrArray(m_mp);
		const ULONG ulDistrCols = pmdrel->DistrColumnCount();
		for (ULONG ul = 0; ul < ulDistrCols; ul++)
		{
			const gpos::Ref<IMDColumn> pmdcol = pmdrel->GetDistrColAt(ul);
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

	gpos::Ref<CDXLPhysicalCTAS> pdxlopCTAS = GPOS_NEW(m_mp) CDXLPhysicalCTAS(
		m_mp, mdname_schema,
		GPOS_NEW(m_mp) CMDName(m_mp, pmdrel->Mdname().GetMDName()),
		dxl_col_descr_array, pmdrel->GetDxlCtasStorageOption(),
		pmdrel->GetRelDistribution(), pdrgpulDistr, pmdrel->GetDistrOpClasses(),
		pmdrel->IsTemporary(), pmdrel->RetrieveRelStorageType(), pdrgpul,
		vartypemod_array);

	gpos::Ref<CDXLNode> pdxlnCTAS = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopCTAS);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties = GetProperties(pexpr);
	pdxlnCTAS->SetProperties(dxl_properties);

	gpos::Ref<CColRefSet> pcrsOutput = pexpr->Prpp()->PcrsRequired();
	gpos::Ref<CDXLNode> pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrSource);

	pdxlnCTAS->AddChild(pdxlnPrL);
	pdxlnCTAS->AddChild(child_dxlnode);

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
gpos::Ref<CDXLDirectDispatchInfo>
CTranslatorExprToDXL::GetDXLDirectDispatchInfo(gpos::Ref<CExpression> pexprDML)
{
	GPOS_ASSERT(nullptr != pexprDML);

	gpos::Ref<CPhysicalDML> popDML = CPhysicalDML::PopConvert(pexprDML->Pop());
	gpos::Ref<CTableDescriptor> ptabdesc = popDML->Ptabdesc();
	const gpos::Ref<CColumnDescriptorArray> pdrgpcoldescDist =
		ptabdesc->PdrgpcoldescDist();
	if ((CLogicalDML::EdmlInsert != popDML->Edmlop() &&
		 CLogicalDML::EdmlDelete != popDML->Edmlop()) ||
		IMDRelation::EreldistrHash != ptabdesc->GetRelDistribution() ||
		1 < pdrgpcoldescDist->Size())
	{
		// directed dispatch only supported for insert and delete statements
		// on hash-distributed tables with a single distribution column

		return nullptr;
	}


	GPOS_ASSERT(1 == pdrgpcoldescDist->Size());
	gpos::Ref<CColumnDescriptor> pcoldesc = (*pdrgpcoldescDist)[0];
	ULONG ulPos =
		gpopt::CTableDescriptor::UlPos(pcoldesc, ptabdesc->Pdrgpcoldesc());
	GPOS_ASSERT(ulPos < ptabdesc->Pdrgpcoldesc()->Size() && "Column not found");

	gpos::Ref<CColRef> pcrDistrCol = (*popDML->PdrgpcrSource())[ulPos];
	gpos::Ref<CPropConstraint> ppc = (*pexprDML)[0]->DerivePropertyConstraint();

	if (nullptr == ppc->Pcnstr())
	{
		return nullptr;
	}

	gpos::Ref<CConstraint> pcnstrDistrCol =
		ppc->Pcnstr()->Pcnstr(m_mp, pcrDistrCol);
	if (!CPredicateUtils::FConstColumn(pcnstrDistrCol, pcrDistrCol))
	{
		return nullptr;
	}

	GPOS_ASSERT(CConstraint::EctInterval == pcnstrDistrCol->Ect());

	gpos::Ref<CConstraintInterval> pci =
		gpos::dyn_cast<CConstraintInterval>(pcnstrDistrCol);
	GPOS_ASSERT(1 >= pci->Pdrgprng()->Size());

	gpos::Ref<CDXLDatumArray> pdrgpdxldatum =
		GPOS_NEW(m_mp) CDXLDatumArray(m_mp);
	gpos::Ref<CDXLDatum> dxl_datum = nullptr;

	if (1 == pci->Pdrgprng()->Size())
	{
		const gpos::Ref<CRange> prng = (*pci->Pdrgprng())[0];
		dxl_datum = CTranslatorExprToDXLUtils::GetDatumVal(m_mp, m_pmda,
														   prng->PdatumLeft());
	}
	else
	{
		GPOS_ASSERT(pci->FIncludesNull());
		dxl_datum = pcrDistrCol->RetrieveType()->GetDXLDatumNull(m_mp);
	}

	pdrgpdxldatum->Append(dxl_datum);



	gpos::Ref<CDXLDatum2dArray> pdrgpdrgpdxldatum =
		GPOS_NEW(m_mp) CDXLDatum2dArray(m_mp);
	pdrgpdrgpdxldatum->Append(pdrgpdxldatum);
	return GPOS_NEW(m_mp) CDXLDirectDispatchInfo(pdrgpdrgpdxldatum, false);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnSplit
//
//	@doc:
//		Translate a split operator
//
//---------------------------------------------------------------------------
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnSplit(
	gpos::Ref<CExpression> pexpr,
	gpos::Ref<CColRefArray>,  // colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(2 == pexpr->Arity());

	ULONG action_colid = 0;
	ULONG ctid_colid = 0;
	ULONG segid_colid = 0;

	// extract components
	gpos::Ref<CPhysicalSplit> popSplit =
		CPhysicalSplit::PopConvert(pexpr->Pop());

	gpos::Ref<CExpression> pexprChild = (*pexpr)[0];
	gpos::Ref<CExpression> pexprProjList = (*pexpr)[1];

	gpos::Ref<CColRef> pcrAction = popSplit->PcrAction();
	GPOS_ASSERT(nullptr != pcrAction);
	action_colid = pcrAction->Id();

	gpos::Ref<CColRef> pcrCtid = popSplit->PcrCtid();
	GPOS_ASSERT(nullptr != pcrCtid);
	ctid_colid = pcrCtid->Id();

	gpos::Ref<CColRef> pcrSegmentId = popSplit->PcrSegmentId();
	GPOS_ASSERT(nullptr != pcrSegmentId);
	segid_colid = pcrSegmentId->Id();

	gpos::Ref<CColRefArray> pdrgpcrDelete = popSplit->PdrgpcrDelete();
	gpos::Ref<ULongPtrArray> delete_colid_array =
		CUtils::Pdrgpul(m_mp, pdrgpcrDelete);

	gpos::Ref<CColRefArray> pdrgpcrInsert = popSplit->PdrgpcrInsert();
	gpos::Ref<ULongPtrArray> insert_colid_array =
		CUtils::Pdrgpul(m_mp, pdrgpcrInsert);

	gpos::Ref<CColRefSet> pcrsRequired = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsRequired->Include(pdrgpcrInsert);
	pcrsRequired->Include(pdrgpcrDelete);
	gpos::Ref<CColRefArray> pdrgpcrRequired = pcrsRequired->Pdrgpcr(m_mp);

	gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
		pexprChild, pdrgpcrRequired, pdrgpdsBaseTables, pulNonGatherMotions,
		pfDML, true /*fRemap*/, false /*fRoot*/);



	gpos::Ref<CDXLPhysicalSplit> pdxlopSplit = GPOS_NEW(m_mp)
		CDXLPhysicalSplit(m_mp, delete_colid_array, insert_colid_array,
						  action_colid, ctid_colid, segid_colid);

	// project list
	gpos::Ref<CColRefSet> pcrsOutput = pexpr->Prpp()->PcrsRequired();
	gpos::Ref<CDXLNode> pdxlnPrL =
		PdxlnProjList(pexprProjList, pcrsOutput, pdrgpcrDelete);

	gpos::Ref<CDXLNode> pdxlnSplit = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopSplit);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties = GetProperties(pexpr);
	pdxlnSplit->SetProperties(dxl_properties);

	pdxlnSplit->AddChild(pdxlnPrL);
	pdxlnSplit->AddChild(child_dxlnode);

#ifdef GPOS_DEBUG
	pdxlnSplit->GetOperator()->AssertValid(pdxlnSplit,
										   false /* validate_children */);
#endif
	return pdxlnSplit;
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScCmp(gpos::Ref<CExpression> pexprScCmp)
{
	GPOS_ASSERT(nullptr != pexprScCmp);

	// extract components
	gpos::Ref<CExpression> pexprLeft = (*pexprScCmp)[0];
	gpos::Ref<CExpression> pexprRight = (*pexprScCmp)[1];

	// translate children expression
	gpos::Ref<CDXLNode> dxlnode_left = PdxlnScalar(pexprLeft);
	gpos::Ref<CDXLNode> dxlnode_right = PdxlnScalar(pexprRight);

	gpos::Ref<CScalarCmp> popScCmp = CScalarCmp::PopConvert(pexprScCmp->Pop());

	GPOS_ASSERT(nullptr != popScCmp);
	GPOS_ASSERT(nullptr != popScCmp->Pstr());
	GPOS_ASSERT(nullptr != popScCmp->Pstr()->GetBuffer());

	// construct a scalar comparison node
	gpos::Ref<IMDId> mdid = popScCmp->MdIdOp();


	CWStringConst *str_name =
		GPOS_NEW(m_mp) CWStringConst(m_mp, popScCmp->Pstr()->GetBuffer());

	gpos::Ref<CDXLNode> pdxlnCmp = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarComp(m_mp, mdid, str_name));

	// add children
	pdxlnCmp->AddChild(dxlnode_left);
	pdxlnCmp->AddChild(dxlnode_right);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScDistinctCmp(gpos::Ref<CExpression> pexprScDist)
{
	GPOS_ASSERT(nullptr != pexprScDist);

	// extract components
	gpos::Ref<CExpression> pexprLeft = (*pexprScDist)[0];
	gpos::Ref<CExpression> pexprRight = (*pexprScDist)[1];

	// translate children expression
	gpos::Ref<CDXLNode> dxlnode_left = PdxlnScalar(pexprLeft);
	gpos::Ref<CDXLNode> dxlnode_right = PdxlnScalar(pexprRight);

	gpos::Ref<CScalarIsDistinctFrom> popScIDF =
		CScalarIsDistinctFrom::PopConvert(pexprScDist->Pop());

	// construct a scalar distinct comparison node
	gpos::Ref<IMDId> mdid = popScIDF->MdIdOp();


	gpos::Ref<CDXLNode> pdxlnDistCmp = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarDistinctComp(m_mp, mdid));

	// add children
	pdxlnDistCmp->AddChild(dxlnode_left);
	pdxlnDistCmp->AddChild(dxlnode_right);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScOp(gpos::Ref<CExpression> pexprOp)
{
	GPOS_ASSERT(nullptr != pexprOp &&
				((1 == pexprOp->Arity()) || (2 == pexprOp->Arity())));
	gpos::Ref<CScalarOp> pscop = CScalarOp::PopConvert(pexprOp->Pop());

	// construct a scalar opexpr node
	CWStringConst *str_name =
		GPOS_NEW(m_mp) CWStringConst(m_mp, pscop->Pstr()->GetBuffer());

	gpos::Ref<IMDId> mdid_op = pscop->MdIdOp();


	gpos::Ref<IMDId> return_type_mdid = pscop->GetReturnTypeMdId();
	if (nullptr != return_type_mdid)
	{
	}

	gpos::Ref<CDXLNode> pdxlnOpExpr = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarOpExpr(
						   m_mp, mdid_op, return_type_mdid, str_name));

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScBoolExpr(gpos::Ref<CExpression> pexprScBoolOp)
{
	GPOS_ASSERT(nullptr != pexprScBoolOp);
	gpos::Ref<CScalarBoolOp> popScBoolOp =
		CScalarBoolOp::PopConvert(pexprScBoolOp->Pop());
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

	gpos::Ref<CDXLNode> pdxlnBoolExpr = GPOS_NEW(m_mp)
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScId(gpos::Ref<CExpression> pexprIdent)
{
	GPOS_ASSERT(nullptr != pexprIdent);

	gpos::Ref<CScalarIdent> popScId =
		CScalarIdent::PopConvert(pexprIdent->Pop());
	gpos::Ref<CColRef> colref = popScId->Pcr();

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScFuncExpr(gpos::Ref<CExpression> pexprFunc)
{
	GPOS_ASSERT(nullptr != pexprFunc);

	gpos::Ref<CScalarFunc> popScFunc =
		CScalarFunc::PopConvert(pexprFunc->Pop());

	gpos::Ref<IMDId> mdid_func = popScFunc->FuncMdId();


	gpos::Ref<IMDId> mdid_return_type = popScFunc->MdidType();


	const gpos::Ref<IMDFunction> pmdfunc = m_pmda->RetrieveFunc(mdid_func);

	gpos::Ref<CDXLNode> pdxlnFuncExpr = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarFuncExpr(
						   m_mp, mdid_func, mdid_return_type,
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScWindowFuncExpr(
	gpos::Ref<CExpression> pexprWindowFunc)
{
	GPOS_ASSERT(nullptr != pexprWindowFunc);

	gpos::Ref<CScalarWindowFunc> popScWindowFunc =
		CScalarWindowFunc::PopConvert(pexprWindowFunc->Pop());

	gpos::Ref<IMDId> mdid_func = popScWindowFunc->FuncMdId();


	gpos::Ref<IMDId> mdid_return_type = popScWindowFunc->MdidType();


	EdxlWinStage dxl_win_stage = Ews(popScWindowFunc->Ews());
	gpos::Ref<CDXLScalarWindowRef> pdxlopWindowref =
		GPOS_NEW(m_mp) CDXLScalarWindowRef(
			m_mp, mdid_func, mdid_return_type, popScWindowFunc->IsDistinct(),
			popScWindowFunc->IsStarArg(), popScWindowFunc->IsSimpleAgg(),
			dxl_win_stage, 0 /* ulWinspecPosition */
		);

	gpos::Ref<CDXLNode> pdxlnWindowRef =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopWindowref);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScAggref(gpos::Ref<CExpression> pexprAggFunc)
{
	GPOS_ASSERT(nullptr != pexprAggFunc);

	gpos::Ref<CScalarAggFunc> popScAggFunc =
		CScalarAggFunc::PopConvert(pexprAggFunc->Pop());
	gpos::Ref<IMDId> pmdidAggFunc = popScAggFunc->MDId();


	gpos::Ref<IMDId> resolved_rettype = nullptr;
	if (popScAggFunc->FHasAmbiguousReturnType())
	{
		// Agg has an ambiguous return type, use the resolved type instead
		resolved_rettype = popScAggFunc->MdidType();
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

	gpos::Ref<ULongPtrArray> argtypes = popScAggFunc->GetArgTypes();


	gpos::Ref<CDXLScalarAggref> pdxlopAggRef = GPOS_NEW(m_mp) CDXLScalarAggref(
		m_mp, pmdidAggFunc, resolved_rettype, popScAggFunc->IsDistinct(),
		edxlaggrefstage, edxlaggrefkind, argtypes);

	gpos::Ref<CDXLNode> pdxlnAggref =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopAggRef);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScIfStmt(gpos::Ref<CExpression> pexprIfStmt)
{
	GPOS_ASSERT(nullptr != pexprIfStmt);

	GPOS_ASSERT(3 == pexprIfStmt->Arity());

	gpos::Ref<CScalarIf> popScIf = CScalarIf::PopConvert(pexprIfStmt->Pop());

	gpos::Ref<IMDId> mdid_type = popScIf->MdidType();


	gpos::Ref<CDXLNode> pdxlnIfStmt = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarIfStmt(m_mp, mdid_type));
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScSwitch(gpos::Ref<CExpression> pexprSwitch)
{
	GPOS_ASSERT(nullptr != pexprSwitch);
	GPOS_ASSERT(1 < pexprSwitch->Arity());
	gpos::Ref<CScalarSwitch> pop =
		CScalarSwitch::PopConvert(pexprSwitch->Pop());

	gpos::Ref<IMDId> mdid_type = pop->MdidType();


	gpos::Ref<CDXLNode> dxlnode = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarSwitch(m_mp, mdid_type));
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScSwitchCase(gpos::Ref<CExpression> pexprSwitchCase)
{
	GPOS_ASSERT(nullptr != pexprSwitchCase);
	GPOS_ASSERT(2 == pexprSwitchCase->Arity());

	gpos::Ref<CDXLNode> dxlnode = GPOS_NEW(m_mp)
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScNullIf(gpos::Ref<CExpression> pexprScNullIf)
{
	GPOS_ASSERT(nullptr != pexprScNullIf);

	gpos::Ref<CScalarNullIf> pop =
		CScalarNullIf::PopConvert(pexprScNullIf->Pop());

	gpos::Ref<IMDId> mdid = pop->MdIdOp();


	gpos::Ref<IMDId> mdid_type = pop->MdidType();


	gpos::Ref<CDXLScalarNullIf> dxl_op =
		GPOS_NEW(m_mp) CDXLScalarNullIf(m_mp, mdid, mdid_type);
	gpos::Ref<CDXLNode> dxlnode = GPOS_NEW(m_mp) CDXLNode(m_mp, dxl_op);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScCaseTest(gpos::Ref<CExpression> pexprScCaseTest)
{
	GPOS_ASSERT(nullptr != pexprScCaseTest);
	gpos::Ref<CScalarCaseTest> pop =
		CScalarCaseTest::PopConvert(pexprScCaseTest->Pop());

	gpos::Ref<IMDId> mdid_type = pop->MdidType();


	gpos::Ref<CDXLScalarCaseTest> dxl_op =
		GPOS_NEW(m_mp) CDXLScalarCaseTest(m_mp, mdid_type);

	return GPOS_NEW(m_mp) CDXLNode(m_mp, dxl_op);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScNullTest
//
//	@doc:
//		Create a DXL scalar null test node from an optimizer scalar null test expr.
//
//---------------------------------------------------------------------------
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScNullTest(gpos::Ref<CExpression> pexprNullTest)
{
	GPOS_ASSERT(nullptr != pexprNullTest);

	gpos::Ref<CDXLNode> pdxlnNullTest = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarNullTest(m_mp, true /* is_null */));

	// translate child
	GPOS_ASSERT(1 == pexprNullTest->Arity());

	gpos::Ref<CExpression> pexprChild = (*pexprNullTest)[0];
	gpos::Ref<CDXLNode> child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnNullTest->AddChild(child_dxlnode);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScBooleanTest(
	gpos::Ref<CExpression> pexprScBooleanTest)
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

	gpos::Ref<CScalarBooleanTest> popBoolTest =
		CScalarBooleanTest::PopConvert(pexprScBooleanTest->Pop());
	EdxlBooleanTestType edxlbooltest =
		(EdxlBooleanTestType)(rgulBoolTestMapping[popBoolTest->Ebt()][1]);
	gpos::Ref<CDXLNode> pdxlnScBooleanTest = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarBooleanTest(m_mp, edxlbooltest));

	// translate child
	gpos::Ref<CExpression> pexprChild = (*pexprScBooleanTest)[0];
	gpos::Ref<CDXLNode> child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnScBooleanTest->AddChild(child_dxlnode);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScCoalesce(gpos::Ref<CExpression> pexprCoalesce)
{
	GPOS_ASSERT(nullptr != pexprCoalesce);
	GPOS_ASSERT(0 < pexprCoalesce->Arity());
	gpos::Ref<CScalarCoalesce> popScCoalesce =
		CScalarCoalesce::PopConvert(pexprCoalesce->Pop());

	gpos::Ref<IMDId> mdid_type = popScCoalesce->MdidType();


	gpos::Ref<CDXLNode> dxlnode = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarCoalesce(m_mp, mdid_type));
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScMinMax(gpos::Ref<CExpression> pexprMinMax)
{
	GPOS_ASSERT(nullptr != pexprMinMax);
	GPOS_ASSERT(0 < pexprMinMax->Arity());
	gpos::Ref<CScalarMinMax> popScMinMax =
		CScalarMinMax::PopConvert(pexprMinMax->Pop());

	CScalarMinMax::EScalarMinMaxType esmmt = popScMinMax->Esmmt();
	GPOS_ASSERT(CScalarMinMax::EsmmtMin == esmmt ||
				CScalarMinMax::EsmmtMax == esmmt);

	CDXLScalarMinMax::EdxlMinMaxType min_max_type = CDXLScalarMinMax::EmmtMin;
	if (CScalarMinMax::EsmmtMax == esmmt)
	{
		min_max_type = CDXLScalarMinMax::EmmtMax;
	}

	gpos::Ref<IMDId> mdid_type = popScMinMax->MdidType();


	gpos::Ref<CDXLNode> dxlnode = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarMinMax(m_mp, mdid_type, min_max_type));
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
CTranslatorExprToDXL::TranslateScalarChildren(gpos::Ref<CExpression> pexpr,
											  gpos::Ref<CDXLNode> dxlnode)
{
	const ULONG arity = pexpr->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild = (*pexpr)[ul];
		gpos::Ref<CDXLNode> child_dxlnode = PdxlnScalar(pexprChild);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScCast(gpos::Ref<CExpression> pexprCast)
{
	GPOS_ASSERT(nullptr != pexprCast);
	gpos::Ref<CScalarCast> popScCast =
		CScalarCast::PopConvert(pexprCast->Pop());

	gpos::Ref<IMDId> mdid = popScCast->MdidType();


	gpos::Ref<IMDId> mdid_func = popScCast->FuncMdId();


	gpos::Ref<CDXLNode> pdxlnCast = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarCast(m_mp, mdid, mdid_func));

	// translate child
	GPOS_ASSERT(1 == pexprCast->Arity());
	gpos::Ref<CExpression> pexprChild = (*pexprCast)[0];
	gpos::Ref<CDXLNode> child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnCast->AddChild(child_dxlnode);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScCoerceToDomain(gpos::Ref<CExpression> pexprCoerce)
{
	GPOS_ASSERT(nullptr != pexprCoerce);
	gpos::Ref<CScalarCoerceToDomain> popScCoerce =
		CScalarCoerceToDomain::PopConvert(pexprCoerce->Pop());

	gpos::Ref<IMDId> mdid = popScCoerce->MdidType();



	gpos::Ref<CDXLNode> pdxlnCoerce = GPOS_NEW(m_mp) CDXLNode(
		m_mp,
		GPOS_NEW(m_mp) CDXLScalarCoerceToDomain(
			m_mp, mdid, popScCoerce->TypeModifier(),
			(EdxlCoercionForm) popScCoerce
				->Ecf(),  // map Coercion Form directly based on position in enum
			popScCoerce->Location()));

	// translate child
	GPOS_ASSERT(1 == pexprCoerce->Arity());
	gpos::Ref<CExpression> pexprChild = (*pexprCoerce)[0];
	gpos::Ref<CDXLNode> child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnCoerce->AddChild(child_dxlnode);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScCoerceViaIO(gpos::Ref<CExpression> pexprCoerce)
{
	GPOS_ASSERT(nullptr != pexprCoerce);
	gpos::Ref<CScalarCoerceViaIO> popScCerce =
		CScalarCoerceViaIO::PopConvert(pexprCoerce->Pop());

	gpos::Ref<IMDId> mdid = popScCerce->MdidType();



	gpos::Ref<CDXLNode> pdxlnCoerce = GPOS_NEW(m_mp) CDXLNode(
		m_mp,
		GPOS_NEW(m_mp) CDXLScalarCoerceViaIO(
			m_mp, mdid, popScCerce->TypeModifier(),
			(EdxlCoercionForm) popScCerce
				->Ecf(),  // map Coercion Form directly based on position in enum
			popScCerce->Location()));

	// translate child
	GPOS_ASSERT(1 == pexprCoerce->Arity());
	gpos::Ref<CExpression> pexprChild = (*pexprCoerce)[0];
	gpos::Ref<CDXLNode> child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnCoerce->AddChild(child_dxlnode);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScArrayCoerceExpr(
	gpos::Ref<CExpression> pexprArrayCoerceExpr)
{
	GPOS_ASSERT(nullptr != pexprArrayCoerceExpr);
	gpos::Ref<CScalarArrayCoerceExpr> popScArrayCoerceExpr =
		CScalarArrayCoerceExpr::PopConvert(pexprArrayCoerceExpr->Pop());

	gpos::Ref<IMDId> pmdidElemFunc = popScArrayCoerceExpr->PmdidElementFunc();

	gpos::Ref<IMDId> mdid = popScArrayCoerceExpr->MdidType();


	gpos::Ref<CDXLNode> pdxlnArrayCoerceExpr = GPOS_NEW(m_mp) CDXLNode(
		m_mp,
		GPOS_NEW(m_mp) CDXLScalarArrayCoerceExpr(
			m_mp, pmdidElemFunc, mdid, popScArrayCoerceExpr->TypeModifier(),
			popScArrayCoerceExpr->IsExplicit(),
			(EdxlCoercionForm) popScArrayCoerceExpr
				->Ecf(),  // map Coercion Form directly based on position in enum
			popScArrayCoerceExpr->Location()));

	// translate child
	GPOS_ASSERT(1 == pexprArrayCoerceExpr->Arity());
	gpos::Ref<CExpression> pexprChild = (*pexprArrayCoerceExpr)[0];
	gpos::Ref<CDXLNode> child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnArrayCoerceExpr->AddChild(child_dxlnode);

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
gpos::Ref<CDXLWindowFrame>
CTranslatorExprToDXL::GetWindowFrame(gpos::Ref<CWindowFrame> pwf)
{
	GPOS_ASSERT(nullptr != pwf);

	if (CWindowFrame::IsEmpty(pwf))
	{
		// an empty frame is translated as 'no frame'
		return nullptr;
	}

	// mappings for frame info in expression and dxl worlds
	const ULONG rgulSpecMapping[][2] = {
		{CWindowFrame::EfsRows, EdxlfsRow},
		{CWindowFrame::EfsRange, EdxlfsRange},
		{CWindowFrame::EfsGroups, EdxlfsGroups}};

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

	EdxlFrameSpec edxlfs = (EdxlFrameSpec)(rgulSpecMapping[pwf->Efs()][1]);
	EdxlFrameBoundary edxlfbLeading =
		(EdxlFrameBoundary)(rgulBoundaryMapping[pwf->EfbLeading()][1]);
	EdxlFrameBoundary edxlfbTrailing =
		(EdxlFrameBoundary)(rgulBoundaryMapping[pwf->EfbTrailing()][1]);
	EdxlFrameExclusionStrategy frame_exc_strategy =
		(EdxlFrameExclusionStrategy)(
			rgulExclusionStrategyMapping[pwf->Efes()][1]);

	// translate scalar expressions representing leading and trailing frame edges
	gpos::Ref<CDXLNode> pdxlnLeading = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarWindowFrameEdge(
						   m_mp, true /* fLeading */, edxlfbLeading));
	if (nullptr != pwf->PexprLeading())
	{
		pdxlnLeading->AddChild(PdxlnScalar(pwf->PexprLeading()));
	}

	gpos::Ref<CDXLNode> pdxlnTrailing = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarWindowFrameEdge(
						   m_mp, false /* fLeading */, edxlfbTrailing));
	if (nullptr != pwf->PexprTrailing())
	{
		pdxlnTrailing->AddChild(PdxlnScalar(pwf->PexprTrailing()));
	}

	return GPOS_NEW(m_mp) CDXLWindowFrame(
		edxlfs, frame_exc_strategy, pdxlnLeading, pdxlnTrailing,
		pwf->StartInRangeFunc(), pwf->EndInRangeFunc(), pwf->InRangeColl(),
		pwf->InRangeAsc(), pwf->InRangeNullsFirst());
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnWindow
//
//	@doc:
//		Create a DXL window node from physical sequence project expression.
//
//---------------------------------------------------------------------------
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnWindow(
	gpos::Ref<CExpression> pexprSeqPrj, gpos::Ref<CColRefArray> colref_array,
	gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions, BOOL *pfDML)
{
	GPOS_ASSERT(nullptr != pexprSeqPrj);

	gpos::Ref<CPhysicalSequenceProject> popSeqPrj =
		CPhysicalSequenceProject::PopConvert(pexprSeqPrj->Pop());
	gpos::Ref<CDistributionSpec> pds = popSeqPrj->Pds();
	gpos::Ref<ULongPtrArray> colids = GPOS_NEW(m_mp) ULongPtrArray(m_mp);
	gpos::Ref<CExpressionArray> pdrgpexprPartCol = nullptr;
	if (CDistributionSpec::EdtHashed == pds->Edt())
	{
		gpos::Ref<CDistributionSpecHashed> pdshashed =
			CDistributionSpecHashed::PdsConvert(pds);
		pdrgpexprPartCol = pdshashed->Pdrgpexpr();
		const ULONG size = pdrgpexprPartCol->Size();
		for (ULONG ul = 0; ul < size; ul++)
		{
			gpos::Ref<CExpression> pexpr = (*pdrgpexprPartCol)[ul];
			gpos::Ref<CScalarIdent> popScId =
				CScalarIdent::PopConvert(pexpr->Pop());
			colids->Append(GPOS_NEW(m_mp) ULONG(popScId->Pcr()->Id()));
		}
	}

	// translate order specification and window frames into window keys
	CDXLWindowKeyArray *pdrgpdxlwk = GPOS_NEW(m_mp) CDXLWindowKeyArray(m_mp);
	gpos::Ref<COrderSpecArray> pdrgpos = popSeqPrj->Pdrgpos();
	GPOS_ASSERT(nullptr != pdrgpos);
	const ULONG ulOsSize = pdrgpos->Size();
	for (ULONG ul = 0; ul < ulOsSize; ul++)
	{
		gpos::Ref<CDXLWindowKey> pdxlwk = GPOS_NEW(m_mp) CDXLWindowKey();
		gpos::Ref<CDXLNode> sort_col_list_dxlnode =
			GetSortColListDXL((*popSeqPrj->Pdrgpos())[ul]);
		pdxlwk->SetSortColList(sort_col_list_dxlnode);
		pdrgpdxlwk->Append(pdxlwk);
	}

	const ULONG ulFrames = popSeqPrj->Pdrgpwf()->Size();
	for (ULONG ul = 0; ul < ulFrames; ul++)
	{
		gpos::Ref<CDXLWindowFrame> window_frame =
			GetWindowFrame((*popSeqPrj->Pdrgpwf())[ul]);
		if (nullptr != window_frame)
		{
			GPOS_ASSERT(ul <= ulOsSize);
			gpos::Ref<CDXLWindowKey> pdxlwk = (*pdrgpdxlwk)[ul];
			pdxlwk->SetWindowFrame(window_frame);
		}
	}

	// extract physical properties
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GetProperties(pexprSeqPrj);

	// translate relational child
	gpos::Ref<CDXLNode> child_dxlnode = CreateDXLNode(
		(*pexprSeqPrj)[0], nullptr /* colref_array */, pdrgpdsBaseTables,
		pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	GPOS_ASSERT(nullptr != pexprSeqPrj->Prpp());
	gpos::Ref<CColRefSet> pcrsOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsOutput->Include(pexprSeqPrj->Prpp()->PcrsRequired());
	if (nullptr != pdrgpexprPartCol)
	{
		gpos::Ref<CColRefSet> pcrs =
			CUtils::PcrsExtractColumns(m_mp, pdrgpexprPartCol);
		pcrsOutput->Include(pcrs);
	}
	for (ULONG ul = 0; ul < ulOsSize; ul++)
	{
		gpos::Ref<COrderSpec> pos = (*popSeqPrj->Pdrgpos())[ul];
		if (!pos->IsEmpty())
		{
			const gpos::Ref<CColRef> colref = pos->Pcr(ul);
			pcrsOutput->Include(colref);
		}
	}

	// translate project list expression
	gpos::Ref<CDXLNode> pdxlnPrL =
		PdxlnProjList((*pexprSeqPrj)[1], pcrsOutput, colref_array);

	// create an empty one-time filter
	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(nullptr /* pdxlnCond */);

	// construct a Window node
	gpos::Ref<CDXLPhysicalWindow> pdxlopWindow =
		GPOS_NEW(m_mp) CDXLPhysicalWindow(m_mp, colids, pdrgpdxlwk);
	gpos::Ref<CDXLNode> pdxlnWindow =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopWindow);
	pdxlnWindow->SetProperties(dxl_properties);

	// add children
	pdxlnWindow->AddChild(pdxlnPrL);
	pdxlnWindow->AddChild(filter_dxlnode);
	pdxlnWindow->AddChild(child_dxlnode);

#ifdef GPOS_DEBUG
	pdxlopWindow->AssertValid(pdxlnWindow, false /* validate_children */);
#endif



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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnArray(gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	gpos::Ref<CScalarArray> pop = CScalarArray::PopConvert(pexpr->Pop());

	gpos::Ref<IMDId> elem_type_mdid = pop->PmdidElem();


	gpos::Ref<IMDId> array_type_mdid = pop->PmdidArray();


	gpos::Ref<CDXLNode> pdxlnArray = GPOS_NEW(m_mp) CDXLNode(
		m_mp,
		GPOS_NEW(m_mp) CDXLScalarArray(m_mp, elem_type_mdid, array_type_mdid,
									   pop->FMultiDimensional()));

	const ULONG arity = CUtils::UlScalarArrayArity(pexpr);

	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CExpression> pexprChild =
			CUtils::PScalarArrayExprChildAt(m_mp, pexpr, ul);
		gpos::Ref<CDXLNode> child_dxlnode = PdxlnScalar(pexprChild);
		pdxlnArray->AddChild(child_dxlnode);
	}

	return pdxlnArray;
}
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnValuesList(gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<CDXLNode> pdxlnValuesList = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarValuesList(m_mp));

	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		gpos::Ref<CExpression> pexprChild = (*pexpr)[ul];
		gpos::Ref<CDXLNode> child_dxlnode = PdxlnScalar(pexprChild);
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnArrayRef(gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	gpos::Ref<CScalarArrayRef> pop = CScalarArrayRef::PopConvert(pexpr->Pop());

	gpos::Ref<IMDId> elem_type_mdid = pop->PmdidElem();


	gpos::Ref<IMDId> array_type_mdid = pop->PmdidArray();


	gpos::Ref<IMDId> return_type_mdid = pop->MdidType();


	gpos::Ref<CDXLNode> pdxlnArrayref = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarArrayRef(
						   m_mp, elem_type_mdid, pop->TypeModifier(),
						   array_type_mdid, return_type_mdid));

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnArrayRefIndexList(gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	gpos::Ref<CScalarArrayRefIndexList> pop =
		CScalarArrayRefIndexList::PopConvert(pexpr->Pop());

	gpos::Ref<CDXLNode> pdxlnIndexlist = GPOS_NEW(m_mp) CDXLNode(
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnAssertPredicate(gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	gpos::Ref<CDXLNode> pdxlnAssertConstraintList = GPOS_NEW(m_mp)
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnAssertConstraint(gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	gpos::Ref<CScalarAssertConstraint> popAssertConstraint =
		CScalarAssertConstraint::PopConvert(pexpr->Pop());
	CWStringDynamic *pstrErrorMsg = GPOS_NEW(m_mp)
		CWStringDynamic(m_mp, popAssertConstraint->PstrErrorMsg()->GetBuffer());

	gpos::Ref<CDXLNode> pdxlnAssertConstraint = GPOS_NEW(m_mp) CDXLNode(
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnArrayCmp(gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	gpos::Ref<CScalarArrayCmp> pop = CScalarArrayCmp::PopConvert(pexpr->Pop());

	gpos::Ref<IMDId> mdid_op = pop->MdIdOp();


	const CWStringConst *str_opname = pop->Pstr();

	CScalarArrayCmp::EArrCmpType earrcmpt = pop->Earrcmpt();
	GPOS_ASSERT(CScalarArrayCmp::EarrcmpSentinel > earrcmpt);
	EdxlArrayCompType edxlarrcmpt = Edxlarraycomptypeall;
	if (CScalarArrayCmp::EarrcmpAny == earrcmpt)
	{
		edxlarrcmpt = Edxlarraycomptypeany;
	}

	gpos::Ref<CDXLNode> pdxlnArrayCmp = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarArrayComp(
				  m_mp, mdid_op,
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnDMLAction(gpos::Ref<CExpression>
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScConst(gpos::Ref<CExpression> pexprScConst)
{
	GPOS_ASSERT(nullptr != pexprScConst);

	gpos::Ref<CScalarConst> popScConst =
		CScalarConst::PopConvert(pexprScConst->Pop());

	gpos::Ref<IDatum> datum = popScConst->GetDatum();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	const gpos::Ref<IMDType> pmdtype = md_accessor->RetrieveType(datum->MDId());

	gpos::Ref<CDXLNode> dxlnode =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pmdtype->GetDXLOpScConst(m_mp, datum));

	return dxlnode;
}

gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnScSortGroupClause(
	gpos::Ref<CExpression> pexprScSortGroupClause)
{
	GPOS_ASSERT(nullptr != pexprScSortGroupClause);

	gpos::Ref<CScalarSortGroupClause> pop =
		CScalarSortGroupClause::PopConvert(pexprScSortGroupClause->Pop());

	gpos::Ref<CDXLNode> dxlnode = GPOS_NEW(m_mp)
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnFilter(gpos::Ref<CDXLNode> pdxlnCond)
{
	gpos::Ref<CDXLNode> filter_dxlnode =
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
gpos::Ref<CDXLTableDescr>
CTranslatorExprToDXL::MakeDXLTableDescr(
	const gpos::Ref<CTableDescriptor> ptabdesc,
	const gpos::Ref<CColRefArray> pdrgpcrOutput,
	const gpos::Ref<CReqdPropPlan> reqd_prop_plan GPOS_ASSERTS_ONLY)
{
	GPOS_ASSERT(nullptr != ptabdesc);
	GPOS_ASSERT_IMP(nullptr != pdrgpcrOutput,
					ptabdesc->ColumnCount() == pdrgpcrOutput->Size());

	// get tbl name
	CMDName *pmdnameTbl = GPOS_NEW(m_mp) CMDName(m_mp, ptabdesc->Name().Pstr());

	gpos::Ref<CMDIdGPDB> mdid = CMDIdGPDB::CastMdid(ptabdesc->MDId());


	gpos::Ref<CDXLTableDescr> table_descr = GPOS_NEW(m_mp) CDXLTableDescr(
		m_mp, mdid, pmdnameTbl, ptabdesc->GetExecuteAsUserId(),
		ptabdesc->LockMode(), ptabdesc->GetAssignedQueryIdForTargetRel());

	const ULONG ulColumns = ptabdesc->ColumnCount();
	// translate col descriptors
	for (ULONG ul = 0; ul < ulColumns; ul++)
	{
		const gpos::Ref<CColumnDescriptor> pcd = ptabdesc->Pcoldesc(ul);

		GPOS_ASSERT(nullptr != pcd);

		// output col ref for the current col descrs
		gpos::Ref<CColRef> colref = nullptr;
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
		gpos::Ref<CMDIdGPDB> pmdidColType =
			CMDIdGPDB::CastMdid(colref->RetrieveType()->MDId());


		gpos::Ref<CDXLColDescr> pdxlcd = GPOS_NEW(m_mp) CDXLColDescr(
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
gpos::Ref<CDXLPhysicalProperties>
CTranslatorExprToDXL::GetProperties(const gpos::Ref<CExpression> pexpr)
{
	// extract out rows from statistics object
	CWStringDynamic *rows_out_str = GPOS_NEW(m_mp) CWStringDynamic(m_mp);
	const gpos::Ref<IStatistics> stats = pexpr->Pstats();
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
	gpos::Ref<CReqdPropPlan> prpp = pexpr->Prpp();
	gpos::Ref<CColRefSet> pcrs = prpp->PcrsRequired();
	gpos::Ref<ULongPtrArray> colids = GPOS_NEW(m_mp) ULongPtrArray(m_mp);
	pcrs->ExtractColIds(m_mp, colids);
	CWStringDynamic *width_str = GPOS_NEW(m_mp) CWStringDynamic(m_mp);

	if (nullptr != stats)
	{
		width = stats->Width(colids);
	}

	width_str->AppendFormat(GPOS_WSZ_LIT("%lld"), (LINT) width.Get());

	// get the cost from expression node
	CWStringDynamic str(m_mp);
	COstreamString oss(&str);
	oss << pexpr->Cost();

	CWStringDynamic *pstrStartupcost =
		GPOS_NEW(m_mp) CWStringDynamic(m_mp, GPOS_WSZ_LIT("0"));
	CWStringDynamic *pstrTotalcost =
		GPOS_NEW(m_mp) CWStringDynamic(m_mp, str.GetBuffer());

	gpos::Ref<CDXLOperatorCost> cost = GPOS_NEW(m_mp) CDXLOperatorCost(
		pstrStartupcost, pstrTotalcost, rows_out_str, width_str);
	gpos::Ref<CDXLPhysicalProperties> dxl_properties =
		GPOS_NEW(m_mp) CDXLPhysicalProperties(cost);

	return dxl_properties;
}


gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnProjListForChildPart(
	const gpos::Ref<ColRefToUlongMap> root_col_mapping,
	const gpos::Ref<CColRefArray> part_colrefs,
	const gpos::Ref<CColRefSet> reqd_colrefs,
	const gpos::Ref<CColRefArray> colref_array)
{
	gpos::Ref<CColRefArray> mapped_colrefs = GPOS_NEW(m_mp) CColRefArray(m_mp);
	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(m_mp) CColRefSet(m_mp);
	// project columns in order if explicitly asked
	if (nullptr != colref_array)
	{
		for (ULONG i = 0; i < colref_array->Size(); ++i)
		{
			gpos::Ref<CColRef> cr = (*colref_array)[i];
			gpos::Ref<UlongRef> idx = root_col_mapping->Find(cr);
			GPOS_ASSERT(nullptr != idx);
			gpos::Ref<CColRef> mapped_cr = (*part_colrefs)[idx->value()];
			mapped_colrefs->Append(mapped_cr);
			pcrs->Include(mapped_cr);
		}
	}

	CColRefSetIter crsi(*reqd_colrefs);
	while (crsi.Advance())
	{
		gpos::Ref<CColRef> cr = crsi.Pcr();
		gpos::Ref<UlongRef> idx = root_col_mapping->Find(cr);
		GPOS_ASSERT(nullptr != idx);
		gpos::Ref<CColRef> mapped_cr = (*part_colrefs)[idx->value()];
		if (!pcrs->FMember(mapped_cr))
		{
			mapped_colrefs->Append(mapped_cr);
		}
	}

	gpos::Ref<CColRefSet> empty_set = GPOS_NEW(m_mp) CColRefSet(m_mp);
	gpos::Ref<CDXLNode> pdxlnPrL = PdxlnProjList(empty_set, mapped_colrefs);



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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnCondForChildPart(
	const gpos::Ref<ColRefToUlongMap> root_col_mapping,
	const gpos::Ref<CColRefArray> part_colrefs,
	const gpos::Ref<CColRefArray> root_colrefs, gpos::Ref<CExpression> pred)
{
	GPOS_ASSERT(part_colrefs->Size() == root_colrefs->Size());

	// Set up a temporary mapping from root colrefs to partition colref ids
	m_phmcrulPartColId = GPOS_NEW(m_mp) ColRefToUlongMap(m_mp);

	for (ULONG i = 0; i < root_colrefs->Size(); ++i)
	{
		gpos::Ref<CColRef> cr = (*root_colrefs)[i];
		gpos::Ref<UlongRef> idx = root_col_mapping->Find(cr);
		GPOS_ASSERT(nullptr != idx);

		gpos::Ref<CColRef> mapped_cr = (*part_colrefs)[idx->value()];
		m_phmcrulPartColId->Insert(cr,
								   GPOS_NEW(m_mp) UlongRef(mapped_cr->Id()));
	}

	gpos::Ref<CDXLNode> pdxlnCond = nullptr;
	if (nullptr != pred)
	{
		pdxlnCond = PdxlnScalar(pred);
	}

	// clean up the temporary mapping

	m_phmcrulPartColId = nullptr;

	return pdxlnCond;
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnProjList(const gpos::Ref<CColRefSet> pcrsOutput,
									gpos::Ref<CColRefArray> colref_array)
{
	GPOS_ASSERT(nullptr != pcrsOutput);

	gpos::Ref<CDXLScalarProjList> pdxlopPrL =
		GPOS_NEW(m_mp) CDXLScalarProjList(m_mp);
	gpos::Ref<CDXLNode> pdxlnPrL = GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopPrL);

	if (nullptr != colref_array)
	{
		gpos::Ref<CColRefSet> pcrs = GPOS_NEW(m_mp) CColRefSet(m_mp);

		for (ULONG ul = 0; ul < colref_array->Size(); ul++)
		{
			gpos::Ref<CColRef> colref = (*colref_array)[ul];

			gpos::Ref<CDXLNode> pdxlnPrEl =
				CTranslatorExprToDXLUtils::PdxlnProjElem(m_mp, m_phmcrdxln,
														 colref);
			pdxlnPrL->AddChild(pdxlnPrEl);
			pcrs->Include(colref);
		}

		// add the remaining required columns
		CColRefSetIter crsi(*pcrsOutput);
		while (crsi.Advance())
		{
			gpos::Ref<CColRef> colref = crsi.Pcr();

			if (!pcrs->FMember(colref))
			{
				gpos::Ref<CDXLNode> pdxlnPrEl =
					CTranslatorExprToDXLUtils::PdxlnProjElem(m_mp, m_phmcrdxln,
															 colref);
				pdxlnPrL->AddChild(pdxlnPrEl);
				pcrs->Include(colref);
			}
		}
	}
	else
	{
		// no order specified
		CColRefSetIter crsi(*pcrsOutput);
		while (crsi.Advance())
		{
			gpos::Ref<CColRef> colref = crsi.Pcr();
			gpos::Ref<CDXLNode> pdxlnPrEl =
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnProjList(const gpos::Ref<CExpression> pexprProjList,
									const gpos::Ref<CColRefSet> pcrsRequired,
									gpos::Ref<CColRefArray> colref_array)
{
	if (nullptr == colref_array)
	{
		// no order specified
		return PdxlnProjList(pexprProjList, pcrsRequired);
	}

	// translate computed column expressions into DXL and index them on their col ids
	gpos::Ref<gpos::UnorderedMap<ULONG, gpos::Ref<CDXLNode>>>
		phmComputedColumns =
			GPOS_NEW(m_mp) gpos::UnorderedMap<ULONG, gpos::Ref<CDXLNode>>(m_mp);

	for (ULONG ul = 0; nullptr != pexprProjList && ul < pexprProjList->Arity();
		 ul++)
	{
		gpos::Ref<CExpression> pexprProjElem = (*pexprProjList)[ul];

		// translate proj elem
		gpos::Ref<CDXLNode> pdxlnProjElem = PdxlnProjElem(pexprProjElem);

		const gpos::Ref<CScalarProjectElement> popScPrEl =
			CScalarProjectElement::PopConvert(pexprProjElem->Pop());

		BOOL fInserted GPOS_ASSERTS_ONLY =
			phmComputedColumns->Insert(popScPrEl->Pcr()->Id(), pdxlnProjElem);

		GPOS_ASSERT(fInserted);
	}

	// add required columns to the project list
	gpos::Ref<CColRefArray> pdrgpcrCopy = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrCopy->AppendArray(colref_array);
	gpos::Ref<CColRefSet> pcrsOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsOutput->Include(colref_array);
	CColRefSetIter crsi(*pcrsRequired);
	while (crsi.Advance())
	{
		gpos::Ref<CColRef> colref = crsi.Pcr();
		if (!pcrsOutput->FMember(colref))
		{
			pdrgpcrCopy->Append(colref);
		}
	}

	// translate project list according to the specified order
	gpos::Ref<CDXLScalarProjList> pdxlopPrL =
		GPOS_NEW(m_mp) CDXLScalarProjList(m_mp);
	gpos::Ref<CDXLNode> proj_list_dxlnode =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopPrL);

	const ULONG num_cols = pdrgpcrCopy->Size();
	for (ULONG ul = 0; ul < num_cols; ul++)
	{
		gpos::Ref<CColRef> colref = (*pdrgpcrCopy)[ul];
		ULONG ulKey = colref->Id();
		gpos::Ref<CDXLNode> pdxlnProjElem = phmComputedColumns->Find(ulKey);

		if (nullptr == pdxlnProjElem)
		{
			// not a computed column
			pdxlnProjElem = CTranslatorExprToDXLUtils::PdxlnProjElem(
				m_mp, m_phmcrdxln, colref);
		}
		else
		{
		}

		proj_list_dxlnode->AddChild(pdxlnProjElem);
	}

	// cleanup



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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnProjList(const gpos::Ref<CExpression> pexprProjList,
									const gpos::Ref<CColRefSet> pcrsRequired)
{
	gpos::Ref<CDXLScalarProjList> pdxlopPrL =
		GPOS_NEW(m_mp) CDXLScalarProjList(m_mp);
	gpos::Ref<CDXLNode> proj_list_dxlnode =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopPrL);

	// create a copy of the required output columns
	gpos::Ref<CColRefSet> pcrsOutput =
		GPOS_NEW(m_mp) CColRefSet(m_mp, *pcrsRequired);

	if (nullptr != pexprProjList)
	{
		// translate defined columns from project list
		for (ULONG ul = 0; ul < pexprProjList->Arity(); ul++)
		{
			gpos::Ref<CExpression> pexprProjElem = (*pexprProjList)[ul];

			// translate proj elem
			gpos::Ref<CDXLNode> pdxlnProjElem = PdxlnProjElem(pexprProjElem);
			proj_list_dxlnode->AddChild(pdxlnProjElem);

			// exclude proj elem col ref from the output column set as it has been
			// processed already
			const gpos::Ref<CScalarProjectElement> popScPrEl =
				CScalarProjectElement::PopConvert(pexprProjElem->Pop());
			pcrsOutput->Exclude(popScPrEl->Pcr());
		}
	}

	// translate columns which remained after processing the project list: those
	// are columns passed from the level below
	CColRefSetIter crsi(*pcrsOutput);
	while (crsi.Advance())
	{
		gpos::Ref<CColRef> colref = crsi.Pcr();
		gpos::Ref<CDXLNode> pdxlnPrEl =
			CTranslatorExprToDXLUtils::PdxlnProjElem(m_mp, m_phmcrdxln, colref);
		proj_list_dxlnode->AddChild(pdxlnPrEl);
	}

	// cleanup


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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnProjListFromConstTableGet(
	gpos::Ref<CColRefArray> pdrgpcrReqOutput,
	gpos::Ref<CColRefArray> pdrgpcrCTGOutput,
	gpos::Ref<IDatumArray> pdrgpdatumValues)
{
	GPOS_ASSERT(nullptr != pdrgpcrCTGOutput);
	GPOS_ASSERT(nullptr != pdrgpdatumValues);
	GPOS_ASSERT(pdrgpcrCTGOutput->Size() == pdrgpdatumValues->Size());

	gpos::Ref<CDXLNode> proj_list_dxlnode = nullptr;
	gpos::Ref<CColRefSet> pcrsOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsOutput->Include(pdrgpcrCTGOutput);

	if (nullptr != pdrgpcrReqOutput)
	{
		const ULONG arity = pdrgpcrReqOutput->Size();
		gpos::Ref<IDatumArray> pdrgpdatumOrdered =
			GPOS_NEW(m_mp) IDatumArray(m_mp);

		for (ULONG ul = 0; ul < arity; ul++)
		{
			gpos::Ref<CColRef> colref = (*pdrgpcrReqOutput)[ul];
			ULONG ulPos = UlPosInArray(colref, pdrgpcrCTGOutput);
			GPOS_ASSERT(ulPos < pdrgpcrCTGOutput->Size());
			gpos::Ref<IDatum> datum = (*pdrgpdatumValues)[ulPos];

			pdrgpdatumOrdered->Append(datum);
			pcrsOutput->Exclude(colref);
		}

		proj_list_dxlnode = PdxlnProjListFromConstTableGet(
			nullptr, pdrgpcrReqOutput, pdrgpdatumOrdered);
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
		gpos::Ref<CColRef> colref = crsi.Pcr();
		ULONG ulPos = UlPosInArray(colref, pdrgpcrCTGOutput);
		GPOS_ASSERT(ulPos < pdrgpcrCTGOutput->Size());
		gpos::Ref<IDatum> datum = (*pdrgpdatumValues)[ulPos];
		gpos::Ref<CDXLScalarConstValue> pdxlopConstValue =
			colref->RetrieveType()->GetDXLOpScConst(m_mp, datum);
		gpos::Ref<CDXLNode> pdxlnPrEl = PdxlnProjElem(
			colref, GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopConstValue));
		proj_list_dxlnode->AddChild(pdxlnPrEl);
	}

	// cleanup


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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnProjElem(const gpos::Ref<CColRef> colref,
									gpos::Ref<CDXLNode> pdxlnValue)
{
	GPOS_ASSERT(nullptr != colref);

	CMDName *mdname = GPOS_NEW(m_mp) CMDName(m_mp, colref->Name().Pstr());
	gpos::Ref<CDXLNode> pdxlnPrEl = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarProjElem(m_mp, colref->Id(), mdname));

	// attach scalar id expression to proj elem
	pdxlnPrEl->AddChild(pdxlnValue);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnProjElem(const gpos::Ref<CExpression> pexprProjElem)
{
	GPOS_ASSERT(nullptr != pexprProjElem && 1 == pexprProjElem->Arity());

	gpos::Ref<CScalarProjectElement> popScPrEl =
		CScalarProjectElement::PopConvert(pexprProjElem->Pop());

	gpos::Ref<CColRef> colref = popScPrEl->Pcr();

	CMDName *mdname = GPOS_NEW(m_mp) CMDName(m_mp, colref->Name().Pstr());
	gpos::Ref<CDXLNode> pdxlnPrEl = GPOS_NEW(m_mp) CDXLNode(
		m_mp, GPOS_NEW(m_mp) CDXLScalarProjElem(m_mp, colref->Id(), mdname));

	gpos::Ref<CExpression> pexprChild = (*pexprProjElem)[0];
	gpos::Ref<CDXLNode> child_dxlnode = PdxlnScalar(pexprChild);

	pdxlnPrEl->AddChild(child_dxlnode);

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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::GetSortColListDXL(const gpos::Ref<COrderSpec> pos)
{
	GPOS_ASSERT(nullptr != pos);

	gpos::Ref<CDXLNode> sort_col_list_dxlnode = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarSortColList(m_mp));

	for (ULONG ul = 0; ul < pos->UlSortColumns(); ul++)
	{
		// get sort column components
		gpos::Ref<IMDId> sort_op_id = pos->GetMdIdSortOp(ul);


		const gpos::Ref<CColRef> colref = pos->Pcr(ul);

		COrderSpec::ENullTreatment ent = pos->Ent(ul);
		GPOS_ASSERT(COrderSpec::EntFirst == ent || COrderSpec::EntLast == ent ||
					COrderSpec::EntAuto == ent);

		// get sort operator name
		const gpos::Ref<IMDScalarOp> md_scalar_op =
			m_pmda->RetrieveScOp(sort_op_id);

		CWStringConst *sort_op_name = GPOS_NEW(m_mp) CWStringConst(
			m_mp, md_scalar_op->Mdname().GetMDName()->GetBuffer());

		BOOL fSortNullsFirst = false;
		if (COrderSpec::EntFirst == ent)
		{
			fSortNullsFirst = true;
		}

		gpos::Ref<CDXLScalarSortCol> pdxlopSortCol =
			GPOS_NEW(m_mp) CDXLScalarSortCol(m_mp, colref->Id(), sort_op_id,
											 sort_op_name, fSortNullsFirst);

		gpos::Ref<CDXLNode> pdxlnSortCol =
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnHashExprList(
	const gpos::Ref<CExpressionArray> pdrgpexpr,
	const gpos::Ref<IMdIdArray> opfamilies)
{
	GPOS_ASSERT(nullptr != pdrgpexpr);
	GPOS_ASSERT_IMP(GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution),
					nullptr != opfamilies);

	gpos::Ref<CDXLNode> hash_expr_list = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarHashExprList(m_mp));

	for (ULONG ul = 0; ul < pdrgpexpr->Size(); ul++)
	{
		gpos::Ref<CExpression> pexpr = (*pdrgpexpr)[ul];

		gpos::Ref<IMDId> opfamily = nullptr;
		if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution))
		{
			opfamily = (*opfamilies)[ul];
		}

		// constrct a hash expr node for the col ref
		gpos::Ref<CDXLNode> pdxlnHashExpr = GPOS_NEW(m_mp)
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::GetSortColListDXL(gpos::Ref<CExpression> pexprMotion)
{
	gpos::Ref<CDXLNode> sort_col_list_dxlnode = nullptr;
	if (COperator::EopPhysicalMotionGather == pexprMotion->Pop()->Eopid())
	{
		// construct a sorting column list node
		gpos::Ref<CPhysicalMotionGather> popGather =
			CPhysicalMotionGather::PopConvert(pexprMotion->Pop());
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
gpos::Ref<IntPtrArray>
CTranslatorExprToDXL::GetOutputSegIdsArray(gpos::Ref<CExpression> pexprMotion)
{
	gpos::Ref<IntPtrArray> pdrgpi = nullptr;

	gpos::Ref<COperator> pop = pexprMotion->Pop();

	switch (pop->Eopid())
	{
		case COperator::EopPhysicalMotionGather:
		{
			gpos::Ref<CPhysicalMotionGather> popGather =
				CPhysicalMotionGather::PopConvert(pop);

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
gpos::Ref<IntPtrArray>
CTranslatorExprToDXL::GetInputSegIdsArray(gpos::Ref<CExpression> pexprMotion)
{
	GPOS_ASSERT(1 == pexprMotion->Arity());

	// derive the distribution of child expression
	gpos::Ref<CExpression> pexprChild = (*pexprMotion)[0];
	gpos::Ref<CDrvdPropPlan> pdpplan =
		CDrvdPropPlan::Pdpplan(pexprChild->PdpDerive());
	gpos::Ref<CDistributionSpec> pds = pdpplan->Pds();

	if (CDistributionSpec::EdtSingleton == pds->Edt() ||
		CDistributionSpec::EdtStrictSingleton == pds->Edt())
	{
		gpos::Ref<IntPtrArray> pdrgpi = GPOS_NEW(m_mp) IntPtrArray(m_mp);
		INT iSegmentId = m_iMasterId;
		gpos::Ref<CDistributionSpecSingleton> pdss =
			CDistributionSpecSingleton::PdssConvert(pds);
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
		gpos::Ref<IntPtrArray> pdrgpi = GPOS_NEW(m_mp) IntPtrArray(m_mp);
		INT iSegmentId = *((*m_pdrgpiSegments)[0]);
		pdrgpi->Append(GPOS_NEW(m_mp) INT(iSegmentId));
		return pdrgpi;
	}


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
CTranslatorExprToDXL::UlPosInArray(const gpos::Ref<CColRef> colref,
								   const gpos::Ref<CColRefArray> colref_array)
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
gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnResult(
	gpos::Ref<CDXLPhysicalProperties> dxl_properties,
	gpos::Ref<CDXLNode> pdxlnPrL, gpos::Ref<CDXLNode> child_dxlnode)
{
	gpos::Ref<CDXLNode> pdxlnMaterialize = nullptr;
	gpos::Ref<CDXLNode> pdxlnScalarOneTimeFilter = GPOS_NEW(m_mp)
		CDXLNode(m_mp, GPOS_NEW(m_mp) CDXLScalarOneTimeFilter(m_mp));

	// If the result project list contains a subplan with a Broadcast motion,
	// along with other projections from the result's child node with a motion as well,
	// it may result in a deadlock. In such cases, add a materialize node.
	if (FNeedsMaterializeUnderResult(pdxlnPrL, child_dxlnode))
	{
		pdxlnMaterialize = PdxlnMaterialize(child_dxlnode);
	}

	return CTranslatorExprToDXLUtils::PdxlnResult(
		m_mp, dxl_properties, pdxlnPrL, PdxlnFilter(nullptr),
		pdxlnScalarOneTimeFilter,
		pdxlnMaterialize ? pdxlnMaterialize : child_dxlnode);
}

gpos::Ref<CDXLNode>
CTranslatorExprToDXL::PdxlnMaterialize(
	gpos::Ref<CDXLNode> dxlnode	 // node that needs to be materialized
)
{
	GPOS_ASSERT(nullptr != dxlnode);
	GPOS_ASSERT(nullptr != dxlnode->GetProperties());

	gpos::Ref<CDXLPhysicalMaterialize> pdxlopMaterialize =
		GPOS_NEW(m_mp) CDXLPhysicalMaterialize(m_mp, true /* fEager */);
	gpos::Ref<CDXLNode> pdxlnMaterialize =
		GPOS_NEW(m_mp) CDXLNode(m_mp, pdxlopMaterialize);
	gpos::Ref<CDXLPhysicalProperties> pdxlpropChild =
		CDXLPhysicalProperties::PdxlpropConvert(dxlnode->GetProperties());

	pdxlnMaterialize->SetProperties(pdxlpropChild);

	// construct an empty filter node
	gpos::Ref<CDXLNode> filter_dxlnode = PdxlnFilter(nullptr /* pdxlnCond */);

	gpos::Ref<CDXLNode> pdxlnProjListChild = (*dxlnode)[0];
	gpos::Ref<CDXLNode> proj_list_dxlnode =
		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
			m_mp, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	// add children
	pdxlnMaterialize->AddChild(proj_list_dxlnode);
	pdxlnMaterialize->AddChild(filter_dxlnode);
	pdxlnMaterialize->AddChild(dxlnode);
	return pdxlnMaterialize;
}

BOOL
CTranslatorExprToDXL::FNeedsMaterializeUnderResult(
	gpos::Ref<CDXLNode> proj_list_dxlnode, gpos::Ref<CDXLNode> child_dxlnode)
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
		gpos::Ref<CBitSet> pbsScIdentColIds = GPOS_NEW(m_mp) CBitSet(m_mp);

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
	}
	return fMotionHazard;
}

// EOF
