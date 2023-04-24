//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2012 EMC Corp.
//
//	@filename:
//		CXformUtils.h
//
//	@doc:
//		Utility functions for xforms
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformUtils_H
#define GPOPT_CXformUtils_H

#include "gpos/base.h"

#include "gpopt/base/CCastUtils.h"
#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalDML.h"
#include "gpopt/operators/CLogicalDynamicIndexGet.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalIndexGet.h"
#include "gpopt/operators/CLogicalJoin.h"
#include "gpopt/operators/CPhysicalJoin.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/xforms/CXform.h"
#include "naucrates/md/IMDIndex.h"
#include "naucrates/md/IMDScalarOp.h"

namespace gpopt
{
using namespace gpos;

// forward declarations
class CGroupExpression;
class CExpression;
class CLogical;
class CLogicalDynamicGet;
class CPartConstraint;
class CTableDescriptor;

// map of expression to array of expressions
using ExprToExprArrayMap =
	CHashMap<CExpression, CExpressionArray, CExpression::HashValue,
			 CUtils::Equals, CleanupRelease<CExpression>,
			 CleanupRelease<CExpressionArray>>;

// iterator of map of expression to array of expressions
using ExprToExprArrayMapIter =
	CHashMapIter<CExpression, CExpressionArray, CExpression::HashValue,
				 CUtils::Equals, CleanupRelease<CExpression>,
				 CleanupRelease<CExpressionArray>>;

// array of array of expressions
using CExpressionArrays = CDynamicPtrArray<CExpressionArray, CleanupRelease>;

//---------------------------------------------------------------------------
//	@class:
//		CXformUtils
//
//	@doc:
//		Utility functions for xforms
//
//---------------------------------------------------------------------------
class CXformUtils
{
private:
	// enum marking the index column types
	enum EIndexCols
	{
		EicKey,
		EicIncluded
	};

	// create a logical assert for the not nullable columns of the given table
	// on top of the given child expression
	static gpos::Ref<CExpression> PexprAssertNotNull(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprChild,
		gpos::Ref<CTableDescriptor> ptabdesc, CColRefArray *colref_array);

	// create a logical assert for the check constraints on the given table
	static gpos::Ref<CExpression> PexprAssertCheckConstraints(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprChild,
		gpos::Ref<CTableDescriptor> ptabdesc, CColRefArray *colref_array);

	// helper for extracting foreign key
	static gpos::Ref<CColRefSet> PcrsFKey(CMemoryPool *mp,
										  CExpressionArray *pdrgpexpr,
										  gpos::Ref<CColRefSet> prcsOutput,
										  gpos::Ref<CColRefSet> pcrsKey);

	// return the set of columns from the given array of columns which appear
	// in the index included / key columns
	static gpos::Ref<CColRefSet> PcrsIndexColumns(
		CMemoryPool *mp, CColRefArray *colref_array,
		const gpos::Ref<IMDIndex> pmdindex, const gpos::Ref<IMDRelation> pmdrel,
		EIndexCols eic);

	// return the ordered array of columns from the given array of columns which appear
	// in the index included / key columns
	static CColRefArray *PdrgpcrIndexColumns(
		CMemoryPool *mp, CColRefArray *colref_array,
		const gpos::Ref<IMDIndex> pmdindex, const gpos::Ref<IMDRelation> pmdrel,
		EIndexCols eic);

	// lookup join keys in scalar child group
	static void LookupJoinKeys(CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
							   CExpressionArray **ppdrgpexprOuter,
							   CExpressionArray **ppdrgpexprInner,
							   IMdIdArray **join_opfamilies);

	// cache join keys on scalar child group
	static void CacheJoinKeys(gpos::Ref<CExpression> pexpr,
							  CExpressionArray *pdrgpexprOuter,
							  CExpressionArray *pdrgpexprInner,
							  IMdIdArray *join_opfamilies);

	// helper to extract equality from a given expression
	static BOOL FExtractEquality(
		gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpression> *
			ppexprEquality,	 // output: extracted equality expression, set to NULL if extraction failed
		gpos::Ref<CExpression> *
			ppexprOther	 // output: sibling of equality expression, set to NULL if extraction failed
	);

	// check if given xform id is in the given array of xforms
	static BOOL FXformInArray(CXform::EXformId exfid,
							  const CXform::EXformId rgXforms[],
							  ULONG ulXforms);

#ifdef GPOS_DEBUG
	// check whether the given join type is swapable
	static BOOL FSwapableJoinType(COperator::EOperatorId op_id);
#endif	// GPOS_DEBUG

	// helper function for adding hash join alternative
	template <class T>
	static void AddHashOrMergeJoinAlternative(CMemoryPool *mp,
											  gpos::Ref<CExpression> pexprJoin,
											  CExpressionArray *pdrgpexprOuter,
											  CExpressionArray *pdrgpexprInner,
											  IMdIdArray *join_opfamilies,
											  gpos::Ref<CXformResult> pxfres,
											  BOOL is_hash_join_null_aware);

	// helper for transforming SubqueryAll into aggregate subquery
	static void SubqueryAllToAgg(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprSubquery,
		gpos::Ref<CExpression>
			*ppexprNewSubquery,	 // output argument for new scalar subquery
		gpos::Ref<CExpression>
			*ppexprNewScalar  // output argument for new scalar expression
	);

	// helper for transforming SubqueryAny into aggregate subquery
	static void SubqueryAnyToAgg(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprSubquery,
		gpos::Ref<CExpression>
			*ppexprNewSubquery,	 // output argument for new scalar subquery
		gpos::Ref<CExpression>
			*ppexprNewScalar  // output argument for new scalar expression
	);

	// create the Gb operator to be pushed below a join
	static gpos::Ref<CLogicalGbAgg> PopGbAggPushableBelowJoin(
		CMemoryPool *mp, gpos::Ref<CLogicalGbAgg> popGbAggOld,
		gpos::Ref<CColRefSet> prcsOutput, gpos::Ref<CColRefSet> pcrsGrpCols);

	// check if the preconditions for pushing down Group by through join are satisfied
	static BOOL FCanPushGbAggBelowJoin(
		gpos::Ref<CColRefSet> pcrsGrpCols,
		gpos::Ref<CColRefSet> pcrsJoinOuterChildOutput,
		gpos::Ref<CColRefSet> pcrsJoinScalarUsedFromOuter,
		gpos::Ref<CColRefSet> pcrsGrpByOutput,
		gpos::Ref<CColRefSet> pcrsGrpByUsed, gpos::Ref<CColRefSet> pcrsFKey);

	// construct an expression representing a new access path using the given functors for
	// operator constructors and rewritten access path
	static gpos::Ref<CExpression> PexprBuildBtreeIndexPlan(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::Ref<CExpression> pexprGet, ULONG ulOriginOpId,
		CExpressionArray *pdrgpexprConds, gpos::Ref<CColRefSet> pcrsReqd,
		gpos::Ref<CColRefSet> pcrsScalarExpr, gpos::Ref<CColRefSet> outer_refs,
		const gpos::Ref<IMDIndex> pmdindex,
		const gpos::Ref<IMDRelation> pmdrel);

	// create a dynamic operator for a btree index plan
	static gpos::Ref<CLogical>
	PopDynamicBtreeIndexOpConstructor(
		CMemoryPool *mp, const gpos::Ref<IMDIndex> pmdindex,
		gpos::Ref<CTableDescriptor> ptabdesc, ULONG ulOriginOpId, CName *pname,
		ULONG ulPartIndex, CColRefArray *pdrgpcrOutput,
		CColRef2dArray *pdrgpdrgpcrPart, IMdIdArray *partition_mdids)
	{
		return GPOS_NEW(mp) CLogicalDynamicIndexGet(
			mp, pmdindex, ptabdesc, ulOriginOpId, pname, ulPartIndex,
			pdrgpcrOutput, pdrgpdrgpcrPart, partition_mdids);
	}

	//	create a static operator for a btree index plan
	static gpos::Ref<CLogical>
	PopStaticBtreeIndexOpConstructor(CMemoryPool *mp,
									 const gpos::Ref<IMDIndex> pmdindex,
									 gpos::Ref<CTableDescriptor> ptabdesc,
									 ULONG ulOriginOpId, CName *pname,
									 CColRefArray *pdrgpcrOutput)
	{
		return GPOS_NEW(mp) CLogicalIndexGet(
			mp, pmdindex, ptabdesc, ulOriginOpId, pname, pdrgpcrOutput);
	}

	//	produce an expression representing a new btree index path
	static gpos::Ref<CExpression>
	PexprRewrittenBtreeIndexPath(CMemoryPool *mp,
								 gpos::Ref<CExpression> pexprIndexCond,
								 gpos::Ref<CExpression> pexprResidualCond,
								 const gpos::Ref<IMDIndex>,	   // pmdindex
								 gpos::Ref<CTableDescriptor>,  // ptabdesc
								 gpos::Ref<COperator> popLogical)
	{
		// create the expression containing the logical index get operator
		return CUtils::PexprSafeSelect(
			mp, GPOS_NEW(mp) CExpression(mp, popLogical, pexprIndexCond),
			pexprResidualCond);
	}

	// construct a bitmap index path expression for the given predicate
	// out of the children of the given expression
	static gpos::Ref<CExpression> PexprBitmapLookupWithPredicateBreakDown(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::Ref<CExpression> pexprOriginalPred,
		gpos::Ref<CExpression> pexprPred, gpos::Ref<CTableDescriptor> ptabdesc,
		const gpos::Ref<IMDRelation> pmdrel, CColRefArray *pdrgpcrOutput,
		gpos::Ref<CColRefSet> outer_refs, gpos::Ref<CColRefSet> pcrsReqd,
		gpos::Ref<CExpression> *ppexprRecheck,
		gpos::Ref<CExpression> *ppexprResidual);

	// compute the residual predicate for a bitmap table scan
	static void ComputeBitmapTableScanResidualPredicate(
		CMemoryPool *mp, BOOL fConjunction,
		gpos::Ref<CExpression> pexprOriginalPred,
		gpos::Ref<CExpression> *ppexprResidual,
		CExpressionArray *pdrgpexprResidualNew);

	// construct a bitmap index path expression for the given predicate coming
	// from a condition without outer references
	static gpos::Ref<CExpression> PexprBitmapSelectBestIndex(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::Ref<CExpression> pexprPred, gpos::Ref<CTableDescriptor> ptabdesc,
		const gpos::Ref<IMDRelation> pmdrel, CColRefArray *pdrgpcrOutput,
		gpos::Ref<CColRefSet> pcrsReqd, gpos::Ref<CColRefSet> pcrsOuterRefs,
		gpos::Ref<CExpression> *ppexprRecheck,
		gpos::Ref<CExpression> *ppexprResidual, BOOL alsoConsiderBTreeIndexes);

	// iterate over given hash map and return array of arrays of project elements sorted by the column id of the first entries
	static CExpressionArrays *PdrgpdrgpexprSortedPrjElemsArray(
		CMemoryPool *mp, ExprToExprArrayMap *phmexprdrgpexpr);

	// comparator used in sorting arrays of project elements based on the column id of the first entry
	static INT ICmpPrjElemsArr(const void *pvFst, const void *pvSnd);

public:
	CXformUtils(const CXformUtils &) = delete;

	// helper function for implementation xforms on binary operators
	// with predicates (e.g. joins)
	template <class T>
	static void TransformImplementBinaryOp(gpos::Ref<CXformContext> pxfctxt,
										   gpos::Ref<CXformResult> pxfres,
										   gpos::Ref<CExpression> pexpr);

	// helper function for implementation of hash joins
	template <class T>
	static void ImplementHashJoin(gpos::Ref<CXformContext> pxfctxt,
								  gpos::Ref<CXformResult> pxfres,
								  gpos::Ref<CExpression> pexpr);

	// helper function for implementation of merge joins
	template <class T>
	static void ImplementMergeJoin(gpos::Ref<CXformContext> pxfctxt,
								   gpos::Ref<CXformResult> pxfres,
								   gpos::Ref<CExpression> pexpr);

	// helper function for implementation of nested loops joins
	template <class T>
	static void ImplementNLJoin(gpos::Ref<CXformContext> pxfctxt,
								gpos::Ref<CXformResult> pxfres,
								gpos::Ref<CExpression> pexpr);

	// helper for removing IsNotFalse join predicate for GPDB anti-semi hash join
	static BOOL FProcessGPDBAntiSemiHashJoin(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpression> *ppexprResult);

	// check the applicability of logical join to physical join xform
	static CXform::EXformPromise ExfpLogicalJoin2PhysicalJoin(
		CExpressionHandle &exprhdl);

	// check the applicability of semi join to cross product
	static CXform::EXformPromise ExfpSemiJoin2CrossProduct(
		CExpressionHandle &exprhdl);

	// check the applicability of N-ary join expansion
	static CXform::EXformPromise ExfpExpandJoinOrder(
		CExpressionHandle &exprhdl, const gpos::Ref<CXform> xform);

	// extract foreign key
	static gpos::Ref<CColRefSet> PcrsFKey(CMemoryPool *mp,
										  gpos::Ref<CExpression> pexprOuter,
										  gpos::Ref<CExpression> pexprInner,
										  gpos::Ref<CExpression> pexprScalar);

	// compute a swap of the two given joins
	static gpos::Ref<CExpression> PexprSwapJoins(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprTopJoin,
		gpos::Ref<CExpression> pexprBottomJoin);

	// push a Gb, optionally with a having clause below the child join
	static gpos::Ref<CExpression> PexprPushGbBelowJoin(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// check if the the array of aligned input columns are of the same type
	static BOOL FSameDatatype(CColRef2dArray *pdrgpdrgpcrInput);

	// helper function to separate subquery predicates in a top Select node
	static gpos::Ref<CExpression> PexprSeparateSubqueryPreds(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// helper for creating inverse predicate for unnesting subquery ALL
	static gpos::Ref<CExpression> PexprInversePred(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprSubquery);

	// helper for creating a null indicator expression
	static gpos::Ref<CExpression> PexprNullIndicator(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// helper for creating a logical DML on top of a project
	static gpos::Ref<CExpression> PexprLogicalDMLOverProject(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprChild,
		CLogicalDML::EDMLOperator edmlop, gpos::Ref<CTableDescriptor> ptabdesc,
		CColRefArray *colref_array, gpos::Ref<CColRef> pcrCtid,
		gpos::Ref<CColRef> pcrSegmentId);

	// return partition filter expressions given a table
	// descriptor and the given column references
	static CExpressionArray *PdrgpexprPartEqFilters(
		CMemoryPool *mp, gpos::Ref<CTableDescriptor> ptabdesc,
		CColRefArray *pdrgpcrSource);

	// helper for creating Agg expression equivalent to quantified subquery
	static void QuantifiedToAgg(CMemoryPool *mp,
								gpos::Ref<CExpression> pexprSubquery,
								gpos::Ref<CExpression> *ppexprNewSubquery,
								gpos::Ref<CExpression> *ppexprNewScalar);

	// helper for creating Agg expression equivalent to existential subquery
	static void ExistentialToAgg(CMemoryPool *mp,
								 gpos::Ref<CExpression> pexprSubquery,
								 gpos::Ref<CExpression> *ppexprNewSubquery,
								 gpos::Ref<CExpression> *ppexprNewScalar);

	// create a logical assert for the check constraints on the given table
	static gpos::Ref<CExpression> PexprAssertConstraints(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprChild,
		gpos::Ref<CTableDescriptor> ptabdesc, CColRefArray *colref_array);

	// return true if stats derivation is needed for this xform
	static BOOL FDeriveStatsBeforeXform(gpos::Ref<CXform> pxform);

	// return true if xform is a subquery decorrelation xform
	static BOOL FSubqueryDecorrelation(gpos::Ref<CXform> pxform);

	// return true if xform is a subquery unnesting xform
	static BOOL FSubqueryUnnesting(gpos::Ref<CXform> pxform);

	// return true if xform should be applied to the next binding
	static BOOL FApplyToNextBinding(gpos::Ref<CXform> pxform,
									gpos::Ref<CExpression> pexprLastBinding);

	// return a formatted error message for the given exception
	static CWStringConst *PstrErrorMessage(CMemoryPool *mp, ULONG major,
										   ULONG minor, ...);

	// return the array of key columns from the given array of columns which appear
	// in the index key columns
	static CColRefArray *PdrgpcrIndexKeys(CMemoryPool *mp,
										  CColRefArray *colref_array,
										  const gpos::Ref<IMDIndex> pmdindex,
										  const gpos::Ref<IMDRelation> pmdrel);

	// return the set of key columns from the given array of columns which appear
	// in the index key columns
	static gpos::Ref<CColRefSet> PcrsIndexKeys(
		CMemoryPool *mp, CColRefArray *colref_array,
		const gpos::Ref<IMDIndex> pmdindex,
		const gpos::Ref<IMDRelation> pmdrel);

	// return the set of key columns from the given array of columns which appear
	// in the index included columns
	static gpos::Ref<CColRefSet> PcrsIndexIncludedCols(
		CMemoryPool *mp, CColRefArray *colref_array,
		const gpos::Ref<IMDIndex> pmdindex,
		const gpos::Ref<IMDRelation> pmdrel);

	// check if an index is applicable given the required, output and scalar
	// expression columns
	static BOOL FIndexApplicable(
		CMemoryPool *mp, const gpos::Ref<IMDIndex> pmdindex,
		const gpos::Ref<IMDRelation> pmdrel, CColRefArray *pdrgpcrOutput,
		gpos::Ref<CColRefSet> pcrsReqd, gpos::Ref<CColRefSet> pcrsScalar,
		IMDIndex::EmdindexType emdindtype,
		IMDIndex::EmdindexType altindtype = IMDIndex::EmdindSentinel);

	// check whether a CTE should be inlined
	static BOOL FInlinableCTE(ULONG ulCTEId);

	// return the column reference of the n-th project element
	static gpos::Ref<CColRef> PcrProjectElement(gpos::Ref<CExpression> pexpr,
												ULONG ulIdxProjElement);

	// create an expression with "row_number" window function
	static gpos::Ref<CExpression> PexprRowNumber(CMemoryPool *mp);

	// create a logical sequence project with a "row_number" window function
	static gpos::Ref<CExpression> PexprWindowWithRowNumber(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprWindowChild,
		CColRefArray *pdrgpcrInput);

	// generate a logical Assert expression that errors out when more than one row is generated
	static gpos::Ref<CExpression> PexprAssertOneRow(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprChild);

	// helper for adding CTE producer to global CTE info structure
	static gpos::Ref<CExpression> PexprAddCTEProducer(
		CMemoryPool *mp, ULONG ulCTEId, CColRefArray *colref_array,
		gpos::Ref<CExpression> pexpr);

	// does transformation generate an Apply expression
	static BOOL
	FGenerateApply(CXform::EXformId exfid)
	{
		return CXform::ExfSelect2Apply == exfid ||
			   CXform::ExfProject2Apply == exfid ||
			   CXform::ExfGbAgg2Apply == exfid ||
			   CXform::ExfSubqJoin2Apply == exfid ||
			   CXform::ExfSubqNAryJoin2Apply == exfid ||
			   CXform::ExfSequenceProject2Apply == exfid;
	}

	// helper for creating IndexGet/DynamicIndexGet expression
	static gpos::Ref<CExpression>
	PexprLogicalIndexGet(CMemoryPool *mp, CMDAccessor *md_accessor,
						 gpos::Ref<CExpression> pexprGet, ULONG ulOriginOpId,
						 CExpressionArray *pdrgpexprConds,
						 gpos::Ref<CColRefSet> pcrsReqd,
						 gpos::Ref<CColRefSet> pcrsScalarExpr,
						 gpos::Ref<CColRefSet> outer_refs,
						 const gpos::Ref<IMDIndex> pmdindex,
						 const gpos::Ref<IMDRelation> pmdrel)
	{
		return PexprBuildBtreeIndexPlan(
			mp, md_accessor, pexprGet, ulOriginOpId, pdrgpexprConds, pcrsReqd,
			pcrsScalarExpr, outer_refs, pmdindex, pmdrel);
	}

	// helper for creating bitmap bool op expressions
	static gpos::Ref<CExpression> PexprScalarBitmapBoolOp(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::Ref<CExpression> pexprOriginalPred, CExpressionArray *pdrgpexpr,
		gpos::Ref<CTableDescriptor> ptabdesc,
		const gpos::Ref<IMDRelation> pmdrel, CColRefArray *pdrgpcrOutput,
		gpos::Ref<CColRefSet> outer_refs, gpos::Ref<CColRefSet> pcrsReqd,
		BOOL fConjunction, gpos::Ref<CExpression> *ppexprRecheck,
		gpos::Ref<CExpression> *ppexprResidual, BOOL isAPartialPredicate);

	// construct a bitmap bool op given the left and right bitmap access
	// path expressions
	static gpos::Ref<CExpression> PexprBitmapBoolOp(
		CMemoryPool *mp, gpos::Ref<IMDId> pmdidBitmapType,
		gpos::Ref<CExpression> pexprLeft, gpos::Ref<CExpression> pexprRight,
		BOOL fConjunction);

	// given an array of predicate expressions, construct a bitmap access path
	// expression for each predicate and accumulate it in the pdrgpexprBitmap array
	static void CreateBitmapIndexProbeOps(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::Ref<CExpression> pexprOriginalPred,
		CExpressionArray *pdrgpexprPreds, gpos::Ref<CTableDescriptor> ptabdesc,
		const gpos::Ref<IMDRelation> pmdrel, CColRefArray *pdrgpcrOutput,
		gpos::Ref<CColRefSet> outer_refs, gpos::Ref<CColRefSet> pcrsReqd,
		BOOL fConjunction, CExpressionArray *pdrgpexprBitmap,
		CExpressionArray *pdrgpexprRecheck, CExpressionArray *pdrgpexprResidual,
		BOOL isAPartialPredicate);

	static void CreateBitmapIndexProbesWithOrWithoutPredBreakdown(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::Ref<CExpression> pexprOriginalPred,
		gpos::Ref<CExpression> pexprPred, gpos::Ref<CTableDescriptor> ptabdesc,
		const gpos::Ref<IMDRelation> pmdrel, CColRefArray *pdrgpcrOutput,
		gpos::Ref<CColRefSet> outer_refs, gpos::Ref<CColRefSet> pcrsReqd,
		gpos::Ref<CExpression> *pexprBitmapResult,
		gpos::Ref<CExpression> *pexprRecheckResult,
		CExpressionArray *pdrgpexprResidualResult, BOOL isAPartialPredicate);

	// check if expression has any scalar node with ambiguous return type
	static BOOL FHasAmbiguousType(gpos::Ref<CExpression> pexpr,
								  CMDAccessor *md_accessor);

	// construct a Bitmap(Dynamic)TableGet over BitmapBoolOp for the given
	// logical operator if bitmap indexes exist
	static gpos::Ref<CExpression> PexprBitmapTableGet(
		CMemoryPool *mp, gpos::Ref<CLogical> popGet, ULONG ulOriginOpId,
		gpos::Ref<CTableDescriptor> ptabdesc,
		gpos::Ref<CExpression> pexprScalar, gpos::Ref<CColRefSet> outer_refs,
		gpos::Ref<CColRefSet> pcrsReqd);

	// transform a Select over a (dynamic) table get into a bitmap table scan
	// over bitmap bool op
	static gpos::Ref<CExpression> PexprSelect2BitmapBoolOp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// remap the expression from the old columns to the new ones
	static gpos::Ref<CExpression> PexprRemapColumns(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprScalar,
		CColRefArray *pdrgpcrA, CColRefArray *pdrgpcrRemappedA,
		CColRefArray *pdrgpcrB, CColRefArray *pdrgpcrRemappedB);

	// create a new CTE consumer for the given CTE id
	static gpos::Ref<CExpression> PexprCTEConsumer(
		CMemoryPool *mp, ULONG ulCTEId, CColRefArray *pdrgpcrConsumer);

	// return a new array containing the columns from the given column array 'colref_array'
	// at the positions indicated by the given ULONG array 'pdrgpulIndexesOfRefs'
	// e.g., colref_array = {col1, col2, col3}, pdrgpulIndexesOfRefs = {2, 1}
	// the result will be {col3, col2}
	static CColRefArray *PdrgpcrReorderedSubsequence(
		CMemoryPool *mp, CColRefArray *colref_array,
		ULongPtrArray *pdrgpulIndexesOfRefs);

	// check if given xform is an Agg splitting xform
	static BOOL FSplitAggXform(CXform::EXformId exfid);

	// check if given xform is an Agg CXformSplitDQA
	static BOOL FAggGenBySplitDQAXform(gpos::Ref<CExpression> pexprAgg);

	// check if given expression is a multi-stage Agg based on origin xform
	static BOOL FMultiStageAgg(gpos::Ref<CExpression> pexprAgg);

	static BOOL FLocalAggCreatedByEagerAggXform(
		gpos::Ref<CExpression> pexprAgg);

	// check if expression handle is attached to a Join with a predicate that uses columns from only one child
	static BOOL FJoinPredOnSingleChild(CMemoryPool *mp,
									   CExpressionHandle &exprhdl);

	// add a redundant SELECT node on top of Dynamic (Bitmap) IndexGet to be able to use index
	// predicate in partition elimination
	static gpos::Ref<CExpression> PexprRedundantSelectForDynamicIndex(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// convert an Agg window function into regular Agg
	static gpos::Ref<CExpression> PexprWinFuncAgg2ScalarAgg(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprWinFunc);

	// create a map from the argument of each Distinct Agg to the array of project elements that define Distinct Aggs on the same argument
	static void MapPrjElemsWithDistinctAggs(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprPrjList,
		ExprToExprArrayMap **pphmexprdrgpexpr, ULONG *pulDifferentDQAs);

	// convert GbAgg with distinct aggregates to a join
	static gpos::Ref<CExpression> PexprGbAggOnCTEConsumer2Join(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprGbAgg);

	// combine the individual bitmap access paths to form a bitmap bool op expression
	static void JoinBitmapIndexProbes(CMemoryPool *pmp,
									  CExpressionArray *pdrgpexprBitmapOld,
									  CExpressionArray *pdrgpexprRecheckOld,
									  BOOL fConjunction,
									  gpos::Ref<CExpression> *ppexprBitmap,
									  gpos::Ref<CExpression> *ppexprRecheck);

	static gpos::Ref<CExpression> AddALinearStackOfUnaryExpressions(
		CMemoryPool *mp, gpos::Ref<CExpression> lowerPartOfExpr,
		gpos::Ref<CExpression> topOfStack,
		gpos::Ref<CExpression> exclusiveBottomOfStack);

};	// class CXformUtils


//---------------------------------------------------------------------------
//	@function:
//		CXformUtils::TransformImplementBinaryOp
//
//	@doc:
//		Helper function for implementation xforms on binary operators
//		with predicates (e.g. joins)
//
//---------------------------------------------------------------------------
template <class T>
void
CXformUtils::TransformImplementBinaryOp(gpos::Ref<CXformContext> pxfctxt,
										gpos::Ref<CXformResult> pxfres,
										gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(nullptr != pexpr);

	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components
	gpos::Ref<CExpression> pexprLeft = (*pexpr)[0];
	gpos::Ref<CExpression> pexprRight = (*pexpr)[1];
	gpos::Ref<CExpression> pexprScalar = (*pexpr)[2];

	// addref all children
	pexprLeft->AddRef();
	pexprRight->AddRef();
	pexprScalar->AddRef();

	// assemble physical operator
	gpos::Ref<CExpression> pexprBinary = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) T(mp), pexprLeft, pexprRight, pexprScalar);

#ifdef GPOS_DEBUG
	COperator::EOperatorId op_id = pexprBinary->Pop()->Eopid();
#endif	// GPOS_DEBUG
	GPOS_ASSERT(COperator::EopPhysicalInnerNLJoin == op_id ||
				COperator::EopPhysicalLeftOuterNLJoin == op_id ||
				COperator::EopPhysicalLeftSemiNLJoin == op_id ||
				COperator::EopPhysicalLeftAntiSemiNLJoin == op_id ||
				COperator::EopPhysicalLeftAntiSemiNLJoinNotIn == op_id);

	// add alternative to results
	pxfres->Add(pexprBinary);
}

template <class T>
void
CXformUtils::AddHashOrMergeJoinAlternative(CMemoryPool *mp,
										   gpos::Ref<CExpression> pexprJoin,
										   CExpressionArray *pdrgpexprOuter,
										   CExpressionArray *pdrgpexprInner,
										   IMdIdArray *opfamilies,
										   gpos::Ref<CXformResult> pxfres,
										   BOOL is_hash_join_null_aware)
{
	GPOS_ASSERT(CUtils::FLogicalJoin(pexprJoin->Pop()));
	GPOS_ASSERT(3 == pexprJoin->Arity());
	GPOS_ASSERT(nullptr != pdrgpexprOuter);
	GPOS_ASSERT(nullptr != pdrgpexprInner);
	GPOS_ASSERT(nullptr != pxfres);

	for (ULONG ul = 0; ul < 3; ul++)
	{
		(*pexprJoin)[ul]->AddRef();
	}
	gpos::Ref<CLogicalJoin> popLogicalJoin =
		CLogicalJoin::PopConvert(pexprJoin->Pop());
	T *op =
		GPOS_NEW(mp) T(mp, pdrgpexprOuter, pdrgpexprInner, opfamilies,
					   is_hash_join_null_aware, popLogicalJoin->OriginXform());
	gpos::Ref<CExpression> pexprResult = GPOS_NEW(mp)
		CExpression(mp, op, (*pexprJoin)[0], (*pexprJoin)[1], (*pexprJoin)[2]);
	pxfres->Add(pexprResult);
}


//---------------------------------------------------------------------------
//	@function:
//		CXformUtils::ImplementHashJoin
//
//	@doc:
//		Helper function for implementation of hash joins
//
//---------------------------------------------------------------------------
template <class T>
void
CXformUtils::ImplementHashJoin(gpos::Ref<CXformContext> pxfctxt,
							   gpos::Ref<CXformResult> pxfres,
							   gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pxfctxt);

	// if there are outer references, then we cannot build a hash join
	if (CUtils::HasOuterRefs(pexpr))
	{
		return;
	}

	CMemoryPool *mp = pxfctxt->Pmp();
	CExpressionArray *pdrgpexprOuter = nullptr;
	CExpressionArray *pdrgpexprInner = nullptr;
	IMdIdArray *join_opfamilies = nullptr;

	// check if we have already computed hash join keys for the scalar child
	LookupJoinKeys(mp, pexpr, &pdrgpexprOuter, &pdrgpexprInner,
				   &join_opfamilies);

	gpos::Ref<CExpression> pexprOuter = (*pexpr)[0];
	gpos::Ref<CExpression> pexprInner = (*pexpr)[1];
	gpos::Ref<CExpression> pexprScalar = (*pexpr)[2];
	CExpressionArray *pdrgpexpr =
		CCastUtils::PdrgpexprCastEquality(mp, pexprScalar);
	ULONG ulPreds = pdrgpexpr->Size();
	BOOL is_hash_join_null_aware = false;

	if (nullptr != pdrgpexprOuter)
	{
		GPOS_ASSERT(nullptr != pdrgpexprInner);
		if (0 == pdrgpexprOuter->Size())
		{
			GPOS_ASSERT(0 == pdrgpexprInner->Size());

			// we failed before to find hash join keys for scalar child,
			// no reason to try to do the same again
			pdrgpexprOuter->Release();
			pdrgpexprInner->Release();
			SafeRelease(join_opfamilies);
		}
		else
		{
			// we have computed hash join keys on scalar child before, reuse them
			for (ULONG ul = 0; ul < ulPreds; ul++)
			{
				gpos::Ref<CExpression> pexprPred = (*pdrgpexpr)[ul];
				if (!is_hash_join_null_aware)
				{
					is_hash_join_null_aware = CPredicateUtils::FINDF(pexprPred);
				}
			}

			AddHashOrMergeJoinAlternative<T>(mp, pexpr, pdrgpexprOuter,
											 pdrgpexprInner, join_opfamilies,
											 pxfres, is_hash_join_null_aware);
		}

		pdrgpexpr->Release();
		return;
	}

	// split the predicate into arrays of conjuncts based on if they are
	// output from inner or outer child
	pdrgpexprOuter = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprInner = GPOS_NEW(mp) CExpressionArray(mp);

	if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution))
	{
		join_opfamilies = GPOS_NEW(mp) IMdIdArray(mp);
	}

	for (ULONG ul = 0; ul < ulPreds; ul++)
	{
		gpos::Ref<CExpression> pexprPred = (*pdrgpexpr)[ul];
		if (CPhysicalJoin::FHashJoinCompatible(pexprPred, pexprOuter,
											   pexprInner))
		{
			if (!is_hash_join_null_aware)
			{
				is_hash_join_null_aware = CPredicateUtils::FINDF(pexprPred);
			}

			gpos::Ref<CExpression> pexprPredInner;
			gpos::Ref<CExpression> pexprPredOuter;
			gpos::Ref<IMDId> mdid_scop;
			CPhysicalJoin::AlignJoinKeyOuterInner(pexprPred, pexprOuter,
												  pexprInner, &pexprPredOuter,
												  &pexprPredInner, &mdid_scop);

			pexprPredInner->AddRef();
			pexprPredOuter->AddRef();
			pdrgpexprOuter->Append(pexprPredOuter);
			pdrgpexprInner->Append(pexprPredInner);

			if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution))
			{
				CMDAccessor *mda = COptCtxt::PoctxtFromTLS()->Pmda();
				gpos::Ref<IMDId> hash_opfamily =
					mda->RetrieveScOp(mdid_scop)->HashOpfamilyMdid();
				GPOS_ASSERT(nullptr != hash_opfamily &&
							hash_opfamily->IsValid());
				hash_opfamily->AddRef();
				join_opfamilies->Append(hash_opfamily);
			}
		}
	}
	GPOS_ASSERT(pdrgpexprInner->Size() == pdrgpexprOuter->Size());
	GPOS_ASSERT_IMP(GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution),
					pdrgpexprInner->Size() == join_opfamilies->Size());

	// construct new HashJoin expression using explicit casting, if needed
	pexpr->Pop()->AddRef();
	pexprOuter->AddRef();
	pexprInner->AddRef();
	gpos::Ref<CExpression> pexprResult = GPOS_NEW(mp)
		CExpression(mp, pexpr->Pop(), pexprOuter, pexprInner,
					CPredicateUtils::PexprConjunction(mp, pdrgpexpr));

	// cache hash join keys on scalar child group
	CacheJoinKeys(pexprResult, pdrgpexprOuter, pdrgpexprInner, join_opfamilies);

	// Add an alternative only if we found at least one hash-joinable predicate
	if (0 != pdrgpexprOuter->Size())
	{
		AddHashOrMergeJoinAlternative<T>(mp, pexprResult, pdrgpexprOuter,
										 pdrgpexprInner, join_opfamilies,
										 pxfres, is_hash_join_null_aware);
	}
	else
	{
		// clean up
		pdrgpexprOuter->Release();
		pdrgpexprInner->Release();
		SafeRelease(join_opfamilies);
	}

	pexprResult->Release();
}

template <class T>
void
CXformUtils::ImplementMergeJoin(gpos::Ref<CXformContext> pxfctxt,
								gpos::Ref<CXformResult> pxfres,
								gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pxfctxt);

	// if there are outer references, then we cannot build a merge join
	if (CUtils::HasOuterRefs(pexpr))
	{
		return;
	}

	CMemoryPool *mp = pxfctxt->Pmp();
	CExpressionArray *pdrgpexprOuter = nullptr;
	CExpressionArray *pdrgpexprInner = nullptr;
	IMdIdArray *join_opfamilies = nullptr;

	// check if we have already computed join keys for the scalar child
	LookupJoinKeys(mp, pexpr, &pdrgpexprOuter, &pdrgpexprInner,
				   &join_opfamilies);
	if (nullptr != pdrgpexprOuter)
	{
		GPOS_ASSERT(nullptr != pdrgpexprInner);
		if (0 == pdrgpexprOuter->Size())
		{
			GPOS_ASSERT(0 == pdrgpexprInner->Size());

			// we failed before to find join keys for scalar child,
			// no reason to try to do the same again
			pdrgpexprOuter->Release();
			pdrgpexprInner->Release();
			SafeRelease(join_opfamilies);
		}
		else
		{
			// we have computed join keys on scalar child before, reuse them
			AddHashOrMergeJoinAlternative<T>(
				mp, pexpr, pdrgpexprOuter, pdrgpexprInner, join_opfamilies,
				pxfres, true /*is_hash_join_null_aware*/);
		}

		return;
	}

	gpos::Ref<CExpression> pexprOuter = (*pexpr)[0];
	gpos::Ref<CExpression> pexprInner = (*pexpr)[1];
	gpos::Ref<CExpression> pexprScalar = (*pexpr)[2];

	// split the predicate into arrays of conjuncts based on if they are
	// output from inner or outer child
	pdrgpexprOuter = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprInner = GPOS_NEW(mp) CExpressionArray(mp);

	if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution))
	{
		join_opfamilies = GPOS_NEW(mp) IMdIdArray(mp);
	}

	CExpressionArray *pdrgpexpr =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprScalar);
	ULONG ulPreds = pdrgpexpr->Size();
	for (ULONG ul = 0; ul < ulPreds; ul++)
	{
		gpos::Ref<CExpression> pexprPred = (*pdrgpexpr)[ul];
		if (CPhysicalJoin::FMergeJoinCompatible(pexprPred, pexprOuter,
												pexprInner))
		{
			gpos::Ref<CExpression> pexprPredInner;
			gpos::Ref<CExpression> pexprPredOuter;
			gpos::Ref<IMDId> mdid_scop;
			CPhysicalJoin::AlignJoinKeyOuterInner(pexprPred, pexprOuter,
												  pexprInner, &pexprPredOuter,
												  &pexprPredInner, &mdid_scop);

			pexprPredInner->AddRef();
			pexprPredOuter->AddRef();
			pdrgpexprOuter->Append(pexprPredOuter);
			pdrgpexprInner->Append(pexprPredInner);

			if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution))
			{
				CMDAccessor *mda = COptCtxt::PoctxtFromTLS()->Pmda();
				gpos::Ref<IMDId> hash_opfamily =
					mda->RetrieveScOp(mdid_scop)->HashOpfamilyMdid();
				GPOS_ASSERT(nullptr != hash_opfamily &&
							hash_opfamily->IsValid());
				hash_opfamily->AddRef();
				join_opfamilies->Append(hash_opfamily);
			}
		}
		else
		{
			// In case of FULL merge joins, all the merge clauses must be merge
			// compatible or we cannot create a merge join.
			pdrgpexpr->Release();
			pdrgpexprOuter->Release();
			pdrgpexprInner->Release();
			SafeRelease(join_opfamilies);
			return;
		}
	}
	GPOS_ASSERT(pdrgpexprInner->Size() == pdrgpexprOuter->Size());
	GPOS_ASSERT_IMP(GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution),
					pdrgpexprInner->Size() == join_opfamilies->Size());

	// construct new MergeJoin expression using explicit casting, if needed
	pexpr->Pop()->AddRef();
	pexprOuter->AddRef();
	pexprInner->AddRef();
	gpos::Ref<CExpression> pexprResult = GPOS_NEW(mp)
		CExpression(mp, pexpr->Pop(), pexprOuter, pexprInner,
					CPredicateUtils::PexprConjunction(mp, pdrgpexpr));

	// cache hash join keys on scalar child group
	CacheJoinKeys(pexprResult, pdrgpexprOuter, pdrgpexprInner, join_opfamilies);

	// Add an alternative only if we found at least one merge-joinable predicate
	if (0 != pdrgpexprOuter->Size())
	{
		AddHashOrMergeJoinAlternative<T>(
			mp, pexprResult, pdrgpexprOuter, pdrgpexprInner, join_opfamilies,
			pxfres, true /*is_hash_join_null_aware*/);
	}
	else
	{
		// clean up
		pdrgpexprOuter->Release();
		pdrgpexprInner->Release();
	}
	SafeRelease(join_opfamilies);

	pexprResult->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CXformUtils::ImplementNLJoin
//
//	@doc:
//		Helper function for implementation of nested loops joins
//
//---------------------------------------------------------------------------
template <class T>
void
CXformUtils::ImplementNLJoin(gpos::Ref<CXformContext> pxfctxt,
							 gpos::Ref<CXformResult> pxfres,
							 gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pxfctxt);

	CMemoryPool *mp = pxfctxt->Pmp();

	CColRefArray *pdrgpcrOuter = GPOS_NEW(mp) CColRefArray(mp);
	CColRefArray *pdrgpcrInner = GPOS_NEW(mp) CColRefArray(mp);

	TransformImplementBinaryOp<T>(pxfctxt, pxfres, pexpr);

	// clean up
	pdrgpcrOuter->Release();
	pdrgpcrInner->Release();
}
}  // namespace gpopt

#endif	// !GPOPT_CXformUtils_H

// EOF
