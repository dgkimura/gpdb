//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CUtils.h
//
//	@doc:
//		Optimizer utility functions
//---------------------------------------------------------------------------
#ifndef GPOPT_CUtils_H
#define GPOPT_CUtils_H

#include "gpos/common/CHashSet.h"
#include "gpos/common/Ref.h"
#include "gpos/common/owner.h"
#include "gpos/common/ranges.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CDistributionSpec.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CWindowFrame.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CScalarAggFunc.h"
#include "gpopt/operators/CScalarArrayCmp.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/xforms/CXform.h"

// fwd declarations
namespace gpmd
{
class IMDId;
}

namespace gpopt
{
using namespace gpos;

// fwd declaration
class CMemo;
class CLogicalCTEConsumer;
class CLogicalCTEProducer;
class CLogical;
class CLogicalGbAgg;

//---------------------------------------------------------------------------
//	@class:
//		CUtils
//
//	@doc:
//		General utility functions
//
//---------------------------------------------------------------------------
class CUtils
{
private:
	// check if the expression is a scalar boolean const
	static BOOL FScalarConstBool(gpos::Ref<CExpression> pexpr, BOOL value);

	// check if two expressions have the same children in any order
	static BOOL FMatchChildrenUnordered(
		const gpos::Ref<CExpression> pexprLeft,
		const gpos::Ref<CExpression> pexprRight);

	// check if two expressions have the same children in the same order
	static BOOL FMatchChildrenOrdered(const gpos::Ref<CExpression> pexprLeft,
									  const gpos::Ref<CExpression> pexprRight);

	// checks that the given type has all the comparisons: Eq, NEq, L, LEq, G, GEq
	static BOOL FHasAllDefaultComparisons(const gpos::Ref<IMDType> pmdtype);

	//	append the expressions in the source array to destination array
	static void AppendArrayExpr(gpos::Ref<CExpressionArray> pdrgpexprSrc,
								gpos::Ref<CExpressionArray> pdrgpexprDest);

public:
	enum EExecLocalityType
	{
		EeltMaster,
		EeltSegments,
		EeltSingleton
	};

#ifdef GPOS_DEBUG

	// print given expression to debug trace
	static void PrintExpression(gpos::Ref<CExpression> pexpr);

	// print memo to debug trace
	static void PrintMemo(CMemo *pmemo);

#endif	// GPOS_DEBUG

	static IOstream &OsPrintDrgPcoldesc(
		IOstream &os, CColumnDescriptorArray *pdrgpcoldescIncludedCols,
		ULONG length);

	//-------------------------------------------------------------------
	// Helpers for generating expressions
	//-------------------------------------------------------------------

	// recursively check for a plan with CTE, if both CTEProducer and CTEConsumer are executed on the same locality.
	// raises an exception if CTE Producer and CTE Consumer does not have the same locality
	static void ValidateCTEProducerConsumerLocality(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, EExecLocalityType edt,
		gpos::Ref<UlongToUlongMap> phmulul);

	// generate a comparison expression for two column references
	static gpos::Ref<CExpression> PexprScalarCmp(CMemoryPool *mp,
												 const CColRef *pcrLeft,
												 const CColRef *pcrRight,
												 CWStringConst strOp,
												 gpos::Ref<IMDId> mdid_op);

	// generate a comparison expression for a column reference and an expression
	static gpos::Ref<CExpression> PexprScalarCmp(
		CMemoryPool *mp, const CColRef *pcrLeft,
		gpos::Ref<CExpression> pexprRight, CWStringConst strOp,
		gpos::Ref<IMDId> mdid_op);

	// generate a comparison expression for an expression and a column reference
	static gpos::Ref<CExpression> PexprScalarCmp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
		const CColRef *pcrRight, CWStringConst strOp, gpos::Ref<IMDId> mdid_op);

	// generate a comparison expression for two expressions
	static gpos::Ref<CExpression> PexprScalarCmp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
		gpos::Ref<CExpression> pexprRight, CWStringConst strOp,
		gpos::Ref<IMDId> mdid_op);

	// generate a comparison expression for two expressions
	static gpos::Ref<CExpression> PexprScalarCmp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
		gpos::Ref<CExpression> pexprRight, gpos::Ref<IMDId> mdid_scop);

	// generate a comparison expression for a column reference and an expression
	static gpos::Ref<CExpression> PexprScalarCmp(
		CMemoryPool *mp, const CColRef *pcrLeft,
		gpos::Ref<CExpression> pexprRight, IMDType::ECmpType cmp_type);

	// generate a comparison expression between two column references
	static gpos::Ref<CExpression> PexprScalarCmp(CMemoryPool *mp,
												 const CColRef *pcrLeft,
												 const CColRef *pcrRight,
												 IMDType::ECmpType cmp_type);

	// generate a comparison expression between an expression and a column reference
	static gpos::Ref<CExpression> PexprScalarCmp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
		const CColRef *pcrRight, IMDType::ECmpType cmp_type);

	// generate a comparison expression for two expressions
	static gpos::Ref<CExpression> PexprScalarCmp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
		gpos::Ref<CExpression> pexprRight, IMDType::ECmpType cmp_type);

	// generate a comparison against Zero
	static gpos::Ref<CExpression> PexprCmpWithZero(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
		gpos::Ref<IMDId> mdid_type_left, IMDType::ECmpType ecmptype);

	// generate an equality comparison expression for column references
	static gpos::Ref<CExpression> PexprScalarEqCmp(CMemoryPool *mp,
												   const CColRef *pcrLeft,
												   const CColRef *pcrRight);

	// generate an equality comparison expression for two expressions
	static gpos::Ref<CExpression> PexprScalarEqCmp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
		gpos::Ref<CExpression> pexprRight);

	// generate an equality comparison expression for a column reference and an expression
	static gpos::Ref<CExpression> PexprScalarEqCmp(
		CMemoryPool *mp, const CColRef *pcrLeft,
		gpos::Ref<CExpression> pexprRight);

	// generate an equality comparison expression for an expression and a column reference
	static gpos::Ref<CExpression> PexprScalarEqCmp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
		const CColRef *pcrRight);

	// generate an array comparison expression for a column reference and an expression
	static gpos::Ref<CExpression> PexprScalarArrayCmp(
		CMemoryPool *mp, CScalarArrayCmp::EArrCmpType earrcmptype,
		IMDType::ECmpType ecmptype,
		gpos::Ref<CExpressionArray> pexprScalarChildren, const CColRef *colref);

	// generate an Is Distinct From expression
	static gpos::Ref<CExpression> PexprIDF(CMemoryPool *mp,
										   gpos::Ref<CExpression> pexprLeft,
										   gpos::Ref<CExpression> pexprRight);

	static gpos::Ref<CExpression> PexprIDF(CMemoryPool *mp,
										   gpos::Ref<CExpression> pexprLeft,
										   gpos::Ref<CExpression> pexprRight,
										   gpos::Ref<IMDId> mdid_scop);

	// generate an Is NOT Distinct From expression for two column references
	static gpos::Ref<CExpression> PexprINDF(CMemoryPool *mp,
											const CColRef *pcrLeft,
											const CColRef *pcrRight);

	// generate an Is NOT Distinct From expression for scalar expressions
	static gpos::Ref<CExpression> PexprINDF(CMemoryPool *mp,
											gpos::Ref<CExpression> pexprLeft,
											gpos::Ref<CExpression> pexprRight);

	static gpos::Ref<CExpression> PexprINDF(CMemoryPool *mp,
											gpos::Ref<CExpression> pexprLeft,
											gpos::Ref<CExpression> pexprRight,
											gpos::Ref<IMDId> mdid_scop);

	// generate an Is NULL expression
	static gpos::Ref<CExpression> PexprIsNull(CMemoryPool *mp,
											  gpos::Ref<CExpression> pexpr);

	// generate an Is NOT NULL expression
	static gpos::Ref<CExpression> PexprIsNotNull(CMemoryPool *mp,
												 gpos::Ref<CExpression> pexpr);

	// generate an Is NOT FALSE expression
	static gpos::Ref<CExpression> PexprIsNotFalse(CMemoryPool *mp,
												  gpos::Ref<CExpression> pexpr);

	// find if a scalar expression uses a nullable columns from the output of a logical expression
	static BOOL FUsesNullableCol(CMemoryPool *mp,
								 gpos::Ref<CExpression> pexprScalar,
								 gpos::Ref<CExpression> pexprLogical);

	// generate a scalar op expression for a column reference and an expression
	static gpos::Ref<CExpression> PexprScalarOp(
		CMemoryPool *mp, const CColRef *pcrLeft, gpos::Ref<CExpression> pexpr,
		CWStringConst strOp, gpos::Ref<IMDId> mdid_op,
		gpos::Ref<IMDId> return_type_mdid = nullptr);

	// generate a scalar bool op expression
	static gpos::Ref<CExpression> PexprScalarBoolOp(
		CMemoryPool *mp, CScalarBoolOp::EBoolOperator eboolop,
		gpos::Ref<CExpressionArray> pdrgpexpr);

	// negate the given expression
	static gpos::Ref<CExpression> PexprNegate(CMemoryPool *mp,
											  gpos::Ref<CExpression> pexpr);

	// generate a scalar ident expression
	static gpos::Ref<CExpression> PexprScalarIdent(CMemoryPool *mp,
												   const CColRef *colref);

	// generate a scalar project element expression
	static gpos::Ref<CExpression> PexprScalarProjectElement(
		CMemoryPool *mp, CColRef *colref, gpos::Ref<CExpression> pexpr);

	// generate an aggregate function operator
	static gpos::Ref<CScalarAggFunc> PopAggFunc(
		CMemoryPool *mp, gpos::Ref<IMDId> pmdidAggFunc,
		const CWStringConst *pstrAggFunc, BOOL is_distinct,
		EAggfuncStage eaggfuncstage, BOOL fSplit,
		gpos::Ref<IMDId>
			pmdidResolvedReturnType,  // return type to be used if original return type is ambiguous
		EAggfuncKind aggkind, gpos::Ref<ULongPtrArray> argtypes);

	// generate an aggregate function
	static gpos::Ref<CExpression> PexprAggFunc(
		CMemoryPool *mp, gpos::Ref<IMDId> pmdidAggFunc,
		const CWStringConst *pstrAggFunc, const CColRef *colref,
		BOOL is_distinct, EAggfuncStage eaggfuncstage, BOOL fSplit);

	// generate a count(*) expression
	static gpos::Ref<CExpression> PexprCountStar(CMemoryPool *mp);

	// generate a GbAgg with count(*) function over the given expression
	static gpos::Ref<CExpression> PexprCountStar(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLogical);

	// return True if passed expression is a Project Element defined on count(*)/count(any) agg
	static BOOL FCountAggProjElem(gpos::Ref<CExpression> pexprPrjElem,
								  CColRef **ppcrCount);

	// check if given expression has a count(*)/count(Any) agg
	static BOOL FHasCountAgg(gpos::Ref<CExpression> pexpr, CColRef **ppcrCount);

	// check if given expression has count matching the given column, returns the Logical GroupBy Agg above
	static BOOL FHasCountAggMatchingColumn(const gpos::Ref<CExpression> pexpr,
										   const CColRef *colref,
										   gpos::Ref<CLogicalGbAgg> &ppgbAgg);

	// generate a GbAgg with count(*) and sum(col) over the given expression
	static gpos::Ref<CExpression> PexprCountStarAndSum(
		CMemoryPool *mp, const CColRef *colref,
		gpos::Ref<CExpression> pexprLogical);

	// generate a sum(col) expression
	static gpos::Ref<CExpression> PexprSum(CMemoryPool *mp,
										   const CColRef *colref);

	// generate a GbAgg with sum(col) expressions for all columns in the given array
	static gpos::Ref<CExpression> PexprGbAggSum(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLogical,
		gpos::Ref<CColRefArray> pdrgpcrSum);

	// generate a count(col) expression
	static gpos::Ref<CExpression> PexprCount(CMemoryPool *mp,
											 const CColRef *colref,
											 BOOL is_distinct);

	// generate a min(col) expression
	static gpos::Ref<CExpression> PexprMin(CMemoryPool *mp,
										   CMDAccessor *md_accessor,
										   const CColRef *colref);

	// generate an aggregate expression
	static gpos::Ref<CExpression> PexprAgg(CMemoryPool *mp,
										   CMDAccessor *md_accessor,
										   IMDType::EAggType agg_type,
										   const CColRef *colref,
										   BOOL is_distinct);

	// generate a select expression
	static gpos::Ref<CExpression> PexprLogicalSelect(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpression> pexprPredicate);

	// if predicate is True return logical expression, otherwise return a new select node
	static gpos::Ref<CExpression> PexprSafeSelect(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLogical,
		gpos::Ref<CExpression> pexprPredicate);

	// generate a select expression, if child is another Select expression collapse both Selects into one expression
	static gpos::Ref<CExpression> PexprCollapseSelect(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpression> pexprPredicate);

	// generate a project expression
	static gpos::Ref<CExpression> PexprLogicalProject(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpression> pexprPrjList, BOOL fNewComputedCol);

	// generate a sequence project expression
	static gpos::Ref<CExpression> PexprLogicalSequenceProject(
		CMemoryPool *mp, gpos::Ref<CDistributionSpec> pds,
		gpos::Ref<COrderSpecArray> pdrgpos,
		gpos::Ref<CWindowFrameArray> pdrgpwf, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpression> pexprPrjList);

	// generate a projection of NULL constants
	// to the map 'colref_mapping', and add the mappings to the colref_mapping map if not NULL
	static gpos::Ref<CExpression> PexprLogicalProjectNulls(
		CMemoryPool *mp, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CExpression> pexpr,
		gpos::Ref<UlongToColRefMap> colref_mapping = nullptr);

	// construct a project list using the given columns and datums
	// store the mapping in the colref_mapping map if not NULL
	static gpos::Ref<CExpression> PexprScalarProjListConst(
		CMemoryPool *mp, gpos::Ref<CColRefArray> colref_array,
		IDatumArray *pdrgpdatum, gpos::Ref<UlongToColRefMap> colref_mapping);

	// generate a project expression
	static gpos::Ref<CExpression> PexprAddProjection(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpression> pexprProjected);

	// generate a project expression with one or more additional project elements
	static gpos::Ref<CExpression> PexprAddProjection(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpressionArray> pdrgpexprProjected,
		BOOL fNewComputedCol = true);

	// generate an aggregate expression
	static gpos::Ref<CExpression> PexprLogicalGbAggGlobal(
		CMemoryPool *mp, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CExpression> pexpr, gpos::Ref<CExpression> pexprPrL);

	// generate an aggregate expression
	static gpos::Ref<CExpression> PexprLogicalGbAgg(
		CMemoryPool *mp, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<CExpression> pexpr, gpos::Ref<CExpression> pexprPrL,
		COperator::EGbAggType egbaggtype);

	// check if the aggregate is local or global
	static BOOL FHasGlobalAggFunc(const gpos::Ref<CExpression> pexprProjList);

	// check if given project list has only aggregate functions
	// that can be safely executed on replicated slices
	static BOOL FContainsOnlyReplicationSafeAggFuncs(
		const gpos::Ref<CExpression> pexprProjList);

	// generate a bool expression
	static gpos::Ref<CExpression> PexprScalarConstBool(CMemoryPool *mp,
													   BOOL value,
													   BOOL is_null = false);

	// generate an int4 expression
	static gpos::Ref<CExpression> PexprScalarConstInt4(CMemoryPool *mp,
													   INT val);

	// generate an int8 expression
	static gpos::Ref<CExpression> PexprScalarConstInt8(CMemoryPool *mp,
													   LINT val,
													   BOOL is_null = false);

	// generate an oid constant expression
	static gpos::Ref<CExpression> PexprScalarConstOid(CMemoryPool *mp,
													  OID oid_val);

	// generate a NULL constant of a given type
	static gpos::Ref<CExpression> PexprScalarConstNull(
		CMemoryPool *mp, const gpos::Ref<IMDType> typ, INT type_modifier);

	// comparison operator type
	static IMDType::ECmpType ParseCmpType(gpos::Ref<IMDId> mdid);

	// comparison operator type
	static IMDType::ECmpType ParseCmpType(CMDAccessor *md_accessor,
										  gpos::Ref<IMDId> mdid);

	// generate a binary join expression
	template <class T>
	static gpos::Ref<CExpression> PexprLogicalJoin(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
		gpos::Ref<CExpression> pexprRight,
		gpos::Ref<CExpression> pexprPredicate,
		CXform::EXformId origin_xform = CXform::ExfSentinel);

	// generate an apply expression
	template <class T>
	static gpos::Ref<CExpression> PexprLogicalApply(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
		gpos::Ref<CExpression> pexprRight,
		gpos::Ref<CExpression> pexprPred = nullptr);

	// generate an apply expression with a known inner column
	template <class T>
	static gpos::Ref<CExpression> PexprLogicalApply(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
		gpos::Ref<CExpression> pexprRight, const CColRef *pcrInner,
		COperator::EOperatorId eopidOriginSubq,
		gpos::Ref<CExpression> pexprPred = nullptr);

	// generate an apply expression with a known array of inner columns
	template <class T>
	static gpos::Ref<CExpression> PexprLogicalApply(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
		gpos::Ref<CExpression> pexprRight, gpos::Ref<CColRefArray> pdrgpcrInner,
		COperator::EOperatorId eopidOriginSubq,
		gpos::Ref<CExpression> pexprPred = nullptr);

	// generate a correlated apply for quantified subquery with a known array of inner columns
	template <class T>
	static gpos::Ref<CExpression> PexprLogicalCorrelatedQuantifiedApply(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
		gpos::Ref<CExpression> pexprRight, gpos::Ref<CColRefArray> pdrgpcrInner,
		COperator::EOperatorId eopidOriginSubq,
		gpos::Ref<CExpression> pexprPred = nullptr);

	//-------------------------------------------------------------------
	// Helpers for partitioning
	//-------------------------------------------------------------------

	// extract the nth partition key from the given array of partition keys
	static CColRef *PcrExtractPartKey(CColRef2dArray *pdrgpdrgpcr,
									  ULONG ulLevel);

	//-------------------------------------------------------------------
	// Helpers for comparisons
	//-------------------------------------------------------------------

	// deduplicate array of expressions
	static gpos::Ref<CExpressionArray> PdrgpexprDedup(
		CMemoryPool *mp, gpos::Ref<CExpressionArray> pdrgpexpr);

	// deep equality of expression trees
	static BOOL Equals(const gpos::Ref<CExpression> pexprLeft,
					   const gpos::Ref<CExpression> pexprRight);

	// compare expression against an array of expressions
	static BOOL FEqualAny(const gpos::Ref<CExpression> pexpr,
						  const gpos::Ref<CExpressionArray> pdrgpexpr);

	// deep equality of expression arrays
	static BOOL Equals(const gpos::Ref<CExpressionArray> pdrgpexprLeft,
					   const gpos::Ref<CExpressionArray> pdrgpexprRight);

	// check if first expression array contains all expressions in second array
	static BOOL Contains(const gpos::Ref<CExpressionArray> pdrgpexprFst,
						 const gpos::Ref<CExpressionArray> pdrgpexprSnd);

	// return the number of occurrences of the given expression in the given
	// array of expressions
	static ULONG UlOccurrences(const gpos::Ref<CExpression> pexpr,
							   gpos::Ref<CExpressionArray> pdrgpexpr);

	//-------------------------------------------------------------------
	// Helpers for datums
	//-------------------------------------------------------------------

	// check to see if the expression is a scalar const TRUE
	static BOOL FScalarConstTrue(gpos::Ref<CExpression> pexpr);

	// check to see if the expression is a scalar const FALSE
	static BOOL FScalarConstFalse(gpos::Ref<CExpression> pexpr);

	// check if the given expression is an INT, the template parameter is an INT type
	template <class T>
	static BOOL FScalarConstInt(gpos::Ref<CExpression> pexpr);

	//-------------------------------------------------------------------
	// Helpers for printing
	//-------------------------------------------------------------------

	// column reference array print helper
	static IOstream &OsPrintDrgPcr(IOstream &os,
								   const gpos::Ref<CColRefArray> colref_array,
								   ULONG = gpos::ulong_max);

	//-------------------------------------------------------------------
	// Helpers for column reference sets
	//-------------------------------------------------------------------

	// create an array of output columns including a key for grouping
	static gpos::Ref<CColRefArray> PdrgpcrGroupingKey(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CColRefArray> *ppdrgpcrKey);

	// add an equivalence class (col ref set) to the array. If the new equiv
	// class contains columns from existing equiv classes, then these are merged
	static gpos::Ref<CColRefSetArray> AddEquivClassToArray(
		CMemoryPool *mp, const gpos::Ref<CColRefSet> pcrsNew,
		const gpos::Ref<CColRefSetArray> pdrgpcrs);

	// merge 2 arrays of equivalence classes
	static gpos::Ref<CColRefSetArray> PdrgpcrsMergeEquivClasses(
		CMemoryPool *mp, gpos::Ref<CColRefSetArray> pdrgpcrsFst,
		gpos::Ref<CColRefSetArray> pdrgpcrsSnd);

	// intersect 2 arrays of equivalence classes
	static gpos::Ref<CColRefSetArray> PdrgpcrsIntersectEquivClasses(
		CMemoryPool *mp, gpos::Ref<CColRefSetArray> pdrgpcrsFst,
		gpos::Ref<CColRefSetArray> pdrgpcrsSnd);

	// return a copy of equivalence classes from all children
	static gpos::Ref<CColRefSetArray> PdrgpcrsCopyChildEquivClasses(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

	// return a copy of the given array of columns, excluding the columns
	// in the given colrefset
	static gpos::Ref<CColRefArray> PdrgpcrExcludeColumns(
		CMemoryPool *mp, gpos::Ref<CColRefArray> pdrgpcrOriginal,
		gpos::Ref<CColRefSet> pcrsExcluded);

	//-------------------------------------------------------------------
	// General helpers
	//-------------------------------------------------------------------

	// append elements from input array to output array, starting from given index, after add-refing them
	template <class T, void (*CleanupFn)(T *)>
	static void AddRefAppend(CDynamicPtrArray<T, CleanupFn> *pdrgptOutput,
							 CDynamicPtrArray<T, CleanupFn> *pdrgptInput,
							 ULONG ulStart = 0);

	template <class T>
	static void AddRefAppend(Vector<Ref<T>> output, const Vector<Ref<T>> input,
							 ULONG start = 0);

	template <class T>
	static void AddRefAppend(Vector<Ref<T>> *output,
							 const Vector<Ref<T>> *input, ULONG start = 0);

	// check for existence of subqueries
	static BOOL FHasSubquery(gpos::Ref<CExpression> pexpr);

	// check existence of subqueries or Apply operators in deep expression tree
	static BOOL FHasSubqueryOrApply(gpos::Ref<CExpression> pexpr,
									BOOL fCheckRoot = true);

	// check existence of correlated apply operators in deep expression tree
	static BOOL FHasCorrelatedApply(gpos::Ref<CExpression> pexpr,
									BOOL fCheckRoot = true);

	// check for existence of CTE anchor
	static BOOL FHasCTEAnchor(gpos::Ref<CExpression> pexpr);

	// check for existence of outer references
	static BOOL HasOuterRefs(gpos::Ref<CExpression> pexpr);

	// check if a given operator is a logical join
	static BOOL FLogicalJoin(gpos::Ref<COperator> pop);

	// check if a given operator is a logical set operation
	static BOOL FLogicalSetOp(gpos::Ref<COperator> pop);

	// check if a given operator is a logical unary operator
	static BOOL FLogicalUnary(gpos::Ref<COperator> pop);

	// check if a given operator is a physical join
	static BOOL FPhysicalJoin(gpos::Ref<COperator> pop);

	// check if a given operator is a physical left outer join
	static BOOL FPhysicalLeftOuterJoin(gpos::Ref<COperator> pop);

	// check if a given operator is a physical scan
	static BOOL FPhysicalScan(gpos::Ref<COperator> pop);

	// check if a given operator is a physical agg
	static BOOL FPhysicalAgg(gpos::Ref<COperator> pop);

	// check if given expression has any one stage agg nodes
	static BOOL FHasOneStagePhysicalAgg(const gpos::Ref<CExpression> pexpr);

	// check if a given operator is a physical motion
	static BOOL FPhysicalMotion(gpos::Ref<COperator> pop);

	// check if duplicate values can be generated by the given distribution spec
	static BOOL FDuplicateHazardDistributionSpec(
		gpos::Ref<CDistributionSpec> pds);

	// check if duplicate values can be generated by the given Motion expression
	static BOOL FDuplicateHazardMotion(gpos::Ref<CExpression> pexprMotion);

	// check if a given operator is an enforcer
	static BOOL FEnforcer(gpos::Ref<COperator> pop);

	// check if a given operator is a hash join
	static BOOL FHashJoin(gpos::Ref<COperator> pop);

	// check if a given operator is a correlated nested loops join
	static BOOL FCorrelatedNLJoin(gpos::Ref<COperator> pop);

	// check if a given operator is a nested loops join
	static BOOL FNLJoin(gpos::Ref<COperator> pop);

	// check if a given operator is an Apply
	static BOOL FApply(gpos::Ref<COperator> pop);

	// check if a given operator is a correlated Apply
	static BOOL FCorrelatedApply(gpos::Ref<COperator> pop);

	// check if a given operator is left semi apply
	static BOOL FLeftSemiApply(gpos::Ref<COperator> pop);

	// check if a given operator is left anti semi apply
	static BOOL FLeftAntiSemiApply(gpos::Ref<COperator> pop);

	// check if a given operator is a subquery
	static BOOL FSubquery(gpos::Ref<COperator> pop);

	// check if a given operator is existential subquery
	static BOOL FExistentialSubquery(gpos::Ref<COperator> pop);

	// check if a given operator is quantified subquery
	static BOOL FQuantifiedSubquery(gpos::Ref<COperator> pop);

	// check if given expression is a Project Element with scalar subquery
	static BOOL FProjElemWithScalarSubq(gpos::Ref<CExpression> pexpr);

	// check if given expression is a scalar subquery with a ConstTableGet as the only child
	static BOOL FScalarSubqWithConstTblGet(gpos::Ref<CExpression> pexpr);

	// check if given expression is a Project on ConstTable with one scalar subquery in Project List
	static BOOL FProjectConstTableWithOneScalarSubq(
		gpos::Ref<CExpression> pexpr);

	// check if an expression is a 0 offset
	static BOOL FScalarConstIntZero(gpos::Ref<CExpression> pexprOffset);

	// check if a limit expression has 0 offset
	static BOOL FHasZeroOffset(gpos::Ref<CExpression> pexpr);

	// check if expression is scalar comparison
	static BOOL FScalarCmp(gpos::Ref<CExpression> pexpr);

	// check if expression is scalar array comparison
	static BOOL FScalarArrayCmp(gpos::Ref<CExpression> pexpr);

	// check if given operator exists in the given list
	static BOOL FOpExists(const gpos::Ref<COperator> pop,
						  const COperator::EOperatorId *peopid, ULONG ulOps);

	// check if given expression has any operator in the given list
	static BOOL FHasOp(const gpos::Ref<CExpression> pexpr,
					   const COperator::EOperatorId *peopid, ULONG ulOps);

	// return number of inlinable CTEs in the given expression
	static ULONG UlInlinableCTEs(gpos::Ref<CExpression> pexpr,
								 ULONG ulDepth = 1);

	// return number of joins in the given expression
	static ULONG UlJoins(gpos::Ref<CExpression> pexpr);

	// return number of subqueries in the given expression
	static ULONG UlSubqueries(gpos::Ref<CExpression> pexpr);

	// check if expression is scalar bool op
	static BOOL FScalarBoolOp(gpos::Ref<CExpression> pexpr);

	// is the given expression a scalar bool op of the passed type?
	static BOOL FScalarBoolOp(gpos::Ref<CExpression> pexpr,
							  CScalarBoolOp::EBoolOperator eboolop);

	// check if expression is scalar null test
	static BOOL FScalarNullTest(gpos::Ref<CExpression> pexpr);

	// check if given expression is a NOT NULL predicate
	static BOOL FScalarNotNull(gpos::Ref<CExpression> pexpr);

	// check if expression is scalar identifier
	static BOOL FScalarIdent(gpos::Ref<CExpression> pexpr);

	// check if expression is scalar identifier (with or without a cast)
	static BOOL FScalarIdentIgnoreCast(gpos::Ref<CExpression> pexpr);

	static BOOL FScalarConstAndScalarIdentArray(
		gpos::Ref<CExpression> pexprArray);

	// check if expression is scalar identifier of boolean type
	static BOOL FScalarIdentBoolType(gpos::Ref<CExpression> pexpr);

	// check if expression is scalar array
	static BOOL FScalarArray(gpos::Ref<CExpression> pexpr);

	// returns number of children or constants of it is all constants
	static ULONG UlScalarArrayArity(gpos::Ref<CExpression> pexpr);

	// returns constant operator of a scalar array expression
	static gpos::Ref<CScalarConst> PScalarArrayConstChildAt(
		gpos::Ref<CExpression> pexprArray, ULONG ul);

	// returns constant expression of a scalar array expression
	static gpos::Ref<CExpression> PScalarArrayExprChildAt(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprArray, ULONG ul);

	// returns the scalar array expression child of CScalarArrayComp
	static gpos::Ref<CExpression> PexprScalarArrayChild(
		gpos::Ref<CExpression> pexpr);

	// returns if the scalar array has all constant elements or children
	static BOOL FScalarConstArray(gpos::Ref<CExpression> pexpr);

	// returns if the scalar constant is an array
	static BOOL FIsConstArray(gpos::Ref<CExpression> pexpr);

	// returns MDId for gp_percentile based on return type
	static gpos::Ref<CMDIdGPDB> GetPercentileAggMDId(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprAggFn);

	// returns if the scalar constant array has already been collapased
	static BOOL FScalarArrayCollapsed(gpos::Ref<CExpression> pexprArray);

	// returns true if the subquery is a ScalarSubqueryAny
	static BOOL FAnySubquery(gpos::Ref<COperator> pop);

	static gpos::Ref<CScalarProjectElement> PNthProjectElement(
		gpos::Ref<CExpression> pexpr, ULONG ul);

	// returns the expression under the Nth project element of a CLogicalProject
	static gpos::Ref<CExpression> PNthProjectElementExpr(
		gpos::Ref<CExpression> pexpr, ULONG ul);

	// check if the Project list has an inner reference assuming project list has one projecet element
	static BOOL FInnerRefInProjectList(gpos::Ref<CExpression> pexpr);

	// Check if expression tree has a col being referenced in the CColRefSet passed as input
	static BOOL FExprHasAnyCrFromCrs(gpos::Ref<CExpression> pexpr,
									 gpos::Ref<CColRefSet> pcrs);

	// If it's a scalar array of all CScalarConst, collapse it into a single
	// expression but keep the constants in the operator.
	static gpos::Ref<CExpression> PexprCollapseConstArray(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprArray);

	// check if expression is scalar array coerce
	static BOOL FScalarArrayCoerce(gpos::Ref<CExpression> pexpr);

	// is the given expression a scalar identifier with the given column reference
	static BOOL FScalarIdent(gpos::Ref<CExpression> pexpr, CColRef *colref);

	// check if expression is scalar const
	static BOOL FScalarConst(gpos::Ref<CExpression> pexpr);

	// check if this is a variable-free expression
	static BOOL FVarFreeExpr(gpos::Ref<CExpression> pexpr);

	// check if expression is a predicate
	static BOOL FPredicate(gpos::Ref<CExpression> pexpr);

	// is this type supported in contradiction detection using stats logic
	static BOOL FIntType(gpos::Ref<IMDId> mdid_type);

	// is this type supported in contradiction detection
	static BOOL FConstrainableType(gpos::Ref<IMDId> mdid_type);

	// check if a binary operator uses only columns produced by its children
	static BOOL FUsesChildColsOnly(CExpressionHandle &exprhdl);

	// check if inner child of a binary operator uses columns not produced by outer child
	static BOOL FInnerUsesExternalCols(CExpressionHandle &exprhdl);

	// check if inner child of a binary operator uses only columns not produced by outer child
	static BOOL FInnerUsesExternalColsOnly(CExpressionHandle &exprhdl);

	// check if comparison operators are available for the given columns
	static BOOL FComparisonPossible(gpos::Ref<CColRefArray> colref_array,
									IMDType::ECmpType cmp_type);

	static ULONG UlCountOperator(const gpos::Ref<CExpression> pexpr,
								 COperator::EOperatorId op_id);

	// return the max subset of redistributable columns for the given columns
	static gpos::Ref<CColRefArray> PdrgpcrRedistributableSubset(
		CMemoryPool *mp, gpos::Ref<CColRefArray> colref_array);

	// check if hashing is possible for the given columns
	static BOOL IsHashable(gpos::Ref<CColRefArray> colref_array);

	// check if the given operator is a logical DML operator
	static BOOL FLogicalDML(gpos::Ref<COperator> pop);

	// return regular string from wide-character string
	static CHAR *CreateMultiByteCharStringFromWCString(CMemoryPool *mp,
													   WCHAR *wsz);

	// return column reference defined by project element
	static CColRef *PcrFromProjElem(gpos::Ref<CExpression> pexprPrEl);

	// construct an array of colids from the given array of column references
	static gpos::Ref<ULongPtrArray> Pdrgpul(
		CMemoryPool *mp, const gpos::Ref<CColRefArray> colref_array);

	// generate a timestamp-based file name
	static void GenerateFileName(CHAR *buf, const CHAR *szPrefix,
								 const CHAR *szExt, ULONG length,
								 ULONG ulSessionId, ULONG ulCmdId);

	// return the mapping of the given colref based on the given hashmap
	static CColRef *PcrRemap(const CColRef *colref,
							 gpos::Ref<UlongToColRefMap> colref_mapping,
							 BOOL must_exist);

	// create a new colrefset corresponding to the given colrefset
	// and based on the given mapping
	static gpos::Ref<CColRefSet> PcrsRemap(
		CMemoryPool *mp, gpos::Ref<CColRefSet> pcrs,
		gpos::Ref<UlongToColRefMap> colref_mapping, BOOL must_exist);

	// create an array of column references corresponding to the given array
	// and based on the given mapping
	static gpos::Ref<CColRefArray> PdrgpcrRemap(
		CMemoryPool *mp, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<UlongToColRefMap> colref_mapping, BOOL must_exist);

	// create an array of column references corresponding to the given array
	// and based on the given mapping and create new colrefs if necessary
	static gpos::Ref<CColRefArray> PdrgpcrRemapAndCreate(
		CMemoryPool *mp, gpos::Ref<CColRefArray> colref_array,
		gpos::Ref<UlongToColRefMap> colref_mapping);

	// create an array of column arrays corresponding to the given array
	// and based on the given mapping
	static gpos::Ref<CColRef2dArray> PdrgpdrgpcrRemap(
		CMemoryPool *mp, CColRef2dArray *pdrgpdrgpcr,
		gpos::Ref<UlongToColRefMap> colref_mapping, BOOL must_exist);

	// remap given array of expressions with provided column mappings
	static gpos::Ref<CExpressionArray> PdrgpexprRemap(
		CMemoryPool *mp, gpos::Ref<CExpressionArray> pdrgpexpr,
		gpos::Ref<UlongToColRefMap> colref_mapping);

	// create ColRef->ColRef mapping using the given ColRef arrays
	static gpos::Ref<UlongToColRefMap> PhmulcrMapping(
		CMemoryPool *mp, gpos::Ref<CColRefArray> pdrgpcrFrom,
		gpos::Ref<CColRefArray> pdrgpcrTo);

	// add col ID->ColRef mappings to the given hashmap based on the
	// given ColRef arrays
	static void AddColumnMapping(CMemoryPool *mp,
								 gpos::Ref<UlongToColRefMap> colref_mapping,
								 gpos::Ref<CColRefArray> pdrgpcrFrom,
								 gpos::Ref<CColRefArray> pdrgpcrTo);

	// create a copy of the array of column references
	static gpos::Ref<CColRefArray> PdrgpcrExactCopy(
		CMemoryPool *mp, gpos::Ref<CColRefArray> colref_array);

	// create an array of new column references with the same names and
	// types as the given column references.
	// if the passed map is not null, mappings from old to copied variables are added to it
	static gpos::Ref<CColRefArray> PdrgpcrCopy(
		CMemoryPool *mp, gpos::Ref<CColRefArray> colref_array,
		BOOL fAllComputed = false,
		gpos::Ref<UlongToColRefMap> colref_mapping = nullptr);

	// equality check between two arrays of column refs. Inputs can be NULL
	static BOOL Equals(gpos::Ref<CColRefArray> pdrgpcrFst,
					   gpos::Ref<CColRefArray> pdrgpcrSnd);

	// compute hash value for an array of column references
	static ULONG UlHashColArray(const gpos::Ref<CColRefArray> colref_array,
								const ULONG ulMaxCols = 5);

	// return the set of column reference from the CTE Producer corresponding to the
	// subset of input columns from the CTE Consumer
	static gpos::Ref<CColRefSet> PcrsCTEProducerColumns(
		CMemoryPool *mp, gpos::Ref<CColRefSet> pcrsInput,
		gpos::Ref<CLogicalCTEConsumer> popCTEConsumer);

	// construct the join condition (AND-tree of INDF operators)
	// from the array of input columns reference arrays (aligned)
	static gpos::Ref<CExpression> PexprConjINDFCond(
		CMemoryPool *mp, CColRef2dArray *pdrgpdrgpcrInput);

	// check whether a colref array contains repeated items
	static BOOL FHasDuplicates(const gpos::Ref<CColRefArray> colref_array);

	// cast the input expression to the destination mdid
	static gpos::Ref<CExpression> PexprCast(CMemoryPool *mp,
											CMDAccessor *md_accessor,
											gpos::Ref<CExpression> pexpr,
											gpos::Ref<IMDId> mdid_dest);

	// construct a logical join expression of the given type, with the given children
	static gpos::Ref<CExpression> PexprLogicalJoin(
		CMemoryPool *mp, EdxlJoinType edxljointype,
		gpos::Ref<CExpressionArray> pdrgpexpr);

	// construct an array of scalar ident expressions from the given array
	// of column references
	static gpos::Ref<CExpressionArray> PdrgpexprScalarIdents(
		CMemoryPool *mp, gpos::Ref<CColRefArray> colref_array);

	// return the columns from the scalar ident expressions in the given array
	static gpos::Ref<CColRefSet> PcrsExtractColumns(
		CMemoryPool *mp, const gpos::Ref<CExpressionArray> pdrgpexpr);

	// create a new bitset of the given length, where all the bits are set
	static gpos::Ref<CBitSet> PbsAllSet(CMemoryPool *mp, ULONG size);

	// return a new bitset, setting the bits in the given array
	static gpos::Ref<CBitSet> Pbs(CMemoryPool *mp,
								  gpos::Ref<ULongPtrArray> pdrgpul);

	// helper to create a dummy constant table expression
	static gpos::Ref<CExpression> PexprLogicalCTGDummy(CMemoryPool *mp);

	// map a column from source array to destination array based on position
	static CColRef *PcrMap(CColRef *pcrSource,
						   gpos::Ref<CColRefArray> pdrgpcrSource,
						   gpos::Ref<CColRefArray> pdrgpcrTarget);

	//	return index of the set containing given column
	static ULONG UlPcrIndexContainingSet(gpos::Ref<CColRefSetArray> pdrgpcrs,
										 const CColRef *colref);

	// collapse the top two project nodes, if unable return NULL
	static gpos::Ref<CExpression> PexprCollapseProjects(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr);

	// match function between index get/scan operators
	template <class T>
	static BOOL FMatchIndex(T *pop1, gpos::Ref<COperator> pop2);

	// match function between dynamic index get/scan operators
	template <class T>
	static BOOL FMatchDynamicIndex(T *pop1, gpos::Ref<COperator> pop2);

	// match function between dynamic get/scan operators
	template <class T>
	static BOOL FMatchDynamicScan(T *pop1, gpos::Ref<COperator> pop2);

	// match function between dynamic bitmap get/scan operators
	template <class T>
	static BOOL FMatchDynamicBitmapScan(T *pop1, gpos::Ref<COperator> pop2);

	// match function between bitmap get/scan operators
	template <class T>
	static BOOL FMatchBitmapScan(T *pop1, gpos::Ref<COperator> pop2);

	// compares two Idatums, useful for sorting functions
	static INT IDatumCmp(const void *val1, const void *val2);

	// compares two CPoints, useful for sorting functions
	static INT CPointCmp(const void *val1, const void *val2);

	// check if the equivalance classes are disjoint
	static BOOL FEquivalanceClassesDisjoint(
		CMemoryPool *mp, const gpos::Ref<CColRefSetArray> pdrgpcrs);

	// check if the equivalance classes are same
	static BOOL FEquivalanceClassesEqual(
		CMemoryPool *mp, gpos::Ref<CColRefSetArray> pdrgpcrsFst,
		gpos::Ref<CColRefSetArray> pdrgpcrsSnd);

	// get execution locality
	static EExecLocalityType ExecLocalityType(gpos::Ref<CDistributionSpec> pds);

	// generate a limit expression on top of the given relational child with the given offset and limit count
	static gpos::Ref<CExpression> PexprLimit(CMemoryPool *mp,
											 gpos::Ref<CExpression> pexpr,
											 ULONG ulOffSet, ULONG count);

	// generate part oid
	static BOOL FGeneratePartOid(gpos::Ref<IMDId> mdid);

	// return true if given expression contains window aggregate function
	static BOOL FHasAggWindowFunc(gpos::Ref<CExpression> pexpr);

	// return true if given expression contains ordered aggregate function
	static BOOL FHasOrderedAggToSplit(gpos::Ref<CExpression> pexpr);

	// return true if the given expression is a cross join
	static BOOL FCrossJoin(gpos::Ref<CExpression> pexpr);

	// return true if can create hash join for the expression
	static BOOL IsHashJoinPossible(CMemoryPool *mp,
								   gpos::Ref<CExpression> pexpr);

	// is this scalar expression an NDV-preserving function (used for join stats derivation)
	static BOOL IsExprNDVPreserving(gpos::Ref<CExpression> pexpr,
									const CColRef **underlying_colref);

	// search the given array of predicates for predicates with equality or IS NOT
	// DISTINCT FROM operators that has one side equal to the given expression
	static gpos::Ref<CExpression> PexprMatchEqualityOrINDF(
		gpos::Ref<CExpression> pexprToMatch,
		gpos::Ref<CExpressionArray> pdrgpexpr  // array of predicates to inspect
	);

	static gpos::Ref<CExpression> MakeJoinWithoutInferredPreds(
		CMemoryPool *mp, gpos::Ref<CExpression> join_expr);

	static BOOL Contains(const gpos::Ref<CExpressionArray> exprs,
						 gpos::Ref<CExpression> expr_to_match);

	static BOOL Equals(const gpos::Ref<CExpressionArrays> exprs_arr,
					   const gpos::Ref<CExpressionArrays> other_exprs_arr);

	static BOOL Equals(const gpos::Ref<IMdIdArray> mdids,
					   const gpos::Ref<IMdIdArray> other_mdids);

	static BOOL Equals(const gpos::Ref<IMDId> mdid,
					   const gpos::Ref<IMDId> other_mdid);

	static BOOL CanRemoveInferredPredicates(COperator::EOperatorId op_id);

	static gpos::Ref<CExpressionArrays> GetCombinedExpressionArrays(
		CMemoryPool *mp, gpos::Ref<CExpressionArrays> exprs_array,
		gpos::Ref<CExpressionArrays> exprs_array_other);

	static void AddExprs(gpos::Ref<CExpressionArrays> results_exprs,
						 gpos::Ref<CExpressionArrays> input_exprs);

	static BOOL FScalarConstBoolNull(gpos::Ref<CExpression> pexpr);
};	// class CUtils

// hash set from expressions
using ExprHashSet =
	gpos::UnorderedSet<gpos::Ref<CExpression>,
					   gpos::RefHash<CExpression, CExpression::UlHashDedup>,
					   gpos::RefEq<CExpression, CUtils::Equals>>;


//---------------------------------------------------------------------------
//	@function:
//		CUtils::PexprLogicalJoin
//
//	@doc:
//		Generate a join expression from given expressions
//
//---------------------------------------------------------------------------
template <class T>
gpos::Ref<CExpression>
CUtils::PexprLogicalJoin(CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
						 gpos::Ref<CExpression> pexprRight,
						 gpos::Ref<CExpression> pexprPredicate,
						 CXform::EXformId origin_xform)
{
	GPOS_ASSERT(nullptr != pexprLeft);
	GPOS_ASSERT(nullptr != pexprRight);
	GPOS_ASSERT(nullptr != pexprPredicate);

	return GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) T(mp, origin_xform), std::move(pexprLeft),
					std::move(pexprRight), std::move(pexprPredicate));
}

//---------------------------------------------------------------------------
//	@function:
//		CUtils::PexprLogicalApply
//
//	@doc:
//		Generate an apply expression from given expressions
//
//---------------------------------------------------------------------------
template <class T>
gpos::Ref<CExpression>
CUtils::PexprLogicalApply(CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
						  gpos::Ref<CExpression> pexprRight,
						  gpos::Ref<CExpression> pexprPred)
{
	GPOS_ASSERT(nullptr != pexprLeft);
	GPOS_ASSERT(nullptr != pexprRight);

	gpos::Ref<CExpression> pexprScalar = pexprPred;
	if (nullptr == pexprPred)
	{
		pexprScalar = PexprScalarConstBool(mp, true /*value*/);
	}

	return GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) T(mp), pexprLeft, pexprRight, pexprScalar);
}


//---------------------------------------------------------------------------
//	@function:
//		CUtils::PexprLogicalApply
//
//	@doc:
//		Generate an apply expression with a known inner column
//
//---------------------------------------------------------------------------
template <class T>
gpos::Ref<CExpression>
CUtils::PexprLogicalApply(CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
						  gpos::Ref<CExpression> pexprRight,
						  const CColRef *pcrInner,
						  COperator::EOperatorId eopidOriginSubq,
						  gpos::Ref<CExpression> pexprPred)
{
	GPOS_ASSERT(nullptr != pexprLeft);
	GPOS_ASSERT(nullptr != pexprRight);
	GPOS_ASSERT(nullptr != pcrInner);

	gpos::Ref<CExpression> pexprScalar = pexprPred;
	if (nullptr == pexprPred)
	{
		pexprScalar = PexprScalarConstBool(mp, true /*value*/);
	}

	gpos::Ref<CColRefArray> colref_array = GPOS_NEW(mp) CColRefArray(mp);
	colref_array->Append(const_cast<CColRef *>(pcrInner));
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) T(mp, std::move(colref_array), eopidOriginSubq),
		std::move(pexprLeft), std::move(pexprRight), std::move(pexprScalar));
}

//---------------------------------------------------------------------------
//	@function:
//		CUtils::PexprLogicalApply
//
//	@doc:
//		Generate an apply expression with known array of inner columns
//
//---------------------------------------------------------------------------
template <class T>
gpos::Ref<CExpression>
CUtils::PexprLogicalApply(CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
						  gpos::Ref<CExpression> pexprRight,
						  gpos::Ref<CColRefArray> pdrgpcrInner,
						  COperator::EOperatorId eopidOriginSubq,
						  gpos::Ref<CExpression> pexprPred)
{
	GPOS_ASSERT(nullptr != pexprLeft);
	GPOS_ASSERT(nullptr != pexprRight);
	GPOS_ASSERT(nullptr != pdrgpcrInner);
	GPOS_ASSERT(0 < pdrgpcrInner->Size());

	gpos::Ref<CExpression> pexprScalar = pexprPred;
	if (nullptr == pexprPred)
	{
		pexprScalar = PexprScalarConstBool(mp, true /*value*/);
	}

	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) T(mp, std::move(pdrgpcrInner), eopidOriginSubq),
		std::move(pexprLeft), std::move(pexprRight), std::move(pexprScalar));
}

//---------------------------------------------------------------------------
//	@class:
//		CUtils::PexprLogicalCorrelatedQuantifiedApply
//
//	@doc:
//		Helper to create a left semi correlated apply from left semi apply
//
//---------------------------------------------------------------------------
template <class T>
gpos::Ref<CExpression>
CUtils::PexprLogicalCorrelatedQuantifiedApply(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprLeft,
	gpos::Ref<CExpression> pexprRight, gpos::Ref<CColRefArray> pdrgpcrInner,
	COperator::EOperatorId eopidOriginSubq, gpos::Ref<CExpression> pexprPred)
{
	GPOS_ASSERT(nullptr != pexprLeft);
	GPOS_ASSERT(nullptr != pexprRight);
	GPOS_ASSERT(nullptr != pdrgpcrInner);
	GPOS_ASSERT(0 < pdrgpcrInner->Size());

	gpos::Ref<CExpression> pexprScalar = pexprPred;
	if (nullptr == pexprPred)
	{
		pexprScalar = PexprScalarConstBool(mp, true /*value*/);
	}

	if (COperator::EopLogicalSelect != pexprRight->Pop()->Eopid())
	{
		// quantified comparison was pushed down, we create a dummy comparison here
		GPOS_ASSERT(
			!CUtils::HasOuterRefs(pexprRight.get()) &&
			"unexpected outer references in inner child of Semi Apply expression ");
		;
		pexprScalar = PexprScalarConstBool(mp, true /*value*/);
	}
	else
	{
		// quantified comparison is now on top of inner expression, skip to child
		;
		gpos::Ref<CExpression> pexprNewPredicate = (*pexprRight)[1];
		;
		pexprScalar = pexprNewPredicate;

		;
		gpos::Ref<CExpression> pexprChild = (*pexprRight)[0];
		;
		pexprRight = pexprChild;
	}

	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) T(mp, std::move(pdrgpcrInner), eopidOriginSubq),
		std::move(pexprLeft), std::move(pexprRight), std::move(pexprScalar));
}

//---------------------------------------------------------------------------
//	@function:
//		CUtils::AddRefAppend
//
//	@doc:
//		Append elements from input array to output array, starting from
//		given index, after add-refing them
//
//---------------------------------------------------------------------------
template <class T, void (*CleanupFn)(T *)>
void
CUtils::AddRefAppend(CDynamicPtrArray<T, CleanupFn> *pdrgptOutput,
					 CDynamicPtrArray<T, CleanupFn> *pdrgptInput, ULONG ulStart)
{
	static_assert(&gpos::CleanupRelease<T> == CleanupFn, "");

	GPOS_ASSERT(nullptr != pdrgptOutput);
	GPOS_ASSERT(nullptr != pdrgptInput);

	const ULONG size = pdrgptInput->Size();
	GPOS_ASSERT_IMP(0 < size, ulStart < size);

	for (ULONG ul = ulStart; ul < size; ul++)
	{
		T *pt = (*pdrgptInput)[ul];
		gpos::Ref<CRefCount<T>> prc = dynamic_cast<CRefCount<T> *>(pt);
		;
		pdrgptOutput->Append(pt);
	}
}

template <class T>
void
CUtils::AddRefAppend(Vector<Ref<T>> output, const Vector<Ref<T>> input,
					 ULONG start)
{
	for (const auto &t : gpos::subrange(*input).next(start))
	{
		output->Append(t);
	}
}

template <class T>
void
CUtils::AddRefAppend(Vector<Ref<T>> *output, const Vector<Ref<T>> *input,
					 ULONG start)
{
	for (ULONG ul = start; ul < input->Size(); ul++)
	{
		output->Append((*input)[ul]);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CUtils::FScalarConstInt
//
//	@doc:
//		Check if the given expression is an INT,
//		the template parameter is an INT type
//
//---------------------------------------------------------------------------
template <class T>
BOOL
CUtils::FScalarConstInt(gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	IMDType::ETypeInfo type_info = T::GetTypeInfo();
	GPOS_ASSERT(IMDType::EtiInt2 == type_info ||
				IMDType::EtiInt4 == type_info || IMDType::EtiInt8 == type_info);

	gpos::Ref<COperator> pop = pexpr->Pop();
	if (COperator::EopScalarConst == pop->Eopid())
	{
		gpos::Ref<CScalarConst> popScalarConst =
			gpos::dyn_cast<CScalarConst>(pop);
		if (type_info == popScalarConst->GetDatum()->GetDatumType())
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CUtils::FMatchIndex
//
//	@doc:
//		Match function between index get/scan operators
//
//---------------------------------------------------------------------------
template <class T>
BOOL
CUtils::FMatchIndex(T *pop1, gpos::Ref<COperator> pop2)
{
	if (pop1->Eopid() != pop2->Eopid())
	{
		return false;
	}

	T *popIndex = gpos::dyn_cast<T>(pop2).get();

	return pop1->UlOriginOpId() == popIndex->UlOriginOpId() &&
		   pop1->Ptabdesc()->MDId()->Equals(popIndex->Ptabdesc()->MDId()) &&
		   pop1->Pindexdesc()->MDId()->Equals(popIndex->Pindexdesc()->MDId()) &&
		   pop1->PdrgpcrOutput()->Equals(popIndex->PdrgpcrOutput());
}

//---------------------------------------------------------------------------
//	@function:
//		CUtils::FMatchDynamicIndex
//
//	@doc:
//		Match function between dynamic index get/scan operators
//
//---------------------------------------------------------------------------
template <class T>
BOOL
CUtils::FMatchDynamicIndex(T *pop1, gpos::Ref<COperator> pop2)
{
	if (pop1->Eopid() != pop2->Eopid())
	{
		return false;
	}

	T *popIndex2 = gpos::dyn_cast<T>(pop2).get();

	// match if the index descriptors are identical
	// we will compare MDIds, so both indexes should be partial or non-partial.
	// Possible future improvement:
	// For heterogeneous indexes, we use pointer comparison of part constraints.
	// That was to avoid memory allocation because matching function was used while
	// holding spin locks. This is no longer an issue, as we don't use spin locks
	// anymore. Using a match function would mean improved matching for heterogeneous
	// indexes.
	return pop1->UlOriginOpId() == popIndex2->UlOriginOpId() &&
		   pop1->ScanId() == popIndex2->ScanId() &&
		   pop1->Ptabdesc()->MDId()->Equals(popIndex2->Ptabdesc()->MDId()) &&
		   pop1->Pindexdesc()->MDId()->Equals(
			   popIndex2->Pindexdesc()->MDId()) &&
		   pop1->PdrgpcrOutput()->Equals(popIndex2->PdrgpcrOutput());
}

//---------------------------------------------------------------------------
//	@function:
//		CUtils::FMatchDynamicScan
//
//	@doc:
//		Match function between dynamic get/scan operators
//
//---------------------------------------------------------------------------
template <class T>
BOOL
CUtils::FMatchDynamicScan(T *pop1, gpos::Ref<COperator> pop2)
{
	if (pop1->Eopid() != pop2->Eopid())
	{
		return false;
	}

	T *popScan2 = gpos::dyn_cast<T>(pop2).get();

	// match if the table descriptors are identical
	// Possible improvement:
	// For partial scans, we use pointer comparison of part constraints to avoid
	// memory allocation because matching function was used while holding spin locks.
	// Using a match function would mean improved matches for partial scans.
	return pop1->ScanId() == popScan2->ScanId() &&
		   pop1->Ptabdesc()->MDId()->Equals(popScan2->Ptabdesc()->MDId()) &&
		   pop1->PdrgpcrOutput()->Equals(popScan2->PdrgpcrOutput());
}


//---------------------------------------------------------------------------
//	@function:
//		CUtils::FMatchDynamicBitmapScan
//
//	@doc:
//		Match function between dynamic bitmap get/scan operators
//
//---------------------------------------------------------------------------
template <class T>
BOOL
CUtils::FMatchDynamicBitmapScan(T *pop1, gpos::Ref<COperator> pop2)
{
	if (pop1->Eopid() != pop2->Eopid())
	{
		return false;
	}

	T *popDynamicBitmapScan2 = gpos::dyn_cast<T>(pop2).get();

	return pop1->UlOriginOpId() == popDynamicBitmapScan2->UlOriginOpId() &&
		   FMatchDynamicScan(
			   pop1,
			   pop2);  // call match dynamic scan to compare other member vars
}


//---------------------------------------------------------------------------
//	@function:
//		CUtils::FMatchBitmapScan
//
//	@doc:
//		Match function between bitmap get/scan operators
//
//---------------------------------------------------------------------------
template <class T>
BOOL
CUtils::FMatchBitmapScan(T *pop1, gpos::Ref<COperator> pop2)
{
	if (pop1->Eopid() != pop2->Eopid())
	{
		return false;
	}

	T *popScan2 = gpos::dyn_cast<T>(pop2).get();

	return pop1->UlOriginOpId() == popScan2->UlOriginOpId() &&
		   pop1->Ptabdesc()->MDId()->Equals(popScan2->Ptabdesc()->MDId()) &&
		   pop1->PdrgpcrOutput()->Equals(popScan2->PdrgpcrOutput());
}
}  // namespace gpopt


#ifdef GPOS_DEBUG

// helper to print given expression
// outside of namespace to make sure gdb can resolve the symbol easily
void PrintExpr(void *pv);

// helper to print memo structure
void PrintMemo(void *pv);

#endif	// GPOS_DEBUG

#endif	// !GPOPT_CUtils_H

// EOF
