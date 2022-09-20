//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDifference.h
//
//	@doc:
//		Logical Difference operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalDifference_H
#define GPOPT_CLogicalDifference_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalSetOp.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalDifference
//
//	@doc:
//		Difference operators
//
//---------------------------------------------------------------------------
class CLogicalDifference : public CLogicalSetOp
{
private:
public:
	CLogicalDifference(const CLogicalDifference &) = delete;

	// ctor
	explicit CLogicalDifference(CMemoryPool *mp);

	CLogicalDifference(CMemoryPool *mp,
					   gpos::owner<CColRefArray *> pdrgpcrOutput,
					   gpos::owner<CColRef2dArray *> pdrgpdrgpcrInput);

	// dtor
	~CLogicalDifference() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalDifference;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalDifference";
	}

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return true;
	}

	// return a copy of the operator with remapped columns
	gpos::owner<COperator *> PopCopyWithRemappedColumns(
		CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
		BOOL must_exist) override;

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive max card
	CMaxCard DeriveMaxCard(CMemoryPool *mp,
						   CExpressionHandle &exprhdl) const override;

	// derive constraint property
	gpos::owner<CPropConstraint *>
	DerivePropertyConstraint(CMemoryPool *mp,
							 CExpressionHandle &exprhdl) const override
	{
		return PpcDeriveConstraintSetop(mp, exprhdl, false /*fIntersect*/);
	}

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::owner<CXformSet *> PxfsCandidates(CMemoryPool *mp) const override;

	//-------------------------------------------------------------------------------------
	// Derived Stats
	//-------------------------------------------------------------------------------------

	// stat promise
	EStatPromise
	Esp(CExpressionHandle &) const override
	{
		return CLogical::EspHigh;
	}

	// derive statistics
	gpos::owner<IStatistics *> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<IStatisticsArray *> stats_ctxt) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::cast_func<CLogicalDifference *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalDifference == pop->Eopid());

		return dynamic_cast<CLogicalDifference *>(pop);
	}

};	// class CLogicalDifference

}  // namespace gpopt


#endif	// !GPOPT_CLogicalDifference_H

// EOF
