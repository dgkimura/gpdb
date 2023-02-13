//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CScalarCoalesce.h
//
//	@doc:
//		Scalar coalesce operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CScalarCoalesce_H
#define GPOPT_CScalarCoalesce_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDrvdProp.h"
#include "gpopt/operators/CScalar.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CScalarCoalesce
//
//	@doc:
//		Scalar coalesce operator
//
//---------------------------------------------------------------------------
class CScalarCoalesce : public CScalar
{
private:
	// return type
	gpos::Ref<IMDId> m_mdid_type;

	// is operator return type BOOL?
	BOOL m_fBoolReturnType;

public:
	CScalarCoalesce(const CScalarCoalesce &) = delete;

	// ctor
	CScalarCoalesce(CMemoryPool *mp, gpos::Ref<IMDId> mdid_type);

	// dtor
	~CScalarCoalesce() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopScalarCoalesce;
	}

	// operator name
	const CHAR *
	SzId() const override
	{
		return "CScalarCoalesce";
	}

	// return type
	gpos::Ref<IMDId>
	MdidType() const override
	{
		return m_mdid_type.get();
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
	PopCopyWithRemappedColumns(CMemoryPool *,		//mp,
							   UlongToColRefMap *,	//colref_mapping,
							   BOOL					//must_exist
							   ) override
	{
		return PopCopyDefault();
	}

	// boolean expression evaluation
	EBoolEvalResult
	Eber(gpos::Ref<ULongPtrArray> pdrgpulChildren) const override
	{
		// Coalesce returns the first not-null child,
		// if all children are Null, then Coalesce must return Null
		return EberNullOnAllNullChildren(pdrgpulChildren);
	}

	// conversion function
	static gpos::Ref<CScalarCoalesce>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopScalarCoalesce == pop->Eopid());

		return gpos::dyn_cast<CScalarCoalesce>(pop);
	}

};	// class CScalarCoalesce

}  // namespace gpopt

#endif	// !GPOPT_CScalarCoalesce_H

// EOF
