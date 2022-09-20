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
		CMemoryPool *mp, gpos::pointer<COperator *> joinOp,
		CExpression *pexprOuter, gpos::pointer<CExpression *> pexprInner,
		gpos::pointer<CExpression *> pexprScalar, CExpression *origJoinPred,
		gpos::pointer<CExpression *> nodesToInsertAboveIndexGet,
		CExpression *endOfNodesToInsertAboveIndexGet,
		gpos::pointer<CTableDescriptor *> ptabdescInner,
		gpos::pointer<CColRefSet *> pcrsScalarExpr,
		gpos::pointer<CColRefSet *> outer_refs,
		gpos::pointer<CColRefSet *> pcrsReqd, ULONG ulIndices,
		gpos::pointer<CXformResult *> pxfres);

	// helper to add IndexApply expression to given xform results container
	// for homogeneous b-tree indexes
	static void CreateAlternativesForBtreeIndex(
		CMemoryPool *mp, gpos::pointer<COperator *> joinOp,
		CExpression *pexprOuter, gpos::pointer<CExpression *> pexprInner,
		gpos::owner<CExpression *> origJoinPred,
		gpos::pointer<CExpression *> nodesToInsertAboveIndexGet,
		CExpression *endOfNodesToInsertAboveIndexGet, CMDAccessor *md_accessor,
		gpos::pointer<CExpressionArray *> pdrgpexprConjuncts,
		gpos::pointer<CColRefSet *> pcrsScalarExpr,
		gpos::pointer<CColRefSet *> outer_refs,
		gpos::pointer<CColRefSet *> pcrsReqd,
		gpos::pointer<const IMDRelation *> pmdrel,
		gpos::pointer<const IMDIndex *> pmdindex,
		gpos::pointer<CXformResult *> pxfres);

	// helper to add IndexApply expression to given xform results container
	// for homogeneous bitmap indexes
	static void CreateHomogeneousBitmapIndexApplyAlternatives(
		CMemoryPool *mp, gpos::pointer<COperator *> joinOp,
		CExpression *pexprOuter, gpos::pointer<CExpression *> pexprInner,
		CExpression *pexprScalar, gpos::owner<CExpression *> origJoinPred,
		gpos::pointer<CExpression *> nodesToInsertAboveIndexGet,
		CExpression *endOfNodesToInsertAboveIndexGet,
		CTableDescriptor *ptabdescInner, CColRefSet *outer_refs,
		CColRefSet *pcrsReqd, gpos::pointer<CXformResult *> pxfres);

	// based on the inner and the scalar expression, it computes scalar expression
	// columns, outer references and required columns
	static void ComputeColumnSets(CMemoryPool *mp,
								  gpos::pointer<CExpression *> pexprInner,
								  gpos::pointer<CExpression *> pexprScalar,
								  CColRefSet **ppcrsScalarExpr,
								  gpos::owner<CColRefSet *> *ppcrsOuterRefs,
								  gpos::owner<CColRefSet *> *ppcrsReqd);

protected:
	// is the logical join that is being transformed an outer join?
	BOOL m_fOuterJoin;

	// helper to add IndexApply expression to given xform results container
	// for homogeneous indexes
	virtual void CreateHomogeneousIndexApplyAlternatives(
		CMemoryPool *mp, gpos::pointer<COperator *> joinOp,
		CExpression *pexprOuter, gpos::pointer<CExpression *> pexprInner,
		CExpression *pexprScalar, CExpression *origJoinPred,
		gpos::pointer<CExpression *> nodesToInsertAboveIndexGet,
		CExpression *endOfNodesToInsertAboveIndexGet,
		CTableDescriptor *PtabdescInner, gpos::pointer<CXformResult *> pxfres,
		gpmd::IMDIndex::EmdindexType emdtype) const;

public:
	CXformJoin2IndexApply(const CXformJoin2IndexApply &) = delete;

	// ctor
	explicit CXformJoin2IndexApply(gpos::owner<CExpression *> pexprPattern)
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
