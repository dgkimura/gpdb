//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalSequence.cpp
//
//	@doc:
//		Implementation of physical sequence operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalSequence.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CCTEReq.h"
#include "gpopt/base/CDistributionSpecAny.h"
#include "gpopt/base/CDistributionSpecNonSingleton.h"
#include "gpopt/base/CDistributionSpecSingleton.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSequence::CPhysicalSequence
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalSequence::CPhysicalSequence(CMemoryPool *mp)
	: CPhysical(mp), m_pcrsEmpty(nullptr)
{
	// Sequence generates two distribution requests for its children:
	// (1) If incoming distribution from above is Singleton, pass it through
	//		to all children, otherwise request Non-Singleton on all children
	//
	// (2)	Optimize first child with Any distribution requirement, and compute
	//		distribution request on other children based on derived distribution
	//		of first child:
	//			* If distribution of first child is a Singleton, request Singleton
	//				on all children
	//			* If distribution of first child is a Non-Singleton, request
	//				Non-Singleton on all children

	SetDistrRequests(2);

	m_pcrsEmpty = GPOS_NEW(mp) CColRefSet(mp);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSequence::~CPhysicalSequence
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPhysicalSequence::~CPhysicalSequence()
{
	m_pcrsEmpty->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSequence::Matches
//
//	@doc:
//		Match operators
//
//---------------------------------------------------------------------------
BOOL
CPhysicalSequence::Matches(gpos::pointer<COperator *> pop) const
{
	return Eopid() == pop->Eopid();
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSequence::PcrsRequired
//
//	@doc:
//		Compute required output columns of n-th child
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CPhysicalSequence::PcrsRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
								gpos::pointer<CColRefSet *> pcrsRequired,
								ULONG child_index,
								gpos::pointer<CDrvdPropArray *>,  // pdrgpdpCtxt
								ULONG							  // ulOptReq
)
{
	const ULONG arity = exprhdl.Arity();
	if (child_index == arity - 1)
	{
		// request required columns from the last child of the sequence
		return PcrsChildReqd(mp, exprhdl, pcrsRequired, child_index,
							 gpos::ulong_max);
	}

	m_pcrsEmpty->AddRef();
	GPOS_ASSERT(0 == m_pcrsEmpty->Size());

	return m_pcrsEmpty;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSequence::PcteRequired
//
//	@doc:
//		Compute required CTE map of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<CCTEReq *>
CPhysicalSequence::PcteRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
								gpos::pointer<CCTEReq *> pcter,
								ULONG child_index,
								gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
								ULONG  //ulOptReq
) const
{
	GPOS_ASSERT(nullptr != pcter);
	if (child_index < exprhdl.Arity() - 1)
	{
		return pcter->PcterAllOptional(mp);
	}

	// derived CTE maps from previous children
	gpos::owner<CCTEMap *> pcmCombined = PcmCombine(mp, pdrgpdpCtxt);

	// pass the remaining requirements that have not been resolved
	gpos::owner<CCTEReq *> pcterUnresolved =
		pcter->PcterUnresolvedSequence(mp, pcmCombined, pdrgpdpCtxt);
	pcmCombined->Release();

	return pcterUnresolved;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSequence::FProvidesReqdCols
//
//	@doc:
//		Helper for checking if required columns are included in output columns
//
//---------------------------------------------------------------------------
BOOL
CPhysicalSequence::FProvidesReqdCols(CExpressionHandle &exprhdl,
									 gpos::pointer<CColRefSet *> pcrsRequired,
									 ULONG	// ulOptReq
) const
{
	GPOS_ASSERT(nullptr != pcrsRequired);

	// last child must provide required columns
	ULONG arity = exprhdl.Arity();
	GPOS_ASSERT(0 < arity);

	gpos::pointer<CColRefSet *> pcrsChild =
		exprhdl.DeriveOutputColumns(arity - 1);

	return pcrsChild->ContainsAll(pcrsRequired);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSequence::PdsRequired
//
//	@doc:
//		Compute required distribution of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<CDistributionSpec *>
CPhysicalSequence::PdsRequired(CMemoryPool *mp,
							   CExpressionHandle &
#ifdef GPOS_DEBUG
								   exprhdl
#endif	// GPOS_DEBUG
							   ,
							   gpos::pointer<CDistributionSpec *> pdsRequired,
							   ULONG child_index,
							   gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
							   ULONG ulOptReq) const
{
	GPOS_ASSERT(2 == exprhdl.Arity());
	GPOS_ASSERT(child_index < exprhdl.Arity());
	GPOS_ASSERT(ulOptReq < UlDistrRequests());

	if (0 == ulOptReq)
	{
		if (CDistributionSpec::EdtSingleton == pdsRequired->Edt() ||
			CDistributionSpec::EdtStrictSingleton == pdsRequired->Edt())
		{
			// incoming request is a singleton, request singleton on all children
			gpos::pointer<CDistributionSpecSingleton *> pdss =
				gpos::dyn_cast<CDistributionSpecSingleton>(pdsRequired);
			return GPOS_NEW(mp) CDistributionSpecSingleton(pdss->Est());
		}

		// incoming request is a non-singleton, request non-singleton on all children
		return GPOS_NEW(mp) CDistributionSpecNonSingleton();
	}
	GPOS_ASSERT(1 == ulOptReq);

	if (0 == child_index)
	{
		// no distribution requirement on first child
		return GPOS_NEW(mp) CDistributionSpecAny(this->Eopid());
	}

	// get derived plan properties of first child
	gpos::pointer<CDrvdPropPlan *> pdpplan =
		gpos::dyn_cast<CDrvdPropPlan>((*pdrgpdpCtxt)[0]);
	gpos::pointer<CDistributionSpec *> pds = pdpplan->Pds();

	if (pds->FSingletonOrStrictSingleton())
	{
		// first child is singleton, request singleton distribution on second child
		gpos::pointer<CDistributionSpecSingleton *> pdss =
			gpos::dyn_cast<CDistributionSpecSingleton>(pds);
		return GPOS_NEW(mp) CDistributionSpecSingleton(pdss->Est());
	}

	if (CDistributionSpec::EdtUniversal == pds->Edt())
	{
		// first child is universal, impose no requirements on second child
		return GPOS_NEW(mp) CDistributionSpecAny(this->Eopid());
	}

	// first child is non-singleton, request a non-singleton distribution on second child
	return GPOS_NEW(mp) CDistributionSpecNonSingleton();
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSequence::PosRequired
//
//	@doc:
//		Compute required sort order of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<COrderSpec *>
CPhysicalSequence::PosRequired(CMemoryPool *mp,
							   CExpressionHandle &,				 // exprhdl,
							   gpos::pointer<COrderSpec *>,		 // posRequired,
							   ULONG,							 // child_index,
							   gpos::pointer<CDrvdPropArray *>,	 // pdrgpdpCtxt
							   ULONG							 // ulOptReq
) const
{
	// no order requirement on the children
	return GPOS_NEW(mp) COrderSpec(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSequence::PrsRequired
//
//	@doc:
//		Compute required rewindability order of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<CRewindabilitySpec *>
CPhysicalSequence::PrsRequired(CMemoryPool *,		 // mp,
							   CExpressionHandle &,	 // exprhdl,
							   gpos::pointer<CRewindabilitySpec *> prsRequired,
							   ULONG,							 // child_index,
							   gpos::pointer<CDrvdPropArray *>,	 // pdrgpdpCtxt
							   ULONG							 // ulOptReq
) const
{
	// TODO: shardikar; Handle outer refs in the subtree correctly, by passing
	// "Rescannable' Also, maybe it should pass through the prsRequired, since it
	// doesn't materialize any results? It's important to consider performance
	// consequences of that also.
	return GPOS_NEW(m_mp)
		CRewindabilitySpec(CRewindabilitySpec::ErtNone, prsRequired->Emht());
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSequence::PosDerive
//
//	@doc:
//		Derive sort order
//
//---------------------------------------------------------------------------
gpos::owner<COrderSpec *>
CPhysicalSequence::PosDerive(CMemoryPool *,	 // mp,
							 CExpressionHandle &exprhdl) const
{
	// pass through sort order from last child
	const ULONG arity = exprhdl.Arity();

	GPOS_ASSERT(1 <= arity);

	gpos::owner<COrderSpec *> pos =
		exprhdl.Pdpplan(arity - 1 /*child_index*/)->Pos();
	pos->AddRef();

	return pos;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSequence::PdsDerive
//
//	@doc:
//		Derive distribution
//
//---------------------------------------------------------------------------
gpos::owner<CDistributionSpec *>
CPhysicalSequence::PdsDerive(CMemoryPool *,	 // mp,
							 CExpressionHandle &exprhdl) const
{
	// pass through distribution from last child
	const ULONG arity = exprhdl.Arity();

	GPOS_ASSERT(1 <= arity);

	gpos::owner<CDistributionSpec *> pds =
		exprhdl.Pdpplan(arity - 1 /*child_index*/)->Pds();
	pds->AddRef();

	return pds;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSequence::PrsDerive
//
//	@doc:
//		Derive rewindability
//
//---------------------------------------------------------------------------
gpos::owner<CRewindabilitySpec *>
CPhysicalSequence::PrsDerive(CMemoryPool *,	 //mp
							 CExpressionHandle &exprhdl) const
{
	const ULONG arity = exprhdl.Arity();
	GPOS_ASSERT(1 <= arity);

	CRewindabilitySpec::EMotionHazardType motion_hazard =
		CRewindabilitySpec::EmhtNoMotion;
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::pointer<CRewindabilitySpec *> prs = exprhdl.Pdpplan(ul)->Prs();
		if (prs->HasMotionHazard())
		{
			motion_hazard = CRewindabilitySpec::EmhtMotion;
			break;
		}
	}

	// TODO: shardikar; Fix this implementation. Although CPhysicalSequence is
	// not rewindable, all its children might be rewindable. This implementation
	// ignores the rewindability of the op's children
	return GPOS_NEW(m_mp)
		CRewindabilitySpec(CRewindabilitySpec::ErtNone, motion_hazard);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSequence::EpetOrder
//
//	@doc:
//		Return the enforcing type for the order property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalSequence::EpetOrder(CExpressionHandle &exprhdl,
							 gpos::pointer<const CEnfdOrder *> peo) const
{
	GPOS_ASSERT(nullptr != peo);

	// get order delivered by the sequence node
	gpos::pointer<COrderSpec *> pos =
		gpos::dyn_cast<CDrvdPropPlan>(exprhdl.Pdp())->Pos();

	if (peo->FCompatible(pos))
	{
		// required order will be established by the sequence operator
		return CEnfdProp::EpetUnnecessary;
	}

	// required distribution will be enforced on sequence's output
	return CEnfdProp::EpetRequired;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSequence::EpetRewindability
//
//	@doc:
//		Return the enforcing type for rewindability property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalSequence::EpetRewindability(
	CExpressionHandle &,					   // exprhdl
	gpos::pointer<const CEnfdRewindability *>  // per
) const
{
	// rewindability must be enforced on operator's output
	return CEnfdProp::EpetRequired;
}


// EOF
