//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CScalarIf.cpp
//
//	@doc:
//		Implementation of scalar if operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CScalarIf.h"

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
//		CScalarIf::CScalarIf
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarIf::CScalarIf(CMemoryPool *mp, gpos::owner<IMDId *> mdid)
	: CScalar(mp), m_mdid_type(std::move(mdid)), m_fBoolReturnType(false)
{
	GPOS_ASSERT(m_mdid_type->IsValid());

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	m_fBoolReturnType = CMDAccessorUtils::FBoolType(md_accessor, m_mdid_type);
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarIf::HashValue
//
//	@doc:
//		Operator specific hash function; combined hash of operator id and
//		return type id
//
//---------------------------------------------------------------------------
ULONG
CScalarIf::HashValue() const
{
	return gpos::CombineHashes(COperator::HashValue(),
							   m_mdid_type->HashValue());
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarIf::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarIf::Matches(gpos::pointer<COperator *> pop) const
{
	if (pop->Eopid() == Eopid())
	{
		gpos::pointer<CScalarIf *> popScIf = gpos::dyn_cast<CScalarIf>(pop);

		// match if return types are identical
		return popScIf->MdidType()->Equals(m_mdid_type);
	}

	return false;
}

// EOF
