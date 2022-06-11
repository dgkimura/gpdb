//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CPhysicalMotion.h
//
//	@doc:
//		Base class for Motion operators
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalMotion_H
#define GPOPT_CPhysicalMotion_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CPhysical.h"


namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalMotion
//
//	@doc:
//		Base class for Motion operators
//
//---------------------------------------------------------------------------
class CPhysicalMotion : public CPhysical
{
private:
protected:
	// ctor
	explicit CPhysicalMotion(CMemoryPool *mp) : CPhysical(mp)
	{
	}

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return true;
	}

public:
	CPhysicalMotion(const CPhysicalMotion &) = delete;

	// output distribution accessor
	virtual gpos::pointer<CDistributionSpec *> Pds() const = 0;

	// check if optimization contexts is valid
	BOOL FValidContext(
		CMemoryPool *mp, gpos::pointer<COptimizationContext *> poc,
		gpos::pointer<COptimizationContextArray *> pdrgpocChild) const override;

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required ctes of the n-th child
	gpos::owner<CCTEReq *> PcteRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CCTEReq *> pcter, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// compute required distribution of the n-th child
	gpos::owner<CDistributionSpec *> PdsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CDistributionSpec *> pdsRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// compute required rewindability of the n-th child
	gpos::owner<CRewindabilitySpec *> PrsRequired(
		CMemoryPool *mp,
		CExpressionHandle &,				  // exprhdl
		gpos::pointer<CRewindabilitySpec *>,  // prsRequired
		ULONG,								  // child_index
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// compute required partition propoagation spec of the n-th child
	gpos::owner<CPartitionPropagationSpec *> PppsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CPartitionPropagationSpec *> pppsRequired,
		ULONG child_index, gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive distribution
	gpos::owner<CDistributionSpec *> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive rewindability
	gpos::owner<CRewindabilitySpec *> PrsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derived properties: derive partition propagation spec
	gpos::owner<CPartitionPropagationSpec *> PppsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return distribution property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetDistribution(
		CExpressionHandle &exprhdl,
		gpos::pointer<const CEnfdDistribution *> ped) const override;

	// return rewindability property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetRewindability(
		CExpressionHandle &,					   // exprhdl
		gpos::pointer<const CEnfdRewindability *>  // per
	) const override;

	// return true if operator passes through stats obtained from children,
	// this is used when computing stats during costing
	BOOL
	FPassThruStats() const override
	{
		return true;
	}

	// conversion function
	static gpos::cast_func<CPhysicalMotion *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(CUtils::FPhysicalMotion(pop));

		return dynamic_cast<CPhysicalMotion *>(pop);
	}

};	// class CPhysicalMotion

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalMotion_H

// EOF
