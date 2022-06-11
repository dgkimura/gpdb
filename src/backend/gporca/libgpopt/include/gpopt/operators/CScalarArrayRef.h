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
#include "gpos/common/owner.h"

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
	gpos::owner<IMDId *> m_pmdidElem;

	// element type modifier
	INT m_type_modifier;

	// array type id
	gpos::owner<IMDId *> m_pmdidArray;

	// return type id
	gpos::owner<IMDId *> m_mdid_type;

public:
	CScalarArrayRef(const CScalarArrayRef &) = delete;

	// ctor
	CScalarArrayRef(CMemoryPool *mp, IMDId *elem_type_mdid, INT type_modifier,
					IMDId *array_type_mdid, IMDId *return_type_mdid);

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
	gpos::pointer<IMDId *>
	PmdidElem() const
	{
		return m_pmdidElem;
	}

	// element type modifier
	INT TypeModifier() const override;

	// array type id
	gpos::pointer<IMDId *>
	PmdidArray() const
	{
		return m_pmdidArray;
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(COperator *pop) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return true;
	}

	// return a copy of the operator with remapped columns
	COperator *
	PopCopyWithRemappedColumns(
		CMemoryPool *,						//mp,
		gpos::pointer<UlongToColRefMap *>,	//colref_mapping,
		BOOL								//must_exist
		) override
	{
		return PopCopyDefault();
	}

	// type of expression's result
	gpos::pointer<IMDId *>
	MdidType() const override
	{
		return m_mdid_type;
	}

	// conversion function
	static gpos::cast_func<CScalarArrayRef *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopScalarArrayRef == pop->Eopid());

		return dynamic_cast<CScalarArrayRef *>(pop);
	}

};	// class CScalarArrayRef
}  // namespace gpopt

#endif	// !GPOPT_CScalarArrayRef_H

// EOF
