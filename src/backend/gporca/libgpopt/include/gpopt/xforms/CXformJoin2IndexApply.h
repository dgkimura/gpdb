//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 VMware, Inc. or its affiliates.
//
//	Transform Inner/Outer Join to Index Apply
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformJoin2IndexApply_H
#define GPOPT_CXformJoin2IndexApply_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/xforms/CXformExploration.h"
#include "gpopt/xforms/CXformUtils.h"

namespace gpopt
{
using namespace gpos;

// fwd declaration
class CLogicalDynamicGet;

class CXformJoin2IndexApply : public CXformExploration
{
private:
	// helper to add IndexApply expression to given xform results container
	// for homogeneous b-tree indexes
	static void CreateHomogeneousBtreeIndexApplyAlternatives(
		CMemoryPool *mp, gpos::Ref<COperator> joinOp,
		gpos::Ref<CExpression> pexprOuter, gpos::Ref<CExpression> pexprInner,
		gpos::Ref<CExpression> pexprScalar, gpos::Ref<CExpression> origJoinPred,
		gpos::Ref<CExpression> nodesToInsertAboveIndexGet,
		gpos::Ref<CExpression> endOfNodesToInsertAboveIndexGet,
		gpos::Ref<CTableDescriptor> ptabdescInner,
		gpos::Ref<CColRefSet> pcrsScalarExpr, gpos::Ref<CColRefSet> outer_refs,
		gpos::Ref<CColRefSet> pcrsReqd, ULONG ulIndices,
		gpos::Ref<CXformResult> pxfres);

	// helper to add IndexApply expression to given xform results container
	// for homogeneous b-tree indexes
	static void CreateAlternativesForBtreeIndex(
		CMemoryPool *mp, gpos::Ref<COperator> joinOp,
		gpos::Ref<CExpression> pexprOuter, gpos::Ref<CExpression> pexprInner,
		gpos::Ref<CExpression> origJoinPred,
		gpos::Ref<CExpression> nodesToInsertAboveIndexGet,
		gpos::Ref<CExpression> endOfNodesToInsertAboveIndexGet,
		CMDAccessor *md_accessor,
		gpos::Ref<CExpressionArray> pdrgpexprConjuncts,
		gpos::Ref<CColRefSet> pcrsScalarExpr, gpos::Ref<CColRefSet> outer_refs,
		gpos::Ref<CColRefSet> pcrsReqd, const IMDRelation *pmdrel,
		const IMDIndex *pmdindex, gpos::Ref<CXformResult> pxfres);

	// helper to add IndexApply expression to given xform results container
	// for homogeneous bitmap indexes
	static void CreateHomogeneousBitmapIndexApplyAlternatives(
		CMemoryPool *mp, gpos::Ref<COperator> joinOp,
		gpos::Ref<CExpression> pexprOuter, gpos::Ref<CExpression> pexprInner,
		gpos::Ref<CExpression> pexprScalar, gpos::Ref<CExpression> origJoinPred,
		gpos::Ref<CExpression> nodesToInsertAboveIndexGet,
		gpos::Ref<CExpression> endOfNodesToInsertAboveIndexGet,
		gpos::Ref<CTableDescriptor> ptabdescInner,
		gpos::Ref<CColRefSet> outer_refs, gpos::Ref<CColRefSet> pcrsReqd,
		gpos::Ref<CXformResult> pxfres);

	// based on the inner and the scalar expression, it computes scalar expression
	// columns, outer references and required columns
	static void ComputeColumnSets(CMemoryPool *mp,
								  gpos::Ref<CExpression> pexprInner,
								  gpos::Ref<CExpression> pexprScalar,
								  gpos::Ref<CColRefSet> *ppcrsScalarExpr,
								  gpos::Ref<CColRefSet> *ppcrsOuterRefs,
								  gpos::Ref<CColRefSet> *ppcrsReqd);

protected:
	// is the logical join that is being transformed an outer join?
	BOOL m_fOuterJoin;

	// helper to add IndexApply expression to given xform results container
	// for homogeneous indexes
	virtual void CreateHomogeneousIndexApplyAlternatives(
		CMemoryPool *mp, gpos::Ref<COperator> joinOp,
		gpos::Ref<CExpression> pexprOuter, gpos::Ref<CExpression> pexprInner,
		gpos::Ref<CExpression> pexprScalar, gpos::Ref<CExpression> origJoinPred,
		gpos::Ref<CExpression> nodesToInsertAboveIndexGet,
		gpos::Ref<CExpression> endOfNodesToInsertAboveIndexGet,
		gpos::Ref<CTableDescriptor> PtabdescInner,
		gpos::Ref<CXformResult> pxfres,
		gpmd::IMDIndex::EmdindexType emdtype) const;

public:
	CXformJoin2IndexApply(const CXformJoin2IndexApply &) = delete;

	// ctor
	explicit CXformJoin2IndexApply(gpos::Ref<CExpression> pexprPattern)
		: CXformExploration(pexprPattern)
	{
		m_fOuterJoin = (COperator::EopLogicalLeftOuterJoin ==
						pexprPattern->Pop()->Eopid());
	}

	// dtor
	~CXformJoin2IndexApply() override = default;

	// compute xform promise for a given expression handle
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

};	// class CXformJoin2IndexApply

}  // namespace gpopt

#endif	// !GPOPT_CXformJoin2IndexApply_H

// EOF
