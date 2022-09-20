//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLogicalLeftSemiApplyIn.h
//
//	@doc:
//		Logical Left Semi Apply operator used in IN/ANY subqueries
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalLeftSemiApplyIn_H
#define GPOPT_CLogicalLeftSemiApplyIn_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalLeftSemiApplyIn
//
//	@doc:
//		Logical Apply operator used in IN/ANY subqueries
//
//---------------------------------------------------------------------------
class CLogicalLeftSemiApplyIn : public CLogicalLeftSemiApply
{
private:
public:
	CLogicalLeftSemiApplyIn(const CLogicalLeftSemiApplyIn &) = delete;

	// ctor
	explicit CLogicalLeftSemiApplyIn(CMemoryPool *mp)
		: CLogicalLeftSemiApply(mp)
	{
	}

	// ctor
	CLogicalLeftSemiApplyIn(CMemoryPool *mp,
							gpos::owner<CColRefArray *> pdrgpcrInner,
							EOperatorId eopidOriginSubq)
		: CLogicalLeftSemiApply(mp, std::move(pdrgpcrInner), eopidOriginSubq)
	{
	}

	// dtor
	~CLogicalLeftSemiApplyIn() override = default;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalLeftSemiApplyIn;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalLeftSemiApplyIn";
	}

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::owner<CXformSet *> PxfsCandidates(CMemoryPool *mp) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// return a copy of the operator with remapped columns
	gpos::owner<COperator *> PopCopyWithRemappedColumns(
		CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
		BOOL must_exist) override;

	// conversion function
	static gpos::cast_func<CLogicalLeftSemiApplyIn *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalLeftSemiApplyIn == pop->Eopid());

		return dynamic_cast<CLogicalLeftSemiApplyIn *>(pop);
	}

};	// class CLogicalLeftSemiApplyIn

}  // namespace gpopt


#endif	// !GPOPT_CLogicalLeftSemiApplyIn_H

// EOF
