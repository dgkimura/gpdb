//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.

#include "gpopt/base/CDistributionSpecStrictRandom.h"

#include "gpos/common/owner.h"

#include "gpopt/search/CGroupExpression.h"

using namespace gpopt;

CDistributionSpecStrictRandom::CDistributionSpecStrictRandom() = default;

BOOL
CDistributionSpecStrictRandom::Matches(
	const gpos::Ref<CDistributionSpec> pds) const
{
	return pds->Edt() == Edt();
}

BOOL
CDistributionSpecStrictRandom::FSatisfies(
	const gpos::Ref<CDistributionSpec> pds) const
{
	return Matches(pds) || EdtAny == pds->Edt() || EdtRandom == pds->Edt() ||
		   EdtNonSingleton == pds->Edt();
}
