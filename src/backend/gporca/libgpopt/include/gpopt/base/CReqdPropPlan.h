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
	gpos::Ref<CColRefSet> m_pcrs{nullptr};

	// required sort order
	gpos::Ref<CEnfdOrder> m_peo{nullptr};

	// required distribution
	gpos::Ref<CEnfdDistribution> m_ped{nullptr};

	// required rewindability
	gpos::Ref<CEnfdRewindability> m_per{nullptr};

	// required partition propagation
	gpos::Ref<CEnfdPartitionPropagation> m_pepp{nullptr};

	// required ctes
	gpos::Ref<CCTEReq> m_pcter{nullptr};

public:
	CReqdPropPlan(const CReqdPropPlan &) = delete;

	// default ctor
	CReqdPropPlan() = default;

	// ctor
	CReqdPropPlan(gpos::Ref<CColRefSet> pcrs, gpos::Ref<CEnfdOrder> peo,
				  gpos::Ref<CEnfdDistribution> ped,
				  gpos::Ref<CEnfdRewindability> per,
				  gpos::Ref<CEnfdPartitionPropagation> pepp,
				  gpos::Ref<CCTEReq> pcter);

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
				 gpos::Ref<CReqdProp> prpInput, ULONG child_index,
				 CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) override;

	// required columns computation function
	void ComputeReqdCols(CMemoryPool *mp, CExpressionHandle &exprhdl,
						 gpos::Ref<CReqdProp> prpInput, ULONG child_index,
						 CDrvdPropArray *pdrgpdpCtxt);

	// required ctes computation function
	void ComputeReqdCTEs(CMemoryPool *mp, CExpressionHandle &exprhdl,
						 gpos::Ref<CReqdProp> prpInput, ULONG child_index,
						 CDrvdPropArray *pdrgpdpCtxt);

	// required columns accessor
	gpos::Ref<CColRefSet>
	PcrsRequired() const
	{
		return m_pcrs.get();
	}

	// required order accessor
	CEnfdOrder *
	Peo() const
	{
		return m_peo.get();
	}

	// required distribution accessor
	CEnfdDistribution *
	Ped() const
	{
		return m_ped.get();
	}

	// required rewindability accessor
	CEnfdRewindability *
	Per() const
	{
		return m_per.get();
	}

	// required partition propagation accessor
	CEnfdPartitionPropagation *
	Pepp() const
	{
		return m_pepp.get();
	}

	// required cte accessor
	gpos::Ref<CCTEReq>
	Pcter() const
	{
		return m_pcter.get();
	}

	// given a property spec type, return the corresponding property spec member
	gpos::Ref<CPropSpec> Pps(ULONG ul) const;

	// equality function
	BOOL Equals(const gpos::Ref<CReqdPropPlan> prpp) const;

	// hash function
	ULONG HashValue() const;

	// check if plan properties are satisfied by the given derived properties
	BOOL FSatisfied(const gpos::Ref<CDrvdPropRelational> pdprel,
					const gpos::Ref<CDrvdPropPlan> pdpplan) const;

	// check if plan properties are compatible with the given derived properties
	BOOL FCompatible(CExpressionHandle &exprhdl,
					 gpos::Ref<CPhysical> popPhysical,
					 const gpos::Ref<CDrvdPropRelational> pdprel,
					 const gpos::Ref<CDrvdPropPlan> pdpplan) const;

	// check if expression attached to handle provides required columns by all plan properties
	BOOL FProvidesReqdCols(CMemoryPool *mp, CExpressionHandle &exprhdl,
						   ULONG ulOptReq) const;

	// shorthand for conversion
	static gpos::Ref<CReqdPropPlan>
	Prpp(gpos::Ref<CReqdProp> prp)
	{
		GPOS_ASSERT(nullptr != prp);

		return gpos::dyn_cast<CReqdPropPlan>(prp);
	}

	//generate empty required properties
	static gpos::Ref<CReqdPropPlan> PrppEmpty(CMemoryPool *mp);

	// hash function used for cost bounding
	static ULONG UlHashForCostBounding(const gpos::Ref<CReqdPropPlan> prpp);
	static ULONG UlHashForCostBounding(const CReqdPropPlan *prpp);

	// equality function used for cost bounding
	static BOOL FEqualForCostBounding(const gpos::Ref<CReqdPropPlan> prppFst,
									  const gpos::Ref<CReqdPropPlan> prppSnd);
	static BOOL FEqualForCostBounding(const CReqdPropPlan *prppFst,
									  const CReqdPropPlan *prppSnd);

	// map input required and derived plan properties into new required plan properties
	static gpos::Ref<CReqdPropPlan> PrppRemapForCTE(
		CMemoryPool *mp, gpos::Ref<CReqdPropPlan> prppInput,
		gpos::Ref<CDrvdPropPlan> pdpplanInput,
		UlongToColRefMap *colref_mapping);

	// print function
	IOstream &OsPrint(IOstream &os) const override;

};	// class CReqdPropPlan

}  // namespace gpopt


#endif	// !GPOPT_CReqdPropPlan_H

// EOF
