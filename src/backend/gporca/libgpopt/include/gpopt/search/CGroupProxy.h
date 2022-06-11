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
#include "gpos/common/owner.h"

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
	gpos::pointer<CGroup *> m_pgroup;

	// skip group expressions starting from the given expression;
	CGroupExpression *PgexprSkip(CGroupExpression *pgexprStart,
								 BOOL fSkipLogical);

public:
	// ctor
	explicit CGroupProxy(gpos::pointer<CGroup *> pgroup);

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
	SetJoinKeys(gpos::pointer<CExpressionArray *> pdrgpexprOuter,
				gpos::pointer<CExpressionArray *> pdrgpexprInner,
				gpos::pointer<IMdIdArray *> join_opfamilies)
	{
		m_pgroup->SetJoinKeys(pdrgpexprOuter, pdrgpexprInner, join_opfamilies);
	}

	// insert group expression
	void Insert(CGroupExpression *pgexpr);

	// move duplicate group expression to duplicates list
	void MoveDuplicateGExpr(CGroupExpression *pgexpr);

	// initialize group's properties;
	void InitProperties(CDrvdProp *pdp);

	// initialize group's stat;
	void InitStats(IStatistics *stats);

	// retrieve first group expression
	CGroupExpression *PgexprFirst();

	// retrieve next group expression
	CGroupExpression *PgexprNext(gpos::pointer<CGroupExpression *> pgexpr);

	// get the first non-logical group expression following the given expression
	CGroupExpression *PgexprSkipLogical(
		gpos::pointer<CGroupExpression *> pgexpr);

	// get the next logical group expression following the given expression
	CGroupExpression *PgexprNextLogical(
		gpos::pointer<CGroupExpression *> pgexpr);

	// lookup best expression under optimization context
	CGroupExpression *PgexprLookup(COptimizationContext *poc) const;


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
