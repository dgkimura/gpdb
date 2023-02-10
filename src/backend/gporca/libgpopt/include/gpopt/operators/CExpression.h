//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 - 2011 EMC CORP.
//
//	@filename:
//		CExpression.h
//
//	@doc:
//		Basic tree/DAG-based representation for an expression
//---------------------------------------------------------------------------
#ifndef GPOPT_CExpression_H
#define GPOPT_CExpression_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/CRefCount.h"
#include "gpos/common/DbgPrintMixin.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CDrvdPropRelational.h"
#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/CPrintPrefix.h"
#include "gpopt/base/CReqdProp.h"
#include "gpopt/cost/CCost.h"
#include "gpopt/operators/CExpression_Fwd.h"
#include "gpopt/operators/COperator.h"
#include "naucrates/statistics/IStatistics.h"

namespace gpopt
{
// cleanup function for arrays
class CExpression;
using CExpressionArray = gpos::Vector<gpos::Ref<CExpression>>;

// array of arrays of expression pointers
using CExpressionArrays = gpos::Vector<gpos::Ref<CExpressionArray>>;

class CGroupExpression;
class CDrvdPropPlan;
class CDrvdPropCtxt;
class CDrvdPropCtxtPlan;
class CPropConstraint;
class CCostContext;

using namespace gpos;
using namespace gpnaucrates;

//---------------------------------------------------------------------------
//	@class:
//		CExpression
//
//	@doc:
//		Simply dynamic array for pointer types
//
//---------------------------------------------------------------------------
class CExpression : public CRefCount, public gpos::DbgPrintMixin<CExpression>
{
	friend class CExpressionHandle;

private:
	// memory pool
	CMemoryPool *m_mp;

	// operator class
	gpos::Ref<COperator> m_pop;

	// array of children
	gpos::Ref<CExpressionArray> m_pdrgpexpr;

	// derived relational properties
	gpos::Ref<CDrvdPropRelational> m_pdprel;

	// derived stats
	gpos::Ref<IStatistics> m_pstats;

	// required plan properties
	gpos::Ref<CReqdPropPlan> m_prpp;

	// derived physical properties
	gpos::Ref<CDrvdPropPlan> m_pdpplan;

	// derived scalar properties
	gpos::Ref<CDrvdPropScalar> m_pdpscalar;

	// group reference to Memo
	gpos::Ref<CGroupExpression> m_pgexpr;

	// cost of physical expression node when copied out of the memo
	CCost m_cost;

	// id of origin group, used for debugging expressions extracted from memo
	ULONG m_ulOriginGrpId;

	// id of origin group expression, used for debugging expressions extracted from memo
	ULONG m_ulOriginGrpExprId;

	// get expression's derived property given its type
	gpos::Ref<CDrvdProp> Pdp(const CDrvdProp::EPropType ept) const;

#ifdef GPOS_DEBUG

	// assert valid property derivation
	void AssertValidPropDerivation(const CDrvdProp::EPropType ept);

	// print expression properties
	void PrintProperties(IOstream &os, CPrintPrefix &pfx) const;

#endif	// GPOS_DEBUG

#if 0
	// check if the expression satisfies partition enforcer condition
	BOOL FValidPartEnforcers(CDrvdPropCtxtPlan *pdpctxtplan);
#endif

	// check if the distributions of all children are compatible
	BOOL FValidChildrenDistribution(CDrvdPropCtxtPlan *pdpctxtplan);

	// copy group properties and stats to expression
	void CopyGroupPropsAndStats(gpos::Ref<IStatistics> input_stats);

public:
	CExpression(const CExpression &) = delete;

	// ctor's with different arity

	// ctor for leaf nodes
	CExpression(CMemoryPool *mp, gpos::Ref<COperator> pop,
				gpos::Ref<CGroupExpression> pgexpr = nullptr);

	// ctor for unary expressions
	CExpression(CMemoryPool *mp, gpos::Ref<COperator> pop,
				gpos::Ref<CExpression> pexpr);

	// ctor for binary expressions
	CExpression(CMemoryPool *mp, gpos::Ref<COperator> pop,
				gpos::Ref<CExpression> pexprChildFirst,
				gpos::Ref<CExpression> pexprChildSecond);

	// ctor for ternary expressions
	CExpression(CMemoryPool *mp, gpos::Ref<COperator> pop,
				gpos::Ref<CExpression> pexprChildFirst,
				gpos::Ref<CExpression> pexprChildSecond,
				gpos::Ref<CExpression> pexprChildThird);

	// ctor n-ary expressions
	CExpression(CMemoryPool *mp, gpos::Ref<COperator> pop,
				gpos::Ref<CExpressionArray> pdrgpexpr);

	// ctor for n-ary expression with origin group expression
	CExpression(CMemoryPool *mp, gpos::Ref<COperator> pop,
				gpos::Ref<CGroupExpression> pgexpr,
				gpos::Ref<CExpressionArray> pdrgpexpr,
				gpos::Ref<CReqdPropPlan> prpp,
				gpos::Ref<IStatistics> input_stats, CCost cost);

	// dtor
	~CExpression() override;

	// shorthand to access children
	gpos::Ref<CExpression>
	operator[](ULONG ulPos) const
	{
		GPOS_ASSERT(nullptr != m_pdrgpexpr);
		return (*m_pdrgpexpr)[ulPos].get();
	};

	// arity function
	ULONG
	Arity() const
	{
		return m_pdrgpexpr == nullptr ? 0 : m_pdrgpexpr->Size();
	}

	// accessor for operator
	gpos::Ref<COperator>
	Pop() const
	{
		GPOS_ASSERT(nullptr != m_pop);
		return m_pop.get();
	}

	// accessor of children array
	CExpressionArray *
	PdrgPexpr() const
	{
		return m_pdrgpexpr.get();
	}

	// accessor for origin group expression
	gpos::Ref<CGroupExpression>
	Pgexpr() const
	{
		return m_pgexpr;
	}

	// accessor for computed required plan props
	gpos::Ref<CReqdPropPlan>
	Prpp() const
	{
		return m_prpp.get();
	}

	CDrvdPropRelational *GetDrvdPropRelational() const;

	CDrvdPropPlan *GetDrvdPropPlan() const;

	CDrvdPropScalar *GetDrvdPropScalar() const;

	// get derived statistics object
	const gpos::Ref<IStatistics>
	Pstats() const
	{
		return m_pstats.get();
	}

	// cost accessor
	CCost
	Cost() const
	{
		return m_cost;
	}

	// get the suitable derived property type based on operator
	CDrvdProp::EPropType Ept() const;

	// Derive all properties immediately. The suitable derived property is
	// determined internally. To derive properties on an on-demand bases, use
	// DeriveXXX() methods.
	gpos::Ref<CDrvdProp> PdpDerive(gpos::Ref<CDrvdPropCtxt> pdpctxt = nullptr);

	// derive statistics
	gpos::Ref<IStatistics> PstatsDerive(CReqdPropRelational *prprel,
										IStatisticsArray *stats_ctxt);

	// reset a derived property
	void ResetDerivedProperty(CDrvdProp::EPropType ept);

	// reset all derived properties
	void ResetDerivedProperties();

	// reset expression stats
	void ResetStats();

	// check for outer references
	BOOL HasOuterRefs();

	// print driver
	IOstream &OsPrint(IOstream &os) const;

	// print driver, customized for expressions
	IOstream &OsPrintExpression(IOstream &os, const CPrintPrefix * = nullptr,
								BOOL fLast = true) const;

	// match with group expression
	BOOL FMatchPattern(gpos::Ref<CGroupExpression> pgexpr) const;

	// return a copy of the expression with remapped columns
	gpos::Ref<CExpression> PexprCopyWithRemappedColumns(
		CMemoryPool *mp, UlongToColRefMap *colref_mapping,
		BOOL must_exist) const;

	// compare entire expression rooted here
	BOOL Matches(gpos::Ref<CExpression> pexpr) const;

#ifdef GPOS_DEBUG
	// match against given pattern
	BOOL FMatchPattern(gpos::Ref<CExpression> pexpr) const;

	// match children against given pattern
	BOOL FMatchPatternChildren(gpos::Ref<CExpression> pexpr) const;

	// compare entire expression rooted here
	BOOL FMatchDebug(gpos::Ref<CExpression> pexpr) const;

	// debug print; for interactive debugging sessions only
	// prints expression properties as well
	void DbgPrintWithProperties() const;
#endif	// GPOS_DEBUG

	// check if the expression satisfies given required properties
	BOOL FValidPlan(const gpos::Ref<CReqdPropPlan> prpp,
					CDrvdPropCtxtPlan *pdpctxtplan);

	// static hash function
	static ULONG HashValue(const gpos::Ref<CExpression> pexpr);

	// static hash function
	static ULONG UlHashDedup(const gpos::Ref<CExpression> pexpr);

	// rehydrate expression from a given cost context and child expressions
	static gpos::Ref<CExpression> PexprRehydrate(
		CMemoryPool *mp, gpos::Ref<CCostContext> pcc,
		gpos::Ref<CExpressionArray> pdrgpexpr, CDrvdPropCtxtPlan *pdpctxtplan);

	// Relational property accessors - derived as needed
	gpos::Ref<CColRefSet> DeriveOuterReferences();
	gpos::Ref<CColRefSet> DeriveOutputColumns();
	gpos::Ref<CColRefSet> DeriveNotNullColumns();
	gpos::Ref<CColRefSet> DeriveCorrelatedApplyColumns();
	gpos::Ref<CKeyCollection> DeriveKeyCollection();
	gpos::Ref<CPropConstraint> DerivePropertyConstraint();
	CMaxCard DeriveMaxCard();
	ULONG DeriveJoinDepth();
	gpos::Ref<CFunctionProp> DeriveFunctionProperties();
	CFunctionalDependencyArray *DeriveFunctionalDependencies();
	gpos::Ref<CPartInfo> DerivePartitionInfo();
	gpos::Ref<CTableDescriptor> DeriveTableDescriptor();

	// Scalar property accessors - derived as needed
	gpos::Ref<CColRefSet> DeriveDefinedColumns();
	gpos::Ref<CColRefSet> DeriveUsedColumns();
	gpos::Ref<CColRefSet> DeriveSetReturningFunctionColumns();
	BOOL DeriveHasSubquery();
	gpos::Ref<CPartInfo> DeriveScalarPartitionInfo();
	gpos::Ref<CFunctionProp> DeriveScalarFunctionProperties();
	BOOL DeriveHasNonScalarFunction();
	ULONG DeriveTotalDistinctAggs();
	BOOL DeriveHasMultipleDistinctAggs();
	BOOL DeriveHasScalarArrayCmp();
	BOOL DeriveHasScalarFuncProject();
	BOOL DeriveContainsOnlyReplicationSafeAggFuncs();
	ULONG DeriveTotalOrderedAggs();

};	// class CExpression


// shorthand for printing
inline IOstream &
operator<<(IOstream &os, CExpression &expr)
{
	return expr.OsPrint(os);
}

// hash map from ULONG to expression
using UlongToExprMap =
	CHashMap<ULONG, CExpression, gpos::HashValue<ULONG>, gpos::Equals<ULONG>,
			 CleanupDelete<ULONG>, CleanupRelease<CExpression>>;

// map iterator
using UlongToExprMapIter =
	CHashMapIter<ULONG, CExpression, gpos::HashValue<ULONG>,
				 gpos::Equals<ULONG>, CleanupDelete<ULONG>,
				 CleanupRelease<CExpression>>;

REF_PTR_ADDREF_RELEASE_INLINE_DEF(CExpression);

}  // namespace gpopt


#endif	// !GPOPT_CExpression_H

// EOF
