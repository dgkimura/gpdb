//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalIntersect.h
//
//	@doc:
//		Logical Intersect operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalIntersect_H
#define GPOPT_CLogicalIntersect_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalSetOp.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalIntersect
//
//	@doc:
//		Intersect operators
//
//---------------------------------------------------------------------------
class CLogicalIntersect : public CLogicalSetOp
{
private:
public:
	CLogicalIntersect(const CLogicalIntersect &) = delete;

	// ctor
	explicit CLogicalIntersect(CMemoryPool *mp);

	CLogicalIntersect(CMemoryPool *mp,
					  gpos::owner<CColRefArray *> pdrgpcrOutput,
					  gpos::owner<CColRef2dArray *> pdrgpdrgpcrInput);

	// dtor
	~CLogicalIntersect() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalIntersect;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalIntersect";
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
		return PpcDeriveConstraintSetop(mp, exprhdl, true /*fIntersect*/);
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
	static gpos::cast_func<CLogicalIntersect *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalIntersect == pop->Eopid());

		return dynamic_cast<CLogicalIntersect *>(pop);
	}

};	// class CLogicalIntersect

}  // namespace gpopt


#endif	// !GPOPT_CLogicalIntersect_H

// EOF
