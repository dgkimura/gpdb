//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 VMware, Inc. or its affiliates.
//
//	@filename:
//		CScalarArrayCoerceExpr.cpp
//
//	@doc:
//		Implementation of scalar array coerce expr operator
//
//	@owner:
//
//	@test:
//
//
//---------------------------------------------------------------------------

#include "gpopt/operators/CScalarArrayCoerceExpr.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

using namespace gpopt;
using namespace gpmd;


//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCoerceExpr::CScalarArrayCoerceExpr
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarArrayCoerceExpr::CScalarArrayCoerceExpr(
	CMemoryPool *mp, gpos::owner<IMDId *> element_func,
	gpos::owner<IMDId *> result_type_mdid, INT type_modifier, BOOL is_explicit,
	ECoercionForm ecf, INT location)
	: CScalarCoerceBase(mp, std::move(result_type_mdid), type_modifier, ecf,
						location),
	  m_pmdidElementFunc(std::move(element_func)),
	  m_is_explicit(is_explicit)
{
	GPOS_ASSERT(nullptr != m_pmdidElementFunc);
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCoerceExpr::~CScalarArrayCoerceExpr
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CScalarArrayCoerceExpr::~CScalarArrayCoerceExpr()
{
	m_pmdidElementFunc->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCoerceExpr::PmdidElementFunc
//
//	@doc:
//		Return metadata id of element coerce function
//
//---------------------------------------------------------------------------
gpos::pointer<IMDId *>
CScalarArrayCoerceExpr::PmdidElementFunc() const
{
	return m_pmdidElementFunc;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCoerceExpr::IsExplicit
//
//	@doc:
//		Conversion semantics flag to pass to func
//
//---------------------------------------------------------------------------
BOOL
CScalarArrayCoerceExpr::IsExplicit() const
{
	return m_is_explicit;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCoerceExpr::Eopid
//
//	@doc:
//		Return operator identifier
//
//---------------------------------------------------------------------------
CScalar::EOperatorId
CScalarArrayCoerceExpr::Eopid() const
{
	return EopScalarArrayCoerceExpr;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCoerceExpr::SzId
//
//	@doc:
//		Return a string for operator name
//
//---------------------------------------------------------------------------
const CHAR *
CScalarArrayCoerceExpr::SzId() const
{
	return "CScalarArrayCoerceExpr";
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCoerceExpr::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarArrayCoerceExpr::Matches(gpos::pointer<COperator *> pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	gpos::pointer<CScalarArrayCoerceExpr *> popCoerce =
		gpos::dyn_cast<CScalarArrayCoerceExpr>(pop);

	return popCoerce->PmdidElementFunc()->Equals(m_pmdidElementFunc) &&
		   popCoerce->MdidType()->Equals(MdidType()) &&
		   popCoerce->TypeModifier() == TypeModifier() &&
		   popCoerce->IsExplicit() == m_is_explicit &&
		   popCoerce->Ecf() == Ecf() && popCoerce->Location() == Location();
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCoerceExpr::FInputOrderSensitive
//
//	@doc:
//		Sensitivity to order of inputs
//
//---------------------------------------------------------------------------
BOOL
CScalarArrayCoerceExpr::FInputOrderSensitive() const
{
	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarArrayCoerceExpr::PopConvert
//
//	@doc:
//		Conversion function
//
//---------------------------------------------------------------------------
gpos::cast_func<CScalarArrayCoerceExpr *>
CScalarArrayCoerceExpr::PopConvert(COperator *pop)
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(EopScalarArrayCoerceExpr == pop->Eopid());

	return dynamic_cast<CScalarArrayCoerceExpr *>(pop);
}


// EOF
