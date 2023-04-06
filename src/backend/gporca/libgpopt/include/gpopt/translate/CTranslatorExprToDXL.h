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
	ColRefToDXLNodeMap *m_phmcrdxln;

	// mappings CColRef -> CDXLNode used to for index predicates with outer references
	ColRefToDXLNodeMap *m_phmcrdxlnIndexLookup;

	// mappings CColRef -> ColId for translating filters for child partitions
	// (see PdxlnFilterForChildPart())
	gpos::Ref<ColRefToUlongMap> m_phmcrulPartColId = nullptr;

	// derived plan properties of the translated expression
	gpos::Ref<CDrvdPropPlan> m_pdpplan;

	// a copy of the pointer to column factory, obtained at construction time
	CColumnFactory *m_pcf;

	// segment ids on target system
	gpos::Ref<IntPtrArray> m_pdrgpiSegments;

	// id of master node
	INT m_iMasterId;

	// private copy ctor
	CTranslatorExprToDXL(const CTranslatorExprToDXL &);

	static EdxlBoolExprType Edxlbooltype(
		const CScalarBoolOp::EBoolOperator eboolop);

	// return the EdxlDmlType for a given DML op type
	static EdxlDmlType Edxldmloptype(const CLogicalDML::EDMLOperator edmlop);

	// return outer refs in correlated join inner child
	static gpos::Ref<CColRefSet> PcrsOuterRefsForCorrelatedNLJoin(
		gpos::Ref<CExpression> pexpr);

	// functions translating different optimizer expressions into their
	// DXL counterparts

	gpos::Ref<CDXLNode> PdxlnTblScan(
		gpos::Ref<CExpression> pexprTblScan, gpos::Ref<CColRefSet> pcrsOutput,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		gpos::Ref<CExpression> pexprScalarCond,
		gpos::Ref<CDXLPhysicalProperties> dxl_properties);

	// create a (dynamic) index scan node after inlining the given scalar condition, if needed
	gpos::Ref<CDXLNode> PdxlnIndexScanWithInlinedCondition(
		gpos::Ref<CExpression> pexprIndexScan,
		gpos::Ref<CExpression> pexprScalarCond,
		gpos::Ref<CDXLPhysicalProperties> dxl_properties,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables);

	// translate index scan based on passed properties
	gpos::Ref<CDXLNode> PdxlnIndexScan(
		gpos::Ref<CExpression> pexprIndexScan,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDXLPhysicalProperties> dxl_properties,
		gpos::Ref<CReqdPropPlan> prpp);

	// translate index scan
	gpos::Ref<CDXLNode> PdxlnIndexScan(
		gpos::Ref<CExpression> pexprIndexScan,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnIndexOnlyScan(
		gpos::Ref<CExpression> pexprIndexScan,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnIndexOnlyScan(
		gpos::Ref<CExpression> pexprIndexScan,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDXLPhysicalProperties> dxl_properties,
		gpos::Ref<CReqdPropPlan> prpp);

	// translate a bitmap index probe expression to DXL
	gpos::Ref<CDXLNode> PdxlnBitmapIndexProbe(
		gpos::Ref<CExpression> pexprBitmapIndexProbe);

	// translate a bitmap bool op expression to DXL
	gpos::Ref<CDXLNode> PdxlnBitmapBoolOp(
		gpos::Ref<CExpression> pexprBitmapBoolOp);

	// translate a bitmap table scan expression to DXL
	gpos::Ref<CDXLNode> PdxlnBitmapTableScan(
		gpos::Ref<CExpression> pexprBitmapTableScan,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a bitmap table scan expression to DXL
	gpos::Ref<CDXLNode> PdxlnBitmapTableScan(
		gpos::Ref<CExpression> pexprBitmapTableScan,
		gpos::Ref<CColRefSet> pcrsOutput, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		gpos::Ref<CExpression> pexprScalar,
		gpos::Ref<CDXLPhysicalProperties> dxl_properties);

	// create a DXL result node from an optimizer filter node
	gpos::Ref<CDXLNode> PdxlnResultFromFilter(
		gpos::Ref<CExpression> pexprFilter,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnResult(
		gpos::Ref<CExpression> pexprFilter,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// create a DXL result node for the given project list
	gpos::Ref<CDXLNode> PdxlnResult(
		gpos::Ref<CDXLNode> proj_list_dxlnode,
		gpos::Ref<CExpression> pexprFilter,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// given a DXL plan tree child_dxlnode which represents the physical plan pexprRelational, construct a DXL
	// Result node that filters on top of it using the scalar condition pdxlnScalar
	gpos::Ref<CDXLNode> PdxlnAddScalarFilterOnRelationalChild(
		gpos::Ref<CDXLNode> pdxlnRelationalChild,
		gpos::Ref<CDXLNode> pdxlnScalarChild,
		gpos::Ref<CDXLPhysicalProperties> dxl_properties,
		gpos::Ref<CColRefSet> pcrsOutput, gpos::Ref<CColRefArray> pdrgpcrOrder);

	gpos::Ref<CDXLNode> PdxlnFromFilter(
		gpos::Ref<CExpression> pexprFilter,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML,
		gpos::Ref<CDXLPhysicalProperties> dxl_properties);

	gpos::Ref<CDXLNode> PdxlnResult(
		gpos::Ref<CExpression> pexprRelational,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML,
		gpos::Ref<CDXLNode> pdxlnScalar,
		gpos::Ref<CDXLPhysicalProperties> dxl_properties);

	gpos::Ref<CDXLNode> PdxlnResult(
		gpos::Ref<CExpression> pexprRelational,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML,
		gpos::Ref<CDXLNode> pdxlnScalar);

	gpos::Ref<CDXLNode> PdxlnComputeScalar(
		gpos::Ref<CExpression> pexprComputeScalar,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnAggregate(
		gpos::Ref<CExpression> pexprHashAgg,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnAggregateDedup(
		gpos::Ref<CExpression> pexprAgg, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnAggregate(
		gpos::Ref<CExpression> pexprAgg, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML,
		EdxlAggStrategy dxl_agg_strategy,
		const gpos::Ref<CColRefArray> pdrgpcrGroupingCols,
		gpos::Ref<CColRefSet> pcrsKeys);

	gpos::Ref<CDXLNode> PdxlnSort(
		gpos::Ref<CExpression> pexprSort, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnLimit(
		gpos::Ref<CExpression> pexprLimit, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnWindow(
		gpos::Ref<CExpression> pexprSeqPrj,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnNLJoin(
		gpos::Ref<CExpression> pexprNLJ, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnMergeJoin(
		gpos::Ref<CExpression> pexprMJ, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnHashJoin(
		gpos::Ref<CExpression> pexprHJ, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnCorrelatedNLJoin(
		gpos::Ref<CExpression> pexprNLJ, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnCTEProducer(
		gpos::Ref<CExpression> pexprCTEProducer,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnCTEConsumer(
		gpos::Ref<CExpression> pexprCTEConsumer,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// store outer references in index NLJ inner child into global map
	void StoreIndexNLJOuterRefs(gpos::Ref<CPhysical> pop);

	// build a scalar DXL subplan node
	void BuildDxlnSubPlan(gpos::Ref<CDXLNode> pdxlnRelChild,
						  const gpos::Ref<CColRef> colref,
						  CDXLColRefArray *dxl_colref_array);

	// build a boolean scalar dxl node with a subplan as its child
	gpos::Ref<CDXLNode> PdxlnBooleanScalarWithSubPlan(
		gpos::Ref<CDXLNode> pdxlnRelChild, CDXLColRefArray *dxl_colref_array);

	gpos::Ref<CDXLNode> PdxlnScBoolExpr(EdxlBoolExprType boolexptype,
										gpos::Ref<CDXLNode> dxlnode_left,
										gpos::Ref<CDXLNode> dxlnode_right);

	gpos::Ref<CDXLNode> PdxlnTblScanFromNLJoinOuter(
		gpos::Ref<CExpression> pexprRelational, gpos::Ref<CDXLNode> pdxlnScalar,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions,
		gpos::Ref<CDXLPhysicalProperties> dxl_properties);

	gpos::Ref<CDXLNode> PdxlnResultFromNLJoinOuter(
		gpos::Ref<CExpression> pexprRelational, gpos::Ref<CDXLNode> pdxlnScalar,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML,
		gpos::Ref<CDXLPhysicalProperties> dxl_properties);

	gpos::Ref<CDXLNode> PdxlnMotion(
		gpos::Ref<CExpression> pexprMotion,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	gpos::Ref<CDXLNode> PdxlnMaterialize(
		gpos::Ref<CExpression> pexprSpool, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a sequence expression
	gpos::Ref<CDXLNode> PdxlnSequence(
		gpos::Ref<CExpression> pexprSequence,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a dynamic table scan
	gpos::Ref<CDXLNode> PdxlnDynamicTableScan(
		gpos::Ref<CExpression> pexprDTS, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a dynamic table scan with a scalar condition
	gpos::Ref<CDXLNode> PdxlnDynamicTableScan(
		gpos::Ref<CExpression> pexprDTS, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		gpos::Ref<CExpression> pexprScalarCond,
		gpos::Ref<CDXLPhysicalProperties> dxl_properties);

	// translate a dynamic bitmap table scan
	gpos::Ref<CDXLNode> PdxlnDynamicBitmapTableScan(
		gpos::Ref<CExpression> pexprDynamicBitmapTableScan,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a dynamic foreign scan
	gpos::Ref<CDXLNode> PdxlnDynamicForeignScan(
		gpos::Ref<CExpression> pexprDFS, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a dynamic foreign scan with a scalar condition
	gpos::Ref<CDXLNode> PdxlnDynamicForeignScan(
		gpos::Ref<CExpression> pexprDFS, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		gpos::Ref<CExpression> pexprScalarCond,
		gpos::Ref<CDXLPhysicalProperties> dxl_properties);

	// Construct a table descr for a child partition
	gpos::Ref<CTableDescriptor> MakeTableDescForPart(
		const gpos::Ref<IMDRelation> part,
		gpos::Ref<CTableDescriptor> root_table_desc);

	// Construct a dxl index descriptor for a child partition
	static gpos::Ref<CDXLIndexDescr> PdxlnIndexDescForPart(
		CMemoryPool *m_mp, MdidHashSet *child_index_mdids_set,
		const gpos::Ref<IMDRelation> part, const CWStringConst *index_name);

	// translate a dynamic bitmap table scan
	gpos::Ref<CDXLNode> PdxlnDynamicBitmapTableScan(
		gpos::Ref<CExpression> pexprDynamicBitmapTableScan,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		gpos::Ref<CExpression> pexprScalar,
		gpos::Ref<CDXLPhysicalProperties> dxl_properties);

	// translate a dynamic index scan based on passed properties
	gpos::Ref<CDXLNode> PdxlnDynamicIndexScan(
		gpos::Ref<CExpression> pexprDIS, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDXLPhysicalProperties> dxl_properties,
		gpos::Ref<CReqdPropPlan> prpp);

	// translate a dynamic index scan
	gpos::Ref<CDXLNode> PdxlnDynamicIndexScan(
		gpos::Ref<CExpression> pexprDIS, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a const table get into a result node
	gpos::Ref<CDXLNode> PdxlnResultFromConstTableGet(
		gpos::Ref<CExpression> pexprCTG, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a const table get into a result node
	gpos::Ref<CDXLNode> PdxlnResultFromConstTableGet(
		gpos::Ref<CExpression> pexprCTG, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CExpression> pexprScalarCond);

	// translate a table-valued function
	gpos::Ref<CDXLNode> PdxlnTVF(
		gpos::Ref<CExpression> pexprTVF, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate an union all op
	gpos::Ref<CDXLNode> PdxlnAppend(
		gpos::Ref<CExpression> pexprUnionAll,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a partition selector
	gpos::Ref<CDXLNode> PdxlnPartitionSelector(
		gpos::Ref<CExpression> pexpr, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a DML operator
	gpos::Ref<CDXLNode> PdxlnDML(
		gpos::Ref<CExpression> pexpr, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a CTAS operator
	gpos::Ref<CDXLNode> PdxlnCTAS(
		gpos::Ref<CExpression> pexpr,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a split operator
	gpos::Ref<CDXLNode> PdxlnSplit(
		gpos::Ref<CExpression> pexpr, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate an assert operator
	gpos::Ref<CDXLNode> PdxlnAssert(
		gpos::Ref<CExpression> pexprAssert,
		gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// translate a scalar If statement
	gpos::Ref<CDXLNode> PdxlnScIfStmt(gpos::Ref<CExpression> pexprScIf);

	// translate a scalar switch
	gpos::Ref<CDXLNode> PdxlnScSwitch(gpos::Ref<CExpression> pexprScSwitch);

	// translate a scalar switch case
	gpos::Ref<CDXLNode> PdxlnScSwitchCase(
		gpos::Ref<CExpression> pexprScSwitchCase);

	// translate a scalar case test
	gpos::Ref<CDXLNode> PdxlnScCaseTest(gpos::Ref<CExpression> pexprScCaseTest);

	// translate a scalar comparison
	gpos::Ref<CDXLNode> PdxlnScCmp(gpos::Ref<CExpression> pexprScCmp);

	// translate a scalar distinct comparison
	gpos::Ref<CDXLNode> PdxlnScDistinctCmp(
		gpos::Ref<CExpression> pexprScIsDistFrom);

	// translate a scalar op node
	gpos::Ref<CDXLNode> PdxlnScOp(gpos::Ref<CExpression> pexprScOp);

	// translate a scalar constant
	gpos::Ref<CDXLNode> PdxlnScConst(gpos::Ref<CExpression> pexprScConst);

	gpos::Ref<CDXLNode> PdxlnScSortGroupClause(
		gpos::Ref<CExpression> pexprScSortGroupClause);

	// translate a scalar coalesce
	gpos::Ref<CDXLNode> PdxlnScCoalesce(gpos::Ref<CExpression> pexprScCoalesce);

	// translate a scalar MinMax
	gpos::Ref<CDXLNode> PdxlnScMinMax(gpos::Ref<CExpression> pexprScMinMax);

	// translate a scalar boolean expression
	gpos::Ref<CDXLNode> PdxlnScBoolExpr(gpos::Ref<CExpression> pexprScBoolExpr);

	// translate a scalar identifier
	gpos::Ref<CDXLNode> PdxlnScId(gpos::Ref<CExpression> pexprScId);

	// translate a scalar function expression
	gpos::Ref<CDXLNode> PdxlnScFuncExpr(gpos::Ref<CExpression> pexprScFunc);

	// translate a scalar window function expression
	gpos::Ref<CDXLNode> PdxlnScWindowFuncExpr(
		gpos::Ref<CExpression> pexprScFunc);

	// get the DXL representation of the window stage
	static EdxlWinStage Ews(CScalarWindowFunc::EWinStage ews);

	// translate a scalar aggref
	gpos::Ref<CDXLNode> PdxlnScAggref(gpos::Ref<CExpression> pexprScAggFunc);

	// translate a scalar nullif
	gpos::Ref<CDXLNode> PdxlnScNullIf(gpos::Ref<CExpression> pexprScNullIf);

	// translate a scalar null test
	gpos::Ref<CDXLNode> PdxlnScNullTest(gpos::Ref<CExpression> pexprScNullTest);

	// translate a scalar boolean test
	gpos::Ref<CDXLNode> PdxlnScBooleanTest(
		gpos::Ref<CExpression> pexprScBooleanTest);

	// translate a scalar cast
	gpos::Ref<CDXLNode> PdxlnScCast(gpos::Ref<CExpression> pexprScCast);

	// translate a scalar coerce
	gpos::Ref<CDXLNode> PdxlnScCoerceToDomain(
		gpos::Ref<CExpression> pexprScCoerce);

	// translate a scalar coerce using I/O functions
	gpos::Ref<CDXLNode> PdxlnScCoerceViaIO(
		gpos::Ref<CExpression> pexprScCoerce);

	// translate a scalar array coerce expr with element coerce function
	gpos::Ref<CDXLNode> PdxlnScArrayCoerceExpr(
		gpos::Ref<CExpression> pexprScArrayCoerceExpr);

	gpos::Ref<CDXLNode> PdxlnValuesList(gpos::Ref<CExpression> pexpr);

	// translate an array
	gpos::Ref<CDXLNode> PdxlnArray(gpos::Ref<CExpression> pexpr);

	// translate an arrayref
	gpos::Ref<CDXLNode> PdxlnArrayRef(gpos::Ref<CExpression> pexpr);

	// translate an arrayref index list
	gpos::Ref<CDXLNode> PdxlnArrayRefIndexList(gpos::Ref<CExpression> pexpr);

	// translate the arrayref index list bound
	static CDXLScalarArrayRefIndexList::EIndexListBound Eilb(
		const CScalarArrayRefIndexList::EIndexListType eilt);

	// translate an array compare
	gpos::Ref<CDXLNode> PdxlnArrayCmp(gpos::Ref<CExpression> pexpr);

	// translate an assert predicate
	gpos::Ref<CDXLNode> PdxlnAssertPredicate(gpos::Ref<CExpression> pexpr);

	// translate an assert constraint
	gpos::Ref<CDXLNode> PdxlnAssertConstraint(gpos::Ref<CExpression> pexpr);

	// translate a DML action expression
	gpos::Ref<CDXLNode> PdxlnDMLAction(gpos::Ref<CExpression> pexpr);

	// translate a window frame
	gpos::Ref<CDXLWindowFrame> GetWindowFrame(gpos::Ref<CWindowFrame> pwf);

	gpos::Ref<CDXLTableDescr> MakeDXLTableDescr(
		const gpos::Ref<CTableDescriptor> ptabdesc,
		const gpos::Ref<CColRefArray> pdrgpcrOutput,
		const gpos::Ref<CReqdPropPlan> requiredProperties);

	// compute physical properties like operator cost from the expression
	gpos::Ref<CDXLPhysicalProperties> GetProperties(
		const gpos::Ref<CExpression> pexpr);

	// translate a colref set of output col into a dxl proj list
	gpos::Ref<CDXLNode> PdxlnProjList(const gpos::Ref<CColRefSet> pcrsOutput,
									  gpos::Ref<CColRefArray> colref_array);

	// Construct a project list for a child partition
	gpos::Ref<CDXLNode> PdxlnProjListForChildPart(
		const gpos::Ref<ColRefToUlongMap> root_col_mapping,
		const gpos::Ref<CColRefArray> part_colrefs,
		const gpos::Ref<CColRefSet> reqd_colrefs,
		const gpos::Ref<CColRefArray> colref_array);

	// translate a filter expr on the root for a child partition
	gpos::Ref<CDXLNode> PdxlnCondForChildPart(
		const gpos::Ref<ColRefToUlongMap> root_col_mapping,
		const gpos::Ref<CColRefArray> part_colrefs,
		const gpos::Ref<CColRefArray> root_colrefs,
		gpos::Ref<CExpression> pred);

	// translate a project list expression into a DXL proj list node
	// according to the order specified in the dynamic array
	gpos::Ref<CDXLNode> PdxlnProjList(
		const gpos::Ref<CExpression> pexprProjList,
		const gpos::Ref<CColRefSet> pcrsOutput,
		gpos::Ref<CColRefArray> colref_array);

	// translate a project list expression into a DXL proj list node
	gpos::Ref<CDXLNode> PdxlnProjList(
		const gpos::Ref<CExpression> pexprProjList,
		const gpos::Ref<CColRefSet> pcrsOutput);

	// create a project list for a result node from a tuple of a
	// const table get operator
	gpos::Ref<CDXLNode> PdxlnProjListFromConstTableGet(
		gpos::Ref<CColRefArray> pdrgpcrReqOutput,
		gpos::Ref<CColRefArray> pdrgpcrCTGOutput,
		gpos::Ref<IDatumArray> pdrgpdatumValues);

	// create a DXL project elem node from a proj element expression
	gpos::Ref<CDXLNode> PdxlnProjElem(
		const gpos::Ref<CExpression> pexprProjElem);

	// create a project element for a computed column from a column reference
	// and value expresison
	gpos::Ref<CDXLNode> PdxlnProjElem(const gpos::Ref<CColRef> colref,
									  gpos::Ref<CDXLNode> pdxlnValue);

	// create a DXL sort col list node from an order spec
	gpos::Ref<CDXLNode> GetSortColListDXL(const gpos::Ref<COrderSpec> pos);

	// create a DXL sort col list node for a Motion expression
	gpos::Ref<CDXLNode> GetSortColListDXL(gpos::Ref<CExpression> pexprMotion);

	// create a DXL hash expr list from an array of hash columns
	gpos::Ref<CDXLNode> PdxlnHashExprList(
		const gpos::Ref<CExpressionArray> pdrgpexpr,
		const gpos::Ref<IMdIdArray> opfamilies);

	// create a DXL filter node with the given scalar expression
	gpos::Ref<CDXLNode> PdxlnFilter(gpos::Ref<CDXLNode> pdxlnCond);

	// construct an array with input segment ids for the given motion expression
	gpos::Ref<IntPtrArray> GetInputSegIdsArray(
		gpos::Ref<CExpression> pexprMotion);

	// construct an array with output segment ids for the given motion expression
	gpos::Ref<IntPtrArray> GetOutputSegIdsArray(
		gpos::Ref<CExpression> pexprMotion);

	// find the position of the given colref in the array
	static ULONG UlPosInArray(const gpos::Ref<CColRef> colref,
							  const gpos::Ref<CColRefArray> colref_array);

	// return hash join type
	static EdxlJoinType EdxljtHashJoin(gpos::Ref<CPhysicalHashJoin> popHJ);

	// main translation routine for Expr tree -> DXL tree
	gpos::Ref<CDXLNode> CreateDXLNode(
		gpos::Ref<CExpression> pexpr, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML, BOOL fRemap, BOOL fRoot);

	// translate expression children and add them as children of the DXL node
	void TranslateScalarChildren(gpos::Ref<CExpression> pexpr,
								 gpos::Ref<CDXLNode> dxlnode);

	// add a result node, if required a materialize node is added below result node to avoid deadlock hazard
	gpos::Ref<CDXLNode> PdxlnResult(
		gpos::Ref<CDXLPhysicalProperties> dxl_properties,
		gpos::Ref<CDXLNode> pdxlnPrL, gpos::Ref<CDXLNode> child_dxlnode);

	// add a materialize node
	gpos::Ref<CDXLNode> PdxlnMaterialize(gpos::Ref<CDXLNode> dxlnode);

	// add result node if necessary
	gpos::Ref<CDXLNode> PdxlnRemapOutputColumns(
		gpos::Ref<CExpression> pexpr, gpos::Ref<CDXLNode> dxlnode,
		gpos::Ref<CColRefArray> pdrgpcrRequired,
		gpos::Ref<CColRefArray> pdrgpcrOrder);

	// combines the ordered columns and required columns into a single list
	static gpos::Ref<CColRefArray> PdrgpcrMerge(
		CMemoryPool *mp, gpos::Ref<CColRefArray> pdrgpcrOrder,
		gpos::Ref<CColRefArray> pdrgpcrRequired);

	// helper to add a project of bool constant
	gpos::Ref<CDXLNode> PdxlnProjectBoolConst(gpos::Ref<CDXLNode> dxlnode,
											  BOOL value);

	// helper to build a Result expression with project list restricted to required column
	gpos::Ref<CDXLNode> PdxlnRestrictResult(gpos::Ref<CDXLNode> dxlnode,
											const gpos::Ref<CColRef> colref);

	// helper to build a Result expression with project list restricted to required columns
	gpos::Ref<CDXLNode> PdxlnRestrictResult(
		gpos::Ref<CDXLNode> dxlnode, const gpos::Ref<CColRefSet> colrefs);

	//	helper to build subplans from correlated LOJ
	void BuildSubplansForCorrelatedLOJ(
		gpos::Ref<CExpression> pexprCorrelatedLOJ,
		CDXLColRefArray *dxl_colref_array,
		gpos::Ref<CDXLNode> *
			ppdxlnScalar,  // output: scalar condition after replacing inner child reference with subplan
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// helper to build subplans of different types
	void BuildSubplans(
		gpos::Ref<CExpression> pexprCorrelatedNLJoin,
		CDXLColRefArray *dxl_colref_array,
		gpos::Ref<CDXLNode> *
			ppdxlnScalar,  // output: scalar condition after replacing inner child reference with subplan
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// helper to build scalar subplans from inner column references and store them
	// in subplan map
	void BuildScalarSubplans(
		gpos::Ref<CColRefArray> pdrgpcrInner, gpos::Ref<CExpression> pexprInner,
		CDXLColRefArray *dxl_colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// helper to build subplans for quantified (ANY/ALL) subqueries
	gpos::Ref<CDXLNode> PdxlnQuantifiedSubplan(
		gpos::Ref<CColRefArray> pdrgpcrInner,
		gpos::Ref<CExpression> pexprCorrelatedNLJoin,
		CDXLColRefArray *dxl_colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// helper to build subplans for existential subqueries
	gpos::Ref<CDXLNode> PdxlnExistentialSubplan(
		gpos::Ref<CColRefArray> pdrgpcrInner,
		gpos::Ref<CExpression> pexprCorrelatedNLJoin,
		CDXLColRefArray *dxl_colref_array,
		gpos::Ref<CDistributionSpecArray> pdrgpdsBaseTables,
		ULONG *pulNonGatherMotions, BOOL *pfDML);

	// compute the direct dispatch info for the given DML expression
	gpos::Ref<CDXLDirectDispatchInfo> GetDXLDirectDispatchInfo(
		gpos::Ref<CExpression> pexprDML);

	// check if result node imposes a motion hazard
	BOOL FNeedsMaterializeUnderResult(gpos::Ref<CDXLNode> proj_list_dxlnode,
									  gpos::Ref<CDXLNode> child_dxlnode);

	void AddPartForScanId(ULONG scanid, ULONG index);

	// helper to find subplan type from a correlated left outer join expression
	static EdxlSubPlanType EdxlsubplantypeCorrelatedLOJ(
		gpos::Ref<CExpression> pexprCorrelatedLOJ);

	// helper to find subplan type from a correlated join expression
	static EdxlSubPlanType Edxlsubplantype(
		gpos::Ref<CExpression> pexprCorrelatedNLJoin);

	// add used columns in the bitmap re-check and the remaining scalar filter condition to the
	// required output column
	static void AddBitmapFilterColumns(
		CMemoryPool *mp, gpos::Ref<CPhysicalScan> pop,
		gpos::Ref<CExpression> pexprRecheckCond,
		gpos::Ref<CExpression> pexprScalar,
		gpos::Ref<CColRefSet>
			pcrsReqdOutput	// append the required column reference
	);

public:
	// ctor
	CTranslatorExprToDXL(CMemoryPool *mp, CMDAccessor *md_accessor,
						 gpos::Ref<IntPtrArray> pdrgpiSegments,
						 BOOL fInitColumnFactory = true);

	// dtor
	~CTranslatorExprToDXL();

	// main driver
	gpos::Ref<CDXLNode> PdxlnTranslate(gpos::Ref<CExpression> pexpr,
									   gpos::Ref<CColRefArray> colref_array,
									   gpos::Ref<CMDNameArray> pdrgpmdname);

	// translate a scalar expression into a DXL scalar node
	// if the expression is not a scalar, an UnsupportedOp exception is raised
	gpos::Ref<CDXLNode> PdxlnScalar(gpos::Ref<CExpression> pexpr);
};
}  // namespace gpopt

#endif	// !GPOPT_CTranslatorExprToDXL_H

// EOF
