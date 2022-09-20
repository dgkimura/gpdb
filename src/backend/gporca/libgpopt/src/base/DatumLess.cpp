//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------

#include "gpopt/base/DatumLess.h"

#include "gpos/common/owner.h"

#include "gpopt/base/COptCtxt.h"

namespace gpopt
{
bool
DatumLess::operator()(const gpnaucrates::IDatum *a,
					  const gpnaucrates::IDatum *b) const
{
	const IComparator *pcomp = COptCtxt::PoctxtFromTLS()->Pcomp();
	return pcomp->IsLessThan(a, b);
}

bool
DatumLess::operator()(const Ref<gpnaucrates::IDatum> &a,
					  const Ref<gpnaucrates::IDatum> &b) const
{
	return operator()(a.get(), b.get());
}
}  // namespace gpopt
