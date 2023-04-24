//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDML.h
//
//	@doc:
//		Logical DML operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalDML_H
#define GPOPT_CLogicalDML_H

#include "gpos/base.h"

#include "gpopt/operators/CLogical.h"

namespace gpopt
{
// fwd declarations
class CTableDescriptor;

//---------------------------------------------------------------------------
//	@class:
//		CLogicalDML
//
//	@doc:
//		Logical DML operator
//
//---------------------------------------------------------------------------
class CLogicalDML : public CLogical
{
public:
	// enum of DML operators
	enum EDMLOperator
	{
		EdmlInsert,
		EdmlDelete,
		EdmlUpdate,
		EdmlSentinel
	};


private:
	// dml operator
	EDMLOperator m_edmlop;

	// table descriptor
	gpos::Ref<CTableDescriptor> m_ptabdesc;

	// source columns
	CColRefArray *m_pdrgpcrSource;

	// set of modified columns from the target table
	gpos::Ref<CBitSet> m_pbsModified;

	// action column
	gpos::Ref<CColRef> m_pcrAction;

	// ctid column
	gpos::Ref<CColRef> m_pcrCtid;

	// segmentId column
	gpos::Ref<CColRef> m_pcrSegmentId;

	// Split Update
	BOOL m_fSplit;

public:
	CLogicalDML(const CLogicalDML &) = delete;

	// ctor
	explicit CLogicalDML(CMemoryPool *mp);

	// ctor
	CLogicalDML(CMemoryPool *mp, EDMLOperator edmlop,
				gpos::Ref<CTableDescriptor> ptabdesc,
				CColRefArray *colref_array, gpos::Ref<CBitSet> pbsModified,
				gpos::Ref<CColRef> pcrAction, gpos::Ref<CColRef> pcrCtid,
				gpos::Ref<CColRef> pcrSegmentId, BOOL fSplit);

	// dtor
	~CLogicalDML() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalDML;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalDML";
	}

	// dml operator
	EDMLOperator
	Edmlop() const
	{
		return m_edmlop;
	}

	// source columns
	CColRefArray *
	PdrgpcrSource() const
	{
		return m_pdrgpcrSource;
	}

	// modified columns set
	gpos::Ref<CBitSet>
	PbsModified() const
	{
		return m_pbsModified;
	}

	// action column
	gpos::Ref<CColRef>
	PcrAction() const
	{
		return m_pcrAction;
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

	// return table's descriptor
	gpos::Ref<CTableDescriptor>
	Ptabdesc() const
	{
		return m_ptabdesc;
	}

	// Is update using split
	BOOL
	FSplit() const
	{
		return m_fSplit;
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(gpos::Ref<COperator> pop) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return false;
	}

	// return a copy of the operator with remapped columns
	gpos::Ref<COperator> PopCopyWithRemappedColumns(
		CMemoryPool *mp, UlongToColRefMap *colref_mapping,
		BOOL must_exist) override;

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::Ref<CColRefSet> DeriveOutputColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) override;

	// derive constraint property
	gpos::Ref<CPropConstraint> DerivePropertyConstraint(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

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
	PcrsStat(CMemoryPool *,		   // mp
			 CExpressionHandle &,  // exprhdl
			 gpos::Ref<CColRefSet> pcrsInput,
			 ULONG	// child_index
	) const override
	{
		return PcrsStatsPassThru(pcrsInput);
	}

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	CXformSet *PxfsCandidates(CMemoryPool *mp) const override;

	// derive key collections
	gpos::Ref<CKeyCollection> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive statistics
	gpos::Ref<IStatistics> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		IStatisticsArray *stats_ctxt) const override;

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
	static gpos::Ref<CLogicalDML>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalDML == pop->Eopid());

		return dynamic_cast<gpos::Ref<CLogicalDML> >(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

	// Helper function to print DML operator type.
	static void PrintOperatorType(IOstream &os, EDMLOperator, BOOL fSplit);

};	// class CLogicalDML
}  // namespace gpopt

#endif	// !GPOPT_CLogicalDML_H

// EOF
