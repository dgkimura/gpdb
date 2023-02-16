//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CScalarSubqueryExistential.h
//
//	@doc:
//		Parent class for existential subquery operators
//---------------------------------------------------------------------------
#ifndef GPOPT_CScalarSubqueryExistential_H
#define GPOPT_CScalarSubqueryExistential_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CScalar.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CScalarSubqueryExistential
//
//	@doc:
//		Parent class for EXISTS/NOT EXISTS subquery operators
//
//---------------------------------------------------------------------------
class CScalarSubqueryExistential : public CScalar
{
private:
public:
	CScalarSubqueryExistential(const CScalarSubqueryExistential &) = delete;

	// ctor
	CScalarSubqueryExistential(CMemoryPool *mp);

	// dtor
	~CScalarSubqueryExistential() override;

	// return the type of the scalar expression
	gpos::Ref<IMDId> MdidType() const override;

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

	// derive partition consumer info
	gpos::Ref<CPartInfo> PpartinfoDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// conversion function
	static gpos::Ref<CScalarSubqueryExistential>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopScalarSubqueryExists == pop->Eopid() ||
					EopScalarSubqueryNotExists == pop->Eopid());

		return gpos::dyn_cast<CScalarSubqueryExistential>(pop);
	}

};	// class CScalarSubqueryExistential
}  // namespace gpopt

#endif	// !GPOPT_CScalarSubqueryExistential_H

// EOF
