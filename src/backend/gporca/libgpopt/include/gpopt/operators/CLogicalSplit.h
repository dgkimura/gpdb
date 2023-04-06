//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalSplit.h
//
//	@doc:
//		Logical split operator used for DML updates
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalSplit_H
#define GPOPT_CLogicalSplit_H

#include "gpos/base.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogical.h"

namespace gpopt
{
// fwd declarations
class CTableDescriptor;

//---------------------------------------------------------------------------
//	@class:
//		CLogicalSplit
//
//	@doc:
//		Logical split operator
//
//---------------------------------------------------------------------------
class CLogicalSplit : public CLogical
{
private:
	// deletion columns
	gpos::Ref<CColRefArray> m_pdrgpcrDelete;

	// insertion columns
	gpos::Ref<CColRefArray> m_pdrgpcrInsert;

	// ctid column
	gpos::Ref<CColRef> m_pcrCtid;

	// segmentId column
	gpos::Ref<CColRef> m_pcrSegmentId;

	// action column
	gpos::Ref<CColRef> m_pcrAction;

public:
	CLogicalSplit(const CLogicalSplit &) = delete;

	// ctor
	explicit CLogicalSplit(CMemoryPool *mp);

	// ctor
	CLogicalSplit(CMemoryPool *mp, gpos::Ref<CColRefArray> pdrgpcrDelete,
				  gpos::Ref<CColRefArray> pdrgpcrInsert,
				  gpos::Ref<CColRef> pcrCtid, gpos::Ref<CColRef> pcrSegmentId,
				  gpos::Ref<CColRef> pcrAction);

	// dtor
	~CLogicalSplit() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalSplit;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalSplit";
	}

	// deletion columns
	gpos::Ref<CColRefArray>
	PdrgpcrDelete() const
	{
		return m_pdrgpcrDelete;
	}

	// insertion columns
	gpos::Ref<CColRefArray>
	PdrgpcrInsert() const
	{
		return m_pdrgpcrInsert;
	}

	// ctid column
	gpos::Ref<CColRef>
	PcrCtid() const
	{
		return m_pcrCtid;
	}

	// segmentId column
	gpos::Ref<CColRef>
	PcrSegmentId() const
	{
		return m_pcrSegmentId;
	}

	// action column
	gpos::Ref<CColRef>
	PcrAction() const
	{
		return m_pcrAction;
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(gpos::Ref<COperator> pop) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return true;
	}

	// return a copy of the operator with remapped columns
	gpos::Ref<COperator> PopCopyWithRemappedColumns(
		CMemoryPool *mp, gpos::Ref<UlongToColRefMap> colref_mapping,
		BOOL must_exist) override;

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::Ref<CColRefSet> DeriveOutputColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) override;


	// derive constraint property
	gpos::Ref<CPropConstraint>
	DerivePropertyConstraint(CMemoryPool *,	 // mp
							 CExpressionHandle &exprhdl) const override
	{
		return CLogical::PpcDeriveConstraintPassThru(exprhdl, 0 /*ulChild*/);
	}

	// derive max card
	CMaxCard DeriveMaxCard(CMemoryPool *mp,
						   CExpressionHandle &exprhdl) const override;

	// derive partition consumer info
	gpos::Ref<CPartInfo>
	DerivePartitionInfo(CMemoryPool *,	// mp,
						CExpressionHandle &exprhdl) const override
	{
		return PpartinfoPassThruOuter(exprhdl);
	}

	// compute required stats columns of the n-th child
	gpos::Ref<CColRefSet>
	PcrsStat(CMemoryPool *mp, CExpressionHandle &exprhdl,
			 gpos::Ref<CColRefSet> pcrsInput, ULONG child_index) const override
	{
		return PcrsReqdChildStats(mp, exprhdl, pcrsInput,
								  exprhdl.DeriveUsedColumns(1), child_index);
	}

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::Ref<CXformSet> PxfsCandidates(CMemoryPool *mp) const override;

	// derive key collections
	gpos::Ref<CKeyCollection> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive statistics
	gpos::Ref<IStatistics> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<IStatisticsArray> stats_ctxt) const override;

	// stat promise
	EStatPromise
	Esp(CExpressionHandle &) const override
	{
		return CLogical::EspHigh;
	}

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::Ref<CLogicalSplit>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalSplit == pop->Eopid());

		return gpos::dyn_cast<CLogicalSplit>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CLogicalSplit
}  // namespace gpopt

#endif	// !GPOPT_CLogicalSplit_H

// EOF
