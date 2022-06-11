//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CPhysicalMotionBroadcast.h
//
//	@doc:
//		Physical Broadcast motion operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalMotionBroadcast_H
#define GPOPT_CPhysicalMotionBroadcast_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpecReplicated.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/operators/CPhysicalMotion.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalMotionBroadcast
//
//	@doc:
//		Broadcast motion operator
//
//---------------------------------------------------------------------------
class CPhysicalMotionBroadcast : public CPhysicalMotion
{
private:
	// output distribution
	gpos::owner<CDistributionSpecReplicated *> m_pdsReplicated;

public:
	CPhysicalMotionBroadcast(const CPhysicalMotionBroadcast &) = delete;

	// ctor
	explicit CPhysicalMotionBroadcast(CMemoryPool *mp);

	// dtor
	~CPhysicalMotionBroadcast() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalMotionBroadcast;
	}

	const CHAR *
	SzId() const override
	{
		return "CPhysicalMotionBroadcast";
	}

	// output distribution accessor
	gpos::pointer<CDistributionSpec *>
	Pds() const override
	{
		return m_pdsReplicated;
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
	static gpos::cast_func<CPhysicalMotionBroadcast *> PopConvert(
		COperator *pop);

};	// class CPhysicalMotionBroadcast

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalMotionBroadcast_H

// EOF
