//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (c) 2023 VMware, Inc. or its affiliates. All Rights Reserved.
//
//	@filename:
//		CHintUtils.cpp
//
//	@doc:
//		Utilitiies for plan hint objects
//---------------------------------------------------------------------------

#include "gpopt/hints/CHintUtils.h"

using namespace gpopt;


// Find a scan hint that matches the operator based on the relation or alias
// name.
template <typename T>
static CScanHint *
GetScanHint(T *pop, CPlanHint *plan_hint)
{
	GPOS_ASSERT(nullptr != pop);

	if (nullptr == plan_hint)
	{
		// no parsed hints
		return nullptr;
	}

	const CWStringConst *name = pop->Ptabdesc()->Name().Pstr();

	return static_cast<CScanHint *>(plan_hint->GetHint(name));
}


BOOL
CHintUtils::SatisfiesPlanHints(CLogicalGet *pop, CPlanHint *plan_hint)
{
	GPOS_ASSERT(nullptr != pop);

	CScanHint *scan_hint = GetScanHint(pop, plan_hint);
	if (scan_hint == nullptr)
	{
		// no matched hint, so everything goes...
		return true;
	}

	// If opertor matches hint operator _or_ it doesn't match and is a not.
	return (scan_hint->GetOperator() == pop->Eopid() &&
			!scan_hint->GetIsNot()) ||
		   (scan_hint->GetOperator() != pop->Eopid() && scan_hint->GetIsNot());
}

BOOL
CHintUtils::SatisfiesPlanHints(CLogicalIndexGet *pop, CPlanHint *plan_hint)
{
	GPOS_ASSERT(nullptr != pop);

	CScanHint *scan_hint = GetScanHint(pop, plan_hint);
	if (scan_hint == nullptr)
	{
		// no matched hint, so everything goes...
		return true;
	}

	for (ULONG ul = 0; ul < scan_hint->GetIndexNames()->Size(); ul++)
	{
		if (pop->Pindexdesc()->Name().Pstr()->Equals(
				(*scan_hint->GetIndexNames())[ul]))
		{
			// If opertor matches hint operator and index matches hint index.
			return scan_hint->GetOperator() == pop->Eopid();
		}
	}

	// If opertor matches hint operator and no index was specified
	// If opertor doesn't match hint operator and is a not.
	return (scan_hint->GetOperator() == pop->Eopid() &&
			scan_hint->GetIndexNames()->Size() == 0 &&
			!scan_hint->GetIsNot()) ||
		   (scan_hint->GetOperator() != pop->Eopid() && scan_hint->GetIsNot());
}


BOOL
CHintUtils::SatisfiesPlanHints(CLogicalDynamicGet *pop, CPlanHint *plan_hint)
{
	GPOS_ASSERT(nullptr != pop);

	CScanHint *scan_hint = GetScanHint(pop, plan_hint);
	if (scan_hint == nullptr)
	{
		// no matched hint, so everything goes...
		return true;
	}

	// If opertor matches hint operator _or_ it doesn't match and is a not.
	return (scan_hint->GetOperator() == pop->Eopid() &&
			!scan_hint->GetIsNot()) ||
		   (scan_hint->GetOperator() != pop->Eopid() && scan_hint->GetIsNot());
}


BOOL
CHintUtils::SatisfiesPlanHints(CLogicalDynamicIndexGet *pop,
							   CPlanHint *plan_hint)
{
	GPOS_ASSERT(nullptr != pop);

	CScanHint *scan_hint = GetScanHint(pop, plan_hint);
	if (scan_hint == nullptr)
	{
		// no matched hint, so everything goes...
		return true;
	}

	for (ULONG ul = 0; ul < scan_hint->GetIndexNames()->Size(); ul++)
	{
		if (pop->Pindexdesc()->Name().Pstr()->Equals(
				(*scan_hint->GetIndexNames())[ul]))
		{
			// If opertor matches hint operator and index matches hint index.
			return scan_hint->GetOperator() == pop->Eopid();
		}
	}

	// If opertor matches hint operator and no index was specified
	// If opertor doesn't match hint operator and is a not.
	return (scan_hint->GetOperator() == pop->Eopid() &&
			scan_hint->GetIndexNames()->Size() == 0 &&
			!scan_hint->GetIsNot()) ||
		   (scan_hint->GetOperator() != pop->Eopid() && scan_hint->GetIsNot());
}


BOOL
CHintUtils::SatisfiesPlanHints(CScalarBitmapIndexProbe *pop,
							   CPlanHint *plan_hint)
{
	GPOS_ASSERT(nullptr != pop);

	CScanHint *scan_hint = GetScanHint(pop, plan_hint);
	if (scan_hint == nullptr)
	{
		// no matched hint, so everything goes...
		return true;
	}

	for (ULONG ul = 0; ul < scan_hint->GetIndexNames()->Size(); ul++)
	{
		if (pop->Pindexdesc()->Name().Pstr()->Equals(
				(*scan_hint->GetIndexNames())[ul]))
		{
			// If opertor matches hint operator and index matches hint index.
			return scan_hint->GetOperator() == pop->Eopid();
		}
	}

	// If opertor matches hint operator and no index was specified
	// If opertor doesn't match hint operator and is a not.
	return (scan_hint->GetOperator() == pop->Eopid() &&
			scan_hint->GetIndexNames()->Size() == 0 &&
			!scan_hint->GetIsNot()) ||
		   (scan_hint->GetOperator() != pop->Eopid() && scan_hint->GetIsNot());
}
