//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalCTEAnchor.h
//
//	@doc:
//		Logical CTE anchor operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalCTEAnchor_H
#define GPOPT_CLogicalCTEAnchor_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalCTEAnchor
//
//	@doc:
//		CTE anchor operator
//
//---------------------------------------------------------------------------
class CLogicalCTEAnchor : public CLogical
{
private:
	// cte identifier
	ULONG m_id;

public:
	CLogicalCTEAnchor(const CLogicalCTEAnchor &) = delete;

	// ctor
	explicit CLogicalCTEAnchor(CMemoryPool *mp);

	// ctor
	CLogicalCTEAnchor(CMemoryPool *mp, ULONG id);

	// dtor
	~CLogicalCTEAnchor() override = default;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalCTEAnchor;
	}

	const CHAR *
	SzId() const override
	{
		return "CLogicalCTEAnchor";
	}

	// cte identifier
	ULONG
	Id() const
	{
		return m_id;
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(gpos::pointer<COperator *> pop) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return false;
	}

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

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::owner<CColRefSet *> DeriveOutputColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) override;

	// dervive keys
	gpos::owner<CKeyCollection *> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive max card
	CMaxCard DeriveMaxCard(CMemoryPool *mp,
						   CExpressionHandle &exprhdl) const override;

	// derive constraint property
	gpos::owner<CPropConstraint *>
	DerivePropertyConstraint(CMemoryPool *,	 //mp,
							 CExpressionHandle &exprhdl) const override
	{
		return PpcDeriveConstraintPassThru(exprhdl, 0 /*ulChild*/);
	}

	// derive partition consumer info
	gpos::owner<CPartInfo *> DerivePartitionInfo(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// compute required stats columns of the n-th child
	gpos::owner<CColRefSet *>
	PcrsStat(CMemoryPool *,		   // mp
			 CExpressionHandle &,  // exprhdl
			 gpos::pointer<CColRefSet *> pcrsInput,
			 ULONG	// child_index
	) const override
	{
		return PcrsStatsPassThru(pcrsInput);
	}

	// derive statistics
	gpos::owner<IStatistics *>
	PstatsDerive(CMemoryPool *,	 //mp,
				 CExpressionHandle &exprhdl,
				 gpos::pointer<IStatisticsArray *>	//stats_ctxt
	) const override
	{
		return PstatsPassThruOuter(exprhdl);
	}

	// stat promise
	EStatPromise
	Esp(CExpressionHandle &) const override
	{
		return CLogical::EspHigh;
	}

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::owner<CXformSet *> PxfsCandidates(CMemoryPool *mp) const override;

	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::cast_func<CLogicalCTEAnchor *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalCTEAnchor == pop->Eopid());

		return dynamic_cast<CLogicalCTEAnchor *>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CLogicalCTEAnchor

}  // namespace gpopt

#endif	// !GPOPT_CLogicalCTEAnchor_H

// EOF
