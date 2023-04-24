//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CScalarNullTest.h
//
//	@doc:
//		Scalar null test
//---------------------------------------------------------------------------
#ifndef GPOPT_CScalarNullTest_H
#define GPOPT_CScalarNullTest_H

#include "gpos/base.h"

#include "gpopt/base/CDrvdProp.h"
#include "gpopt/operators/CScalar.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CScalarNullTest
//
//	@doc:
//		Scalar null test operator
//
//---------------------------------------------------------------------------
class CScalarNullTest : public CScalar
{
private:
public:
	CScalarNullTest(const CScalarNullTest &) = delete;

	// ctor
	explicit CScalarNullTest(CMemoryPool *mp) : CScalar(mp)
	{
	}

	// dtor
	~CScalarNullTest() override = default;


	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopScalarNullTest;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CScalarNullTest";
	}

	// match function
	BOOL Matches(gpos::Ref<COperator>) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return false;
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

	// the type of the scalar expression
	gpos::Ref<IMDId> MdidType() const override;

	// boolean expression evaluation
	EBoolEvalResult Eber(
		gpos::Ref<ULongPtrArray> pdrgpulChildren) const override;

	// conversion function
	static gpos::Ref<CScalarNullTest>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopScalarNullTest == pop->Eopid());

		return gpos::dyn_cast<CScalarNullTest>(pop);
	}

};	// class CScalarNullTest

}  // namespace gpopt


#endif	// !GPOPT_CScalarNullTest_H

// EOF
