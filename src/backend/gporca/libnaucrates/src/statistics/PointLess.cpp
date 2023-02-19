//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------

#include "naucrates/statistics/PointLess.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CDrvdPropCtxtPlan.h"
#include "gpopt/search/CGroupExpression.h"
#include "naucrates/statistics/CPoint.h"

namespace gpnaucrates
{
bool
PointLess::operator()(const gpos::Ref<gpnaucrates::CPoint> a,
					  const gpos::Ref<gpnaucrates::CPoint> b) const
{
	return a->IsLessThan(b);
}
}  // namespace gpnaucrates
