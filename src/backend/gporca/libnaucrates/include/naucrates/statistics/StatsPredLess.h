//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_StatsPredLess_H
#define GPNAUCRATES_StatsPredLess_H

#include "gpos/common/Ref.h"
#include "gpos/common/owner.h"

namespace gpnaucrates
{
class CStatsPred;

struct StatsPredColIdLess
{
	bool operator()(const gpnaucrates::CStatsPred *a,
					const gpnaucrates::CStatsPred *b) const;

	bool operator()(const gpos::Ref<gpnaucrates::CStatsPred> &a,
					const gpos::Ref<gpnaucrates::CStatsPred> &b) const;
};
}  // namespace gpnaucrates

#endif	//GPNAUCRATES_StatsPredLess_H
