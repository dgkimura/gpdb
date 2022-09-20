//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_PointLess_H
#define GPNAUCRATES_PointLess_H

#include "gpos/common/Ref.h"
#include "gpos/common/owner.h"

namespace gpnaucrates
{
class CPoint;

struct PointLess
{
	bool operator()(const gpnaucrates::CPoint *a,
					const gpnaucrates::CPoint *b) const;

	bool operator()(const gpos::Ref<gpnaucrates::CPoint> &a,
					const gpos::Ref<gpnaucrates::CPoint> &b) const;
};
}  // namespace gpnaucrates

#endif	//GPNAUCRATES_PointLess_H
