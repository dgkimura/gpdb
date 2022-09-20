//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDelete.h
//
//	@doc:
//		Logical Delete operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalDelete_H
#define GPOPT_CLogicalDelete_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CLogical.h"

namespace gpopt
{
// fwd declarations
class CTableDescriptor;

//---------------------------------------------------------------------------
//	@class:
//		CLogicalDelete
//
//	@doc:
//		Logical Delete operator
//
//---------------------------------------------------------------------------
class CLogicalDelete : public CLogical
{
private:
	// table descriptor
	gpos::owner<CTableDescriptor *> m_ptabdesc;

	// columns to delete
	gpos::owner<CColRefArray *> m_pdrgpcr;

	// ctid column
	CColRef *m_pcrCtid;

	// segmentId column
	CColRef *m_pcrSegmentId;

public:
	CLogicalDelete(const CLogicalDelete &) = delete;

	// ctor
	explicit CLogicalDelete(CMemoryPool *mp);

	// ctor
	CLogicalDelete(CMemoryPool *mp, gpos::owner<CTableDescriptor *> ptabdesc,
				   gpos::owner<CColRefArray *> colref_array, CColRef *pcrCtid,
				   CColRef *pcrSegmentId);

	// dtor
	~CLogicalDelete() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalDelete;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalDelete";
	}

	// columns to delete
	gpos::pointer<CColRefArray *>
	Pdrgpcr() const
	{
		return m_pdrgpcr;
	}

	// ctid column
	CColRef *
	PcrCtid() const
	{
		return m_pcrCtid;
	}

	// segmentId column
	CColRef *
	PcrSegmentId() const
	{
		return m_pcrSegmentId;
	}

	// return table's descriptor
	gpos::pointer<CTableDescriptor *>
	Ptabdesc() const
	{
		return m_ptabdesc;
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


	// derive constraint property
	gpos::owner<CPropConstraint *>
	DerivePropertyConstraint(CMemoryPool *,	 // mp
							 CExpressionHandle &exprhdl) const override
	{
		return CLogical::PpcDeriveConstraintPassThru(exprhdl, 0 /*ulChild*/);
	}

	// derive max card
	CMaxCard DeriveMaxCard(CMemoryPool *mp,
						   CExpressionHandle &exprhdl) const override;

	// derive partition consumer info
	gpos::owner<CPartInfo *>
	DerivePartitionInfo(CMemoryPool *,	// mp,
						CExpressionHandle &exprhdl) const override
	{
		return PpartinfoPassThruOuter(exprhdl);
	}

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

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::owner<CXformSet *> PxfsCandidates(CMemoryPool *mp) const override;

	// derive key collections
	gpos::owner<CKeyCollection *> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive statistics
	gpos::owner<IStatistics *> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<IStatisticsArray *> stats_ctxt) const override;

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
	static gpos::cast_func<CLogicalDelete *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalDelete == pop->Eopid());

		return dynamic_cast<CLogicalDelete *>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CLogicalDelete
}  // namespace gpopt

#endif	// !GPOPT_CLogicalDelete_H

// EOF
