//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalMotionRoutedDistribute.h
//
//	@doc:
//		Physical routed distribute motion operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalMotionRoutedDistribute_H
#define GPOPT_CPhysicalMotionRoutedDistribute_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpecRouted.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/operators/CPhysicalMotion.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalMotionRoutedDistribute
//
//	@doc:
//		Routed distribute motion operator
//
//---------------------------------------------------------------------------
class CPhysicalMotionRoutedDistribute : public CPhysicalMotion
{
private:
	// routed distribution spec
	gpos::owner<CDistributionSpecRouted *> m_pdsRouted;

	// required columns in distribution spec
	gpos::owner<CColRefSet *> m_pcrsRequiredLocal;

public:
	CPhysicalMotionRoutedDistribute(const CPhysicalMotionRoutedDistribute &) =
		delete;

	// ctor
	CPhysicalMotionRoutedDistribute(
		CMemoryPool *mp, gpos::owner<CDistributionSpecRouted *> pdsRouted);

	// dtor
	~CPhysicalMotionRoutedDistribute() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalMotionRoutedDistribute;
	}

	const CHAR *
	SzId() const override
	{
		return "CPhysicalMotionRoutedDistribute";
	}

	// output distribution accessor
	gpos::pointer<CDistributionSpec *>
	Pds() const override
	{
		return m_pdsRouted;
	}

	// match function
	BOOL Matches(gpos::pointer<COperator *>) const override;

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
	static gpos::cast_func<CPhysicalMotionRoutedDistribute *> PopConvert(
		COperator *pop);

};	// class CPhysicalMotionRoutedDistribute

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalMotionRoutedDistribute_H

// EOF
