//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CScalarArrayRef.h
//
//	@doc:
//		Class for scalar arrayref
//---------------------------------------------------------------------------
#ifndef GPOPT_CScalarArrayRef_H
#define GPOPT_CScalarArrayRef_H

#include "gpos/base.h"

#include "gpopt/operators/CScalar.h"
#include "naucrates/md/IMDId.h"

namespace gpopt
{
using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@class:
//		CScalarArrayRef
//
//	@doc:
//		Scalar arrayref
//
//		Arrayrefs are used to reference array elements or subarrays
//		e.g. select a[1], b[1][2][3], c[3:5], d[1:2][4:9] from arrtest;
//
//---------------------------------------------------------------------------
class CScalarArrayRef : public CScalar
{
private:
	// element type id
	gpos::Ref<IMDId> m_pmdidElem;

	// element type modifier
	INT m_type_modifier;

	// array type id
	gpos::Ref<IMDId> m_pmdidArray;

	// return type id
	gpos::Ref<IMDId> m_mdid_type;

public:
	CScalarArrayRef(const CScalarArrayRef &) = delete;

	// ctor
	CScalarArrayRef(CMemoryPool *mp, gpos::Ref<IMDId> elem_type_mdid,
					INT type_modifier, gpos::Ref<IMDId> array_type_mdid,
					gpos::Ref<IMDId> return_type_mdid);

	// dtor
	~CScalarArrayRef() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopScalarArrayRef;
	}

	// operator name
	const CHAR *
	SzId() const override
	{
		return "CScalarArrayRef";
	}

	// element type id
	gpos::Ref<IMDId>
	PmdidElem() const
	{
		return m_pmdidElem;
	}

	// element type modifier
	INT TypeModifier() const override;

	// array type id
	gpos::Ref<IMDId>
	PmdidArray() const
	{
		return m_pmdidArray;
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(gpos::Ref<COperator> pop) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return true;
	}

	// return a copy of the operator with remapped columns
	gpos::Ref<COperator>
	PopCopyWithRemappedColumns(CMemoryPool *,				 //mp,
							   gpos::Ref<UlongToColRefMap>,	 //colref_mapping,
							   BOOL							 //must_exist
							   ) override
	{
		return PopCopyDefault();
	}

	// type of expression's result
	gpos::Ref<IMDId>
	MdidType() const override
	{
		return m_mdid_type;
	}

	// conversion function
	static gpos::Ref<CScalarArrayRef>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopScalarArrayRef == pop->Eopid());

		return gpos::dyn_cast<CScalarArrayRef>(pop);
	}

};	// class CScalarArrayRef
}  // namespace gpopt

#endif	// !GPOPT_CScalarArrayRef_H

// EOF
