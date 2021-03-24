//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------

#include "naucrates/statistics/PointLess.h"

#include "naucrates/statistics/CPoint.h"

namespace gpnaucrates
{
bool
PointLess::operator()(const gpnaucrates::CPoint *a,
					  const gpnaucrates::CPoint *b) const
{
	return a->IsLessThan(b);
}

bool
PointLess::operator()(const gpos::Ref<gpnaucrates::CPoint> &a,
					  const gpos::Ref<gpnaucrates::CPoint> &b) const
{
	return operator()(a.get(), b.get());
}
}  // namespace gpnaucrates
