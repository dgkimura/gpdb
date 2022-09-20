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
	UlongToColRefMap *m_phmulcr;

	// mappings CTE Id (in DXL) -> CTE Id (in expr)
	UlongToUlongMap *m_phmululCTE;

	// array of output ColRefId
	ULongPtrArray *m_pdrgpulOutputColRefs;

	// array of output column names
	CMDNameArray *m_pdrgpmdname;

	// maintains the mapping between CTE identifier and DXL representation of the corresponding CTE producer
	IdToCDXLNodeMap *m_phmulpdxlnCTEProducer;

	// id of CTE that we are currently processing (gpos::ulong_max for main query)
	ULONG m_ulCTEId;

	// a copy of the pointer to column factory, obtained at construction time
	CColumnFactory *m_pcf;

	// private copy ctor
	CTranslatorDXLToExpr(const CTranslatorDXLToExpr &);

	// collapse a not node based on its child, return NULL if it is not collapsible.
	CExpression *PexprCollapseNot(
		gpos::pointer<const CDXLNode *> pdxlnBoolExpr);

	// helper for creating quantified subquery
	CExpression *PexprScalarSubqueryQuantified(
		Edxlopid edxlopid, IMDId *scalar_op_mdid, const CWStringConst *str,
		ULONG colid, CDXLNode *pdxlnLogicalChild, CDXLNode *pdxlnScalarChild);

	// translate a logical DXL operator into an optimizer expression
	CExpression *PexprLogical(gpos::pointer<const CDXLNode *> dxlnode);

	// translate a DXL logical select into an expr logical select
	CExpression *PexprLogicalSelect(
		gpos::pointer<const CDXLNode *> pdxlnLgSelect);

	// translate a DXL logical project into an expr logical project
	CExpression *PexprLogicalProject(
		gpos::pointer<const CDXLNode *> pdxlnLgProject);

	// translate a DXL logical window into an expr logical project
	CExpression *PexprLogicalSeqPr(
		gpos::pointer<const CDXLNode *> pdxlnLgProject);

	// create the array of column reference used in the partition by column
	// list of a window specification
	CColRefArray *PdrgpcrPartitionByCol(
		gpos::pointer<const ULongPtrArray *> partition_by_colid_array);

	// translate a DXL logical window into an expr logical project
	CExpression *PexprCreateWindow(
		gpos::pointer<const CDXLNode *> pdxlnLgProject);

	// translate a DXL logical set op into an expr logical set op
	CExpression *PexprLogicalSetOp(
		gpos::pointer<const CDXLNode *> pdxlnLgProject);

	// return a project element on a cast expression
	CExpression *PexprCastPrjElem(IMDId *pmdidSource, IMDId *mdid_dest,
								  const CColRef *pcrToCast,
								  CColRef *pcrToReturn);

	// build expression and columns of SetOpChild
	void BuildSetOpChild(
		gpos::pointer<const CDXLNode *> pdxlnSetOp, ULONG child_index,
		CExpression **ppexprChild,	   // output: generated child expression
		CColRefArray **ppdrgpcrChild,  // output: generated child input columns
		CExpressionArray **
			ppdrgpexprChildProjElems  // output: project elements to remap child input columns
	);

	// preprocess inputs to the set operator (adding casts to columns  when needed)
	CExpressionArray *PdrgpexprPreprocessSetOpInputs(
		gpos::pointer<const CDXLNode *> dxlnode,
		CColRef2dArray *pdrgdrgpcrInput, ULongPtrArray *pdrgpulOutput);

	// create new column reference and add to the hashmap maintaining
	// the mapping between DXL ColIds and column reference.
	CColRef *PcrCreate(const CColRef *colref,
					   gpos::pointer<const IMDType *> pmdtype,
					   INT type_modifier, BOOL fStoreMapping, ULONG colid);

	// check if we currently support the casting of such column types
	static BOOL FCastingUnknownType(gpos::pointer<IMDId *> pmdidSource,
									gpos::pointer<IMDId *> mdid_dest);

	// translate a DXL logical get into an expr logical get
	CExpression *PexprLogicalGet(gpos::pointer<const CDXLNode *> pdxlnLgGet);

	// translate a DXL logical func get into an expr logical TVF
	CExpression *PexprLogicalTVF(gpos::pointer<const CDXLNode *> pdxlnLgTVF);

	// translate a DXL logical group by into an expr logical group by
	CExpression *PexprLogicalGroupBy(
		gpos::pointer<const CDXLNode *> pdxlnLgSelect);

	// translate a DXL limit node into an expr logical limit expression
	CExpression *PexprLogicalLimit(
		gpos::pointer<const CDXLNode *> pdxlnLgLimit);

	// translate a DXL logical join into an expr logical join
	CExpression *PexprLogicalJoin(gpos::pointer<const CDXLNode *> pdxlnLgJoin);

	// translate a DXL right outer join
	CExpression *PexprRightOuterJoin(gpos::pointer<const CDXLNode *> dxlnode);

	// translate a DXL logical CTE anchor into an expr logical CTE anchor
	CExpression *PexprLogicalCTEAnchor(
		gpos::pointer<const CDXLNode *> pdxlnLgCTEAnchor);

	// translate a DXL logical CTE producer into an expr logical CTE producer
	CExpression *PexprLogicalCTEProducer(
		gpos::pointer<const CDXLNode *> pdxlnLgCTEProducer);

	// translate a DXL logical CTE consumer into an expr logical CTE consumer
	CExpression *PexprLogicalCTEConsumer(
		gpos::pointer<const CDXLNode *> pdxlnLgCTEConsumer);

	// get cte id for the given dxl cte id
	ULONG UlMapCTEId(const ULONG ulIdOld);

	// translate a DXL logical insert into expression
	CExpression *PexprLogicalInsert(gpos::pointer<const CDXLNode *> dxlnode);

	// translate a DXL logical delete into expression
	CExpression *PexprLogicalDelete(gpos::pointer<const CDXLNode *> dxlnode);

	// translate a DXL logical update into expression
	CExpression *PexprLogicalUpdate(gpos::pointer<const CDXLNode *> dxlnode);

	// translate a DXL logical CTAS into an INSERT expression
	CExpression *PexprLogicalCTAS(gpos::pointer<const CDXLNode *> dxlnode);

	// translate existential subquery
	CExpression *PexprScalarSubqueryExistential(Edxlopid edxlopid,
												CDXLNode *pdxlnLogicalChild);

	// translate a DXL logical const table into the corresponding optimizer object
	CExpression *PexprLogicalConstTableGet(
		gpos::pointer<const CDXLNode *> pdxlnConstTableGet);

	// translate a DXL ANY/ALL-quantified subquery into the corresponding subquery expression
	CExpression *PexprScalarSubqueryQuantified(
		gpos::pointer<const CDXLNode *> pdxlnSubqueryAny);

	// translate a DXL scalar into an expr scalar
	CExpression *PexprScalar(gpos::pointer<const CDXLNode *> dxlnode);

	// translate a DXL scalar if stmt into a scalar if
	CExpression *PexprScalarIf(gpos::pointer<const CDXLNode *> pdxlnIf);

	// translate a DXL scalar switch into a scalar switch
	CExpression *PexprScalarSwitch(gpos::pointer<const CDXLNode *> pdxlnSwitch);

	// translate a DXL scalar switch case into a scalar switch case
	CExpression *PexprScalarSwitchCase(
		gpos::pointer<const CDXLNode *> pdxlnSwitchCase);

	// translate a DXL scalar case test into a scalar case test
	CExpression *PexprScalarCaseTest(
		gpos::pointer<const CDXLNode *> pdxlnCaseTest);

	// translate a DXL scalar coalesce into a scalar coalesce
	CExpression *PexprScalarCoalesce(
		gpos::pointer<const CDXLNode *> pdxlnCoalesce);

	// translate a DXL scalar Min/Max into a scalar Min/Max
	CExpression *PexprScalarMinMax(gpos::pointer<const CDXLNode *> pdxlnMinMax);

	// translate a DXL scalar compare into an expr scalar compare
	CExpression *PexprScalarCmp(gpos::pointer<const CDXLNode *> pdxlnCmp);

	// translate a DXL scalar distinct compare into an expr scalar is distinct from
	CExpression *PexprScalarIsDistinctFrom(
		gpos::pointer<const CDXLNode *> pdxlnDistCmp);

	// translate a DXL scalar bool expr into scalar bool operator in the optimizer
	CExpression *PexprScalarBoolOp(
		gpos::pointer<const CDXLNode *> pdxlnBoolExpr);

	// translate a DXL scalar operation into an expr scalar op
	CExpression *PexprScalarOp(gpos::pointer<const CDXLNode *> pdxlnOpExpr);

	// translate a DXL scalar func expr into scalar func operator in the optimizer
	CExpression *PexprScalarFunc(gpos::pointer<const CDXLNode *> pdxlnFuncExpr);

	// translate a DXL scalar agg ref expr into scalar agg func operator in the optimizer
	CExpression *PexprAggFunc(gpos::pointer<const CDXLNode *> pdxlnAggref);

	// translate a DXL scalar window ref expr into scalar window function operator in the optimizer
	CExpression *PexprWindowFunc(
		gpos::pointer<const CDXLNode *> pdxlnWindowRef);

	// translate the DXL representation of the window stage
	static CScalarWindowFunc::EWinStage Ews(EdxlWinStage edxlws);

	// translate the DXL representation of window frame into its respective representation in the optimizer
	CWindowFrame *Pwf(gpos::pointer<const CDXLWindowFrame *> window_frame);

	// translate the DXL representation of window frame boundary into its respective representation in the optimizer
	static CWindowFrame::EFrameBoundary Efb(EdxlFrameBoundary frame_boundary);

	// translate the DXL representation of window frame exclusion strategy into its respective representation in the optimizer
	static CWindowFrame::EFrameExclusionStrategy Efes(
		EdxlFrameExclusionStrategy edxlfeb);

	// translate a DXL scalar array
	CExpression *PexprArray(gpos::pointer<const CDXLNode *> dxlnode);

	// translate a DXL scalar arrayref
	CExpression *PexprArrayRef(gpos::pointer<const CDXLNode *> dxlnode);

	CExpression *PexprValuesList(gpos::pointer<const CDXLNode *> dxlnode);

	// translate a DXL scalar arrayref index list
	CExpression *PexprArrayRefIndexList(
		gpos::pointer<const CDXLNode *> dxlnode);

	// translate the arrayref index list type
	static CScalarArrayRefIndexList::EIndexListType Eilt(
		const CDXLScalarArrayRefIndexList::EIndexListBound eilb);

	// translate a DXL scalar array compare
	CExpression *PexprArrayCmp(gpos::pointer<const CDXLNode *> dxlnode);

	// translate a DXL scalar ident into an expr scalar ident
	CExpression *PexprScalarIdent(gpos::pointer<const CDXLNode *> pdxlnIdent);

	// translate a DXL scalar nullif into a scalar nullif expression
	CExpression *PexprScalarNullIf(gpos::pointer<const CDXLNode *> pdxlnNullIf);

	// translate a DXL scalar null test into a scalar null test
	gpos::owner<CExpression *> PexprScalarNullTest(
		gpos::pointer<const CDXLNode *> pdxlnNullTest);

	// translate a DXL scalar boolean test into a scalar boolean test
	CExpression *PexprScalarBooleanTest(
		gpos::pointer<const CDXLNode *> pdxlnScBoolTest);

	// translate a DXL scalar cast type into a scalar cast type
	CExpression *PexprScalarCast(gpos::pointer<const CDXLNode *> pdxlnCast);

	// translate a DXL scalar coerce a scalar coerce
	CExpression *PexprScalarCoerceToDomain(
		gpos::pointer<const CDXLNode *> pdxlnCoerce);

	// translate a DXL scalar coerce a scalar coerce using I/O functions
	CExpression *PexprScalarCoerceViaIO(
		gpos::pointer<const CDXLNode *> pdxlnCoerce);

	// translate a DXL scalar array coerce expression using given element coerce function
	CExpression *PexprScalarArrayCoerceExpr(
		gpos::pointer<const CDXLNode *> pdxlnArrayCoerceExpr);

	// translate a DXL scalar subquery operator into a scalar subquery expression
	CExpression *PexprScalarSubquery(
		gpos::pointer<const CDXLNode *> pdxlnSubquery);

	// translate a DXL scalar const value into a
	// scalar constant representation in optimizer
	CExpression *PexprScalarConst(gpos::pointer<const CDXLNode *> pdxlnConst);

	CExpression *PexprSortGroupClause(
		gpos::pointer<const CDXLNode *> pdxlnSortGroupClause);

	// translate a DXL project list node into a project list expression
	CExpression *PexprScalarProjList(
		gpos::pointer<const CDXLNode *> proj_list_dxlnode);

	// translate a DXL project elem node into a project elem expression
	CExpression *PexprScalarProjElem(
		gpos::pointer<const CDXLNode *> pdxlnProjElem);

	// construct an order spec from a dxl sort col list node
	COrderSpec *Pos(gpos::pointer<const CDXLNode *> sort_col_list_dxlnode);

	// translate a dxl node into an expression tree
	CExpression *Pexpr(gpos::pointer<const CDXLNode *> dxlnode);

	// update table descriptor's distribution columns from the MD cache object
	static void AddDistributionColumns(
		CTableDescriptor *ptabdesc, gpos::pointer<const IMDRelation *> pmdrel,
		IntToUlongMap *phmiulAttnoColMapping);

	// main translation routine for DXL tree -> Expr tree
	CExpression *Pexpr(
		gpos::pointer<const CDXLNode *> dxlnode,
		gpos::pointer<const CDXLNodeArray *> query_output_dxlnode_array,
		gpos::pointer<const CDXLNodeArray *> cte_producers);

	// translate children of a DXL node
	CExpressionArray *PdrgpexprChildren(
		gpos::pointer<const CDXLNode *> dxlnode);

	// construct a table descriptor from DXL
	CTableDescriptor *Ptabdesc(CDXLTableDescr *table_descr);

	// construct a table descriptor for a CTAS operator
	CTableDescriptor *PtabdescFromCTAS(CDXLLogicalCTAS *pdxlopCTAS);

	// register MD provider for serving MD relation entry for CTAS
	void RegisterMDRelationCtas(CDXLLogicalCTAS *pdxlopCTAS);

	// create an array of column references from an array of dxl column references
	CColRefArray *Pdrgpcr(
		gpos::pointer<const CDXLColDescrArray *> dxl_col_descr_array);

	// construct the mapping between the DXL ColId and CColRef
	void ConstructDXLColId2ColRefMapping(
		gpos::pointer<const CDXLColDescrArray *> dxl_col_descr_array,
		gpos::pointer<const CColRefArray *> colref_array);

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
	CExpression *PexprTranslateQuery(
		gpos::pointer<const CDXLNode *> dxlnode,
		gpos::pointer<const CDXLNodeArray *> query_output_dxlnode_array,
		gpos::pointer<const CDXLNodeArray *> cte_producers);

	// translate a dxl scalar expression
	CExpression *PexprTranslateScalar(gpos::pointer<const CDXLNode *> dxlnode,
									  CColRefArray *colref_array,
									  ULongPtrArray *colids = nullptr);

	// return the array of query output column reference id
	ULongPtrArray *PdrgpulOutputColRefs();

	// return the array of output column names
	CMDNameArray *
	Pdrgpmdname()
	{
		GPOS_ASSERT(nullptr != m_pdrgpmdname);
		return m_pdrgpmdname;
	}
};
}  // namespace gpopt

#endif	// !GPOPT_CTranslatorDXLToExpr_H

// EOF
