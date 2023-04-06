//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2008 Greenplum, Inc.
//
//	@filename:
//		CGroupProxy.h
//
//	@doc:
//		Lock mechanism for access to a given group
//---------------------------------------------------------------------------
#ifndef GPOPT_CGroupProxy_H
#define GPOPT_CGroupProxy_H

#include "gpos/base.h"

#include "gpopt/search/CGroup.h"

namespace gpopt
{
using namespace gpos;

// forward declarations
class CGroupExpression;
class CDrvdProp;
class COptimizationContext;

//---------------------------------------------------------------------------
//	@class:
//		CGroupProxy
//
//	@doc:
//		Exclusive access to a given group
//
//---------------------------------------------------------------------------
class CGroupProxy
{
private:
	// group we're operating on
	gpos::Ref<CGroup> m_pgroup;

	// skip group expressions starting from the given expression;
	gpos::Ref<CGroupExpression> PgexprSkip(
		gpos::Ref<CGroupExpression> pgexprStart, BOOL fSkipLogical);

public:
	// ctor
	explicit CGroupProxy(gpos::Ref<CGroup> pgroup);

	// dtor
	~CGroupProxy();

	// set group id
	void
	SetId(ULONG id)
	{
		m_pgroup->SetId(id);
	}

	// set group state
	void
	SetState(CGroup::EState estNewState)
	{
		m_pgroup->SetState(estNewState);
	}

	// set hash join keys
	void
	SetJoinKeys(gpos::Ref<CExpressionArray> pdrgpexprOuter,
				gpos::Ref<CExpressionArray> pdrgpexprInner,
				gpos::Ref<IMdIdArray> join_opfamilies)
	{
		m_pgroup->SetJoinKeys(pdrgpexprOuter, pdrgpexprInner, join_opfamilies);
	}

	// insert group expression
	void Insert(gpos::Ref<CGroupExpression> pgexpr);

	// move duplicate group expression to duplicates list
	void MoveDuplicateGExpr(gpos::Ref<CGroupExpression> pgexpr);

	// initialize group's properties;
	void InitProperties(gpos::Ref<CDrvdProp> pdp);

	// initialize group's stat;
	void InitStats(gpos::Ref<IStatistics> stats);

	// retrieve first group expression
	gpos::Ref<CGroupExpression> PgexprFirst();

	// retrieve next group expression
	gpos::Ref<CGroupExpression> PgexprNext(gpos::Ref<CGroupExpression> pgexpr);

	// get the first non-logical group expression following the given expression
	gpos::Ref<CGroupExpression> PgexprSkipLogical(
		gpos::Ref<CGroupExpression> pgexpr);

	// get the next logical group expression following the given expression
	gpos::Ref<CGroupExpression> PgexprNextLogical(
		gpos::Ref<CGroupExpression> pgexpr);

	// lookup best expression under optimization context
	gpos::Ref<CGroupExpression> PgexprLookup(
		gpos::Ref<COptimizationContext> poc) const;


#ifdef GPOS_DEBUG
	// is group transition to given state complete?
	BOOL
	FTransitioned(CGroup::EState estate) const
	{
		return (CGroup::estExplored == estate && m_pgroup->FExplored()) ||
			   (CGroup::estImplemented == estate && m_pgroup->FImplemented()) ||
			   (CGroup::estOptimized == estate && m_pgroup->FOptimized());
	}
#endif	// GPOS_DEBUG

};	// class CGroupProxy

}  // namespace gpopt

#endif	// !GPOPT_CGroupProxy_H


// EOF
