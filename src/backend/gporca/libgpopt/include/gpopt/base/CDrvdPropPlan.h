//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CDrvdPropPlan.h
//
//	@doc:
//		Derived physical properties
//---------------------------------------------------------------------------
#ifndef GPOPT_CDrvdPropPlan_H
#define GPOPT_CDrvdPropPlan_H

#include "gpos/base.h"
#include "gpos/common/CRefCount.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CDistributionSpec.h"
#include "gpopt/base/CDrvdProp.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CRewindabilitySpec.h"

namespace gpopt
{
using namespace gpos;

// fwd declaration
class CExpressionHandle;
class CPartitionPropagationSpec;
class CReqdPropPlan;
class CCTEMap;

//---------------------------------------------------------------------------
//	@class:
//		CDrvdPropPlan
//
//	@doc:
//		Derived plan properties container.
//
//		These are properties that are expression-specific and they depend on
//		the physical implementation. This includes sort order, distribution,
//		rewindability, partition propagation spec and CTE map.
//
//---------------------------------------------------------------------------
class CDrvdPropPlan : public CDrvdProp
{
private:
	// derived sort order
	gpos::owner<COrderSpec *> m_pos{nullptr};

	// derived distribution
	gpos::owner<CDistributionSpec *> m_pds{nullptr};

	// derived rewindability
	gpos::owner<CRewindabilitySpec *> m_prs{nullptr};

	// derived partition propagation spec
	gpos::owner<CPartitionPropagationSpec *> m_ppps{nullptr};

	// derived cte map
	gpos::owner<CCTEMap *> m_pcm{nullptr};

	// copy CTE producer plan properties from given context to current object
	void CopyCTEProducerPlanProps(CMemoryPool *mp,
								  gpos::pointer<CDrvdPropCtxt *> pdpctxt,
								  gpos::pointer<COperator *> pop);

public:
	CDrvdPropPlan(const CDrvdPropPlan &) = delete;

	// ctor
	CDrvdPropPlan();

	// dtor
	~CDrvdPropPlan() override;

	// type of properties
	EPropType
	Ept() override
	{
		return EptPlan;
	}

	// derivation function
	void Derive(CMemoryPool *mp, CExpressionHandle &exprhdl,
				gpos::pointer<CDrvdPropCtxt *> pdpctxt) override;

	// short hand for conversion
	static gpos::cast_func<CDrvdPropPlan *> Pdpplan(CDrvdProp *pdp);

	// sort order accessor
	gpos::pointer<COrderSpec *>
	Pos() const
	{
		return m_pos;
	}

	// distribution accessor
	gpos::pointer<CDistributionSpec *>
	Pds() const
	{
		return m_pds;
	}

	// rewindability accessor
	gpos::pointer<CRewindabilitySpec *>
	Prs() const
	{
		return m_prs;
	}

	gpos::pointer<CPartitionPropagationSpec *>
	Ppps() const
	{
		return m_ppps;
	}

	// cte map
	gpos::pointer<CCTEMap *>
	GetCostModel() const
	{
		return m_pcm;
	}

	// hash function
	virtual ULONG HashValue() const;

	// equality function
	virtual ULONG Equals(gpos::pointer<const CDrvdPropPlan *> pdpplan) const;

	// check for satisfying required plan properties
	BOOL FSatisfies(gpos::pointer<const CReqdPropPlan *> prpp) const override;

	// print function
	IOstream &OsPrint(IOstream &os) const override;

};	// class CDrvdPropPlan

}  // namespace gpopt


#endif	// !GPOPT_CDrvdPropPlan_H

// EOF
