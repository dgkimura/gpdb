//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 - 2011 EMC CORP.
//
//	@filename:
//		CReqdPropPlan.h
//
//	@doc:
//		Derived required relational properties
//---------------------------------------------------------------------------
#ifndef GPOPT_CReqdPropPlan_H
#define GPOPT_CReqdPropPlan_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CCTEReq.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CEnfdDistribution.h"
#include "gpopt/base/CEnfdOrder.h"
#include "gpopt/base/CEnfdRewindability.h"
#include "gpopt/base/CReqdProp.h"

namespace gpopt
{
using namespace gpos;

// forward declaration
class CDrvdPropRelational;
class CDrvdPropPlan;
class CEnfdPartitionPropagation;
class CExpressionHandle;
class CPartInfo;
class CPhysical;
class CPropSpec;

//---------------------------------------------------------------------------
//	@class:
//		CReqdPropPlan
//
//	@doc:
//		Required plan properties container.
//
//---------------------------------------------------------------------------
class CReqdPropPlan : public CReqdProp
{
private:
	// required columns
	gpos::owner<CColRefSet *> m_pcrs{nullptr};

	// required sort order
	gpos::owner<CEnfdOrder *> m_peo{nullptr};

	// required distribution
	gpos::owner<CEnfdDistribution *> m_ped{nullptr};

	// required rewindability
	gpos::owner<CEnfdRewindability *> m_per{nullptr};

	// required partition propagation
	gpos::owner<CEnfdPartitionPropagation *> m_pepp{nullptr};

	// required ctes
	gpos::owner<CCTEReq *> m_pcter{nullptr};

public:
	CReqdPropPlan(const CReqdPropPlan &) = delete;

	// default ctor
	CReqdPropPlan() = default;

	// ctor
	CReqdPropPlan(gpos::owner<CColRefSet *> pcrs, gpos::owner<CEnfdOrder *> peo,
				  gpos::owner<CEnfdDistribution *> ped,
				  gpos::owner<CEnfdRewindability *> per,
				  gpos::owner<CEnfdPartitionPropagation *> pepp,
				  gpos::owner<CCTEReq *> pcter);

	// dtor
	~CReqdPropPlan() override;

	// type of properties
	BOOL
	FPlan() const override
	{
		GPOS_ASSERT(!FRelational());
		return true;
	}

	// required properties computation function
	void Compute(CMemoryPool *mp, CExpressionHandle &exprhdl,
				 gpos::pointer<CReqdProp *> prpInput, ULONG child_index,
				 gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
				 ULONG ulOptReq) override;

	// required columns computation function
	void ComputeReqdCols(CMemoryPool *mp, CExpressionHandle &exprhdl,
						 gpos::pointer<CReqdProp *> prpInput, ULONG child_index,
						 gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt);

	// required ctes computation function
	void ComputeReqdCTEs(CMemoryPool *mp, CExpressionHandle &exprhdl,
						 gpos::pointer<CReqdProp *> prpInput, ULONG child_index,
						 gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt);

	// required columns accessor
	gpos::pointer<CColRefSet *>
	PcrsRequired() const
	{
		return m_pcrs;
	}

	// required order accessor
	gpos::pointer<CEnfdOrder *>
	Peo() const
	{
		return m_peo;
	}

	// required distribution accessor
	gpos::pointer<CEnfdDistribution *>
	Ped() const
	{
		return m_ped;
	}

	// required rewindability accessor
	gpos::pointer<CEnfdRewindability *>
	Per() const
	{
		return m_per;
	}

	// required partition propagation accessor
	gpos::pointer<CEnfdPartitionPropagation *>
	Pepp() const
	{
		return m_pepp;
	}

	// required cte accessor
	gpos::pointer<CCTEReq *>
	Pcter() const
	{
		return m_pcter;
	}

	// given a property spec type, return the corresponding property spec member
	CPropSpec *Pps(ULONG ul) const;

	// equality function
	BOOL Equals(gpos::pointer<const CReqdPropPlan *> prpp) const;

	// hash function
	ULONG HashValue() const;

	// check if plan properties are satisfied by the given derived properties
	BOOL FSatisfied(gpos::pointer<const CDrvdPropRelational *> pdprel,
					gpos::pointer<const CDrvdPropPlan *> pdpplan) const;

	// check if plan properties are compatible with the given derived properties
	BOOL FCompatible(CExpressionHandle &exprhdl,
					 gpos::pointer<CPhysical *> popPhysical,
					 gpos::pointer<const CDrvdPropRelational *> pdprel,
					 gpos::pointer<const CDrvdPropPlan *> pdpplan) const;

	// check if expression attached to handle provides required columns by all plan properties
	BOOL FProvidesReqdCols(CMemoryPool *mp, CExpressionHandle &exprhdl,
						   ULONG ulOptReq) const;

	// shorthand for conversion
	static gpos::cast_func<CReqdPropPlan *>
	Prpp(CReqdProp *prp)
	{
		GPOS_ASSERT(nullptr != prp);

		return dynamic_cast<CReqdPropPlan *>(prp);
	}

	//generate empty required properties
	static gpos::owner<CReqdPropPlan *> PrppEmpty(CMemoryPool *mp);

	// hash function used for cost bounding
	static ULONG UlHashForCostBounding(
		gpos::pointer<const CReqdPropPlan *> prpp);

	// equality function used for cost bounding
	static BOOL FEqualForCostBounding(
		gpos::pointer<const CReqdPropPlan *> prppFst,
		gpos::pointer<const CReqdPropPlan *> prppSnd);

	// map input required and derived plan properties into new required plan properties
	static gpos::owner<CReqdPropPlan *> PrppRemapForCTE(
		CMemoryPool *mp, gpos::pointer<CReqdPropPlan *> prppInput,
		gpos::pointer<CDrvdPropPlan *> pdpplanInput,
		UlongToColRefMap *colref_mapping);

	// print function
	IOstream &OsPrint(IOstream &os) const override;

};	// class CReqdPropPlan

}  // namespace gpopt


#endif	// !GPOPT_CReqdPropPlan_H

// EOF
