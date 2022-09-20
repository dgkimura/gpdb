//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CTranslatorExprToDXL.h
//
//	@doc:
//		Class providing methods for translating optimizer Expression trees
//		into DXL Tree.
//---------------------------------------------------------------------------

#ifndef GPOPT_CTranslatorExprToDXL_H
#define GPOPT_CTranslatorExprToDXL_H


#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/memory/CMemoryPool.h"

#include "gpopt/base/CDrvdPropPlan.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CLogicalDML.h"
#include "gpopt/operators/CPhysicalHashJoin.h"
#include "gpopt/operators/CPhysicalScan.h"
#include "gpopt/operators/CScalarArrayRefIndexList.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "gpopt/operators/CScalarWindowFunc.h"
#include "naucrates/dxl/operators/CDXLColRef.h"
#include "naucrates/dxl/operators/CDXLIndexDescr.h"
#include "naucrates/dxl/operators/CDXLPhysicalAgg.h"
#include "naucrates/dxl/operators/CDXLPhysicalDML.h"
#include "naucrates/dxl/operators/CDXLPhysicalMotion.h"
#include "naucrates/dxl/operators/CDXLScalarArrayRefIndexList.h"
#include "naucrates/dxl/operators/CDXLScalarBoolExpr.h"
#include "naucrates/dxl/operators/CDXLScalarSubPlan.h"
#include "naucrates/dxl/operators/CDXLScalarWindowRef.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"
#include "naucrates/dxl/operators/CDXLWindowFrame.h"

// fwd declaration
namespace gpnaucrates
{
class IStatistics;
}

// forward declarations
namespace gpdxl
{
class CDXLPhysicalProperties;
}  // namespace gpdxl

namespace gpopt
{
using namespace gpmd;
using namespace gpdxl;
using namespace gpnaucrates;

// hash map mapping CColRef -> CDXLNode
using ColRefToDXLNodeMap =
	CHashMap<CColRef, CDXLNode, CColRef::HashValue, CColRef::Equals,
			 CleanupNULL<CColRef>, CleanupRelease<CDXLNode>>;

//---------------------------------------------------------------------------
//	@class:
//		CTranslatorExprToDXL
//
//	@doc:
//		Class providing methods for translating optimizer Expression trees
//		into DXL Trees.
//
//---------------------------------------------------------------------------
class CTranslatorExprToDXL
{
private:
	// memory pool
	CMemoryPool *m_mp;

	// metadata accessor
	CMDAccessor *m_pmda;

	// mappings CColRef -> CDXLNode used to lookup subplans
	gpos::owner<ColRefToDXLNodeMap *> m_phmcrdxln;

	// mappings CColRef -> CDXLNode used to for index predicates with outer references
	gpos::owner<ColRefToDXLNodeMap *> m_phmcrdxlnIndexLookup;

	// mappings CColRef -> ColId for translating filters for child partitions
	// (see PdxlnFilterForChildPart())
	gpos::owner<ColRefToUlongMap *> m_phmcrulPartColId = nullptr;

	// derived plan properties of the translated expression
	gpos::owner<CDrvdPropPlan *> m_pdpplan;

	// a copy of the pointer to column factory, obtained at construction time
	CColumnFactory *m_pcf;

	// segment ids on target system
	gpos::owner<IntPtrArray *> m_pdrgpiSegments;

	// id of master node
	INT m_iMasterId;

	// private copy ctor
	CTranslatorExprToDXL(const CTranslatorExprToDXL &);

	static EdxlBoolExprType Edxlbooltype(
		const CScalarBoolOp::EBoolOperator eboolop);

	// return the EdxlDmlType for a given DML op type
	static EdxlDmlType Edxldmloptype(const CLogicalDML::EDMLOperator edmlop);

	// return outer refs in correlated join inner child
	static CColRefSet *PcrsOuterRefsForCorrelatedNLJoin(
		gpos::pointer<CExpression *> pexpr);

	// functions translating different optimizer expressions into their
	// DXL counterparts

	gpos::owner<CDXLNode *> PdxlnTblScan(
		gpos::pointer<CExpression *> pexprTblScan,
		gpos::pointer<CColRefSet *> pcrsOutput,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
		gpos::pointer<CExpression *> pexprScalarCond,
		gpos::owner<CDXLPhysicalProperties *> dxl_properties);

	// create a (dynamic) index scan node after inlining the given scalar condition, if needed
	gpos::owner<CDXLNode *> PdxlnIndexScanWithInlinedCondition(
		CExpression *pexprIndexScan, CExpression *pexprScalarCond,
		gpos::owner<CDXLPhysicalProperties *> dxl_properties,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables);

	// translate index scan based on passed properties
	gpos::owner<CDXLNode *> PdxlnIndexScan(
		gpos::pointer<CExpression *> pexprIndexScan,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::owner<CDXLPhysicalProperties *> dxl_properties,
		gpos::pointer<CReqdPropPlan *> prpp);

	// translate index scan
	gpos::owner<CDXLNode *> PdxlnIndexScan(
		CExpression *pexprIndexScan, gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnIndexOnlyScan(
		CExpression *pexprIndexScan, gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnIndexOnlyScan(
		gpos::pointer<CExpression *> pexprIndexScan,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::owner<CDXLPhysicalProperties *> dxl_properties,
		gpos::pointer<CReqdPropPlan *> prpp);

	// translate a bitmap index probe expression to DXL
	gpos::owner<CDXLNode *> PdxlnBitmapIndexProbe(
		gpos::pointer<CExpression *> pexprBitmapIndexProbe);

	// translate a bitmap bool op expression to DXL
	gpos::owner<CDXLNode *> PdxlnBitmapBoolOp(
		gpos::pointer<CExpression *> pexprBitmapBoolOp);

	// translate a bitmap table scan expression to DXL
	gpos::owner<CDXLNode *> PdxlnBitmapTableScan(
		CExpression *pexprBitmapTableScan,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a bitmap table scan expression to DXL
	gpos::owner<CDXLNode *> PdxlnBitmapTableScan(
		CExpression *pexprBitmapTableScan,
		gpos::pointer<CColRefSet *> pcrsOutput,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
		gpos::pointer<CExpression *> pexprScalar,
		gpos::owner<CDXLPhysicalProperties *> dxl_properties);

	// create a DXL result node from an optimizer filter node
	gpos::owner<CDXLNode *> PdxlnResultFromFilter(
		gpos::pointer<CExpression *> pexprFilter,
		gpos::pointer<CColRefArray *> colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnResult(
		CExpression *pexprFilter, CColRefArray *colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	// create a DXL result node for the given project list
	CDXLNode *PdxlnResult(CDXLNode *proj_list_dxlnode, CExpression *pexprFilter,
						  CColRefArray *colref_array,
						  CDistributionSpecArray *pdrgpdsBaseTables,
						  ULONG *pulNonGatherMotions, BOOL *pfDML);

	// given a DXL plan tree child_dxlnode which represents the physical plan pexprRelational, construct a DXL
	// Result node that filters on top of it using the scalar condition pdxlnScalar
	gpos::owner<CDXLNode *> PdxlnAddScalarFilterOnRelationalChild(
		gpos::owner<CDXLNode *> pdxlnRelationalChild,
		gpos::owner<CDXLNode *> pdxlnScalarChild,
		gpos::owner<CDXLPhysicalProperties *> dxl_properties,
		gpos::pointer<CColRefSet *> pcrsOutput,
		gpos::pointer<CColRefArray *> pdrgpcrOrder);

	gpos::owner<CDXLNode *> PdxlnFromFilter(
		CExpression *pexprFilter, CColRefArray *colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML, CDXLPhysicalProperties *dxl_properties);

	gpos::owner<CDXLNode *> PdxlnResult(
		CExpression *pexprRelational, CColRefArray *colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML, gpos::owner<CDXLNode *> pdxlnScalar,
		gpos::owner<CDXLPhysicalProperties *> dxl_properties);

	gpos::owner<CDXLNode *> PdxlnResult(
		CExpression *pexprRelational, CColRefArray *colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML, gpos::owner<CDXLNode *> pdxlnScalar);

	gpos::owner<CDXLNode *> PdxlnComputeScalar(
		gpos::pointer<CExpression *> pexprComputeScalar,
		gpos::pointer<CColRefArray *> colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnAggregate(
		gpos::pointer<CExpression *> pexprHashAgg,
		gpos::pointer<CColRefArray *> colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnAggregateDedup(
		gpos::pointer<CExpression *> pexprAgg,
		gpos::pointer<CColRefArray *> colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnAggregate(
		gpos::pointer<CExpression *> pexprAgg,
		gpos::pointer<CColRefArray *> colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML, EdxlAggStrategy dxl_agg_strategy,
		gpos::pointer<const CColRefArray *> pdrgpcrGroupingCols,
		gpos::pointer<CColRefSet *> pcrsKeys);

	gpos::owner<CDXLNode *> PdxlnSort(gpos::pointer<CExpression *> pexprSort,
									  CColRefArray *colref_array,
									  CDistributionSpecArray *pdrgpdsBaseTables,
									  ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnLimit(
		gpos::pointer<CExpression *> pexprLimit, CColRefArray *colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnWindow(
		gpos::pointer<CExpression *> pexprSeqPrj,
		gpos::pointer<CColRefArray *> colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnNLJoin(
		gpos::pointer<CExpression *> pexprNLJ,
		gpos::pointer<CColRefArray *> colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnMergeJoin(
		gpos::pointer<CExpression *> pexprMJ,
		gpos::pointer<CColRefArray *> colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnHashJoin(
		gpos::pointer<CExpression *> pexprHJ,
		gpos::pointer<CColRefArray *> colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnCorrelatedNLJoin(
		gpos::pointer<CExpression *> pexprNLJ, CColRefArray *colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnCTEProducer(
		gpos::pointer<CExpression *> pexprCTEProducer,
		gpos::pointer<CColRefArray *> colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnCTEConsumer(
		gpos::pointer<CExpression *> pexprCTEConsumer,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// store outer references in index NLJ inner child into global map
	void StoreIndexNLJOuterRefs(gpos::pointer<CPhysical *> pop);

	// build a scalar DXL subplan node
	void BuildDxlnSubPlan(gpos::owner<CDXLNode *> pdxlnRelChild,
						  const CColRef *colref,
						  gpos::owner<CDXLColRefArray *> dxl_colref_array);

	// build a boolean scalar dxl node with a subplan as its child
	gpos::owner<CDXLNode *> PdxlnBooleanScalarWithSubPlan(
		gpos::owner<CDXLNode *> pdxlnRelChild,
		gpos::owner<CDXLColRefArray *> dxl_colref_array);

	gpos::owner<CDXLNode *> PdxlnScBoolExpr(
		EdxlBoolExprType boolexptype, gpos::owner<CDXLNode *> dxlnode_left,
		gpos::owner<CDXLNode *> dxlnode_right);

	gpos::owner<CDXLNode *> PdxlnTblScanFromNLJoinOuter(
		gpos::pointer<CExpression *> pexprRelational,
		gpos::owner<CDXLNode *> pdxlnScalar,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, CDXLPhysicalProperties *dxl_properties);

	gpos::owner<CDXLNode *> PdxlnResultFromNLJoinOuter(
		CExpression *pexprRelational, gpos::owner<CDXLNode *> pdxlnScalar,
		CColRefArray *colref_array, CDistributionSpecArray *pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML,
		gpos::owner<CDXLPhysicalProperties *> dxl_properties);

	gpos::owner<CDXLNode *> PdxlnMotion(
		gpos::pointer<CExpression *> pexprMotion, CColRefArray *colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	gpos::owner<CDXLNode *> PdxlnMaterialize(
		gpos::pointer<CExpression *> pexprSpool, CColRefArray *colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	// translate a sequence expression
	gpos::owner<CDXLNode *> PdxlnSequence(
		gpos::pointer<CExpression *> pexprSequence, CColRefArray *colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	// translate a dynamic table scan
	gpos::owner<CDXLNode *> PdxlnDynamicTableScan(
		gpos::pointer<CExpression *> pexprDTS,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a dynamic table scan with a scalar condition
	gpos::owner<CDXLNode *> PdxlnDynamicTableScan(
		gpos::pointer<CExpression *> pexprDTS,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
		gpos::pointer<CExpression *> pexprScalarCond,
		gpos::owner<CDXLPhysicalProperties *> dxl_properties);

	// translate a dynamic bitmap table scan
	gpos::owner<CDXLNode *> PdxlnDynamicBitmapTableScan(
		gpos::pointer<CExpression *> pexprDynamicBitmapTableScan,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// Construct a table descr for a child partition
	gpos::owner<CTableDescriptor *> MakeTableDescForPart(
		gpos::pointer<const IMDRelation *> part,
		gpos::pointer<CTableDescriptor *> root_table_desc);

	// Construct a dxl index descriptor for a child partition
	static gpos::owner<CDXLIndexDescr *> PdxlnIndexDescForPart(
		CMemoryPool *m_mp, gpos::pointer<MdidHashSet *> child_index_mdids_set,
		gpos::pointer<const IMDRelation *> part,
		const CWStringConst *index_name);

	// translate a dynamic bitmap table scan
	gpos::owner<CDXLNode *> PdxlnDynamicBitmapTableScan(
		gpos::pointer<CExpression *> pexprDynamicBitmapTableScan,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
		gpos::pointer<CExpression *> pexprScalar,
		gpos::owner<CDXLPhysicalProperties *> dxl_properties);

	// translate a dynamic index scan based on passed properties
	gpos::owner<CDXLNode *> PdxlnDynamicIndexScan(
		gpos::pointer<CExpression *> pexprDIS,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::owner<CDXLPhysicalProperties *> dxl_properties,
		gpos::pointer<CReqdPropPlan *> prpp);

	// translate a dynamic index scan
	gpos::owner<CDXLNode *> PdxlnDynamicIndexScan(
		gpos::pointer<CExpression *> pexprDIS,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a const table get into a result node
	gpos::owner<CDXLNode *> PdxlnResultFromConstTableGet(
		gpos::pointer<CExpression *> pexprCTG,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a const table get into a result node
	gpos::owner<CDXLNode *> PdxlnResultFromConstTableGet(
		gpos::pointer<CExpression *> pexprCTG,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CExpression *> pexprScalarCond);

	// translate a table-valued function
	gpos::owner<CDXLNode *> PdxlnTVF(
		gpos::pointer<CExpression *> pexprTVF,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate an union all op
	gpos::owner<CDXLNode *> PdxlnAppend(
		gpos::pointer<CExpression *> pexprUnionAll,
		gpos::pointer<CColRefArray *> colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	// translate a partition selector
	gpos::owner<CDXLNode *> PdxlnPartitionSelector(
		gpos::pointer<CExpression *> pexpr, CColRefArray *colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	// translate a DML operator
	gpos::owner<CDXLNode *> PdxlnDML(gpos::pointer<CExpression *> pexpr,
									 gpos::pointer<CColRefArray *> colref_array,
									 CDistributionSpecArray *pdrgpdsBaseTables,
									 ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a CTAS operator
	gpos::owner<CDXLNode *> PdxlnCTAS(gpos::pointer<CExpression *> pexpr,
									  CDistributionSpecArray *pdrgpdsBaseTables,
									  ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a split operator
	gpos::owner<CDXLNode *> PdxlnSplit(
		gpos::pointer<CExpression *> pexpr,
		gpos::pointer<CColRefArray *> colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	// translate an assert operator
	gpos::owner<CDXLNode *> PdxlnAssert(
		gpos::pointer<CExpression *> pexprAssert,
		gpos::pointer<CColRefArray *> colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	// translate a row trigger operator
	gpos::owner<CDXLNode *> PdxlnRowTrigger(
		gpos::pointer<CExpression *> pexpr,
		gpos::pointer<CColRefArray *> colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	// translate a scalar If statement
	gpos::owner<CDXLNode *> PdxlnScIfStmt(
		gpos::pointer<CExpression *> pexprScIf);

	// translate a scalar switch
	gpos::owner<CDXLNode *> PdxlnScSwitch(
		gpos::pointer<CExpression *> pexprScSwitch);

	// translate a scalar switch case
	gpos::owner<CDXLNode *> PdxlnScSwitchCase(
		gpos::pointer<CExpression *> pexprScSwitchCase);

	// translate a scalar case test
	gpos::owner<CDXLNode *> PdxlnScCaseTest(
		gpos::pointer<CExpression *> pexprScCaseTest);

	// translate a scalar comparison
	gpos::owner<CDXLNode *> PdxlnScCmp(gpos::pointer<CExpression *> pexprScCmp);

	// translate a scalar distinct comparison
	gpos::owner<CDXLNode *> PdxlnScDistinctCmp(
		gpos::pointer<CExpression *> pexprScIsDistFrom);

	// translate a scalar op node
	gpos::owner<CDXLNode *> PdxlnScOp(gpos::pointer<CExpression *> pexprScOp);

	// translate a scalar constant
	gpos::owner<CDXLNode *> PdxlnScConst(
		gpos::pointer<CExpression *> pexprScConst);

	gpos::owner<CDXLNode *> PdxlnScSortGroupClause(
		gpos::pointer<CExpression *> pexprScSortGroupClause);

	// translate a scalar coalesce
	gpos::owner<CDXLNode *> PdxlnScCoalesce(
		gpos::pointer<CExpression *> pexprScCoalesce);

	// translate a scalar MinMax
	gpos::owner<CDXLNode *> PdxlnScMinMax(
		gpos::pointer<CExpression *> pexprScMinMax);

	// translate a scalar boolean expression
	gpos::owner<CDXLNode *> PdxlnScBoolExpr(
		gpos::pointer<CExpression *> pexprScBoolExpr);

	// translate a scalar identifier
	gpos::owner<CDXLNode *> PdxlnScId(gpos::pointer<CExpression *> pexprScId);

	// translate a scalar function expression
	gpos::owner<CDXLNode *> PdxlnScFuncExpr(
		gpos::pointer<CExpression *> pexprScFunc);

	// translate a scalar window function expression
	gpos::owner<CDXLNode *> PdxlnScWindowFuncExpr(
		gpos::pointer<CExpression *> pexprScFunc);

	// get the DXL representation of the window stage
	static EdxlWinStage Ews(CScalarWindowFunc::EWinStage ews);

	// translate a scalar aggref
	gpos::owner<CDXLNode *> PdxlnScAggref(
		gpos::pointer<CExpression *> pexprScAggFunc);

	// translate a scalar nullif
	gpos::owner<CDXLNode *> PdxlnScNullIf(
		gpos::pointer<CExpression *> pexprScNullIf);

	// translate a scalar null test
	gpos::owner<CDXLNode *> PdxlnScNullTest(
		gpos::pointer<CExpression *> pexprScNullTest);

	// translate a scalar boolean test
	gpos::owner<CDXLNode *> PdxlnScBooleanTest(
		gpos::pointer<CExpression *> pexprScBooleanTest);

	// translate a scalar cast
	gpos::owner<CDXLNode *> PdxlnScCast(
		gpos::pointer<CExpression *> pexprScCast);

	// translate a scalar coerce
	gpos::owner<CDXLNode *> PdxlnScCoerceToDomain(
		gpos::pointer<CExpression *> pexprScCoerce);

	// translate a scalar coerce using I/O functions
	gpos::owner<CDXLNode *> PdxlnScCoerceViaIO(
		gpos::pointer<CExpression *> pexprScCoerce);

	// translate a scalar array coerce expr with element coerce function
	gpos::owner<CDXLNode *> PdxlnScArrayCoerceExpr(
		gpos::pointer<CExpression *> pexprScArrayCoerceExpr);

	gpos::owner<CDXLNode *> PdxlnValuesList(gpos::pointer<CExpression *> pexpr);

	// translate an array
	gpos::owner<CDXLNode *> PdxlnArray(gpos::pointer<CExpression *> pexpr);

	// translate an arrayref
	gpos::owner<CDXLNode *> PdxlnArrayRef(gpos::pointer<CExpression *> pexpr);

	// translate an arrayref index list
	gpos::owner<CDXLNode *> PdxlnArrayRefIndexList(
		gpos::pointer<CExpression *> pexpr);

	// translate the arrayref index list bound
	static CDXLScalarArrayRefIndexList::EIndexListBound Eilb(
		const CScalarArrayRefIndexList::EIndexListType eilt);

	// translate an array compare
	gpos::owner<CDXLNode *> PdxlnArrayCmp(gpos::pointer<CExpression *> pexpr);

	// translate an assert predicate
	gpos::owner<CDXLNode *> PdxlnAssertPredicate(
		gpos::pointer<CExpression *> pexpr);

	// translate an assert constraint
	gpos::owner<CDXLNode *> PdxlnAssertConstraint(
		gpos::pointer<CExpression *> pexpr);

	// translate a DML action expression
	gpos::owner<CDXLNode *> PdxlnDMLAction(gpos::pointer<CExpression *> pexpr);

	// translate a window frame
	gpos::owner<CDXLWindowFrame *> GetWindowFrame(
		gpos::pointer<CWindowFrame *> pwf);

	gpos::owner<CDXLTableDescr *> MakeDXLTableDescr(
		gpos::pointer<const CTableDescriptor *> ptabdesc,
		gpos::pointer<const CColRefArray *> pdrgpcrOutput,
		gpos::pointer<const CReqdPropPlan *> requiredProperties);

	// compute physical properties like operator cost from the expression
	gpos::owner<CDXLPhysicalProperties *> GetProperties(
		gpos::pointer<const CExpression *> pexpr);

	// translate a colref set of output col into a dxl proj list
	gpos::owner<CDXLNode *> PdxlnProjList(
		gpos::pointer<const CColRefSet *> pcrsOutput,
		gpos::pointer<CColRefArray *> colref_array);

	// Construct a project list for a child partition
	gpos::owner<CDXLNode *> PdxlnProjListForChildPart(
		gpos::pointer<const ColRefToUlongMap *> root_col_mapping,
		gpos::pointer<const CColRefArray *> part_colrefs,
		gpos::pointer<const CColRefSet *> reqd_colrefs,
		gpos::pointer<const CColRefArray *> colref_array);

	// translate a filter expr on the root for a child partition
	gpos::owner<CDXLNode *> PdxlnCondForChildPart(
		gpos::pointer<const ColRefToUlongMap *> root_col_mapping,
		gpos::pointer<const CColRefArray *> part_colrefs,
		gpos::pointer<const CColRefArray *> root_colrefs,
		gpos::pointer<CExpression *> pred);

	// translate a project list expression into a DXL proj list node
	// according to the order specified in the dynamic array
	gpos::owner<CDXLNode *> PdxlnProjList(
		gpos::pointer<const CExpression *> pexprProjList,
		gpos::pointer<const CColRefSet *> pcrsOutput,
		gpos::pointer<CColRefArray *> colref_array);

	// translate a project list expression into a DXL proj list node
	gpos::owner<CDXLNode *> PdxlnProjList(
		gpos::pointer<const CExpression *> pexprProjList,
		gpos::pointer<const CColRefSet *> pcrsOutput);

	// create a project list for a result node from a tuple of a
	// const table get operator
	gpos::owner<CDXLNode *> PdxlnProjListFromConstTableGet(
		gpos::pointer<CColRefArray *> pdrgpcrReqOutput,
		gpos::pointer<CColRefArray *> pdrgpcrCTGOutput,
		gpos::pointer<IDatumArray *> pdrgpdatumValues);

	// create a DXL project elem node from a proj element expression
	gpos::owner<CDXLNode *> PdxlnProjElem(
		gpos::pointer<const CExpression *> pexprProjElem);

	// create a project element for a computed column from a column reference
	// and value expresison
	gpos::owner<CDXLNode *> PdxlnProjElem(const CColRef *colref,
										  gpos::owner<CDXLNode *> pdxlnValue);

	// create a DXL sort col list node from an order spec
	gpos::owner<CDXLNode *> GetSortColListDXL(
		gpos::pointer<const COrderSpec *> pos);

	// create a DXL sort col list node for a Motion expression
	gpos::owner<CDXLNode *> GetSortColListDXL(
		gpos::pointer<CExpression *> pexprMotion);

	// create a DXL hash expr list from an array of hash columns
	gpos::owner<CDXLNode *> PdxlnHashExprList(
		gpos::pointer<const CExpressionArray *> pdrgpexpr,
		gpos::pointer<const IMdIdArray *> opfamilies);

	// create a DXL filter node with the given scalar expression
	gpos::owner<CDXLNode *> PdxlnFilter(gpos::owner<CDXLNode *> pdxlnCond);

	// construct an array with input segment ids for the given motion expression
	gpos::owner<IntPtrArray *> GetInputSegIdsArray(
		gpos::pointer<CExpression *> pexprMotion);

	// construct an array with output segment ids for the given motion expression
	gpos::owner<IntPtrArray *> GetOutputSegIdsArray(
		gpos::pointer<CExpression *> pexprMotion);

	// find the position of the given colref in the array
	static ULONG UlPosInArray(const CColRef *colref,
							  gpos::pointer<const CColRefArray *> colref_array);

	// return hash join type
	static EdxlJoinType EdxljtHashJoin(
		gpos::pointer<CPhysicalHashJoin *> popHJ);

	// main translation routine for Expr tree -> DXL tree
	gpos::owner<CDXLNode *> CreateDXLNode(
		CExpression *pexpr, CColRefArray *colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML, BOOL fRemap, BOOL fRoot);

	// translate expression children and add them as children of the DXL node
	void TranslateScalarChildren(gpos::pointer<CExpression *> pexpr,
								 gpos::pointer<CDXLNode *> dxlnode);

	// add a result node, if required a materialize node is added below result node to avoid deadlock hazard
	gpos::owner<CDXLNode *> PdxlnResult(
		gpos::owner<CDXLPhysicalProperties *> dxl_properties,
		gpos::owner<CDXLNode *> pdxlnPrL,
		gpos::owner<CDXLNode *> child_dxlnode);

	// add a materialize node
	gpos::owner<CDXLNode *> PdxlnMaterialize(gpos::owner<CDXLNode *> dxlnode);

	// add result node if necessary
	gpos::owner<CDXLNode *> PdxlnRemapOutputColumns(
		gpos::pointer<CExpression *> pexpr, gpos::owner<CDXLNode *> dxlnode,
		gpos::pointer<CColRefArray *> pdrgpcrRequired,
		gpos::pointer<CColRefArray *> pdrgpcrOrder);

	// combines the ordered columns and required columns into a single list
	static gpos::owner<CColRefArray *> PdrgpcrMerge(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> pdrgpcrOrder,
		gpos::pointer<CColRefArray *> pdrgpcrRequired);

	// helper to add a project of bool constant
	gpos::owner<CDXLNode *> PdxlnProjectBoolConst(
		gpos::owner<CDXLNode *> dxlnode, BOOL value);

	// helper to build a Result expression with project list restricted to required column
	gpos::owner<CDXLNode *> PdxlnRestrictResult(gpos::owner<CDXLNode *> dxlnode,
												const CColRef *colref);

	// helper to build a Result expression with project list restricted to required columns
	gpos::owner<CDXLNode *> PdxlnRestrictResult(
		gpos::owner<CDXLNode *> dxlnode,
		gpos::pointer<const CColRefSet *> colrefs);

	//	helper to build subplans from correlated LOJ
	void BuildSubplansForCorrelatedLOJ(
		gpos::pointer<CExpression *> pexprCorrelatedLOJ,
		CDXLColRefArray *dxl_colref_array,
		gpos::owner<CDXLNode *> *
			ppdxlnScalar,  // output: scalar condition after replacing inner child reference with subplan
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	// helper to build subplans of different types
	void BuildSubplans(
		gpos::pointer<CExpression *> pexprCorrelatedNLJoin,
		gpos::owner<CDXLColRefArray *> dxl_colref_array,
		gpos::owner<CDXLNode *> *
			ppdxlnScalar,  // output: scalar condition after replacing inner child reference with subplan
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	// helper to build scalar subplans from inner column references and store them
	// in subplan map
	void BuildScalarSubplans(gpos::pointer<CColRefArray *> pdrgpcrInner,
							 CExpression *pexprInner,
							 CDXLColRefArray *dxl_colref_array,
							 CDistributionSpecArray *pdrgpdsBaseTables,
							 ULONG *pulNonGatherMotions, BOOL *pfDML);

	// helper to build subplans for quantified (ANY/ALL) subqueries
	gpos::pointer<CDXLNode *> PdxlnQuantifiedSubplan(
		gpos::pointer<CColRefArray *> pdrgpcrInner,
		gpos::pointer<CExpression *> pexprCorrelatedNLJoin,
		gpos::owner<CDXLColRefArray *> dxl_colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	// helper to build subplans for existential subqueries
	gpos::pointer<CDXLNode *> PdxlnExistentialSubplan(
		gpos::pointer<CColRefArray *> pdrgpcrInner,
		gpos::pointer<CExpression *> pexprCorrelatedNLJoin,
		gpos::owner<CDXLColRefArray *> dxl_colref_array,
		CDistributionSpecArray *pdrgpdsBaseTables, ULONG *pulNonGatherMotions,
		BOOL *pfDML);

	// compute the direct dispatch info for the given DML expression
	gpos::owner<CDXLDirectDispatchInfo *> GetDXLDirectDispatchInfo(
		gpos::pointer<CExpression *> pexprDML);

	// check if result node imposes a motion hazard
	BOOL FNeedsMaterializeUnderResult(
		gpos::pointer<CDXLNode *> proj_list_dxlnode,
		gpos::pointer<CDXLNode *> child_dxlnode);

	void AddPartForScanId(ULONG scanid, ULONG index);

	// helper to find subplan type from a correlated left outer join expression
	static EdxlSubPlanType EdxlsubplantypeCorrelatedLOJ(
		gpos::pointer<CExpression *> pexprCorrelatedLOJ);

	// helper to find subplan type from a correlated join expression
	static EdxlSubPlanType Edxlsubplantype(
		gpos::pointer<CExpression *> pexprCorrelatedNLJoin);

	// add used columns in the bitmap re-check and the remaining scalar filter condition to the
	// required output column
	static void AddBitmapFilterColumns(
		CMemoryPool *mp, gpos::pointer<CPhysicalScan *> pop,
		gpos::pointer<CExpression *> pexprRecheckCond,
		gpos::pointer<CExpression *> pexprScalar,
		gpos::pointer<CColRefSet *>
			pcrsReqdOutput	// append the required column reference
	);

public:
	// ctor
	CTranslatorExprToDXL(CMemoryPool *mp, CMDAccessor *md_accessor,
						 gpos::owner<IntPtrArray *> pdrgpiSegments,
						 BOOL fInitColumnFactory = true);

	// dtor
	~CTranslatorExprToDXL();

	// main driver
	gpos::owner<CDXLNode *> PdxlnTranslate(
		CExpression *pexpr, CColRefArray *colref_array,
		gpos::pointer<CMDNameArray *> pdrgpmdname);

	// translate a scalar expression into a DXL scalar node
	// if the expression is not a scalar, an UnsupportedOp exception is raised
	gpos::owner<CDXLNode *> PdxlnScalar(gpos::pointer<CExpression *> pexpr);
};
}  // namespace gpopt

#endif	// !GPOPT_CTranslatorExprToDXL_H

// EOF
