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
	gpos::owner<IMDId *> m_mdid_type;

	// is operator return type BOOL?
	BOOL m_fBoolReturnType;

public:
	CScalarCoalesce(const CScalarCoalesce &) = delete;

	// ctor
	CScalarCoalesce(CMemoryPool *mp, gpos::owner<IMDId *> mdid_type);

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
	gpos::pointer<IMDId *>
	MdidType() const override
	{
		return m_mdid_type;
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(gpos::pointer<COperator *> pop) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return true;
	}

	// return a copy of the operator with remapped columns
	gpos::owner<COperator *>
	PopCopyWithRemappedColumns(
		CMemoryPool *,						//mp,
		gpos::pointer<UlongToColRefMap *>,	//colref_mapping,
		BOOL								//must_exist
		) override
	{
		return PopCopyDefault();
	}

	// boolean expression evaluation
	EBoolEvalResult
	Eber(gpos::pointer<ULongPtrArray *> pdrgpulChildren) const override
	{
		// Coalesce returns the first not-null child,
		// if all children are Null, then Coalesce must return Null
		return EberNullOnAllNullChildren(pdrgpulChildren);
	}

	// conversion function
	static gpos::cast_func<CScalarCoalesce *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopScalarCoalesce == pop->Eopid());

		return dynamic_cast<CScalarCoalesce *>(pop);
	}

};	// class CScalarCoalesce

}  // namespace gpopt

#endif	// !GPOPT_CScalarCoalesce_H

// EOF
