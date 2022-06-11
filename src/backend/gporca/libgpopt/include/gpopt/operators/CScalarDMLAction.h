//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CScalarDMLAction.h
//
//	@doc:
//		Scalar DML action operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CScalarDMLAction_H
#define GPOPT_CScalarDMLAction_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDrvdProp.h"
#include "gpopt/operators/CScalar.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CScalarDMLAction
//
//	@doc:
//		Scalar casting operator
//
//---------------------------------------------------------------------------
class CScalarDMLAction : public CScalar
{
private:
public:
	CScalarDMLAction(const CScalarDMLAction &) = delete;

	// dml action specification
	enum EDMLAction
	{
		EdmlactionDelete,
		EdmlactionInsert
	};

	// ctor
	CScalarDMLAction(CMemoryPool *mp) : CScalar(mp)
	{
	}

	// dtor
	~CScalarDMLAction() override = default;
	// ident accessors

	// the type of the scalar expression
	gpos::pointer<IMDId *> MdidType() const override;

	EOperatorId
	Eopid() const override
	{
		return EopScalarDMLAction;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CScalarDMLAction";
	}

	// match function
	BOOL Matches(gpos::pointer<COperator *> pop) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return false;
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

	// conversion function
	static gpos::cast_func<CScalarDMLAction *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopScalarDMLAction == pop->Eopid());

		return dynamic_cast<CScalarDMLAction *>(pop);
	}

};	// class CScalarDMLAction
}  // namespace gpopt

#endif	// !GPOPT_CScalarDMLAction_H

// EOF
