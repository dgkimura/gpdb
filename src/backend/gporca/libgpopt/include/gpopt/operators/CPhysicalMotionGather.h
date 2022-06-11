//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CPhysicalMotionGather.h
//
//	@doc:
//		Physical Gather motion operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalMotionGather_H
#define GPOPT_CPhysicalMotionGather_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpecSingleton.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/operators/CPhysicalMotion.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalMotionGather
//
//	@doc:
//		Gather motion operator
//
//---------------------------------------------------------------------------
class CPhysicalMotionGather : public CPhysicalMotion
{
private:
	// type of segment on which this gather runs (master/segment)
	gpos::owner<CDistributionSpecSingleton *> m_pdssSingeton;

	// merge spec if the operator is order-preserving
	gpos::owner<COrderSpec *> m_pos;

	// columns used by order spec
	gpos::owner<CColRefSet *> m_pcrsSort;

public:
	CPhysicalMotionGather(const CPhysicalMotionGather &) = delete;

	// ctor
	CPhysicalMotionGather(CMemoryPool *mp,
						  CDistributionSpecSingleton::ESegmentType est);

	CPhysicalMotionGather(CMemoryPool *mp,
						  CDistributionSpecSingleton::ESegmentType est,
						  COrderSpec *pos);

	// dtor
	~CPhysicalMotionGather() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalMotionGather;
	}

	const CHAR *
	SzId() const override
	{
		return "CPhysicalMotionGather";
	}

	CDistributionSpecSingleton::ESegmentType
	Est() const
	{
		return m_pdssSingeton->Est();
	}

	// output distribution accessor
	gpos::pointer<CDistributionSpec *>
	Pds() const override
	{
		return m_pdssSingeton;
	}

	BOOL
	FOrderPreserving() const
	{
		return !m_pos->IsEmpty();
	}

	BOOL
	FOnMaster() const
	{
		return CDistributionSpecSingleton::EstMaster == Est();
	}

	// order spec
	gpos::pointer<COrderSpec *>
	Pos() const
	{
		return m_pos;
	}

	// match function
	BOOL Matches(COperator *) const override;

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required output columns of the n-th child
	CColRefSet *PcrsRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
							 CColRefSet *pcrsInput, ULONG child_index,
							 CDrvdPropArray *pdrgpdpCtxt,
							 ULONG ulOptReq) override;

	// compute required sort order of the n-th child
	gpos::owner<COrderSpec *> PosRequired(CMemoryPool *mp,
										  CExpressionHandle &exprhdl,
										  gpos::pointer<COrderSpec *> posInput,
										  ULONG child_index,
										  CDrvdPropArray *pdrgpdpCtxt,
										  ULONG ulOptReq) const override;

	// check if required columns are included in output columns
	BOOL FProvidesReqdCols(CExpressionHandle &exprhdl, CColRefSet *pcrsRequired,
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
	static gpos::cast_func<CPhysicalMotionGather *> PopConvert(COperator *pop);

};	// class CPhysicalMotionGather

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalMotionGather_H

// EOF
