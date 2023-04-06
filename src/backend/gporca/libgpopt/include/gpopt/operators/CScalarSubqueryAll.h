//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CScalarSubqueryAll.h
//
//	@doc:
//		Class for scalar subquery ALL operators
//---------------------------------------------------------------------------
#ifndef GPOPT_CScalarSubqueryAll_H
#define GPOPT_CScalarSubqueryAll_H

#include "gpos/base.h"

#include "gpopt/operators/CScalarSubqueryQuantified.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CScalarSubqueryAll
//
//	@doc:
//		Scalar subquery ALL
//		A scalar subquery ALL expression has two children: relational and scalar.
//
//---------------------------------------------------------------------------
class CScalarSubqueryAll : public CScalarSubqueryQuantified
{
private:
public:
	CScalarSubqueryAll(const CScalarSubqueryAll &) = delete;

	// ctor
	CScalarSubqueryAll(CMemoryPool *mp, gpos::Ref<IMDId> scalar_op_mdid,
					   const CWStringConst *pstrScalarOp,
					   const gpos::Ref<CColRef> colref);

	// dtor
	~CScalarSubqueryAll() override = default;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopScalarSubqueryAll;
	}

	// return a string for scalar subquery
	const CHAR *
	SzId() const override
	{
		return "CScalarSubqueryAll";
	}

	// return a copy of the operator with remapped columns
	gpos::Ref<COperator> PopCopyWithRemappedColumns(
		CMemoryPool *mp, gpos::Ref<UlongToColRefMap> colref_mapping,
		BOOL must_exist) override;

	// conversion function
	static gpos::Ref<CScalarSubqueryAll>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopScalarSubqueryAll == pop->Eopid());

		return gpos::dyn_cast<CScalarSubqueryAll>(pop);
	}

};	// class CScalarSubqueryAll
}  // namespace gpopt

#endif	// !GPOPT_CScalarSubqueryAll_H

// EOF
