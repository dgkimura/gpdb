//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalUpdate.h
//
//	@doc:
//		Logical Update operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalUpdate_H
#define GPOPT_CLogicalUpdate_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CLogical.h"

namespace gpopt
{
// fwd declarations
class CTableDescriptor;

//---------------------------------------------------------------------------
//	@class:
//		CLogicalUpdate
//
//	@doc:
//		Logical Update operator
//
//---------------------------------------------------------------------------
class CLogicalUpdate : public CLogical
{
private:
	// table descriptor
	gpos::owner<CTableDescriptor *> m_ptabdesc;

	// columns to delete
	gpos::owner<CColRefArray *> m_pdrgpcrDelete;

	// columns to insert
	gpos::owner<CColRefArray *> m_pdrgpcrInsert;

	// ctid column
	CColRef *m_pcrCtid;

	// segmentId column
	CColRef *m_pcrSegmentId;

	// tuple oid column
	CColRef *m_pcrTupleOid;

public:
	CLogicalUpdate(const CLogicalUpdate &) = delete;

	// ctor
	explicit CLogicalUpdate(CMemoryPool *mp);

	// ctor
	CLogicalUpdate(CMemoryPool *mp, gpos::owner<CTableDescriptor *> ptabdesc,
				   gpos::owner<CColRefArray *> pdrgpcrDelete,
				   gpos::owner<CColRefArray *> pdrgpcrInsert, CColRef *pcrCtid,
				   CColRef *pcrSegmentId, CColRef *pcrTupleOid);

	// dtor
	~CLogicalUpdate() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalUpdate;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalUpdate";
	}

	// columns to delete
	gpos::pointer<CColRefArray *>
	PdrgpcrDelete() const
	{
		return m_pdrgpcrDelete;
	}

	// columns to insert
	gpos::pointer<CColRefArray *>
	PdrgpcrInsert() const
	{
		return m_pdrgpcrInsert;
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

	// tuple oid column
	CColRef *
	PcrTupleOid() const
	{
		return m_pcrTupleOid;
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
	static gpos::cast_func<CLogicalUpdate *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalUpdate == pop->Eopid());

		return dynamic_cast<CLogicalUpdate *>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CLogicalUpdate
}  // namespace gpopt

#endif	// !GPOPT_CLogicalUpdate_H

// EOF
