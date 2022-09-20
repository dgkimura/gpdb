//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOPT_DatumLess_H
#define GPOPT_DatumLess_H

#include "gpos/common/Ref.h"
#include "gpos/common/owner.h"

namespace gpnaucrates
{
class IDatum;
}

namespace gpopt
{
struct DatumLess
{
	bool operator()(const gpnaucrates::IDatum *a,
					const gpnaucrates::IDatum *b) const;

	bool operator()(const gpos::Ref<gpnaucrates::IDatum> &a,
					const gpos::Ref<gpnaucrates::IDatum> &b) const;
};
}  // namespace gpopt

#endif	//GPOPT_DatumLess_H
