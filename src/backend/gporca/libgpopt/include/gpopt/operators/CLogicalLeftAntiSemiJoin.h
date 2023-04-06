//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CLogicalLeftAntiSemiJoin.h
//
//	@doc:
//		Left anti semi join operator
//---------------------------------------------------------------------------
#ifndef GPOS_CLogicalLeftAntiSemiJoin_H
#define GPOS_CLogicalLeftAntiSemiJoin_H

#include "gpos/base.h"

#include "gpopt/operators/CLogicalJoin.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalLeftAntiSemiJoin
//
//	@doc:
//		Left anti semi join operator
//
//---------------------------------------------------------------------------
class CLogicalLeftAntiSemiJoin : public CLogicalJoin
{
private:
public:
	CLogicalLeftAntiSemiJoin(const CLogicalLeftAntiSemiJoin &) = delete;

	// ctor
	explicit CLogicalLeftAntiSemiJoin(
		CMemoryPool *mp, CXform::EXformId origin_xform = CXform::ExfSentinel);

	// dtor
	~CLogicalLeftAntiSemiJoin() override = default;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalLeftAntiSemiJoin;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalLeftAntiSemiJoin";
	}

	// return true if we can pull projections up past this operator from its given child
	BOOL
	FCanPullProjectionsUp(ULONG child_index) const override
	{
		return (0 == child_index);
	}

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::Ref<CColRefSet> DeriveOutputColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) override;

	// derive not nullable output columns
	gpos::Ref<CColRefSet>
	DeriveNotNullColumns(CMemoryPool *,	 // mp
						 CExpressionHandle &exprhdl) const override
	{
		return PcrsDeriveNotNullPassThruOuter(exprhdl);
	}

	// dervive keys
	gpos::Ref<CKeyCollection> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive max card
	virtual CMaxCard MaxCard(CMemoryPool *mp, CExpressionHandle &exprhdl) const;

	// derive constraint property
	gpos::Ref<CPropConstraint>
	DerivePropertyConstraint(CMemoryPool *,	 //mp,
							 CExpressionHandle &exprhdl) const override
	{
		return PpcDeriveConstraintPassThru(exprhdl, 0 /*ulChild*/);
	}

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	CXformSet *PxfsCandidates(CMemoryPool *mp) const override;

	// derive statistics
	gpos::Ref<IStatistics> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		IStatisticsArray *stats_ctxt) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::Ref<CLogicalLeftAntiSemiJoin>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalLeftAntiSemiJoin == pop->Eopid());

		return dynamic_cast<gpos::Ref<CLogicalLeftAntiSemiJoin> >(pop);
	}

};	// class CLogicalLeftAntiSemiJoin

}  // namespace gpopt


#endif	// !GPOS_CLogicalLeftAntiSemiJoin_H

// EOF
