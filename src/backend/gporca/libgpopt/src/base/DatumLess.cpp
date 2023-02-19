//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------

#include "gpopt/base/DatumLess.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CDrvdPropCtxtPlan.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/search/CGroupExpression.h"


namespace gpopt
{
bool
DatumLess::operator()(const gpos::Ref<gpnaucrates::IDatum> a,
					  const gpos::Ref<gpnaucrates::IDatum> b) const
{
	const IComparator *pcomp = COptCtxt::PoctxtFromTLS()->Pcomp();
	return pcomp->IsLessThan(a, b);
}
}  // namespace gpopt
