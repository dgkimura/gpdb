//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CLogicalSelect.h
//
//	@doc:
//		Select operator
//---------------------------------------------------------------------------
#ifndef GPOS_CLogicalSelect_H
#define GPOS_CLogicalSelect_H

#include "gpos/base.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalUnary.h"

namespace gpopt
{
using ExprPredToExprPredPartMap =
	gpos::UnorderedMap<gpos::Ref<CExpression>, gpos::Ref<CExpression>>;

//---------------------------------------------------------------------------
//	@class:
//		CLogicalSelect
//
//	@doc:
//		Select operator
//
//---------------------------------------------------------------------------
class CLogicalSelect : public CLogicalUnary
{
private:
	gpos::Ref<ExprPredToExprPredPartMap> m_phmPexprPartPred;

	// table descriptor
	gpos::Ref<CTableDescriptor> m_ptabdesc;

public:
	CLogicalSelect(const CLogicalSelect &) = delete;

	// ctor
	explicit CLogicalSelect(CMemoryPool *mp);

	// ctor
	CLogicalSelect(CMemoryPool *mp, gpos::Ref<CTableDescriptor> ptabdesc);

	// dtor
	~CLogicalSelect() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalSelect;
	}

	const CHAR *
	SzId() const override
	{
		return "CLogicalSelect";
	}

	// return table's descriptor
	gpos::Ref<CTableDescriptor>
	Ptabdesc() const
	{
		return m_ptabdesc;
	}

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::Ref<CColRefSet> DeriveOutputColumns(CMemoryPool *,
											  CExpressionHandle &) override;

	// dervive keys
	gpos::Ref<CKeyCollection> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive max card
	CMaxCard DeriveMaxCard(CMemoryPool *mp,
						   CExpressionHandle &exprhdl) const override;

	// derive constraint property
	gpos::Ref<CPropConstraint>
	DerivePropertyConstraint(CMemoryPool *mp,
							 CExpressionHandle &exprhdl) const override
	{
		return PpcDeriveConstraintFromPredicates(mp, exprhdl);
	}

	// derive table descriptor
	gpos::Ref<CTableDescriptor>
	DeriveTableDescriptor(CMemoryPool *,  // mp
						  CExpressionHandle &exprhdl) const override
	{
		return exprhdl.DeriveTableDescriptor(0);
	}

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::Ref<CXformSet> PxfsCandidates(CMemoryPool *) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// return true if operator can select a subset of input tuples based on some predicate,
	BOOL
	FSelectionOp() const override
	{
		return true;
	}

	// conversion function
	static gpos::Ref<CLogicalSelect>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalSelect == pop->Eopid());

		return gpos::dyn_cast<CLogicalSelect>(pop);
	}

	// derive statistics
	gpos::Ref<IStatistics> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<IStatisticsArray> stats_ctxt) const override;

};	// class CLogicalSelect

}  // namespace gpopt

#endif	// !GPOS_CLogicalSelect_H

// EOF
