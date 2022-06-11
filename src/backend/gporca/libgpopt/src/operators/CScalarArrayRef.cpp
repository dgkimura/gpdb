//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CScalarArrayRef.cpp
//
//	@doc:
//		Implementation of scalar arrayref
//---------------------------------------------------------------------------

#include "gpopt/operators/CScalarArrayRef.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayRef::CScalarArrayRef
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarArrayRef::CScalarArrayRef(CMemoryPool *mp,
								 gpos::Ref<IMDId> elem_type_mdid,
								 INT type_modifier,
								 gpos::Ref<IMDId> array_type_mdid,
								 gpos::Ref<IMDId> return_type_mdid)
	: CScalar(mp),
	  m_pmdidElem(std::move(elem_type_mdid)),
	  m_type_modifier(type_modifier),
	  m_pmdidArray(std::move(array_type_mdid)),
	  m_mdid_type(std::move(return_type_mdid))
{
	GPOS_ASSERT(m_pmdidElem->IsValid());
	GPOS_ASSERT(m_pmdidArray->IsValid());
	GPOS_ASSERT(m_mdid_type->IsValid());
	GPOS_ASSERT(m_mdid_type->Equals(m_pmdidElem.get()) ||
				m_mdid_type->Equals(m_pmdidArray.get()));
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayRef::~CScalarArrayRef
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CScalarArrayRef::~CScalarArrayRef()
{
	;
	;
	;
}


INT
CScalarArrayRef::TypeModifier() const
{
	return m_type_modifier;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayRef::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CScalarArrayRef::HashValue() const
{
	return gpos::CombineHashes(
		CombineHashes(m_pmdidElem->HashValue(), m_pmdidArray->HashValue()),
		m_mdid_type->HashValue());
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayRef::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarArrayRef::Matches(COperator *pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	CScalarArrayRef *popArrayRef = gpos::dyn_cast<CScalarArrayRef>(pop);

	return m_mdid_type->Equals(popArrayRef->MdidType()) &&
		   m_pmdidElem->Equals(popArrayRef->PmdidElem()) &&
		   m_pmdidArray->Equals(popArrayRef->PmdidArray());
}

// EOF
