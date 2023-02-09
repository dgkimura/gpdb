//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CTranslatorDXLToExpr.h
//
//	@doc:
//		Class providing methods for translating from DXL tree to Expr Tree.
//---------------------------------------------------------------------------

#ifndef GPOPT_CTranslatorDXLToExpr_H
#define GPOPT_CTranslatorDXLToExpr_H

#include "gpos/base.h"
#include "gpos/common/CHashMap.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CQueryContext.h"
#include "gpopt/base/CWindowFrame.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CScalarArrayRefIndexList.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarWindowFunc.h"
#include "gpopt/translate/CTranslatorDXLToExprUtils.h"
#include "naucrates/dxl/operators/CDXLColDescr.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLScalarArrayRefIndexList.h"
#include "naucrates/dxl/operators/CDXLScalarBoolExpr.h"
#include "naucrates/dxl/operators/CDXLScalarWindowFrameEdge.h"
#include "naucrates/dxl/operators/CDXLScalarWindowRef.h"
#include "naucrates/dxl/operators/CDXLWindowFrame.h"

// fwd decl

namespace gpdxl
{
class CDXLTableDescr;
class CDXLLogicalCTAS;
}  // namespace gpdxl

class CColumnFactory;

namespace gpopt
{
using namespace gpos;
using namespace gpmd;
using namespace gpdxl;

// hash maps
using UlongToExprArrayMap =
	CHashMap<ULONG, CExpressionArray, gpos::HashValue<ULONG>,
			 gpos::Equals<ULONG>, CleanupDelete<ULONG>, CleanupNULL>;

// iterator
using UlongToExprArrayMapIter =
	CHashMapIter<ULONG, CExpressionArray, gpos::HashValue<ULONG>,
				 gpos::Equals<ULONG>, CleanupDelete<ULONG>, CleanupNULL>;


//---------------------------------------------------------------------------
//	@class:
//		CTranslatorDXLToExpr
//
//	@doc:
//		Class providing methods for translating from DXL tree to Expr Tree.
//
//---------------------------------------------------------------------------
class CTranslatorDXLToExpr
{
private:
	// memory pool
	CMemoryPool *m_mp;

	// source system id
	CSystemId m_sysid;

	CMDAccessor *m_pmda;

	// mappings DXL ColId -> CColRef used to process scalar expressions
	gpos::Ref<UlongToColRefMap> m_phmulcr;

	// mappings CTE Id (in DXL) -> CTE Id (in expr)
	gpos::Ref<UlongToUlongMap> m_phmululCTE;

	// array of output ColRefId
	gpos::Ref<ULongPtrArray> m_pdrgpulOutputColRefs;

	// array of output column names
	gpos::Ref<CMDNameArray> m_pdrgpmdname;

	// maintains the mapping between CTE identifier and DXL representation of the corresponding CTE producer
	gpos::Ref<IdToCDXLNodeMap> m_phmulpdxlnCTEProducer;

	// id of CTE that we are currently processing (gpos::ulong_max for main query)
	ULONG m_ulCTEId;

	// a copy of the pointer to column factory, obtained at construction time
	CColumnFactory *m_pcf;

	// private copy ctor
	CTranslatorDXLToExpr(const CTranslatorDXLToExpr &);

	// collapse a not node based on its child, return NULL if it is not collapsible.
	gpos::Ref<CExpression> PexprCollapseNot(
		const gpos::Ref<CDXLNode> pdxlnBoolExpr);

	// helper for creating quantified subquery
	gpos::Ref<CExpression> PexprScalarSubqueryQuantified(
		Edxlopid edxlopid, gpos::Ref<IMDId> scalar_op_mdid,
		const CWStringConst *str, ULONG colid,
		gpos::Ref<CDXLNode> pdxlnLogicalChild,
		gpos::Ref<CDXLNode> pdxlnScalarChild);

	// translate a logical DXL operator into an optimizer expression
	gpos::Ref<CExpression> PexprLogical(const gpos::Ref<CDXLNode> dxlnode);

	// translate a DXL logical select into an expr logical select
	gpos::Ref<CExpression> PexprLogicalSelect(
		const gpos::Ref<CDXLNode> pdxlnLgSelect);

	// translate a DXL logical project into an expr logical project
	gpos::Ref<CExpression> PexprLogicalProject(
		const gpos::Ref<CDXLNode> pdxlnLgProject);

	// translate a DXL logical window into an expr logical project
	gpos::Ref<CExpression> PexprLogicalSeqPr(
		const gpos::Ref<CDXLNode> pdxlnLgProject);

	// create the array of column reference used in the partition by column
	// list of a window specification
	gpos::Ref<CColRefArray> PdrgpcrPartitionByCol(
		const ULongPtrArray *partition_by_colid_array);

	// translate a DXL logical window into an expr logical project
	gpos::Ref<CExpression> PexprCreateWindow(
		const gpos::Ref<CDXLNode> pdxlnLgProject);

	// translate a DXL logical set op into an expr logical set op
	gpos::Ref<CExpression> PexprLogicalSetOp(
		const gpos::Ref<CDXLNode> pdxlnLgProject);

	// return a project element on a cast expression
	gpos::Ref<CExpression> PexprCastPrjElem(gpos::Ref<IMDId> pmdidSource,
											gpos::Ref<IMDId> mdid_dest,
											const CColRef *pcrToCast,
											CColRef *pcrToReturn);

	// build expression and columns of SetOpChild
	void BuildSetOpChild(
		const gpos::Ref<CDXLNode> pdxlnSetOp, ULONG child_index,
		gpos::Ref<CExpression>
			*ppexprChild,  // output: generated child expression
		gpos::Ref<CColRefArray>
			*ppdrgpcrChild,	 // output: generated child input columns
		gpos::Ref<CExpressionArray> *
			ppdrgpexprChildProjElems  // output: project elements to remap child input columns
	);

	// preprocess inputs to the set operator (adding casts to columns  when needed)
	gpos::Ref<CExpressionArray> PdrgpexprPreprocessSetOpInputs(
		const gpos::Ref<CDXLNode> dxlnode, CColRef2dArray *pdrgdrgpcrInput,
		ULongPtrArray *pdrgpulOutput);

	// create new column reference and add to the hashmap maintaining
	// the mapping between DXL ColIds and column reference.
	CColRef *PcrCreate(const CColRef *colref, const IMDType *pmdtype,
					   INT type_modifier, BOOL fStoreMapping, ULONG colid);

	// check if we currently support the casting of such column types
	static BOOL FCastingUnknownType(gpos::Ref<IMDId> pmdidSource,
									gpos::Ref<IMDId> mdid_dest);

	// translate a DXL logical get into an expr logical get
	gpos::Ref<CExpression> PexprLogicalGet(
		const gpos::Ref<CDXLNode> pdxlnLgGet);

	// translate a DXL logical func get into an expr logical TVF
	gpos::Ref<CExpression> PexprLogicalTVF(
		const gpos::Ref<CDXLNode> pdxlnLgTVF);

	// translate a DXL logical group by into an expr logical group by
	gpos::Ref<CExpression> PexprLogicalGroupBy(
		const gpos::Ref<CDXLNode> pdxlnLgSelect);

	// translate a DXL limit node into an expr logical limit expression
	gpos::Ref<CExpression> PexprLogicalLimit(
		const gpos::Ref<CDXLNode> pdxlnLgLimit);

	// translate a DXL logical join into an expr logical join
	gpos::Ref<CExpression> PexprLogicalJoin(
		const gpos::Ref<CDXLNode> pdxlnLgJoin);

	// translate a DXL right outer join
	gpos::Ref<CExpression> PexprRightOuterJoin(
		const gpos::Ref<CDXLNode> dxlnode);

	// translate a DXL logical CTE anchor into an expr logical CTE anchor
	gpos::Ref<CExpression> PexprLogicalCTEAnchor(
		const gpos::Ref<CDXLNode> pdxlnLgCTEAnchor);

	// translate a DXL logical CTE producer into an expr logical CTE producer
	gpos::Ref<CExpression> PexprLogicalCTEProducer(
		const gpos::Ref<CDXLNode> pdxlnLgCTEProducer);

	// translate a DXL logical CTE consumer into an expr logical CTE consumer
	gpos::Ref<CExpression> PexprLogicalCTEConsumer(
		const gpos::Ref<CDXLNode> pdxlnLgCTEConsumer);

	// get cte id for the given dxl cte id
	ULONG UlMapCTEId(const ULONG ulIdOld);

	// translate a DXL logical insert into expression
	gpos::Ref<CExpression> PexprLogicalInsert(
		const gpos::Ref<CDXLNode> dxlnode);

	// translate a DXL logical delete into expression
	gpos::Ref<CExpression> PexprLogicalDelete(
		const gpos::Ref<CDXLNode> dxlnode);

	// translate a DXL logical update into expression
	gpos::Ref<CExpression> PexprLogicalUpdate(
		const gpos::Ref<CDXLNode> dxlnode);

	// translate a DXL logical CTAS into an INSERT expression
	gpos::Ref<CExpression> PexprLogicalCTAS(const gpos::Ref<CDXLNode> dxlnode);

	// translate existential subquery
	gpos::Ref<CExpression> PexprScalarSubqueryExistential(
		Edxlopid edxlopid, gpos::Ref<CDXLNode> pdxlnLogicalChild);

	// translate a DXL logical const table into the corresponding optimizer object
	gpos::Ref<CExpression> PexprLogicalConstTableGet(
		const gpos::Ref<CDXLNode> pdxlnConstTableGet);

	// translate a DXL ANY/ALL-quantified subquery into the corresponding subquery expression
	gpos::Ref<CExpression> PexprScalarSubqueryQuantified(
		const gpos::Ref<CDXLNode> pdxlnSubqueryAny);

	// translate a DXL scalar into an expr scalar
	gpos::Ref<CExpression> PexprScalar(const gpos::Ref<CDXLNode> dxlnode);

	// translate a DXL scalar if stmt into a scalar if
	gpos::Ref<CExpression> PexprScalarIf(const gpos::Ref<CDXLNode> pdxlnIf);

	// translate a DXL scalar switch into a scalar switch
	gpos::Ref<CExpression> PexprScalarSwitch(
		const gpos::Ref<CDXLNode> pdxlnSwitch);

	// translate a DXL scalar switch case into a scalar switch case
	gpos::Ref<CExpression> PexprScalarSwitchCase(
		const gpos::Ref<CDXLNode> pdxlnSwitchCase);

	// translate a DXL scalar case test into a scalar case test
	gpos::Ref<CExpression> PexprScalarCaseTest(
		const gpos::Ref<CDXLNode> pdxlnCaseTest);

	// translate a DXL scalar coalesce into a scalar coalesce
	gpos::Ref<CExpression> PexprScalarCoalesce(
		const gpos::Ref<CDXLNode> pdxlnCoalesce);

	// translate a DXL scalar Min/Max into a scalar Min/Max
	gpos::Ref<CExpression> PexprScalarMinMax(
		const gpos::Ref<CDXLNode> pdxlnMinMax);

	// translate a DXL scalar compare into an expr scalar compare
	gpos::Ref<CExpression> PexprScalarCmp(const gpos::Ref<CDXLNode> pdxlnCmp);

	// translate a DXL scalar distinct compare into an expr scalar is distinct from
	gpos::Ref<CExpression> PexprScalarIsDistinctFrom(
		const gpos::Ref<CDXLNode> pdxlnDistCmp);

	// translate a DXL scalar bool expr into scalar bool operator in the optimizer
	gpos::Ref<CExpression> PexprScalarBoolOp(
		const gpos::Ref<CDXLNode> pdxlnBoolExpr);

	// translate a DXL scalar operation into an expr scalar op
	gpos::Ref<CExpression> PexprScalarOp(const gpos::Ref<CDXLNode> pdxlnOpExpr);

	// translate a DXL scalar func expr into scalar func operator in the optimizer
	gpos::Ref<CExpression> PexprScalarFunc(
		const gpos::Ref<CDXLNode> pdxlnFuncExpr);

	// translate a DXL scalar agg ref expr into scalar agg func operator in the optimizer
	gpos::Ref<CExpression> PexprAggFunc(const gpos::Ref<CDXLNode> pdxlnAggref);

	// translate a DXL scalar window ref expr into scalar window function operator in the optimizer
	gpos::Ref<CExpression> PexprWindowFunc(
		const gpos::Ref<CDXLNode> pdxlnWindowRef);

	// translate the DXL representation of the window stage
	static CScalarWindowFunc::EWinStage Ews(EdxlWinStage edxlws);

	// translate the DXL representation of window frame into its respective representation in the optimizer
	gpos::Ref<CWindowFrame> Pwf(const gpos::Ref<CDXLWindowFrame> window_frame);

	// translate the DXL representation of window frame boundary into its respective representation in the optimizer
	static CWindowFrame::EFrameBoundary Efb(EdxlFrameBoundary frame_boundary);

	// translate the DXL representation of window frame exclusion strategy into its respective representation in the optimizer
	static CWindowFrame::EFrameExclusionStrategy Efes(
		EdxlFrameExclusionStrategy edxlfeb);

	// translate a DXL scalar array
	gpos::Ref<CExpression> PexprArray(const gpos::Ref<CDXLNode> dxlnode);

	// translate a DXL scalar arrayref
	gpos::Ref<CExpression> PexprArrayRef(const gpos::Ref<CDXLNode> dxlnode);

	gpos::Ref<CExpression> PexprValuesList(const gpos::Ref<CDXLNode> dxlnode);

	// translate a DXL scalar arrayref index list
	gpos::Ref<CExpression> PexprArrayRefIndexList(
		const gpos::Ref<CDXLNode> dxlnode);

	// translate the arrayref index list type
	static CScalarArrayRefIndexList::EIndexListType Eilt(
		const CDXLScalarArrayRefIndexList::EIndexListBound eilb);

	// translate a DXL scalar array compare
	gpos::Ref<CExpression> PexprArrayCmp(const gpos::Ref<CDXLNode> dxlnode);

	// translate a DXL scalar ident into an expr scalar ident
	gpos::Ref<CExpression> PexprScalarIdent(
		const gpos::Ref<CDXLNode> pdxlnIdent);

	// translate a DXL scalar nullif into a scalar nullif expression
	gpos::Ref<CExpression> PexprScalarNullIf(
		const gpos::Ref<CDXLNode> pdxlnNullIf);

	// translate a DXL scalar null test into a scalar null test
	gpos::Ref<CExpression> PexprScalarNullTest(
		const gpos::Ref<CDXLNode> pdxlnNullTest);

	// translate a DXL scalar boolean test into a scalar boolean test
	gpos::Ref<CExpression> PexprScalarBooleanTest(
		const gpos::Ref<CDXLNode> pdxlnScBoolTest);

	// translate a DXL scalar cast type into a scalar cast type
	gpos::Ref<CExpression> PexprScalarCast(const gpos::Ref<CDXLNode> pdxlnCast);

	// translate a DXL scalar coerce a scalar coerce
	gpos::Ref<CExpression> PexprScalarCoerceToDomain(
		const gpos::Ref<CDXLNode> pdxlnCoerce);

	// translate a DXL scalar coerce a scalar coerce using I/O functions
	gpos::Ref<CExpression> PexprScalarCoerceViaIO(
		const gpos::Ref<CDXLNode> pdxlnCoerce);

	// translate a DXL scalar array coerce expression using given element coerce function
	gpos::Ref<CExpression> PexprScalarArrayCoerceExpr(
		const gpos::Ref<CDXLNode> pdxlnArrayCoerceExpr);

	// translate a DXL scalar subquery operator into a scalar subquery expression
	gpos::Ref<CExpression> PexprScalarSubquery(
		const gpos::Ref<CDXLNode> pdxlnSubquery);

	// translate a DXL scalar const value into a
	// scalar constant representation in optimizer
	gpos::Ref<CExpression> PexprScalarConst(
		const gpos::Ref<CDXLNode> pdxlnConst);

	gpos::Ref<CExpression> PexprSortGroupClause(
		const gpos::Ref<CDXLNode> pdxlnSortGroupClause);

	// translate a DXL project list node into a project list expression
	gpos::Ref<CExpression> PexprScalarProjList(
		const gpos::Ref<CDXLNode> proj_list_dxlnode);

	// translate a DXL project elem node into a project elem expression
	gpos::Ref<CExpression> PexprScalarProjElem(
		const gpos::Ref<CDXLNode> pdxlnProjElem);

	// construct an order spec from a dxl sort col list node
	gpos::Ref<COrderSpec> Pos(const gpos::Ref<CDXLNode> sort_col_list_dxlnode);

	// translate a dxl node into an expression tree
	gpos::Ref<CExpression> Pexpr(const gpos::Ref<CDXLNode> dxlnode);

	// update table descriptor's distribution columns from the MD cache object
	static void AddDistributionColumns(gpos::Ref<CTableDescriptor> ptabdesc,
									   const IMDRelation *pmdrel,
									   IntToUlongMap *phmiulAttnoColMapping);

	// main translation routine for DXL tree -> Expr tree
	gpos::Ref<CExpression> Pexpr(
		const gpos::Ref<CDXLNode> dxlnode,
		const CDXLNodeArray *query_output_dxlnode_array,
		const CDXLNodeArray *cte_producers);

	// translate children of a DXL node
	gpos::Ref<CExpressionArray> PdrgpexprChildren(
		const gpos::Ref<CDXLNode> dxlnode);

	// construct a table descriptor from DXL
	gpos::Ref<CTableDescriptor> Ptabdesc(gpos::Ref<CDXLTableDescr> table_descr);

	// construct a table descriptor for a CTAS operator
	gpos::Ref<CTableDescriptor> PtabdescFromCTAS(CDXLLogicalCTAS *pdxlopCTAS);

	// register MD provider for serving MD relation entry for CTAS
	void RegisterMDRelationCtas(CDXLLogicalCTAS *pdxlopCTAS);

	// create an array of column references from an array of dxl column references
	gpos::Ref<CColRefArray> Pdrgpcr(
		const CDXLColDescrArray *dxl_col_descr_array);

	// construct the mapping between the DXL ColId and CColRef
	void ConstructDXLColId2ColRefMapping(
		const CDXLColDescrArray *dxl_col_descr_array,
		const CColRefArray *colref_array);

	void MarkUnknownColsAsUnused();

	// look up the column reference in the hash map. We raise an exception if
	// the column is not found
	static CColRef *LookupColRef(UlongToColRefMap *colref_mapping, ULONG colid);

public:
	// ctor
	CTranslatorDXLToExpr(CMemoryPool *mp, CMDAccessor *md_accessor,
						 BOOL fInitColumnFactory = true);

	// dtor
	~CTranslatorDXLToExpr();

	// translate the dxl query with its associated output column and CTEs
	gpos::Ref<CExpression> PexprTranslateQuery(
		const gpos::Ref<CDXLNode> dxlnode,
		const CDXLNodeArray *query_output_dxlnode_array,
		const CDXLNodeArray *cte_producers);

	// translate a dxl scalar expression
	gpos::Ref<CExpression> PexprTranslateScalar(
		const gpos::Ref<CDXLNode> dxlnode, CColRefArray *colref_array,
		ULongPtrArray *colids = nullptr);

	// return the array of query output column reference id
	ULongPtrArray *PdrgpulOutputColRefs();

	// return the array of output column names
	CMDNameArray *
	Pdrgpmdname()
	{
		GPOS_ASSERT(nullptr != m_pdrgpmdname);
		return m_pdrgpmdname.get();
	}
};
}  // namespace gpopt

#endif	// !GPOPT_CTranslatorDXLToExpr_H

// EOF
