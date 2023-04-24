//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CLogicalInnerJoin.h
//
//	@doc:
//		Inner join operator
//---------------------------------------------------------------------------
#ifndef GPOS_CLogicalInnerJoin_H
#define GPOS_CLogicalInnerJoin_H

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CLogicalJoin.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalInnerJoin
//
//	@doc:
//		Inner join operator
//
//---------------------------------------------------------------------------
class CLogicalInnerJoin : public CLogicalJoin
{
private:
public:
	CLogicalInnerJoin(const CLogicalInnerJoin &) = delete;

	// ctor
	explicit CLogicalInnerJoin(
		CMemoryPool *mp, CXform::EXformId origin_xform = CXform::ExfSentinel);

	// dtor
	~CLogicalInnerJoin() override = default;


	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalInnerJoin;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalInnerJoin";
	}

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive not nullable columns
	gpos::Ref<CColRefSet>
	DeriveNotNullColumns(CMemoryPool *mp,
						 CExpressionHandle &exprhdl) const override
	{
		return PcrsDeriveNotNullCombineLogical(mp, exprhdl);
	}

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

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	CXformSet *PxfsCandidates(CMemoryPool *mp) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::Ref<CLogicalInnerJoin>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalInnerJoin == pop->Eopid());

		return dynamic_cast<gpos::Ref<CLogicalInnerJoin> >(pop);
	}

	// determine if an innerJoin group expression has
	// less conjuncts than another
	static BOOL FFewerConj(CMemoryPool *mp,
						   gpos::Ref<CGroupExpression> pgexprFst,
						   gpos::Ref<CGroupExpression> pgexprSnd);


};	// class CLogicalInnerJoin

}  // namespace gpopt


#endif	// !GPOS_CLogicalInnerJoin_H

// EOF
