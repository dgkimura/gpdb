//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2023 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLeftJoinPruningPreprocessor.h
//
//	@doc:
//		Preprocessing routines of left join pruning
//---------------------------------------------------------------------------

#ifndef GPOPT_CLeftJoinPruningPreprocessor_H
#define GPOPT_CLeftJoinPruningPreprocessor_H

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
class CLeftJoinPruningPreprocessor
{
private:
	static gpos::Ref<CExpression> PexprJoinPruningScalarSubquery(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprScalar);

	static void ComputeOutputColumns(
		const gpos::Ref<CExpression> pexpr,
		const gpos::Ref<CColRefSet> derived_output_columns,
		gpos::Ref<CColRefSet> output_columns,
		gpos::Ref<CColRefSet> childs_output_columns,
		const gpos::Ref<CColRefSet> pcrsOutput);

	static gpos::Ref<CExpression> JoinPruningTreeTraversal(
		CMemoryPool *mp, const gpos::Ref<CExpression> pexpr,
		gpos::Ref<CExpressionArray> pdrgpexpr,
		const gpos::Ref<CColRefSet> childs_output_columns);

	static gpos::Ref<CExpression> PexprCheckLeftOuterJoinPruningConditions(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprNew,
		gpos::Ref<CColRefSet> output_columns);

	static BOOL CheckJoinPruningCondOnInnerRel(
		const gpos::Ref<CExpression> pexprNew,
		gpos::Ref<CColRefSet> output_columns);

	static BOOL CheckJoinPruningCondOnJoinCond(
		CMemoryPool *mp, const gpos::Ref<CExpression> pexprNew,
		gpos::Ref<CColRefSet> result);

	static BOOL CheckAndCondInJoinCond(
		CMemoryPool *mp, const gpos::Ref<CExpression> join_cond,
		const gpos::Ref<CColRefSet> inner_unique_keys,
		gpos::Ref<CColRefSet> result,
		const gpos::Ref<CColRefSet> outer_rel_columns);

	static void CheckUniqueKeyInJoinCond(
		gpos::Ref<CColRefSet> inner_columns,
		const gpos::Ref<CColRefSet> usedColumns, gpos::Ref<CColRefSet> result,
		const gpos::Ref<CColRefSet> inner_unique_keys,
		const gpos::Ref<CColRefSet> outer_rel_columns);

	static BOOL CheckForFullUniqueKeySetInInnerRel(
		CMemoryPool *mp, const gpos::Ref<CExpression> pexprNew,
		const gpos::Ref<CColRefSet> result);

public:
	CLeftJoinPruningPreprocessor(const CLeftJoinPruningPreprocessor &) = delete;

	// Main Driver
	static gpos::Ref<CExpression> PexprPreprocess(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		const gpos::Ref<CColRefSet> pcrsOutput);
};	// class CLeftJoinPruningPreprocessor
}  // namespace gpopt


#endif
