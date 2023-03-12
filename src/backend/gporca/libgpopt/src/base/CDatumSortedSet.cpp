//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.

#include "gpopt/base/CDatumSortedSet.h"

#include "gpos/common/CAutoRef.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/operators/COperator.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/search/CGroupExpression.h"

using namespace gpopt;

CDatumSortedSet::CDatumSortedSet(CMemoryPool *mp,
								 gpos::Ref<CExpression> pexprArray,
								 const IComparator *pcomp)
	: IDatumArray(mp), m_fIncludesNull(false)
{
	GPOS_ASSERT(COperator::EopScalarArray == pexprArray->Pop()->Eopid());

	const ULONG ulArrayExprArity = CUtils::UlScalarArrayArity(pexprArray);
	GPOS_ASSERT(0 < ulArrayExprArity);

	gpos::Ref<IDatumArray> aprngdatum = GPOS_NEW(mp) IDatumArray(mp);
	for (ULONG ul = 0; ul < ulArrayExprArity; ul++)
	{
		gpos::Ref<CScalarConst> popScConst =
			CUtils::PScalarArrayConstChildAt(pexprArray, ul);
		gpos::Ref<IDatum> datum = popScConst->GetDatum();
		if (datum->IsNull())
		{
			m_fIncludesNull = true;
		}
		else
		{
			aprngdatum->Append(datum);
		}
	}
	aprngdatum->Sort(
		[](const gpos::Ref<IDatum> &lhs, const gpos::Ref<IDatum> &rhs) {
			const IComparator *pcomp = COptCtxt::PoctxtFromTLS()->Pcomp();
			return pcomp->IsLessThan(lhs, rhs);
		});

	// de-duplicate
	const ULONG ulRangeArrayArity = aprngdatum->Size();
	gpos::Ref<IDatum> pdatumPrev = (*aprngdatum)[0];

	Append(pdatumPrev);
	for (ULONG ul = 1; ul < ulRangeArrayArity; ul++)
	{
		if (!pcomp->Equals((*aprngdatum)[ul], pdatumPrev))
		{
			pdatumPrev = (*aprngdatum)[ul];

			Append(pdatumPrev);
		}
	}
}

BOOL
CDatumSortedSet::FIncludesNull() const
{
	return m_fIncludesNull;
}
