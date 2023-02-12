//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CCostContext.h
//
//	@doc:
//		Cost context object stores the cost of a group expression under
//		a given optimization context
//---------------------------------------------------------------------------
#ifndef GPOPT_CCostContext_H
#define GPOPT_CCostContext_H

#include "gpos/base.h"
#include "gpos/common/CRefCount.h"
#include "gpos/common/owner.h"

#include "gpopt/base/COptimizationContext.h"
#include "gpopt/cost/CCost.h"
#include "gpopt/cost/ICostModel.h"

// infinite plan cost
#define GPOPT_INFINITE_COST CCost(1e+100)

// invalid cost value
#define GPOPT_INVALID_COST CCost(-0.5)


namespace gpopt
{
using namespace gpos;

// fwd declarations
class CDrvdPropPlan;
class CCostContext;

// array of cost contexts
using CCostContextArray = gpos::Vector<gpos::Ref<CCostContext>>;

// cost context pointer definition
using COSTCTXT_PTR = gpos::Ref<CCostContext>;

// cost context pointer definition
using CONST_COSTCTXT_PTR = const gpos::Ref<CCostContext>;

//---------------------------------------------------------------------------
//	@class:
//		CCostContext
//
//	@doc:
//		Cost context
//
//---------------------------------------------------------------------------
class CCostContext : public CRefCount<CCostContext>,
					 public DbgPrintMixin<CCostContext>
{
public:
	// states of cost context
	enum EState
	{
		estUncosted,  // initial state

		estCosting,	 // ongoing costing
		estCosted,	 // done costing

		estSentinel
	};

private:
	// memory pool
	CMemoryPool *m_mp;

	// cost of group expression under optimization context
	CCost m_cost;

	// cost context state
	EState m_estate;

	// back pointer to owner group expression
	gpos::Ref<CGroupExpression> m_pgexpr;

	// group expression to be used stats derivation during costing
	gpos::Ref<CGroupExpression> m_pgexprForStats;

	// array of optimization contexts of child groups
	gpos::Ref<COptimizationContextArray> m_pdrgpoc;

	// derived properties of the carried plan
	gpos::Ref<CDrvdPropPlan> m_pdpplan;

	// optimization request number
	ULONG m_ulOptReq;

	// flag to indicate if cost context is pruned,
	// a cost context is pruned during branch-and-bound search if there exists
	// an equivalent context with better cost
	BOOL m_fPruned;

	// stats of owner group expression
	gpos::Ref<IStatistics> m_pstats;

	// derive stats of owner group expression
	void DeriveStats();

	// return the number of rows per host
	CDouble DRowsPerHost() const;

	// for two cost contexts with join plans of the same cost, break the tie based on join depth,
	// if tie-resolution succeeded, store a pointer to preferred cost context in output argument
	static void BreakCostTiesForJoinPlans(const gpos::Ref<CCostContext> pccFst,
										  const gpos::Ref<CCostContext> pccSnd,
										  CONST_COSTCTXT_PTR *ppccPrefered,
										  BOOL *pfTiesResolved);

public:
	CCostContext(const CCostContext &) = delete;

	// main optimization context
	gpos::Ref<COptimizationContext> m_poc;

	// link for cost context hash table in CGroupExpression
	SLink m_link;

	// ctor
	CCostContext(CMemoryPool *mp, gpos::Ref<COptimizationContext> poc,
				 ULONG ulOptReq, gpos::Ref<CGroupExpression> pgexpr);

	// dtor
	~CCostContext() override;

	// main optimization context accessor
	gpos::Ref<COptimizationContext>
	Poc() const
	{
		return m_poc.get();
	}

	// accessor of optimization request number
	ULONG
	UlOptReq() const
	{
		return m_ulOptReq;
	}

	// is context pruned based on cost comparison?
	BOOL
	FPruned() const
	{
		return m_fPruned;
	}

	// set pruned flag
	void
	SetPruned()
	{
		GPOS_ASSERT(!m_fPruned);

		m_fPruned = true;
	}

	// accessor of child optimization contexts array
	COptimizationContextArray *
	Pdrgpoc() const
	{
		return m_pdrgpoc.get();
	}

	// cost accessor
	CCost
	Cost() const
	{
		return m_cost;
	}

	// state accessor
	EState
	Est() const
	{
		return m_estate;
	}

	// owner group expression accessor
	gpos::Ref<CGroupExpression>
	Pgexpr() const
	{
		return m_pgexpr.get();
	}

	// group expression for stats derivation
	gpos::Ref<CGroupExpression>
	PgexprForStats() const
	{
		return m_pgexprForStats.get();
	}

	// return stats of owner group expression
	gpos::Ref<IStatistics>
	Pstats() const
	{
		return m_pstats.get();
	}

	// check if we need to derive stats for this context
	BOOL FNeedsNewStats() const;

	// check if new stats were derived for this context
	BOOL FOwnsStats() const;

	// derived plan properties accessor
	CDrvdPropPlan *
	Pdpplan() const
	{
		return m_pdpplan.get();
	}

	// set cost value
	void
	SetCost(CCost cost)
	{
		GPOS_ASSERT(cost > GPOPT_INVALID_COST);
		m_cost = cost;
	}

	// derive properties of the plan carried by cost context
	void DerivePlanProps(CMemoryPool *mp);

	// set cost context state
	void
	SetState(EState estNewState)
	{
		GPOS_ASSERT(estNewState == (EState)(m_estate + 1));

		m_estate = estNewState;
	}

	// set child contexts
	void
	SetChildContexts(COptimizationContextArray *pdrgpoc)
	{
		GPOS_ASSERT(nullptr == m_pdrgpoc);
		GPOS_ASSERT(nullptr != pdrgpoc);

		m_pdrgpoc = pdrgpoc;
	}


	// check validity by comparing derived and required properties
	BOOL IsValid(CMemoryPool *mp);

	// comparison operator
	BOOL operator==(const CCostContext &cc) const;

	// compute cost
	CCost CostCompute(CMemoryPool *mp, CCostArray *pdrgpcostChildren);

	// is current context better than the given equivalent context based on cost?
	BOOL FBetterThan(const gpos::Ref<CCostContext> pcc) const;

	// is this cost context of a two stage scalar DQA created by CXformSplitDQA
	static BOOL IsTwoStageScalarDQACostCtxt(const gpos::Ref<CCostContext> pcc);

	// is this cost context of a three stage scalar DQA created by CXformSplitDQA
	static BOOL IsThreeStageScalarDQACostCtxt(
		const gpos::Ref<CCostContext> pcc);

	// equality function
	static BOOL
	Equals(const CCostContext &ccLeft, const CCostContext &ccRight)
	{
		// check if we are comparing against invalid context
		if (nullptr == ccLeft.Poc() || nullptr == ccRight.Poc())
		{
			return nullptr == ccLeft.Poc() && nullptr == ccRight.Poc();
		}

		return ccLeft.UlOptReq() == ccRight.UlOptReq() &&
			   ccLeft.Pgexpr() == ccRight.Pgexpr() &&
			   ccLeft.Poc()->Matches(ccRight.Poc().get());
	}

	// equality function
	static BOOL
	Equals(const gpos::Ref<CCostContext> pccLeft,
		   const gpos::Ref<CCostContext> pccRight)
	{
		return Equals(*pccLeft, *pccRight);
	}

	// hash function
	static ULONG
	HashValue(const CCostContext &cc)
	{
		return COptimizationContext::HashValue(*(cc.Poc()));
	}

	// hash function
	static ULONG
	HashValue(const gpos::Ref<CCostContext> pcc)
	{
		return HashValue(*pcc);
	}

	// debug print
	IOstream &OsPrint(IOstream &os) const;

};	// class CCostContext

}  // namespace gpopt


#endif	// !GPOPT_CCostContext_H

// EOF
