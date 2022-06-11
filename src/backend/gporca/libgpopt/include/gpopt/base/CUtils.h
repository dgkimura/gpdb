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
	static BOOL FScalarConstBool(gpos::pointer<CExpression *> pexpr,
								 BOOL value);

	// check if two expressions have the same children in any order
	static BOOL FMatchChildrenUnordered(
		gpos::pointer<const CExpression *> pexprLeft,
		gpos::pointer<const CExpression *> pexprRight);

	// check if two expressions have the same children in the same order
	static BOOL FMatchChildrenOrdered(
		gpos::pointer<const CExpression *> pexprLeft,
		gpos::pointer<const CExpression *> pexprRight);

	// checks that the given type has all the comparisons: Eq, NEq, L, LEq, G, GEq
	static BOOL FHasAllDefaultComparisons(
		gpos::pointer<const IMDType *> pmdtype);

	//	append the expressions in the source array to destination array
	static void AppendArrayExpr(
		gpos::pointer<CExpressionArray *> pdrgpexprSrc,
		gpos::pointer<CExpressionArray *> pdrgpexprDest);

public:
	enum EExecLocalityType
	{
		EeltMaster,
		EeltSegments,
		EeltSingleton
	};

#ifdef GPOS_DEBUG

	// print given expression to debug trace
	static void PrintExpression(CExpression *pexpr);

	// print memo to debug trace
	static void PrintMemo(CMemo *pmemo);

#endif	// GPOS_DEBUG

	static IOstream &OsPrintDrgPcoldesc(
		IOstream &os,
		gpos::pointer<CColumnDescriptorArray *> pdrgpcoldescIncludedCols,
		ULONG length);

	//-------------------------------------------------------------------
	// Helpers for generating expressions
	//-------------------------------------------------------------------

	// recursively check for a plan with CTE, if both CTEProducer and CTEConsumer are executed on the same locality.
	// raises an exception if CTE Producer and CTE Consumer does not have the same locality
	static void ValidateCTEProducerConsumerLocality(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
		EExecLocalityType edt, gpos::owner<UlongToUlongMap *> phmulul);

	// generate a comparison expression for two column references
	static gpos::owner<CExpression *> PexprScalarCmp(
		CMemoryPool *mp, const CColRef *pcrLeft, const CColRef *pcrRight,
		CWStringConst strOp, gpos::owner<IMDId *> mdid_op);

	// generate a comparison expression for a column reference and an expression
	static gpos::owner<CExpression *> PexprScalarCmp(
		CMemoryPool *mp, const CColRef *pcrLeft,
		gpos::owner<CExpression *> pexprRight, CWStringConst strOp,
		gpos::owner<IMDId *> mdid_op);

	// generate a comparison expression for an expression and a column reference
	static gpos::owner<CExpression *> PexprScalarCmp(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
		const CColRef *pcrRight, CWStringConst strOp,
		gpos::owner<IMDId *> mdid_op);

	// generate a comparison expression for two expressions
	static gpos::owner<CExpression *> PexprScalarCmp(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
		gpos::owner<CExpression *> pexprRight, CWStringConst strOp,
		gpos::owner<IMDId *> mdid_op);

	// generate a comparison expression for two expressions
	static gpos::owner<CExpression *> PexprScalarCmp(CMemoryPool *mp,
													 CExpression *pexprLeft,
													 CExpression *pexprRight,
													 IMDId *mdid_scop);

	// generate a comparison expression for a column reference and an expression
	static gpos::owner<CExpression *> PexprScalarCmp(
		CMemoryPool *mp, const CColRef *pcrLeft,
		gpos::owner<CExpression *> pexprRight, IMDType::ECmpType cmp_type);

	// generate a comparison expression between two column references
	static gpos::owner<CExpression *> PexprScalarCmp(
		CMemoryPool *mp, const CColRef *pcrLeft, const CColRef *pcrRight,
		IMDType::ECmpType cmp_type);

	// generate a comparison expression between an expression and a column reference
	static gpos::owner<CExpression *> PexprScalarCmp(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
		const CColRef *pcrRight, IMDType::ECmpType cmp_type);

	// generate a comparison expression for two expressions
	static gpos::owner<CExpression *> PexprScalarCmp(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
		gpos::owner<CExpression *> pexprRight, IMDType::ECmpType cmp_type);

	// generate a comparison against Zero
	static gpos::owner<CExpression *> PexprCmpWithZero(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
		gpos::pointer<IMDId *> mdid_type_left, IMDType::ECmpType ecmptype);

	// generate an equality comparison expression for column references
	static gpos::owner<CExpression *> PexprScalarEqCmp(CMemoryPool *mp,
													   const CColRef *pcrLeft,
													   const CColRef *pcrRight);

	// generate an equality comparison expression for two expressions
	static gpos::owner<CExpression *> PexprScalarEqCmp(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
		gpos::owner<CExpression *> pexprRight);

	// generate an equality comparison expression for a column reference and an expression
	static gpos::owner<CExpression *> PexprScalarEqCmp(
		CMemoryPool *mp, const CColRef *pcrLeft,
		gpos::owner<CExpression *> pexprRight);

	// generate an equality comparison expression for an expression and a column reference
	static gpos::owner<CExpression *> PexprScalarEqCmp(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
		const CColRef *pcrRight);

	// generate an array comparison expression for a column reference and an expression
	static gpos::owner<CExpression *> PexprScalarArrayCmp(
		CMemoryPool *mp, CScalarArrayCmp::EArrCmpType earrcmptype,
		IMDType::ECmpType ecmptype,
		gpos::owner<CExpressionArray *> pexprScalarChildren,
		const CColRef *colref);

	// generate an Is Distinct From expression
	static gpos::owner<CExpression *> PexprIDF(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
		gpos::owner<CExpression *> pexprRight);

	static gpos::owner<CExpression *> PexprIDF(CMemoryPool *mp,
											   CExpression *pexprLeft,
											   CExpression *pexprRight,
											   IMDId *mdid_scop);

	// generate an Is NOT Distinct From expression for two column references
	static gpos::owner<CExpression *> PexprINDF(CMemoryPool *mp,
												const CColRef *pcrLeft,
												const CColRef *pcrRight);

	// generate an Is NOT Distinct From expression for scalar expressions
	static gpos::owner<CExpression *> PexprINDF(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
		gpos::owner<CExpression *> pexprRight);

	static gpos::owner<CExpression *> PexprINDF(CMemoryPool *mp,
												CExpression *pexprLeft,
												CExpression *pexprRight,
												IMDId *mdid_scop);

	// generate an Is NULL expression
	static gpos::owner<CExpression *> PexprIsNull(
		CMemoryPool *mp, gpos::owner<CExpression *> pexpr);

	// generate an Is NOT NULL expression
	static gpos::owner<CExpression *> PexprIsNotNull(
		CMemoryPool *mp, gpos::owner<CExpression *> pexpr);

	// generate an Is NOT FALSE expression
	static gpos::owner<CExpression *> PexprIsNotFalse(
		CMemoryPool *mp, gpos::owner<CExpression *> pexpr);

	// find if a scalar expression uses a nullable columns from the output of a logical expression
	static BOOL FUsesNullableCol(CMemoryPool *mp,
								 gpos::pointer<CExpression *> pexprScalar,
								 gpos::pointer<CExpression *> pexprLogical);

	// generate a scalar op expression for a column reference and an expression
	static gpos::owner<CExpression *> PexprScalarOp(
		CMemoryPool *mp, const CColRef *pcrLeft,
		gpos::owner<CExpression *> pexpr, CWStringConst strOp,
		gpos::owner<IMDId *> mdid_op,
		gpos::owner<IMDId *> return_type_mdid = nullptr);

	// generate a scalar bool op expression
	static gpos::owner<CExpression *> PexprScalarBoolOp(
		CMemoryPool *mp, CScalarBoolOp::EBoolOperator eboolop,
		gpos::owner<CExpressionArray *> pdrgpexpr);

	// negate the given expression
	static gpos::owner<CExpression *> PexprNegate(
		CMemoryPool *mp, gpos::owner<CExpression *> pexpr);

	// generate a scalar ident expression
	static gpos::owner<CExpression *> PexprScalarIdent(CMemoryPool *mp,
													   const CColRef *colref);

	// generate a scalar project element expression
	static gpos::owner<CExpression *> PexprScalarProjectElement(
		CMemoryPool *mp, CColRef *colref, gpos::owner<CExpression *> pexpr);

	// generate an aggregate function operator
	static gpos::owner<CScalarAggFunc *> PopAggFunc(
		CMemoryPool *mp, gpos::owner<IMDId *> pmdidAggFunc,
		const CWStringConst *pstrAggFunc, BOOL is_distinct,
		EAggfuncStage eaggfuncstage, BOOL fSplit,
		gpos::owner<IMDId *>
			pmdidResolvedReturnType,  // return type to be used if original return type is ambiguous
		EAggfuncKind aggkind, gpos::owner<ULongPtrArray *> argtypes);

	// generate an aggregate function
	static gpos::owner<CExpression *> PexprAggFunc(
		CMemoryPool *mp, gpos::owner<IMDId *> pmdidAggFunc,
		const CWStringConst *pstrAggFunc, const CColRef *colref,
		BOOL is_distinct, EAggfuncStage eaggfuncstage, BOOL fSplit);

	// generate a count(*) expression
	static gpos::owner<CExpression *> PexprCountStar(CMemoryPool *mp);

	// generate a GbAgg with count(*) function over the given expression
	static gpos::owner<CExpression *> PexprCountStar(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLogical);

	// return True if passed expression is a Project Element defined on count(*)/count(any) agg
	static BOOL FCountAggProjElem(gpos::pointer<CExpression *> pexprPrjElem,
								  CColRef **ppcrCount);

	// check if given expression has a count(*)/count(Any) agg
	static BOOL FHasCountAgg(gpos::pointer<CExpression *> pexpr,
							 CColRef **ppcrCount);

	// check if given expression has count matching the given column, returns the Logical GroupBy Agg above
	static BOOL FHasCountAggMatchingColumn(
		gpos::pointer<const CExpression *> pexpr, const CColRef *colref,
		gpos::pointer<const CLogicalGbAgg *> *ppgbAgg);

	// generate a GbAgg with count(*) and sum(col) over the given expression
	static gpos::owner<CExpression *> PexprCountStarAndSum(
		CMemoryPool *mp, const CColRef *colref,
		gpos::owner<CExpression *> pexprLogical);

	// generate a sum(col) expression
	static gpos::owner<CExpression *> PexprSum(CMemoryPool *mp,
											   const CColRef *colref);

	// generate a GbAgg with sum(col) expressions for all columns in the given array
	static gpos::owner<CExpression *> PexprGbAggSum(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLogical,
		gpos::pointer<CColRefArray *> pdrgpcrSum);

	// generate a count(col) expression
	static gpos::owner<CExpression *> PexprCount(CMemoryPool *mp,
												 const CColRef *colref,
												 BOOL is_distinct);

	// generate a min(col) expression
	static gpos::owner<CExpression *> PexprMin(CMemoryPool *mp,
											   CMDAccessor *md_accessor,
											   const CColRef *colref);

	// generate an aggregate expression
	static gpos::owner<CExpression *> PexprAgg(CMemoryPool *mp,
											   CMDAccessor *md_accessor,
											   IMDType::EAggType agg_type,
											   const CColRef *colref,
											   BOOL is_distinct);

	// generate a select expression
	static gpos::owner<CExpression *> PexprLogicalSelect(
		CMemoryPool *mp, gpos::owner<CExpression *> pexpr,
		gpos::owner<CExpression *> pexprPredicate);

	// if predicate is True return logical expression, otherwise return a new select node
	static gpos::owner<CExpression *> PexprSafeSelect(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLogical,
		gpos::owner<CExpression *> pexprPredicate);

	// generate a select expression, if child is another Select expression collapse both Selects into one expression
	static gpos::owner<CExpression *> PexprCollapseSelect(
		CMemoryPool *mp, CExpression *pexpr, CExpression *pexprPredicate);

	// generate a project expression
	static gpos::owner<CExpression *> PexprLogicalProject(
		CMemoryPool *mp, gpos::owner<CExpression *> pexpr,
		gpos::owner<CExpression *> pexprPrjList, BOOL fNewComputedCol);

	// generate a sequence project expression
	static gpos::owner<CExpression *> PexprLogicalSequenceProject(
		CMemoryPool *mp, gpos::owner<CDistributionSpec *> pds,
		gpos::owner<COrderSpecArray *> pdrgpos,
		gpos::owner<CWindowFrameArray *> pdrgpwf,
		gpos::owner<CExpression *> pexpr,
		gpos::owner<CExpression *> pexprPrjList);

	// generate a projection of NULL constants
	// to the map 'colref_mapping', and add the mappings to the colref_mapping map if not NULL
	static gpos::owner<CExpression *> PexprLogicalProjectNulls(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> colref_array,
		gpos::owner<CExpression *> pexpr,
		gpos::pointer<UlongToColRefMap *> colref_mapping = nullptr);

	// construct a project list using the given columns and datums
	// store the mapping in the colref_mapping map if not NULL
	static gpos::owner<CExpression *> PexprScalarProjListConst(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<IDatumArray *> pdrgpdatum,
		gpos::pointer<UlongToColRefMap *> colref_mapping);

	// generate a project expression
	static gpos::owner<CExpression *> PexprAddProjection(
		CMemoryPool *mp, gpos::owner<CExpression *> pexpr,
		gpos::owner<CExpression *> pexprProjected);

	// generate a project expression with one or more additional project elements
	static gpos::owner<CExpression *> PexprAddProjection(
		CMemoryPool *mp, gpos::owner<CExpression *> pexpr,
		gpos::pointer<CExpressionArray *> pdrgpexprProjected,
		BOOL fNewComputedCol = true);

	// generate an aggregate expression
	static gpos::owner<CExpression *> PexprLogicalGbAggGlobal(
		CMemoryPool *mp, gpos::owner<CColRefArray *> colref_array,
		gpos::owner<CExpression *> pexpr, gpos::owner<CExpression *> pexprPrL);

	// generate an aggregate expression
	static gpos::owner<CExpression *> PexprLogicalGbAgg(
		CMemoryPool *mp, gpos::owner<CColRefArray *> colref_array,
		gpos::owner<CExpression *> pexpr, gpos::owner<CExpression *> pexprPrL,
		COperator::EGbAggType egbaggtype);

	// check if the aggregate is local or global
	static BOOL FHasGlobalAggFunc(
		gpos::pointer<const CExpression *> pexprProjList);

	// generate a bool expression
	static gpos::owner<CExpression *> PexprScalarConstBool(
		CMemoryPool *mp, BOOL value, BOOL is_null = false);

	// generate an int4 expression
	static gpos::owner<CExpression *> PexprScalarConstInt4(CMemoryPool *mp,
														   INT val);

	// generate an int8 expression
	static gpos::owner<CExpression *> PexprScalarConstInt8(
		CMemoryPool *mp, LINT val, BOOL is_null = false);

	// generate an oid constant expression
	static gpos::owner<CExpression *> PexprScalarConstOid(CMemoryPool *mp,
														  OID oid_val);

	// generate a NULL constant of a given type
	static gpos::owner<CExpression *> PexprScalarConstNull(
		CMemoryPool *mp, gpos::pointer<const IMDType *> typ, INT type_modifier);

	// comparison operator type
	static IMDType::ECmpType ParseCmpType(gpos::pointer<IMDId *> mdid);

	// comparison operator type
	static IMDType::ECmpType ParseCmpType(CMDAccessor *md_accessor,
										  gpos::pointer<IMDId *> mdid);

	// generate a binary join expression
	template <class T>
	static gpos::owner<CExpression *> PexprLogicalJoin(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
		gpos::owner<CExpression *> pexprRight,
		gpos::owner<CExpression *> pexprPredicate,
		CXform::EXformId origin_xform = CXform::ExfSentinel);

	// generate an apply expression
	template <class T>
	static gpos::owner<CExpression *> PexprLogicalApply(
		CMemoryPool *mp, CExpression *pexprLeft, CExpression *pexprRight,
		CExpression *pexprPred = nullptr);

	// generate an apply expression with a known inner column
	template <class T>
	static gpos::owner<CExpression *> PexprLogicalApply(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
		gpos::owner<CExpression *> pexprRight, const CColRef *pcrInner,
		COperator::EOperatorId eopidOriginSubq,
		CExpression *pexprPred = nullptr);

	// generate an apply expression with a known array of inner columns
	template <class T>
	static gpos::owner<CExpression *> PexprLogicalApply(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
		gpos::owner<CExpression *> pexprRight,
		gpos::owner<CColRefArray *> pdrgpcrInner,
		COperator::EOperatorId eopidOriginSubq,
		CExpression *pexprPred = nullptr);

	// generate a correlated apply for quantified subquery with a known array of inner columns
	template <class T>
	static gpos::owner<CExpression *> PexprLogicalCorrelatedQuantifiedApply(
		CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
		gpos::owner<CExpression *> pexprRight,
		gpos::owner<CColRefArray *> pdrgpcrInner,
		COperator::EOperatorId eopidOriginSubq,
		CExpression *pexprPred = nullptr);

	//-------------------------------------------------------------------
	// Helpers for partitioning
	//-------------------------------------------------------------------

	// extract the nth partition key from the given array of partition keys
	static CColRef *PcrExtractPartKey(
		gpos::pointer<CColRef2dArray *> pdrgpdrgpcr, ULONG ulLevel);

	//-------------------------------------------------------------------
	// Helpers for comparisons
	//-------------------------------------------------------------------

	// deduplicate array of expressions
	static gpos::owner<CExpressionArray *> PdrgpexprDedup(
		CMemoryPool *mp, gpos::pointer<CExpressionArray *> pdrgpexpr);

	// deep equality of expression trees
	static BOOL Equals(gpos::pointer<const CExpression *> pexprLeft,
					   gpos::pointer<const CExpression *> pexprRight);

	// compare expression against an array of expressions
	static BOOL FEqualAny(gpos::pointer<const CExpression *> pexpr,
						  gpos::pointer<const CExpressionArray *> pdrgpexpr);

	// deep equality of expression arrays
	static BOOL Equals(gpos::pointer<const CExpressionArray *> pdrgpexprLeft,
					   gpos::pointer<const CExpressionArray *> pdrgpexprRight);

	// check if first expression array contains all expressions in second array
	static BOOL Contains(gpos::pointer<const CExpressionArray *> pdrgpexprFst,
						 gpos::pointer<const CExpressionArray *> pdrgpexprSnd);

	// return the number of occurrences of the given expression in the given
	// array of expressions
	static ULONG UlOccurrences(gpos::pointer<const CExpression *> pexpr,
							   gpos::pointer<CExpressionArray *> pdrgpexpr);

	//-------------------------------------------------------------------
	// Helpers for datums
	//-------------------------------------------------------------------

	// check to see if the expression is a scalar const TRUE
	static BOOL FScalarConstTrue(gpos::pointer<CExpression *> pexpr);

	// check to see if the expression is a scalar const FALSE
	static BOOL FScalarConstFalse(gpos::pointer<CExpression *> pexpr);

	// check if the given expression is an INT, the template parameter is an INT type
	template <class T>
	static BOOL FScalarConstInt(gpos::pointer<CExpression *> pexpr);

	//-------------------------------------------------------------------
	// Helpers for printing
	//-------------------------------------------------------------------

	// column reference array print helper
	static IOstream &OsPrintDrgPcr(
		IOstream &os, gpos::pointer<const CColRefArray *> colref_array,
		ULONG = gpos::ulong_max);

	//-------------------------------------------------------------------
	// Helpers for column reference sets
	//-------------------------------------------------------------------

	// create an array of output columns including a key for grouping
	static gpos::owner<CColRefArray *> PdrgpcrGroupingKey(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
		gpos::owner<CColRefArray *> *ppdrgpcrKey);

	// add an equivalence class (col ref set) to the array. If the new equiv
	// class contains columns from existing equiv classes, then these are merged
	static gpos::owner<CColRefSetArray *> AddEquivClassToArray(
		CMemoryPool *mp, gpos::pointer<const CColRefSet *> pcrsNew,
		gpos::pointer<const CColRefSetArray *> pdrgpcrs);

	// merge 2 arrays of equivalence classes
	static gpos::owner<CColRefSetArray *> PdrgpcrsMergeEquivClasses(
		CMemoryPool *mp, CColRefSetArray *pdrgpcrsFst,
		gpos::pointer<CColRefSetArray *> pdrgpcrsSnd);

	// intersect 2 arrays of equivalence classes
	static gpos::owner<CColRefSetArray *> PdrgpcrsIntersectEquivClasses(
		CMemoryPool *mp, gpos::pointer<CColRefSetArray *> pdrgpcrsFst,
		gpos::pointer<CColRefSetArray *> pdrgpcrsSnd);

	// return a copy of equivalence classes from all children
	static gpos::owner<CColRefSetArray *> PdrgpcrsCopyChildEquivClasses(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

	// return a copy of the given array of columns, excluding the columns
	// in the given colrefset
	static gpos::owner<CColRefArray *> PdrgpcrExcludeColumns(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> pdrgpcrOriginal,
		gpos::pointer<CColRefSet *> pcrsExcluded);

	//-------------------------------------------------------------------
	// General helpers
	//-------------------------------------------------------------------

	// append elements from input array to output array, starting from given index, after add-refing them
	template <class T, void (*CleanupFn)(T *)>
	static void AddRefAppend(CDynamicPtrArray<T, CleanupFn> *pdrgptOutput,
							 CDynamicPtrArray<T, CleanupFn> *pdrgptInput,
							 ULONG ulStart = 0);

	template <class T>
	static void AddRefAppend(Vector<Ref<T>> *output,
							 const Vector<Ref<T>> *input, ULONG start = 0);

	// check for existence of subqueries
	static BOOL FHasSubquery(gpos::pointer<CExpression *> pexpr);

	// check existence of subqueries or Apply operators in deep expression tree
	static BOOL FHasSubqueryOrApply(gpos::pointer<CExpression *> pexpr,
									BOOL fCheckRoot = true);

	// check existence of correlated apply operators in deep expression tree
	static BOOL FHasCorrelatedApply(gpos::pointer<CExpression *> pexpr,
									BOOL fCheckRoot = true);

	// check for existence of CTE anchor
	static BOOL FHasCTEAnchor(gpos::pointer<CExpression *> pexpr);

	// check for existence of outer references
	static BOOL HasOuterRefs(gpos::pointer<CExpression *> pexpr);

	// check if a given operator is a logical join
	static BOOL FLogicalJoin(gpos::pointer<COperator *> pop);

	// check if a given operator is a logical set operation
	static BOOL FLogicalSetOp(gpos::pointer<COperator *> pop);

	// check if a given operator is a logical unary operator
	static BOOL FLogicalUnary(gpos::pointer<COperator *> pop);

	// check if a given operator is a physical join
	static BOOL FPhysicalJoin(gpos::pointer<COperator *> pop);

	// check if a given operator is a physical left outer join
	static BOOL FPhysicalLeftOuterJoin(gpos::pointer<COperator *> pop);

	// check if a given operator is a physical scan
	static BOOL FPhysicalScan(gpos::pointer<COperator *> pop);

	// check if a given operator is a physical agg
	static BOOL FPhysicalAgg(gpos::pointer<COperator *> pop);

	// check if given expression has any one stage agg nodes
	static BOOL FHasOneStagePhysicalAgg(
		gpos::pointer<const CExpression *> pexpr);

	// check if a given operator is a physical motion
	static BOOL FPhysicalMotion(gpos::pointer<COperator *> pop);

	// check if duplicate values can be generated by the given distribution spec
	static BOOL FDuplicateHazardDistributionSpec(
		gpos::pointer<CDistributionSpec *> pds);

	// check if duplicate values can be generated by the given Motion expression
	static BOOL FDuplicateHazardMotion(
		gpos::pointer<CExpression *> pexprMotion);

	// check if a given operator is an enforcer
	static BOOL FEnforcer(gpos::pointer<COperator *> pop);

	// check if a given operator is a hash join
	static BOOL FHashJoin(gpos::pointer<COperator *> pop);

	// check if a given operator is a correlated nested loops join
	static BOOL FCorrelatedNLJoin(gpos::pointer<COperator *> pop);

	// check if a given operator is a nested loops join
	static BOOL FNLJoin(gpos::pointer<COperator *> pop);

	// check if a given operator is an Apply
	static BOOL FApply(gpos::pointer<COperator *> pop);

	// check if a given operator is a correlated Apply
	static BOOL FCorrelatedApply(gpos::pointer<COperator *> pop);

	// check if a given operator is left semi apply
	static BOOL FLeftSemiApply(gpos::pointer<COperator *> pop);

	// check if a given operator is left anti semi apply
	static BOOL FLeftAntiSemiApply(gpos::pointer<COperator *> pop);

	// check if a given operator is a subquery
	static BOOL FSubquery(gpos::pointer<COperator *> pop);

	// check if a given operator is existential subquery
	static BOOL FExistentialSubquery(gpos::pointer<COperator *> pop);

	// check if a given operator is quantified subquery
	static BOOL FQuantifiedSubquery(gpos::pointer<COperator *> pop);

	// check if given expression is a Project Element with scalar subquery
	static BOOL FProjElemWithScalarSubq(gpos::pointer<CExpression *> pexpr);

	// check if given expression is a scalar subquery with a ConstTableGet as the only child
	static BOOL FScalarSubqWithConstTblGet(gpos::pointer<CExpression *> pexpr);

	// check if given expression is a Project on ConstTable with one scalar subquery in Project List
	static BOOL FProjectConstTableWithOneScalarSubq(
		gpos::pointer<CExpression *> pexpr);

	// check if an expression is a 0 offset
	static BOOL FScalarConstIntZero(gpos::pointer<CExpression *> pexprOffset);

	// check if a limit expression has 0 offset
	static BOOL FHasZeroOffset(gpos::pointer<CExpression *> pexpr);

	// check if expression is scalar comparison
	static BOOL FScalarCmp(gpos::pointer<CExpression *> pexpr);

	// check if expression is scalar array comparison
	static BOOL FScalarArrayCmp(gpos::pointer<CExpression *> pexpr);

	// check if given operator exists in the given list
	static BOOL FOpExists(gpos::pointer<const COperator *> pop,
						  const COperator::EOperatorId *peopid, ULONG ulOps);

	// check if given expression has any operator in the given list
	static BOOL FHasOp(gpos::pointer<const CExpression *> pexpr,
					   const COperator::EOperatorId *peopid, ULONG ulOps);

	// return number of inlinable CTEs in the given expression
	static ULONG UlInlinableCTEs(gpos::pointer<CExpression *> pexpr,
								 ULONG ulDepth = 1);

	// return number of joins in the given expression
	static ULONG UlJoins(gpos::pointer<CExpression *> pexpr);

	// return number of subqueries in the given expression
	static ULONG UlSubqueries(gpos::pointer<CExpression *> pexpr);

	// check if expression is scalar bool op
	static BOOL FScalarBoolOp(gpos::pointer<CExpression *> pexpr);

	// is the given expression a scalar bool op of the passed type?
	static BOOL FScalarBoolOp(gpos::pointer<CExpression *> pexpr,
							  CScalarBoolOp::EBoolOperator eboolop);

	// check if expression is scalar null test
	static BOOL FScalarNullTest(gpos::pointer<CExpression *> pexpr);

	// check if given expression is a NOT NULL predicate
	static BOOL FScalarNotNull(gpos::pointer<CExpression *> pexpr);

	// check if expression is scalar identifier
	static BOOL FScalarIdent(gpos::pointer<CExpression *> pexpr);

	// check if expression is scalar identifier (with or without a cast)
	static BOOL FScalarIdentIgnoreCast(gpos::pointer<CExpression *> pexpr);

	static BOOL FScalarConstAndScalarIdentArray(
		gpos::pointer<CExpression *> pexprArray);

	// check if expression is scalar identifier of boolean type
	static BOOL FScalarIdentBoolType(gpos::pointer<CExpression *> pexpr);

	// check if expression is scalar array
	static BOOL FScalarArray(gpos::pointer<CExpression *> pexpr);

	// returns number of children or constants of it is all constants
	static ULONG UlScalarArrayArity(gpos::pointer<CExpression *> pexpr);

	// returns constant operator of a scalar array expression
	static CScalarConst *PScalarArrayConstChildAt(
		gpos::pointer<CExpression *> pexprArray, ULONG ul);

	// returns constant expression of a scalar array expression
	static gpos::owner<CExpression *> PScalarArrayExprChildAt(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprArray, ULONG ul);

	// returns the scalar array expression child of CScalarArrayComp
	static CExpression *PexprScalarArrayChild(
		gpos::pointer<CExpression *> pexpr);

	// returns if the scalar array has all constant elements or children
	static BOOL FScalarConstArray(gpos::pointer<CExpression *> pexpr);

	// returns if the scalar constant is an array
	static BOOL FIsConstArray(gpos::pointer<CExpression *> pexpr);

	// returns MDId for gp_percentile based on return type
	static gpos::owner<CMDIdGPDB *> GetPercentileAggMDId(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprAggFn);

	// returns if the scalar constant array has already been collapased
	static BOOL FScalarArrayCollapsed(gpos::pointer<CExpression *> pexprArray);

	// returns true if the subquery is a ScalarSubqueryAny
	static BOOL FAnySubquery(gpos::pointer<COperator *> pop);

	// returns the expression under the Nth project element of a CLogicalProject
	static CExpression *PNthProjectElementExpr(
		gpos::pointer<CExpression *> pexpr, ULONG ul);

	// check if the Project list has an inner reference assuming project list has one projecet element
	static BOOL FInnerRefInProjectList(gpos::pointer<CExpression *> pexpr);

	// Check if expression tree has a col being referenced in the CColRefSet passed as input
	static BOOL FExprHasAnyCrFromCrs(gpos::pointer<CExpression *> pexpr,
									 gpos::pointer<CColRefSet *> pcrs);

	// If it's a scalar array of all CScalarConst, collapse it into a single
	// expression but keep the constants in the operator.
	static gpos::owner<CExpression *> PexprCollapseConstArray(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprArray);

	// check if expression is scalar array coerce
	static BOOL FScalarArrayCoerce(gpos::pointer<CExpression *> pexpr);

	// is the given expression a scalar identifier with the given column reference
	static BOOL FScalarIdent(gpos::pointer<CExpression *> pexpr,
							 CColRef *colref);

	// check if expression is scalar const
	static BOOL FScalarConst(gpos::pointer<CExpression *> pexpr);

	// check if this is a variable-free expression
	static BOOL FVarFreeExpr(gpos::pointer<CExpression *> pexpr);

	// check if expression is a predicate
	static BOOL FPredicate(gpos::pointer<CExpression *> pexpr);

	// is this type supported in contradiction detection using stats logic
	static BOOL FIntType(gpos::pointer<IMDId *> mdid_type);

	// is this type supported in contradiction detection
	static BOOL FConstrainableType(gpos::pointer<IMDId *> mdid_type);

	// check if a binary operator uses only columns produced by its children
	static BOOL FUsesChildColsOnly(CExpressionHandle &exprhdl);

	// check if inner child of a binary operator uses columns not produced by outer child
	static BOOL FInnerUsesExternalCols(CExpressionHandle &exprhdl);

	// check if inner child of a binary operator uses only columns not produced by outer child
	static BOOL FInnerUsesExternalColsOnly(CExpressionHandle &exprhdl);

	// check if comparison operators are available for the given columns
	static BOOL FComparisonPossible(gpos::pointer<CColRefArray *> colref_array,
									IMDType::ECmpType cmp_type);

	static ULONG UlCountOperator(gpos::pointer<const CExpression *> pexpr,
								 COperator::EOperatorId op_id);

	// return the max subset of redistributable columns for the given columns
	static gpos::owner<CColRefArray *> PdrgpcrRedistributableSubset(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> colref_array);

	// check if hashing is possible for the given columns
	static BOOL IsHashable(gpos::pointer<CColRefArray *> colref_array);

	// check if the given operator is a logical DML operator
	static BOOL FLogicalDML(gpos::pointer<COperator *> pop);

	// return regular string from wide-character string
	static CHAR *CreateMultiByteCharStringFromWCString(CMemoryPool *mp,
													   WCHAR *wsz);

	// return column reference defined by project element
	static CColRef *PcrFromProjElem(gpos::pointer<CExpression *> pexprPrEl);

	// construct an array of colids from the given array of column references
	static gpos::owner<ULongPtrArray *> Pdrgpul(
		CMemoryPool *mp, gpos::pointer<const CColRefArray *> colref_array);

	// generate a timestamp-based file name
	static void GenerateFileName(CHAR *buf, const CHAR *szPrefix,
								 const CHAR *szExt, ULONG length,
								 ULONG ulSessionId, ULONG ulCmdId);

	// return the mapping of the given colref based on the given hashmap
	static CColRef *PcrRemap(const CColRef *colref,
							 gpos::pointer<UlongToColRefMap *> colref_mapping,
							 BOOL must_exist);

	// create a new colrefset corresponding to the given colrefset
	// and based on the given mapping
	static gpos::owner<CColRefSet *> PcrsRemap(
		CMemoryPool *mp, gpos::pointer<CColRefSet *> pcrs,
		gpos::pointer<UlongToColRefMap *> colref_mapping, BOOL must_exist);

	// create an array of column references corresponding to the given array
	// and based on the given mapping
	static gpos::owner<CColRefArray *> PdrgpcrRemap(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<UlongToColRefMap *> colref_mapping, BOOL must_exist);

	// create an array of column references corresponding to the given array
	// and based on the given mapping and create new colrefs if necessary
	static gpos::owner<CColRefArray *> PdrgpcrRemapAndCreate(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<UlongToColRefMap *> colref_mapping);

	// create an array of column arrays corresponding to the given array
	// and based on the given mapping
	static gpos::owner<CColRef2dArray *> PdrgpdrgpcrRemap(
		CMemoryPool *mp, gpos::pointer<CColRef2dArray *> pdrgpdrgpcr,
		gpos::pointer<UlongToColRefMap *> colref_mapping, BOOL must_exist);

	// remap given array of expressions with provided column mappings
	static gpos::owner<CExpressionArray *> PdrgpexprRemap(
		CMemoryPool *mp, gpos::pointer<CExpressionArray *> pdrgpexpr,
		gpos::pointer<UlongToColRefMap *> colref_mapping);

	// create ColRef->ColRef mapping using the given ColRef arrays
	static gpos::owner<UlongToColRefMap *> PhmulcrMapping(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> pdrgpcrFrom,
		gpos::pointer<CColRefArray *> pdrgpcrTo);

	// add col ID->ColRef mappings to the given hashmap based on the
	// given ColRef arrays
	static void AddColumnMapping(
		CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
		gpos::pointer<CColRefArray *> pdrgpcrFrom,
		gpos::pointer<CColRefArray *> pdrgpcrTo);

	// create a copy of the array of column references
	static gpos::owner<CColRefArray *> PdrgpcrExactCopy(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> colref_array);

	// create an array of new column references with the same names and
	// types as the given column references.
	// if the passed map is not null, mappings from old to copied variables are added to it
	static gpos::owner<CColRefArray *> PdrgpcrCopy(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> colref_array,
		BOOL fAllComputed = false,
		gpos::pointer<UlongToColRefMap *> colref_mapping = nullptr);

	// equality check between two arrays of column refs. Inputs can be NULL
	static BOOL Equals(gpos::pointer<CColRefArray *> pdrgpcrFst,
					   gpos::pointer<CColRefArray *> pdrgpcrSnd);

	// compute hash value for an array of column references
	static ULONG UlHashColArray(
		gpos::pointer<const CColRefArray *> colref_array,
		const ULONG ulMaxCols = 5);

	// return the set of column reference from the CTE Producer corresponding to the
	// subset of input columns from the CTE Consumer
	static gpos::owner<CColRefSet *> PcrsCTEProducerColumns(
		CMemoryPool *mp, gpos::pointer<CColRefSet *> pcrsInput,
		gpos::pointer<CLogicalCTEConsumer *> popCTEConsumer);

	// construct the join condition (AND-tree of INDF operators)
	// from the array of input columns reference arrays (aligned)
	static gpos::owner<CExpression *> PexprConjINDFCond(
		CMemoryPool *mp, gpos::pointer<CColRef2dArray *> pdrgpdrgpcrInput);

	// check whether a colref array contains repeated items
	static BOOL FHasDuplicates(
		gpos::pointer<const CColRefArray *> colref_array);

	// cast the input expression to the destination mdid
	static gpos::owner<CExpression *> PexprCast(CMemoryPool *mp,
												CMDAccessor *md_accessor,
												CExpression *pexpr,
												IMDId *mdid_dest);

	// construct a logical join expression of the given type, with the given children
	static gpos::owner<CExpression *> PexprLogicalJoin(
		CMemoryPool *mp, EdxlJoinType edxljointype,
		gpos::owner<CExpressionArray *> pdrgpexpr);

	// construct an array of scalar ident expressions from the given array
	// of column references
	static gpos::owner<CExpressionArray *> PdrgpexprScalarIdents(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> colref_array);

	// return the columns from the scalar ident expressions in the given array
	static gpos::owner<CColRefSet *> PcrsExtractColumns(
		CMemoryPool *mp, gpos::pointer<const CExpressionArray *> pdrgpexpr);

	// create a new bitset of the given length, where all the bits are set
	static gpos::owner<CBitSet *> PbsAllSet(CMemoryPool *mp, ULONG size);

	// return a new bitset, setting the bits in the given array
	static gpos::owner<CBitSet *> Pbs(CMemoryPool *mp,
									  gpos::pointer<ULongPtrArray *> pdrgpul);

	// helper to create a dummy constant table expression
	static gpos::owner<CExpression *> PexprLogicalCTGDummy(CMemoryPool *mp);

	// map a column from source array to destination array based on position
	static CColRef *PcrMap(CColRef *pcrSource,
						   gpos::pointer<CColRefArray *> pdrgpcrSource,
						   gpos::pointer<CColRefArray *> pdrgpcrTarget);

	//	return index of the set containing given column
	static ULONG UlPcrIndexContainingSet(
		gpos::pointer<CColRefSetArray *> pdrgpcrs, const CColRef *colref);

	// collapse the top two project nodes, if unable return NULL
	static gpos::owner<CExpression *> PexprCollapseProjects(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr);

	// match function between index get/scan operators
	template <class T>
	static BOOL FMatchIndex(T *pop1, gpos::pointer<COperator *> pop2);

	// match function between dynamic index get/scan operators
	template <class T>
	static BOOL FMatchDynamicIndex(T *pop1, gpos::pointer<COperator *> pop2);

	// match function between dynamic get/scan operators
	template <class T>
	static BOOL FMatchDynamicScan(T *pop1, gpos::pointer<COperator *> pop2);

	// match function between dynamic bitmap get/scan operators
	template <class T>
	static BOOL FMatchDynamicBitmapScan(T *pop1,
										gpos::pointer<COperator *> pop2);

	// match function between bitmap get/scan operators
	template <class T>
	static BOOL FMatchBitmapScan(T *pop1, gpos::pointer<COperator *> pop2);

	// compares two Idatums, useful for sorting functions
	static INT IDatumCmp(const void *val1, const void *val2);

	// compares two CPoints, useful for sorting functions
	static INT CPointCmp(const void *val1, const void *val2);

	// check if the equivalance classes are disjoint
	static BOOL FEquivalanceClassesDisjoint(
		CMemoryPool *mp, gpos::pointer<const CColRefSetArray *> pdrgpcrs);

	// check if the equivalance classes are same
	static BOOL FEquivalanceClassesEqual(
		CMemoryPool *mp, gpos::pointer<CColRefSetArray *> pdrgpcrsFst,
		gpos::pointer<CColRefSetArray *> pdrgpcrsSnd);

	// get execution locality
	static EExecLocalityType ExecLocalityType(
		gpos::pointer<CDistributionSpec *> pds);

	// generate a limit expression on top of the given relational child with the given offset and limit count
	static gpos::owner<CExpression *> PexprLimit(
		CMemoryPool *mp, gpos::owner<CExpression *> pexpr, ULONG ulOffSet,
		ULONG count);

	// generate part oid
	static BOOL FGeneratePartOid(gpos::pointer<IMDId *> mdid);

	// return true if given expression contains window aggregate function
	static BOOL FHasAggWindowFunc(gpos::pointer<CExpression *> pexpr);

	// return true if given expression contains ordered aggregate function
	static BOOL FHasOrderedAggToSplit(gpos::pointer<CExpression *> pexpr);

	// return true if the given expression is a cross join
	static BOOL FCrossJoin(gpos::pointer<CExpression *> pexpr);

	// return true if can create hash join for the expression
	static BOOL IsHashJoinPossible(CMemoryPool *mp,
								   gpos::pointer<CExpression *> pexpr);

	// is this scalar expression an NDV-preserving function (used for join stats derivation)
	static BOOL IsExprNDVPreserving(gpos::pointer<CExpression *> pexpr,
									const CColRef **underlying_colref);

	// search the given array of predicates for predicates with equality or IS NOT
	// DISTINCT FROM operators that has one side equal to the given expression
	static CExpression *PexprMatchEqualityOrINDF(
		CExpression *pexprToMatch,
		gpos::pointer<CExpressionArray *>
			pdrgpexpr  // array of predicates to inspect
	);

	static gpos::owner<CExpression *> MakeJoinWithoutInferredPreds(
		CMemoryPool *mp, gpos::pointer<CExpression *> join_expr);

	static BOOL Contains(gpos::pointer<const CExpressionArray *> exprs,
						 gpos::pointer<CExpression *> expr_to_match);

	static BOOL Equals(
		gpos::pointer<const CExpressionArrays *> exprs_arr,
		gpos::pointer<const CExpressionArrays *> other_exprs_arr);

	static BOOL Equals(gpos::pointer<const IMdIdArray *> mdids,
					   gpos::pointer<const IMdIdArray *> other_mdids);

	static BOOL Equals(gpos::pointer<const IMDId *> mdid,
					   gpos::pointer<const IMDId *> other_mdid);

	static BOOL CanRemoveInferredPredicates(COperator::EOperatorId op_id);

	static gpos::owner<CExpressionArrays *> GetCombinedExpressionArrays(
		CMemoryPool *mp, gpos::pointer<CExpressionArrays *> exprs_array,
		gpos::pointer<CExpressionArrays *> exprs_array_other);

	static void AddExprs(gpos::pointer<CExpressionArrays *> results_exprs,
						 gpos::pointer<CExpressionArrays *> input_exprs);

	static BOOL FScalarConstBoolNull(gpos::pointer<CExpression *> pexpr);
};	// class CUtils

// hash set from expressions
using ExprHashSet = CHashSet<CExpression, CExpression::UlHashDedup,
							 CUtils::Equals, CleanupRelease<CExpression>>;


//---------------------------------------------------------------------------
//	@function:
//		CUtils::PexprLogicalJoin
//
//	@doc:
//		Generate a join expression from given expressions
//
//---------------------------------------------------------------------------
template <class T>
gpos::owner<CExpression *>
CUtils::PexprLogicalJoin(CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
						 gpos::owner<CExpression *> pexprRight,
						 gpos::owner<CExpression *> pexprPredicate,
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
gpos::owner<CExpression *>
CUtils::PexprLogicalApply(CMemoryPool *mp, CExpression *pexprLeft,
						  CExpression *pexprRight, CExpression *pexprPred)
{
	GPOS_ASSERT(nullptr != pexprLeft);
	GPOS_ASSERT(nullptr != pexprRight);

	gpos::owner<CExpression *> pexprScalar = pexprPred;
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
gpos::owner<CExpression *>
CUtils::PexprLogicalApply(CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
						  gpos::owner<CExpression *> pexprRight,
						  const CColRef *pcrInner,
						  COperator::EOperatorId eopidOriginSubq,
						  CExpression *pexprPred)
{
	GPOS_ASSERT(nullptr != pexprLeft);
	GPOS_ASSERT(nullptr != pexprRight);
	GPOS_ASSERT(nullptr != pcrInner);

	gpos::owner<CExpression *> pexprScalar = pexprPred;
	if (nullptr == pexprPred)
	{
		pexprScalar = PexprScalarConstBool(mp, true /*value*/);
	}

	gpos::owner<CColRefArray *> colref_array = GPOS_NEW(mp) CColRefArray(mp);
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
gpos::owner<CExpression *>
CUtils::PexprLogicalApply(CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
						  gpos::owner<CExpression *> pexprRight,
						  gpos::owner<CColRefArray *> pdrgpcrInner,
						  COperator::EOperatorId eopidOriginSubq,
						  CExpression *pexprPred)
{
	GPOS_ASSERT(nullptr != pexprLeft);
	GPOS_ASSERT(nullptr != pexprRight);
	GPOS_ASSERT(nullptr != pdrgpcrInner);
	GPOS_ASSERT(0 < pdrgpcrInner->Size());

	gpos::owner<CExpression *> pexprScalar = pexprPred;
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
gpos::owner<CExpression *>
CUtils::PexprLogicalCorrelatedQuantifiedApply(
	CMemoryPool *mp, gpos::owner<CExpression *> pexprLeft,
	gpos::owner<CExpression *> pexprRight,
	gpos::owner<CColRefArray *> pdrgpcrInner,
	COperator::EOperatorId eopidOriginSubq, CExpression *pexprPred)
{
	GPOS_ASSERT(nullptr != pexprLeft);
	GPOS_ASSERT(nullptr != pexprRight);
	GPOS_ASSERT(nullptr != pdrgpcrInner);
	GPOS_ASSERT(0 < pdrgpcrInner->Size());

	gpos::owner<CExpression *> pexprScalar = pexprPred;
	if (nullptr == pexprPred)
	{
		pexprScalar = PexprScalarConstBool(mp, true /*value*/);
	}

	if (COperator::EopLogicalSelect != pexprRight->Pop()->Eopid())
	{
		// quantified comparison was pushed down, we create a dummy comparison here
		GPOS_ASSERT(
			!CUtils::HasOuterRefs(pexprRight) &&
			"unexpected outer references in inner child of Semi Apply expression ");
		pexprScalar->Release();
		pexprScalar = PexprScalarConstBool(mp, true /*value*/);
	}
	else
	{
		// quantified comparison is now on top of inner expression, skip to child
		(*pexprRight)[1]->AddRef();
		CExpression *pexprNewPredicate = (*pexprRight)[1];
		pexprScalar->Release();
		pexprScalar = pexprNewPredicate;

		(*pexprRight)[0]->AddRef();
		CExpression *pexprChild = (*pexprRight)[0];
		pexprRight->Release();
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
		gpos::owner<CRefCount *> prc = dynamic_cast<CRefCount *>(pt);
		prc->AddRef();
		pdrgptOutput->Append(pt);
	}
}

template <class T>
void
CUtils::AddRefAppend(Vector<Ref<T>> *output, const Vector<Ref<T>> *input,
					 ULONG start)
{
	for (const auto &t : gpos::subrange(*input).next(start))
	{
		output->Append(t);
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
CUtils::FScalarConstInt(gpos::pointer<CExpression *> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	IMDType::ETypeInfo type_info = T::GetTypeInfo();
	GPOS_ASSERT(IMDType::EtiInt2 == type_info ||
				IMDType::EtiInt4 == type_info || IMDType::EtiInt8 == type_info);

	gpos::pointer<COperator *> pop = pexpr->Pop();
	if (COperator::EopScalarConst == pop->Eopid())
	{
		gpos::pointer<CScalarConst *> popScalarConst =
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
CUtils::FMatchIndex(T *pop1, gpos::pointer<COperator *> pop2)
{
	if (pop1->Eopid() != pop2->Eopid())
	{
		return false;
	}

	T *popIndex = gpos::dyn_cast<T>(pop2);

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
CUtils::FMatchDynamicIndex(T *pop1, gpos::pointer<COperator *> pop2)
{
	if (pop1->Eopid() != pop2->Eopid())
	{
		return false;
	}

	T *popIndex2 = gpos::dyn_cast<T>(pop2);

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
CUtils::FMatchDynamicScan(T *pop1, gpos::pointer<COperator *> pop2)
{
	if (pop1->Eopid() != pop2->Eopid())
	{
		return false;
	}

	T *popScan2 = gpos::dyn_cast<T>(pop2);

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
CUtils::FMatchDynamicBitmapScan(T *pop1, gpos::pointer<COperator *> pop2)
{
	if (pop1->Eopid() != pop2->Eopid())
	{
		return false;
	}

	T *popDynamicBitmapScan2 = gpos::dyn_cast<T>(pop2);

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
CUtils::FMatchBitmapScan(T *pop1, gpos::pointer<COperator *> pop2)
{
	if (pop1->Eopid() != pop2->Eopid())
	{
		return false;
	}

	T *popScan2 = gpos::dyn_cast<T>(pop2);

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
