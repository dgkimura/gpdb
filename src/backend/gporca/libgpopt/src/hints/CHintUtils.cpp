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

#include "gpos/common/clibwrapper.h"

#include "gpopt/operators/CExpressionHandle.h"

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

	return plan_hint->GetScanHint(name);
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
	return scan_hint->SatisfiesOperator(pop);
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
			return scan_hint->SatisfiesOperator(pop);
		}
	}

	return scan_hint->GetIndexNames()->Size() == 0 &&
		   scan_hint->SatisfiesOperator(pop);
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

	return scan_hint->SatisfiesOperator(pop);
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
			return scan_hint->SatisfiesOperator(pop);
		}
	}

	return scan_hint->GetIndexNames()->Size() == 0 &&
		   scan_hint->SatisfiesOperator(pop);
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
			return scan_hint->SatisfiesOperator(pop);
		}
	}

	return scan_hint->GetIndexNames()->Size() == 0 &&
		   scan_hint->SatisfiesOperator(pop);
}

static BOOL
SatisfiesJoinHintsRecurse(CMemoryPool *mp, const CJoinHint::JoinPair *joinpair,
						  CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != joinpair);

	if (pexpr->DeriveTableDescriptor()->Size() == 1)
	{
		// If there only one table descriptor then to satisfy there must be one
		// matching hint alias.
		return nullptr != joinpair->GetName() &&
			   pexpr->DeriveTableDescriptor()->First()->Name().Pstr()->Equals(
				   joinpair->GetName());
	}

	if (nullptr != joinpair->GetName())
	{
		// The hint specifies specific table, so there should only be one.
		return pexpr->DeriveTableDescriptor()->Size() == 1;
	}

	if (pexpr->Arity() <= 1)
	{
		// If no children (e.g. limit) then assume order has already been
		// handled.
		return CHintUtils::ExpressionContainsHint(mp, joinpair, pexpr) &&
			   CHintUtils::HintContainsExpression(mp, joinpair, pexpr);
	}

	// Check whether the hint matches a subtree in pexpr
	//
	// For example:
	//
	//   Hint        Pexpr
	//     x            x                    .
	//    / \          / \                   .
	//  t1   t2       t3  x                  .
	//                   / \                 .
	//                 t1   t2               .

	// left hint on one side and right hint on other side
	if (CHintUtils::ExpressionContainsHint(mp, joinpair->GetOuter(),
										   (*pexpr)[0]) &&
		CHintUtils::ExpressionContainsHint(mp, joinpair->GetInner(),
										   (*pexpr)[1]))
	{
		return SatisfiesJoinHintsRecurse(mp, joinpair->GetOuter(),
										 (*pexpr)[0]) &&
			   SatisfiesJoinHintsRecurse(mp, joinpair->GetInner(), (*pexpr)[1]);
	}
	// left hint on one side and right hint on other side (mirrors above case)
	else if (CHintUtils::ExpressionContainsHint(mp, joinpair->GetOuter(),
												(*pexpr)[1]) &&
			 CHintUtils::ExpressionContainsHint(mp, joinpair->GetInner(),
												(*pexpr)[0]))
	{
		return SatisfiesJoinHintsRecurse(mp, joinpair->GetOuter(),
										 (*pexpr)[1]) &&
			   SatisfiesJoinHintsRecurse(mp, joinpair->GetInner(), (*pexpr)[0]);
	}
	// both hints on left side
	else if (CHintUtils::ExpressionContainsHint(mp, joinpair, (*pexpr)[0]))
	{
		return SatisfiesJoinHintsRecurse(mp, joinpair, (*pexpr)[0]);
	}
	// both hints on right side
	else if (CHintUtils::ExpressionContainsHint(mp, joinpair, (*pexpr)[1]))
	{
		return SatisfiesJoinHintsRecurse(mp, joinpair, (*pexpr)[1]);
	}

	// Check whether pexpr is a subtree in the hint.
	//
	// These mirror cases are required because DPV2 join algorithm is executed
	// bottom up. In this case the join may be an incomplete work in progress.
	//
	// For example:
	//
	//   Hint       Pexpr
	//     x           x                    .
	//    / \         / \                   .
	//   t3  x       t1  t2                 .
	//      / \                             .
	//    t1   t2                           .
	else if (CHintUtils::HintContainsExpression(mp, joinpair->GetOuter(),
												pexpr))
	{
		return SatisfiesJoinHintsRecurse(mp, joinpair->GetOuter(), pexpr);
	}
	else if (CHintUtils::HintContainsExpression(mp, joinpair->GetInner(),
												pexpr))
	{
		return SatisfiesJoinHintsRecurse(mp, joinpair->GetInner(), pexpr);
	}
	else if (CHintUtils::HintContainsExpression(mp, joinpair, pexpr))
	{
		// If expression is broken along different branches of the hint, then
		// it is not satisfied.
		//
		// Note that the case where the trees are symmetric is handled
		// previously while checking if hint is a subrree of the expression.
		return false;
	}

	// hint is not a subtree of pexpr and pexpr is not a subtree of hint. so
	// let it go through
	return true;
}

BOOL
CHintUtils::SatisfiesJoinHints(CMemoryPool *mp, CExpression *pexpr,
							   CPlanHint *plan_hint)
{
	GPOS_ASSERT(nullptr != pexpr);
	if (plan_hint == nullptr)
	{
		// no matched hint, so everything goes...
		return true;
	}

	CJoinHint *join_hint = plan_hint->GetJoinHint();
	if (join_hint == nullptr)
	{
		// no matched hint, so everything goes...
		return true;
	}

	const CJoinHint::JoinPair *joinpair = join_hint->GetJoinPair();
	GPOS_ASSERT(nullptr != joinpair);

	return SatisfiesJoinHintsRecurse(mp, joinpair, pexpr);
}


BOOL
CHintUtils::ExpressionContainsHint(CMemoryPool *mp,
								   const CJoinHint::JoinPair *joinpair,
								   CExpression *pexpr)
{
	CExpressionHandle exprhdl(mp);
	exprhdl.Attach(pexpr);
	exprhdl.DeriveProps(nullptr /*pdpctxt*/);

	CWStringConstHashSet *aliases = GPOS_NEW(mp) CWStringConstHashSet(mp);

	CTableDescriptorHashSetIter hsiter(exprhdl.DeriveTableDescriptor());
	while (hsiter.Advance())
	{
		const CTableDescriptor *tabdesc = hsiter.Get();
		aliases->Insert(tabdesc->Name().Pstr());
	}

	// Check that all hint aliases are in the table aliases
	CJoinHint::CWStringConstHashSetIter aliasiter(joinpair->GetAliases());
	while (aliasiter.Advance())
	{
		const CWStringConst *alias = aliasiter.Get();
		if (!aliases->Contains(alias))
		{
			return false;
		}
	}

	return true;
}


BOOL
CHintUtils::HintContainsExpression(CMemoryPool *mp,
								   const CJoinHint::JoinPair *joinpair,
								   CExpression *pexpr)
{
	CExpressionHandle exprhdl(mp);
	exprhdl.Attach(pexpr);
	exprhdl.DeriveProps(nullptr /*pdpctxt*/);

	// Check that all table aliases are in the hint aliases
	CTableDescriptorHashSetIter hsiter(exprhdl.DeriveTableDescriptor());
	while (hsiter.Advance())
	{
		const CTableDescriptor *tabdesc = hsiter.Get();
		if (!joinpair->GetAliases()->Contains(tabdesc->Name().Pstr()))
		{
			return false;
		}
	}

	return true;
}

const WCHAR *
CHintUtils::ScanHintEnumToString(CScanHint::EType type)
{
	switch (type)
	{
		case CScanHint::SeqScan:
		{
			return GPOS_WSZ_LIT("SeqScan");
		}
		case CScanHint::NoSeqScan:
		{
			return GPOS_WSZ_LIT("NoSeqScan");
		}
		case CScanHint::IndexScan:
		{
			return GPOS_WSZ_LIT("IndexScan");
		}
		case CScanHint::NoIndexScan:
		{
			return GPOS_WSZ_LIT("NoIndexScan");
		}
		case CScanHint::IndexOnlyScan:
		{
			return GPOS_WSZ_LIT("IndexOnlyScan");
		}
		case CScanHint::NoIndexOnlyScan:
		{
			return GPOS_WSZ_LIT("NoIndexOnlyScan");
		}
		case CScanHint::BitmapScan:
		{
			return GPOS_WSZ_LIT("BitmapScan");
		}
		case CScanHint::NoBitmapScan:
		{
			return GPOS_WSZ_LIT("NoBitmapScan");
		}
		default:
		{
			return nullptr;
		}
	}
}

CScanHint::EType
CHintUtils::ScanHintStringToEnum(const WCHAR *type)
{
	if (0 ==
		clib::Wcsncmp(type, GPOS_WSZ_LIT("SeqScan"), gpos::clib::Wcslen(type)))
	{
		return CScanHint::SeqScan;
	}
	if (0 == clib::Wcsncmp(type, GPOS_WSZ_LIT("NoSeqScan"),
						   gpos::clib::Wcslen(type)))
	{
		return CScanHint::NoSeqScan;
	}
	if (0 == clib::Wcsncmp(type, GPOS_WSZ_LIT("IndexScan"),
						   gpos::clib::Wcslen(type)))
	{
		return CScanHint::IndexScan;
	}
	if (0 == clib::Wcsncmp(type, GPOS_WSZ_LIT("NoIndexScan"),
						   gpos::clib::Wcslen(type)))
	{
		return CScanHint::NoIndexScan;
	}
	if (0 == clib::Wcsncmp(type, GPOS_WSZ_LIT("IndexOnlyScan"),
						   gpos::clib::Wcslen(type)))
	{
		return CScanHint::IndexOnlyScan;
	}
	if (0 == clib::Wcsncmp(type, GPOS_WSZ_LIT("NoIndexOnlyScan"),
						   gpos::clib::Wcslen(type)))
	{
		return CScanHint::NoIndexOnlyScan;
	}
	if (0 == clib::Wcsncmp(type, GPOS_WSZ_LIT("BitmapScan"),
						   gpos::clib::Wcslen(type)))
	{
		return CScanHint::BitmapScan;
	}
	if (0 == clib::Wcsncmp(type, GPOS_WSZ_LIT("NoBitmapScan"),
						   gpos::clib::Wcslen(type)))
	{
		return CScanHint::NoBitmapScan;
	}
	return CScanHint::Sentinal;
}
