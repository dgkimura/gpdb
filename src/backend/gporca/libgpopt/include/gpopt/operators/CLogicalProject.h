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
	static gpos::owner<CColRefSetArray *> PdrgpcrsEquivClassFromScIdent(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprPrEl,
		gpos::pointer<CColRefSet *> not_null_columns);

	// extract constraint from scalar constant project element
	static void ExtractConstraintFromScConst(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexprPrEl,
		gpos::pointer<CConstraintArray *> pdrgpcnstr,
		gpos::pointer<CColRefSetArray *> pdrgpcrs);

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
	gpos::owner<CColRefSet *> DeriveOutputColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) override;

	// dervive keys
	gpos::owner<CKeyCollection *> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive max card
	CMaxCard DeriveMaxCard(CMemoryPool *mp,
						   CExpressionHandle &exprhdl) const override;

	// derive constraint property
	gpos::owner<CPropConstraint *> DerivePropertyConstraint(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::owner<CXformSet *> PxfsCandidates(CMemoryPool *mp) const override;

	// derive statistics
	gpos::owner<IStatistics *> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<IStatisticsArray *> stats_ctxt) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::cast_func<CLogicalProject *>
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
