//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------

#include "naucrates/statistics/StatsPredLess.h"

#include "gpos/common/owner.h"

#include "naucrates/statistics/CStatsPred.h"

namespace gpnaucrates
{
bool
StatsPredColIdLess::operator()(
	gpos::pointer<const gpnaucrates::CStatsPred *> a,
	gpos::pointer<const gpnaucrates::CStatsPred *> b) const
{
	return a->GetColId() < b->GetColId();
}

bool
StatsPredColIdLess::operator()(
	const gpos::Ref<gpnaucrates::CStatsPred> &a,
	const gpos::Ref<gpnaucrates::CStatsPred> &b) const
{
	return operator()(a.get(), b.get());
}
}  // namespace gpnaucrates
