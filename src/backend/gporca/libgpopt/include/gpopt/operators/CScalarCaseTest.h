//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CScalarCaseTest.h
//
//	@doc:
//		Scalar case test operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CScalarCaseTest_H
#define GPOPT_CScalarCaseTest_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDrvdProp.h"
#include "gpopt/operators/CScalar.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CScalarCaseTest
//
//	@doc:
//		Scalar case test operator
//
//---------------------------------------------------------------------------
class CScalarCaseTest : public CScalar
{
private:
	// type id
	gpos::owner<IMDId *> m_mdid_type;

public:
	CScalarCaseTest(const CScalarCaseTest &) = delete;

	// ctor
	CScalarCaseTest(CMemoryPool *mp, gpos::owner<IMDId *> mdid_type);

	// dtor
	~CScalarCaseTest() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopScalarCaseTest;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CScalarCaseTest";
	}

	// the type of the scalar expression
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
	BOOL FInputOrderSensitive() const override;

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
	static gpos::cast_func<CScalarCaseTest *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopScalarCaseTest == pop->Eopid());

		return dynamic_cast<CScalarCaseTest *>(pop);
	}

};	// class CScalarCaseTest

}  // namespace gpopt


#endif	// !GPOPT_CScalarCaseTest_H

// EOF
