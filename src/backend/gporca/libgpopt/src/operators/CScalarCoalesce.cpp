//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CScalarCoalesce.cpp
//
//	@doc:
//		Implementation of scalar coalesce operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CScalarCoalesce.h"

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
//		CScalarCoalesce::CScalarCoalesce
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarCoalesce::CScalarCoalesce(CMemoryPool *mp,
								 gpos::owner<IMDId *> mdid_type)
	: CScalar(mp), m_mdid_type(std::move(mdid_type)), m_fBoolReturnType(false)
{
	GPOS_ASSERT(m_mdid_type->IsValid());

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	m_fBoolReturnType = CMDAccessorUtils::FBoolType(md_accessor, m_mdid_type);
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarCoalesce::~CScalarCoalesce
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CScalarCoalesce::~CScalarCoalesce()
{
	m_mdid_type->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarCoalesce::HashValue
//
//	@doc:
//		Operator specific hash function; combined hash of operator id and
//		return type id
//
//---------------------------------------------------------------------------
ULONG
CScalarCoalesce::HashValue() const
{
	return gpos::CombineHashes(COperator::HashValue(),
							   m_mdid_type->HashValue());
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarCoalesce::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarCoalesce::Matches(gpos::pointer<COperator *> pop) const
{
	if (pop->Eopid() == Eopid())
	{
		gpos::pointer<CScalarCoalesce *> popScCoalesce =
			gpos::dyn_cast<CScalarCoalesce>(pop);

		// match if return types are identical
		return popScCoalesce->MdidType()->Equals(m_mdid_type);
	}

	return false;
}


// EOF
