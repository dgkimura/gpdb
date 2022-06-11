//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalMotionRandom.h
//
//	@doc:
//		Physical Random motion operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalMotionRandom_H
#define GPOPT_CPhysicalMotionRandom_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpecRandom.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/operators/CPhysicalMotion.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalMotionRandom
//
//	@doc:
//		Random motion operator
//
//---------------------------------------------------------------------------
class CPhysicalMotionRandom : public CPhysicalMotion
{
private:
	// distribution spec
	gpos::owner<CDistributionSpecRandom *> m_pdsRandom;

public:
	CPhysicalMotionRandom(const CPhysicalMotionRandom &) = delete;

	// ctor
	CPhysicalMotionRandom(CMemoryPool *mp,
						  gpos::owner<CDistributionSpecRandom *> pdsRandom);

	// dtor
	~CPhysicalMotionRandom() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalMotionRandom;
	}

	const CHAR *
	SzId() const override
	{
		return "CPhysicalMotionRandom";
	}

	// output distribution accessor
	gpos::pointer<CDistributionSpec *>
	Pds() const override
	{
		return m_pdsRandom;
	}

	// is distribution duplicate sensitive
	BOOL
	IsDuplicateSensitive() const
	{
		return m_pdsRandom->IsDuplicateSensitive();
	}

	// match function
	BOOL Matches(gpos::pointer<COperator *> pop) const override;

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required output columns of the n-th child
	gpos::owner<CColRefSet *> PcrsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CColRefSet *> pcrsInput, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt, ULONG ulOptReq) override;

	// compute required sort order of the n-th child
	gpos::owner<COrderSpec *> PosRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<COrderSpec *> posInput, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// check if required columns are included in output columns
	BOOL FProvidesReqdCols(CExpressionHandle &exprhdl,
						   gpos::pointer<CColRefSet *> pcrsRequired,
						   ULONG ulOptReq) const override;

	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive sort order
	gpos::owner<COrderSpec *> PosDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl,
		gpos::pointer<const CEnfdOrder *> peo) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// print
	IOstream &OsPrint(IOstream &) const override;

	// conversion function
	static gpos::cast_func<CPhysicalMotionRandom *> PopConvert(COperator *pop);

};	// class CPhysicalMotionRandom

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalMotionRandom_H

// EOF
