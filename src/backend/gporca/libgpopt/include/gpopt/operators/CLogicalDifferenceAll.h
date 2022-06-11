//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDifferenceAll.h
//
//	@doc:
//		Logical Difference all operator (Difference all does not remove
//		duplicates from the left child)
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalDifferenceAll_H
#define GPOPT_CLogicalDifferenceAll_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalSetOp.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalDifferenceAll
//
//	@doc:
//		Difference all operators
//
//---------------------------------------------------------------------------
class CLogicalDifferenceAll : public CLogicalSetOp
{
private:
public:
	CLogicalDifferenceAll(const CLogicalDifferenceAll &) = delete;

	// ctor
	explicit CLogicalDifferenceAll(CMemoryPool *mp);

	CLogicalDifferenceAll(CMemoryPool *mp,
						  gpos::owner<CColRefArray *> pdrgpcrOutput,
						  gpos::owner<CColRef2dArray *> pdrgpdrgpcrInput);

	// dtor
	~CLogicalDifferenceAll() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalDifferenceAll;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalDifferenceAll";
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

	// derive key collections
	gpos::owner<CKeyCollection *> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

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

	// stat promise
	EStatPromise
	Esp(CExpressionHandle &	 // exprhdl
	) const override
	{
		return CLogical::EspLow;
	}

	// derive statistics
	gpos::owner<IStatistics *> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<IStatisticsArray *> stats_ctxt) const override;

	// conversion function
	static gpos::cast_func<CLogicalDifferenceAll *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalDifferenceAll == pop->Eopid());

		return dynamic_cast<CLogicalDifferenceAll *>(pop);
	}

};	// class CLogicalDifferenceAll

}  // namespace gpopt

#endif	// !GPOPT_CLogicalDifferenceAll_H

// EOF
