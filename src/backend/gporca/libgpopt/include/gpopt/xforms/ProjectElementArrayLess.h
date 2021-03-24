//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOPT_ProjectElementArrayLess_H
#define GPOPT_ProjectElementArrayLess_H

#include "gpos/common/Ref.h"

#include "gpopt/operators/CExpression.h"

namespace gpopt
{
struct ProjectElementArrayLess
{
	bool operator()(const CExpressionArray *a, const CExpressionArray *b) const;
	bool operator()(const gpos::Ref<CExpressionArray> &a,
					const gpos::Ref<CExpressionArray> &b) const;
};
}  // namespace gpopt

#endif	//GPOPT_ProjectElementArrayLess_H
