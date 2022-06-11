//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalRowTrigger.h
//
//	@doc:
//		Logical row-level trigger operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalRowTrigger_H
#define GPOPT_CLogicalRowTrigger_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CLogical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalRowTrigger
//
//	@doc:
//		Logical row-level trigger operator
//
//---------------------------------------------------------------------------
class CLogicalRowTrigger : public CLogical
{
private:
	// relation id on which triggers are to be executed
	gpos::owner<IMDId *> m_rel_mdid;

	// trigger type
	INT m_type;

	// old columns
	gpos::owner<CColRefArray *> m_pdrgpcrOld;

	// new columns
	gpos::owner<CColRefArray *> m_pdrgpcrNew;

	// stability
	IMDFunction::EFuncStbl m_efs{IMDFunction::EfsImmutable};

	// initialize function properties
	void InitFunctionProperties();

	// return the type of a given trigger as an integer
	static INT ITriggerType(gpos::pointer<const IMDTrigger *> pmdtrigger);

public:
	CLogicalRowTrigger(const CLogicalRowTrigger &) = delete;

	// ctor
	explicit CLogicalRowTrigger(CMemoryPool *mp);

	// ctor
	CLogicalRowTrigger(CMemoryPool *mp, gpos::owner<IMDId *> rel_mdid, INT type,
					   gpos::owner<CColRefArray *> pdrgpcrOld,
					   gpos::owner<CColRefArray *> pdrgpcrNew);

	// dtor
	~CLogicalRowTrigger() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalRowTrigger;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalRowTrigger";
	}

	// relation id
	gpos::pointer<IMDId *>
	GetRelMdId() const
	{
		return m_rel_mdid;
	}

	// trigger type
	INT
	GetType() const
	{
		return m_type;
	}

	// old columns
	gpos::pointer<CColRefArray *>
	PdrgpcrOld() const
	{
		return m_pdrgpcrOld;
	}

	// new columns
	gpos::pointer<CColRefArray *>
	PdrgpcrNew() const
	{
		return m_pdrgpcrNew;
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

	// derive function properties
	gpos::owner<CFunctionProp *> DeriveFunctionProperties(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

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
	static gpos::cast_func<CLogicalRowTrigger *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalRowTrigger == pop->Eopid());

		return dynamic_cast<CLogicalRowTrigger *>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CLogicalRowTrigger
}  // namespace gpopt

#endif	// !GPOPT_CLogicalRowTrigger_H

// EOF
