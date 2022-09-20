//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CLogicalProject.h
//
//	@doc:
//		Project operator
//---------------------------------------------------------------------------
#ifndef GPOS_CLogicalProject_H
#define GPOS_CLogicalProject_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalUnary.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalProject
//
//	@doc:
//		Project operator
//
//---------------------------------------------------------------------------
class CLogicalProject : public CLogicalUnary
{
private:
	// return equivalence class from scalar ident project element
	static gpos::Ref<CColRefSetArray> PdrgpcrsEquivClassFromScIdent(
		CMemoryPool *mp, CExpression *pexprPrEl, CColRefSet *not_null_columns);

	// extract constraint from scalar constant project element
	static void ExtractConstraintFromScConst(CMemoryPool *mp,
											 CExpression *pexprPrEl,
											 CConstraintArray *pdrgpcnstr,
											 CColRefSetArray *pdrgpcrs);

public:
	CLogicalProject(const CLogicalProject &) = delete;

	// ctor
	explicit CLogicalProject(CMemoryPool *mp);

	// dtor
	~CLogicalProject() override = default;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalProject;
	}

	const CHAR *
	SzId() const override
	{
		return "CLogicalProject";
	}

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::Ref<CColRefSet> DeriveOutputColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) override;

	// dervive keys
	gpos::Ref<CKeyCollection> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive max card
	CMaxCard DeriveMaxCard(CMemoryPool *mp,
						   CExpressionHandle &exprhdl) const override;

	// derive constraint property
	gpos::Ref<CPropConstraint> DerivePropertyConstraint(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::Ref<CXformSet> PxfsCandidates(CMemoryPool *mp) const override;

	// derive statistics
	gpos::Ref<IStatistics> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		IStatisticsArray *stats_ctxt) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static CLogicalProject *
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalProject == pop->Eopid());

		return dynamic_cast<CLogicalProject *>(pop);
	}

};	// class CLogicalProject

}  // namespace gpopt

#endif	// !GPOS_CLogicalProject_H

// EOF
