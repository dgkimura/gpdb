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
	IMDId *MdidType() const override;

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

	// derive partition consumer info
	gpos::owner<CPartInfo *> PpartinfoDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// conversion function
	static gpos::cast_func<CScalarSubqueryExistential *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopScalarSubqueryExists == pop->Eopid() ||
					EopScalarSubqueryNotExists == pop->Eopid());

		return dynamic_cast<CScalarSubqueryExistential *>(pop);
	}

};	// class CScalarSubqueryExistential
}  // namespace gpopt

#endif	// !GPOPT_CScalarSubqueryExistential_H

// EOF
