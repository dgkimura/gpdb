//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CJoinOrderDP.h
//
//	@doc:
//		Dynamic programming-based join order generation
//---------------------------------------------------------------------------
#ifndef GPOPT_CJoinOrderDP_H
#define GPOPT_CJoinOrderDP_H

#include "gpos/base.h"
#include "gpos/common/CBitSet.h"
#include "gpos/common/CHashMap.h"
#include "gpos/common/DbgPrintMixin.h"
#include "gpos/io/IOstream.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/xforms/CJoinOrder.h"


namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CJoinOrderDP
//
//	@doc:
//		Helper class for creating join orders using dynamic programming
//
//---------------------------------------------------------------------------
class CJoinOrderDP : public CJoinOrder, public gpos::DbgPrintMixin<CJoinOrderDP>
{
private:
	//---------------------------------------------------------------------------
	//	@struct:
	//		SComponentPair
	//
	//	@doc:
	//		Struct to capture a pair of components
	//
	//---------------------------------------------------------------------------
	struct SComponentPair : public CRefCount<SComponentPair>
	{
		// first component
		gpos::Ref<CBitSet> m_pbsFst;

		// second component
		gpos::Ref<CBitSet> m_pbsSnd;

		// ctor
		SComponentPair(gpos::Ref<CBitSet> pbsFst, gpos::Ref<CBitSet> pbsSnd);

		// dtor
		~SComponentPair() override;

		// hashing function
		static ULONG HashValue(const gpos::Ref<SComponentPair> pcomppair);

		// equality function
		static BOOL Equals(const gpos::Ref<SComponentPair> pcomppairFst,
						   const gpos::Ref<SComponentPair> pcomppairSnd);
	};

	// hashing function
	static ULONG
	UlHashBitSet(const gpos::Ref<CBitSet> pbs)
	{
		GPOS_ASSERT(nullptr != pbs);

		return pbs->HashValue();
	}

	// equality function
	static BOOL
	FEqualBitSet(const gpos::Ref<CBitSet> pbsFst,
				 const gpos::Ref<CBitSet> pbsSnd)
	{
		GPOS_ASSERT(nullptr != pbsFst);
		GPOS_ASSERT(nullptr != pbsSnd);

		return pbsFst->Equals(pbsSnd);
	}

	// hash map from component to best join order
	using BitSetToExpressionMap =
		CHashMap<CBitSet, CExpression, UlHashBitSet, FEqualBitSet,
				 CleanupRelease<CBitSet>, CleanupRelease<CExpression>>;

	// hash map from component pair to connecting edges
	using ComponentPairToExpressionMap =
		CHashMap<SComponentPair, CExpression, SComponentPair::HashValue,
				 SComponentPair::Equals, CleanupRelease<SComponentPair>,
				 CleanupRelease<CExpression>>;

	// hash map from expression to cost of best join order
	using ExpressionToCostMap =
		CHashMap<CExpression, CDouble, CExpression::HashValue, CUtils::Equals,
				 CleanupRelease<CExpression>, CleanupDelete<CDouble>>;

	// lookup table for links
	ComponentPairToExpressionMap *m_phmcomplink;

	// dynamic programming table
	BitSetToExpressionMap *m_phmbsexpr;

	// map of expressions to its cost
	ExpressionToCostMap *m_phmexprcost;

	// array of top-k join expression
	gpos::Ref<CExpressionArray> m_pdrgpexprTopKOrders;

	// dummy expression to used for non-joinable components
	gpos::Ref<CExpression> m_pexprDummy;

	// build expression linking given components
	gpos::Ref<CExpression> PexprBuildPred(gpos::Ref<CBitSet> pbsFst,
										  gpos::Ref<CBitSet> pbsSnd);

	// lookup best join order for given set
	gpos::Ref<CExpression> PexprLookup(gpos::Ref<CBitSet> pbs);

	// extract predicate joining the two given sets
	gpos::Ref<CExpression> PexprPred(gpos::Ref<CBitSet> pbsFst,
									 gpos::Ref<CBitSet> pbsSnd);

	// join expressions in the given two sets
	gpos::Ref<CExpression> PexprJoin(gpos::Ref<CBitSet> pbsFst,
									 gpos::Ref<CBitSet> pbsSnd);

	// join expressions in the given set
	gpos::Ref<CExpression> PexprJoin(gpos::Ref<CBitSet> pbs);

	// find best join order for given component using dynamic programming
	gpos::Ref<CExpression> PexprBestJoinOrderDP(gpos::Ref<CBitSet> pbs);

	// find best join order for given component
	gpos::Ref<CExpression> PexprBestJoinOrder(gpos::Ref<CBitSet> pbs);

	// generate cross product for the given components
	gpos::Ref<CExpression> PexprCross(gpos::Ref<CBitSet> pbs);

	// join a covered subset with uncovered subset
	gpos::Ref<CExpression> PexprJoinCoveredSubsetWithUncoveredSubset(
		gpos::Ref<CBitSet> pbs, gpos::Ref<CBitSet> pbsCovered,
		gpos::Ref<CBitSet> pbsUncovered);

	// return a subset of the given set covered by one or more edges
	gpos::Ref<CBitSet> PbsCovered(gpos::Ref<CBitSet> pbsInput);

	// add given join order to best results
	void AddJoinOrder(gpos::Ref<CExpression> pexprJoin, CDouble dCost);

	// compute cost of given join expression
	CDouble DCost(gpos::Ref<CExpression> pexpr);

	// derive stats on given expression
	void DeriveStats(gpos::Ref<CExpression> pexpr) override;

	// add expression to cost map
	void InsertExpressionCost(gpos::Ref<CExpression> pexpr, CDouble dCost,
							  BOOL fValidateInsert);

	// generate all subsets of the given array of elements
	static void GenerateSubsets(CMemoryPool *mp, gpos::Ref<CBitSet> pbsCurrent,
								ULONG *pulElems, ULONG size, ULONG ulIndex,
								gpos::Ref<CBitSetArray> pdrgpbsSubsets);

	// driver of subset generation
	static gpos::Ref<CBitSetArray> PdrgpbsSubsets(CMemoryPool *mp,
												  gpos::Ref<CBitSet> pbs);

public:
	// ctor
	CJoinOrderDP(CMemoryPool *mp,
				 gpos::Ref<CExpressionArray> pdrgpexprComponents,
				 gpos::Ref<CExpressionArray> pdrgpexprConjuncts);

	// dtor
	~CJoinOrderDP() override;

	// main handler
	virtual gpos::Ref<CExpression> PexprExpand();

	// best join orders
	gpos::Ref<CExpressionArray>
	PdrgpexprTopK() const
	{
		return m_pdrgpexprTopKOrders;
	}

	// print function
	IOstream &OsPrint(IOstream &) const;

	CXform::EXformId
	EOriginXForm() const override
	{
		return CXform::ExfExpandNAryJoinDP;
	}

};	// class CJoinOrderDP

}  // namespace gpopt

#endif	// !GPOPT_CJoinOrderDP_H

// EOF
