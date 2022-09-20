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
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalUnary.h"

namespace gpopt
{
using ExprPredToExprPredPartMap =
	CHashMap<CExpression, CExpression, CExpression::HashValue, CUtils::Equals,
			 CleanupRelease<CExpression>, CleanupRelease<CExpression>>;

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
	gpos::owner<ExprPredToExprPredPartMap *> m_phmPexprPartPred;

	// table descriptor
	gpos::pointer<CTableDescriptor *> m_ptabdesc;

public:
	CLogicalSelect(const CLogicalSelect &) = delete;

	// ctor
	explicit CLogicalSelect(CMemoryPool *mp);

	// ctor
	CLogicalSelect(CMemoryPool *mp, gpos::pointer<CTableDescriptor *> ptabdesc);

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
	gpos::pointer<CTableDescriptor *>
	Ptabdesc() const
	{
		return m_ptabdesc;
	}

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::owner<CColRefSet *> DeriveOutputColumns(CMemoryPool *,
												  CExpressionHandle &) override;

	// dervive keys
	gpos::owner<CKeyCollection *> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive max card
	CMaxCard DeriveMaxCard(CMemoryPool *mp,
						   CExpressionHandle &exprhdl) const override;

	// derive constraint property
	gpos::owner<CPropConstraint *>
	DerivePropertyConstraint(CMemoryPool *mp,
							 CExpressionHandle &exprhdl) const override
	{
		return PpcDeriveConstraintFromPredicates(mp, exprhdl);
	}

	// derive table descriptor
	gpos::pointer<CTableDescriptor *>
	DeriveTableDescriptor(CMemoryPool *,  // mp
						  CExpressionHandle &exprhdl) const override
	{
		return exprhdl.DeriveTableDescriptor(0);
	}

	// compute partition predicate to pass down to n-th child
	gpos::owner<CExpression *> PexprPartPred(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CExpression *> pexprInput,
		ULONG child_index) const override;

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::owner<CXformSet *> PxfsCandidates(CMemoryPool *) const override;

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
	static gpos::cast_func<CLogicalSelect *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalSelect == pop->Eopid());

		return dynamic_cast<CLogicalSelect *>(pop);
	}

	// derive statistics
	gpos::owner<IStatistics *> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<IStatisticsArray *> stats_ctxt) const override;

};	// class CLogicalSelect

}  // namespace gpopt

#endif	// !GPOS_CLogicalSelect_H

// EOF
