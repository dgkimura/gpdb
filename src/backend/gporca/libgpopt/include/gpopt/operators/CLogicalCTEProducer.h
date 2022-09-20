//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalCTEProducer.h
//
//	@doc:
//		Logical CTE producer operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalCTEProducer_H
#define GPOPT_CLogicalCTEProducer_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalCTEProducer
//
//	@doc:
//		CTE producer operator
//
//---------------------------------------------------------------------------
class CLogicalCTEProducer : public CLogical
{
private:
	// cte identifier
	ULONG m_id;

	// cte columns
	gpos::owner<CColRefArray *> m_pdrgpcr;

	// output columns, same as cte columns but in CColRefSet
	gpos::owner<CColRefSet *> m_pcrsOutput;

public:
	CLogicalCTEProducer(const CLogicalCTEProducer &) = delete;

	// ctor
	explicit CLogicalCTEProducer(CMemoryPool *mp);

	// ctor
	CLogicalCTEProducer(CMemoryPool *mp, ULONG id,
						gpos::owner<CColRefArray *> colref_array);

	// dtor
	~CLogicalCTEProducer() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalCTEProducer;
	}

	const CHAR *
	SzId() const override
	{
		return "CLogicalCTEProducer";
	}

	// cte identifier
	ULONG
	UlCTEId() const
	{
		return m_id;
	}

	// cte columns
	gpos::pointer<CColRefArray *>
	Pdrgpcr() const
	{
		return m_pdrgpcr;
	}

	// cte columns in CColRefSet
	gpos::pointer<CColRefSet *>
	DeriveOutputColumns() const
	{
		return m_pcrsOutput;
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
	gpos::owner<COperator *> PopCopyWithRemappedColumns(
		CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
		BOOL must_exist) override;

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

	// derive not nullable output columns
	gpos::owner<CColRefSet *> DeriveNotNullColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive constraint property
	gpos::owner<CPropConstraint *>
	DerivePropertyConstraint(CMemoryPool *mp,
							 CExpressionHandle &exprhdl) const override
	{
		return PpcDeriveConstraintRestrict(mp, exprhdl, m_pcrsOutput);
	}

	// derive partition consumer info
	gpos::owner<CPartInfo *>
	DerivePartitionInfo(CMemoryPool *,	// mp,
						CExpressionHandle &exprhdl) const override
	{
		return PpartinfoPassThruOuter(exprhdl);
	}

	gpos::pointer<CTableDescriptor *> DeriveTableDescriptor(
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
	static gpos::cast_func<CLogicalCTEProducer *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalCTEProducer == pop->Eopid());

		return dynamic_cast<CLogicalCTEProducer *>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CLogicalCTEProducer

}  // namespace gpopt

#endif	// !GPOPT_CLogicalCTEProducer_H

// EOF
