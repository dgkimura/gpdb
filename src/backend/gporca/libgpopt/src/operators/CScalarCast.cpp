//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CScalarCast.cpp
//
//	@doc:
//		Implementation of scalar relabel type  operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CScalarCast.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/mdcache/CMDAccessorUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "naucrates/md/IMDTypeBool.h"

using namespace gpopt;
using namespace gpmd;


//---------------------------------------------------------------------------
//	@function:
//		CScalarCast::CScalarCast
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarCast::CScalarCast(CMemoryPool *mp, gpos::Ref<IMDId> return_type_mdid,
						 gpos::Ref<IMDId> mdid_func, BOOL is_binary_coercible)
	: CScalar(mp),
	  m_return_type_mdid(return_type_mdid),
	  m_func_mdid(mdid_func),
	  m_is_binary_coercible(is_binary_coercible),
	  m_returns_null_on_null_input(false),
	  m_fBoolReturnType(false)
{
	if (nullptr != m_func_mdid && m_func_mdid->IsValid())
	{
		CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
		const IMDFunction *pmdfunc =
			md_accessor->RetrieveFunc(m_func_mdid.get());

		m_returns_null_on_null_input = pmdfunc->IsStrict();
		m_fBoolReturnType =
			CMDAccessorUtils::FBoolType(md_accessor, m_return_type_mdid.get());
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCast::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarCast::Matches(COperator *pop) const
{
	if (pop->Eopid() == Eopid())
	{
		CScalarCast *pscop = gpos::dyn_cast<CScalarCast>(pop);

		// match if the return type oids are identical
		return pscop->MdidType()->Equals(m_return_type_mdid.get()) &&
			   ((!IMDId::IsValid(pscop->FuncMdId()) &&
				 !IMDId::IsValid(m_func_mdid.get())) ||
				pscop->FuncMdId()->Equals(m_func_mdid.get()));
	}

	return false;
}

// EOF
