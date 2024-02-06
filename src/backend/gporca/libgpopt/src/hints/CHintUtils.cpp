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
SatisfiesJoinHintsInternal(CMemoryPool *mp,
						   const struct CJoinHint::JoinPair *joinpair,
						   CExpression *pexpr)
{
	if (nullptr != joinpair->GetName())
	{
		CHintUtils::CWStringConstHashSet *tabs =
			CHintUtils::GetAliases(mp, pexpr);

		return tabs->Size() == 1 && tabs->First()->Equals(joinpair->GetName());
	}

	CHintUtils::CWStringConstHashSet *outertabs =
		CHintUtils::GetAliases(mp, (*pexpr)[0]);
	CHintUtils::CWStringConstHashSet *innertabs =
		CHintUtils::GetAliases(mp, (*pexpr)[1]);

	GPOS_ASSERT(nullptr != joinpair);
	GPOS_ASSERT(nullptr != outertabs);
	GPOS_ASSERT(nullptr != innertabs);

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

	const struct CJoinHint::JoinPair *joinpair = join_hint->GetJoinPair();
	GPOS_ASSERT(nullptr != joinpair);

	return SatisfiesJoinHintsInternal(mp, joinpair, pexpr);
}


CHintUtils::CWStringConstHashSet *
CHintUtils::GetAliases(CMemoryPool *mp, CExpression *pexpr)
{
	CWStringConstHashSet *aliases = GPOS_NEW(mp) CWStringConstHashSet(mp);

	CExpressionHandle outer_exprhdl(mp);
	outer_exprhdl.Attach(pexpr);
	outer_exprhdl.DeriveProps(nullptr /*pdpctxt*/);

	CTableDescriptorHashSet *tabset = outer_exprhdl.DeriveTableDescriptor();
	CTableDescriptorHashSetIter hsiter(tabset);
	while (hsiter.Advance())
	{
		const CTableDescriptor *tabdesc = hsiter.Get();
		aliases->Insert(const_cast<CWStringConst *>(tabdesc->Name().Pstr()));
	}

	return aliases;
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
