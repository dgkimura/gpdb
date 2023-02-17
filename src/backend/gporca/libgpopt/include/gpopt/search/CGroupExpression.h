//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 - 2011 EMC Corp.
//
//	@filename:
//		CGroupExpression.h
//
//	@doc:
//		Equivalent of CExpression inside Memo structure
//---------------------------------------------------------------------------
#ifndef GPOPT_CGroupExpression_H
#define GPOPT_CGroupExpression_H

#include "gpos/base.h"
#include "gpos/common/CRefCount.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CCostContext.h"
#include "gpopt/engine/CPartialPlan.h"
#include "gpopt/operators/COperator.h"
#include "gpopt/search/CGroup.h"
#include "gpopt/xforms/CXform.h"

#define GPOPT_INVALID_GEXPR_ID gpos::ulong_max

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CGroupExpression
//
//	@doc:
//		Expression representation inside Memo structure
//
//---------------------------------------------------------------------------
class CGroupExpression : public CRefCount<CGroupExpression>,
						 public DbgPrintMixin<CGroupExpression>
{
public:
#ifdef GPOS_DEBUG
	// debug print; for interactive debugging sessions only
	void DbgPrintWithProperties() const;
#endif	// GPOS_DEBUG

	// states of a group expression
	enum EState
	{
		estUnexplored,	// initial state

		estExploring,  // ongoing exploration
		estExplored,   // done exploring

		estImplementing,  // ongoing implementation
		estImplemented,	  // done implementing

		estSentinel
	};

	// circular dependency state
	enum ECircularDependency
	{
		ecdDefault,				// default state
		ecdCircularDependency,	// contains circular dependency

		ecdSentinel
	};

	// type definition of cost context hash table
	using ShtCC = CSyncHashtable<CCostContext, OPTCTXT_PTR>;

private:
	// definition of context hash table accessor
	using ShtAcc = CSyncHashtableAccessByKey<CCostContext, OPTCTXT_PTR>;

	// definition of context hash table iter
	using ShtIter = CSyncHashtableIter<CCostContext, OPTCTXT_PTR>;

	// definition of context hash table iter accessor
	using ShtAccIter = CSyncHashtableAccessByIter<CCostContext, OPTCTXT_PTR>;

	// map of partial plans to their costs
	using PartialPlanToCostMap =
		CHashMap<CPartialPlan, CCost, CPartialPlan::HashValue,
				 CPartialPlan::Equals, CleanupRelease<CPartialPlan>,
				 CleanupDelete<CCost>>;


	// expression id
	ULONG m_id{GPOPT_INVALID_GEXPR_ID};

	// duplicate group expression
	gpos::Ref<CGroupExpression> m_pgexprDuplicate;

	// operator class
	gpos::Ref<COperator> m_pop{nullptr};

	// array of child groups
	gpos::Ref<CGroupArray> m_pdrgpgroup{nullptr};

	// sorted array of children groups for faster comparison
	// of order-insensitive operators
	gpos::Ref<CGroupArray> m_pdrgpgroupSorted{nullptr};

	// back pointer to group
	gpos::Ref<CGroup> m_pgroup{nullptr};

	// id of xform that generated group expression
	CXform::EXformId m_exfidOrigin{CXform::ExfInvalid};

	// group expression that generated current group expression via xform
	gpos::Ref<CGroupExpression> m_pgexprOrigin{nullptr};

	// flag to indicate if group expression was created as a node at some
	// intermediate level when origin expression was inserted to memo
	BOOL m_fIntermediate{false};

	// state of group expression
	EState m_estate{estUnexplored};

	// optimization level
	EOptimizationLevel m_eol{EolLow};

	// map of partial plans to their cost lower bound
	gpos::Ref<PartialPlanToCostMap> m_ppartialplancostmap{nullptr};

	// circular dependency state
	ECircularDependency m_ecirculardependency;

	// hashtable of cost contexts
	ShtCC m_sht;

	// set group back pointer
	void SetGroup(gpos::Ref<CGroup> pgroup);

	// set group expression id
	void SetId(ULONG id);

	// print transformation
	static void PrintXform(CMemoryPool *mp, gpos::Ref<CXform> pxform,
						   gpos::Ref<CExpression> pexpr,
						   gpos::Ref<CXformResult> pxfres, ULONG ulNumResults);

	// preprocessing before applying transformation
	void PreprocessTransform(CMemoryPool *pmpLocal, CMemoryPool *pmpGlobal,
							 gpos::Ref<CXform> pxform);

	// postprocessing after applying transformation
	void PostprocessTransform(CMemoryPool *pmpLocal, CMemoryPool *pmpGlobal,
							  gpos::Ref<CXform> pxform) const;

	// costing scheme
	static CCost CostCompute(CMemoryPool *mp, gpos::Ref<CCostContext> pcc);

	// set optimization level of group expression
	void SetOptimizationLevel();

	// check validity of group expression
	BOOL FValidContext(CMemoryPool *mp, gpos::Ref<COptimizationContext> poc,
					   gpos::Ref<COptimizationContextArray> pdrgpocChild);

	// remove cost context in hash table
	gpos::Ref<CCostContext> PccRemove(gpos::Ref<COptimizationContext> poc,
									  ULONG ulOptReq);

	// insert given context in hash table only if a better context does not exist, return the context that is kept it in hash table
	gpos::Ref<CCostContext> PccInsertBest(gpos::Ref<CCostContext> pcc);

	// print group expression cost contexts
	IOstream &OsPrintCostContexts(IOstream &os, const CHAR *szPrefix) const;

	//private dummy ctor; used for creating invalid gexpr
	CGroupExpression() = default;


public:
	CGroupExpression(const CGroupExpression &) = delete;

	// ctor
	CGroupExpression(CMemoryPool *mp, gpos::Ref<COperator> pop,
					 gpos::Ref<CGroupArray> pdrgpgroup, CXform::EXformId exfid,
					 gpos::Ref<CGroupExpression> pgexprOrigin,
					 BOOL fIntermediate);

	// dtor
	~CGroupExpression() override;

	// duplicate group expression accessor
	gpos::Ref<CGroupExpression>
	PgexprDuplicate() const
	{
		return m_pgexprDuplicate;
	}

	// set duplicate group expression
	void
	SetDuplicate(gpos::Ref<CGroupExpression> pgexpr)
	{
		GPOS_ASSERT(nullptr != pgexpr);

		m_pgexprDuplicate = pgexpr;
	}

	// cleanup cost contexts
	void CleanupContexts();

	// check if cost context already exists in group expression hash table
	BOOL FCostContextExists(gpos::Ref<COptimizationContext> poc,
							gpos::Ref<COptimizationContextArray> pdrgpoc);

	// compute and store expression's cost under a given context
	gpos::Ref<CCostContext> PccComputeCost(
		CMemoryPool *mp, gpos::Ref<COptimizationContext> poc, ULONG ulOptReq,
		gpos::Ref<COptimizationContextArray> pdrgpoc, BOOL fPruned,
		CCost costLowerBound);

	// compute a cost lower bound for plans, rooted by current group expression, and satisfying the given required properties
	CCost CostLowerBound(CMemoryPool *mp, gpos::Ref<CReqdPropPlan> prppInput,
						 gpos::Ref<CCostContext> pccChild, ULONG child_index);

	// initialize group expression
	void Init(gpos::Ref<CGroup> pgroup, ULONG id);

	// reset group expression
	void
	Reset(gpos::Ref<CGroup> pgroup, ULONG id)
	{
		m_pgroup = pgroup;
		m_id = id;
	}

	// optimization level accessor
	EOptimizationLevel
	Eol() const
	{
		return m_eol;
	}

	// shorthand to access children
	gpos::Ref<CGroup>
	operator[](ULONG ulPos) const
	{
		GPOS_ASSERT(nullptr != m_pdrgpgroup);

		gpos::Ref<CGroup> pgroup = (*m_pdrgpgroup)[ulPos];

		// during optimization, the operator returns the duplicate group;
		// in exploration and implementation the group may contain
		// group expressions that have not been processed yet;
		if (0 == pgroup->UlGExprs())
		{
			GPOS_ASSERT(pgroup->FDuplicateGroup());
			return pgroup->PgroupDuplicate();
		}
		return pgroup;
	};

	// arity function
	ULONG
	Arity() const
	{
		return m_pdrgpgroup->Size();
	}

	// accessor for operator
	gpos::Ref<COperator>
	Pop() const
	{
		return m_pop.get();
	}

	// accessor for id
	ULONG
	Id() const
	{
		return m_id;
	}

	// accessor for containing group
	gpos::Ref<CGroup>
	Pgroup() const
	{
		return m_pgroup;
	}

	// origin xform
	CXform::EXformId
	ExfidOrigin() const
	{
		return m_exfidOrigin;
	}

	// origin group expression
	gpos::Ref<CGroupExpression>
	PgexprOrigin() const
	{
		return m_pgexprOrigin;
	}

	// cost contexts hash table accessor
	ShtCC &
	Sht()
	{
		return m_sht;
	}

	// comparison operator for hashtables
	BOOL
	operator==(const CGroupExpression &gexpr) const
	{
		return gexpr.Matches(this);
	}

	// equality function for hash table
	static BOOL
	Equals(const CGroupExpression &gexprLeft,
		   const CGroupExpression &gexprRight)
	{
		return gexprLeft == gexprRight;
	}

	// match group expression against given operator and its children
	BOOL Matches(const gpos::Ref<CGroupExpression>) const;
	BOOL Matches(const CGroupExpression *) const;

	// match non-scalar children of group expression against given children of passed expression
	BOOL FMatchNonScalarChildren(
		const gpos::Ref<CGroupExpression> pgexpr) const;

	// hash function
	ULONG
	HashValue() const
	{
		return HashValue(m_pop.get(), m_pdrgpgroup.get());
	}

	// static hash function for operator and group references
	static ULONG HashValue(gpos::Ref<COperator> pop, CGroupArray *drgpgroup);

	// static hash function for group expression
	static ULONG HashValue(const CGroupExpression &);

	// transform group expression
	void Transform(CMemoryPool *mp, CMemoryPool *pmpLocal,
				   gpos::Ref<CXform> pxform, gpos::Ref<CXformResult> pxfres,
				   ULONG *pulElapsedTime, ULONG *pulNumberOfBindings);

	// set group expression state
	void SetState(EState estNewState);

	// reset group expression state
	void ResetState();

	// check if group expression has been explored
	BOOL
	FExplored() const
	{
		return (estExplored <= m_estate);
	}

	// check if group expression has been implemented
	BOOL
	FImplemented() const
	{
		return (estImplemented == m_estate);
	}

	// check if transition to the given state is completed
	BOOL FTransitioned(EState estate) const;

	CGroupArray *
	Pdrgpgroup() const
	{
		return m_pdrgpgroup.get();
	}

	// lookup cost context in hash table
	gpos::Ref<CCostContext> PccLookup(gpos::Ref<COptimizationContext> poc,
									  ULONG ulOptReq);

	// lookup all cost contexts matching given optimization context
	gpos::Ref<CCostContextArray> PdrgpccLookupAll(
		CMemoryPool *mp, gpos::Ref<COptimizationContext> poc);

	// insert a cost context in hash table
	gpos::Ref<CCostContext> PccInsert(gpos::Ref<CCostContext> pcc);

	// derive statistics recursively on a given group expression
	gpos::Ref<IStatistics> PstatsRecursiveDerive(
		CMemoryPool *pmpLocal, CMemoryPool *pmpGlobal,
		gpos::Ref<CReqdPropRelational> prprel, IStatisticsArray *stats_ctxt,
		BOOL fComputeRootStats = true);

	// print driver
	IOstream &OsPrint(IOstream &os) const;
	IOstream &OsPrintWithPrefix(IOstream &os, const CHAR *prefix) const;

	// link for list in Group
	SLink m_linkGroup;

	// link for group expression hash table
	SLink m_linkMemo;

	// invalid group expression
	static const CGroupExpression m_gexprInvalid;

	virtual BOOL ContainsCircularDependencies();
};	// class CGroupExpression

}  // namespace gpopt


#endif	// !GPOPT_CGroupExpression_H

// EOF
