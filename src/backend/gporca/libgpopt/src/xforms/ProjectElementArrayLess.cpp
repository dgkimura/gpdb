//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2021 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------

#include "gpopt/xforms/ProjectElementArrayLess.h"

#include "gpos/common/Casting.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CScalarProjectElement.h"

namespace gpopt
{
bool
ProjectElementArrayLess::operator()(
	gpos::pointer<const CExpressionArray *> a,
	gpos::pointer<const CExpressionArray *> b) const
{
	CExpression *pexprPrjElemFst = (*a)[0];
	CExpression *pexprPrjElemSnd = (*b)[0];
	ULONG ulIdFst =
		gpos::cast<CScalarProjectElement>(pexprPrjElemFst->Pop())->Pcr()->Id();
	ULONG ulIdSnd =
		gpos::cast<CScalarProjectElement>(pexprPrjElemSnd->Pop())->Pcr()->Id();

	return ulIdFst < ulIdSnd;
}

bool
ProjectElementArrayLess::operator()(const gpos::Ref<CExpressionArray> &a,
									const gpos::Ref<CExpressionArray> &b) const
{
	return operator()(a.get(), b.get());
}
}  // namespace gpopt
