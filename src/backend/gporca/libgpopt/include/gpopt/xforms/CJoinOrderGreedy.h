//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CJoinOrderGreedy.h
//
//	@doc:
//		Cardinality-based join order generation with delayed cross joins
//---------------------------------------------------------------------------
#ifndef GPOPT_CJoinOrderGreedy_H
#define GPOPT_CJoinOrderGreedy_H

#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/io/IOstream.h"

#include "gpopt/xforms/CJoinOrder.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CJoinOrderGreedy
//
//	@doc:
//		Helper class for creating join orders based on cardinality of results
//
//---------------------------------------------------------------------------
class CJoinOrderGreedy : public CJoinOrder
{
private:
	// result component
	gpos::owner<SComponent *> m_pcompResult;

	// returns starting joins with minimal cardinality
	gpos::owner<SComponent *> GetStartingJoins();

public:
	// ctor
	CJoinOrderGreedy(CMemoryPool *pmp, CExpressionArray *pdrgpexprComponents,
					 CExpressionArray *pdrgpexprConjuncts);

	// dtor
	~CJoinOrderGreedy() override;

	// main handler
	virtual gpos::owner<CExpression *> PexprExpand();

	ULONG
	PickBestJoin(CBitSet *candidate_nodes);

	CBitSet *GetAdjacentComponentsToJoinCandidate();

	CXform::EXformId
	EOriginXForm() const override
	{
		return CXform::ExfExpandNAryJoinGreedy;
	}

};	// class CJoinOrderGreedy

}  // namespace gpopt

#endif	// !GPOPT_CJoinOrderGreedy_H

// EOF
