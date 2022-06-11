//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CPhysicalSort.cpp
//
//	@doc:
//		Implementation of physical sort operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalSort.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CCTEMap.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::CPhysicalSort
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalSort::CPhysicalSort(CMemoryPool *mp, gpos::owner<COrderSpec *> pos)
	: CPhysical(mp),
	  m_pos(std::move(pos)),  // caller must add-ref pos
	  m_pcrsSort(nullptr)
{
	GPOS_ASSERT(nullptr != m_pos);

	m_pcrsSort = Pos()->PcrsUsed(mp);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::~CPhysicalSort
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPhysicalSort::~CPhysicalSort()
{
	m_pos->Release();
	m_pcrsSort->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::Matches
//
//	@doc:
//		Match operator
//
//---------------------------------------------------------------------------
BOOL
CPhysicalSort::Matches(gpos::pointer<COperator *> pop) const
{
	if (Eopid() != pop->Eopid())
	{
		return false;
	}

	gpos::pointer<CPhysicalSort *> popSort = gpos::dyn_cast<CPhysicalSort>(pop);
	return m_pos->Matches(popSort->Pos());
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::PcrsRequired
//
//	@doc:
//		Compute required columns of the n-th child;
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CPhysicalSort::PcrsRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
							gpos::pointer<CColRefSet *> pcrsRequired,
							ULONG child_index,
							gpos::pointer<CDrvdPropArray *>,  // pdrgpdpCtxt
							ULONG							  // ulOptReq
)
{
	GPOS_ASSERT(0 == child_index);

	gpos::owner<CColRefSet *> pcrs = GPOS_NEW(mp) CColRefSet(mp, *m_pcrsSort);
	pcrs->Union(pcrsRequired);
	gpos::owner<CColRefSet *> pcrsChildReqd =
		PcrsChildReqd(mp, exprhdl, pcrs, child_index, gpos::ulong_max);
	pcrs->Release();

	return pcrsChildReqd;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::PosRequired
//
//	@doc:
//		Compute required sort order of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<COrderSpec *>
CPhysicalSort::PosRequired(CMemoryPool *mp,
						   CExpressionHandle &,			 // exprhdl
						   gpos::pointer<COrderSpec *>,	 // posRequired
						   ULONG
#ifdef GPOS_DEBUG
							   child_index
#endif	// GPOS_DEBUG
						   ,
						   gpos::pointer<CDrvdPropArray *>,	 // pdrgpdpCtxt
						   ULONG							 // ulOptReq
) const
{
	GPOS_ASSERT(0 == child_index);

	// sort operator is order-establishing and does not require child to deliver
	// any sort order; we return an empty sort order as child requirement
	return GPOS_NEW(mp) COrderSpec(mp);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::PdsRequired
//
//	@doc:
//		Compute required distribution of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<CDistributionSpec *>
CPhysicalSort::PdsRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
						   gpos::pointer<CDistributionSpec *> pdsRequired,
						   ULONG child_index,
						   gpos::pointer<CDrvdPropArray *>,	 // pdrgpdpCtxt
						   ULONG							 // ulOptReq
) const
{
	GPOS_ASSERT(0 == child_index);

	return PdsPassThru(mp, exprhdl, pdsRequired, child_index);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::PrsRequired
//
//	@doc:
//		Compute required rewindability of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<CRewindabilitySpec *>
CPhysicalSort::PrsRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
						   gpos::pointer<CRewindabilitySpec *>,	 //prsRequired,
						   ULONG
#ifdef GPOS_DEBUG
							   child_index
#endif	// GPOPS_DEBUG
						   ,
						   gpos::pointer<CDrvdPropArray *>,	 // pdrgpdpCtxt
						   ULONG							 // ulOptReq
) const
{
	GPOS_ASSERT(0 == child_index);

	// Sort establishes rewindability on its own. It does not require motion
	// hazard handling since it is inherently blocking. However, if it contains
	// outer refs in its subtree, a Rescannable request should be sent, so that
	// an appropriate enforcer is added for any non-rescannable ops below (e.g
	// the subtree contains a Filter with outer refs on top of a Motion op, a
	// Spool op needs to be added above the Motion).
	// NB: This logic should be implemented in any materializing ops (e.g Sort & Spool)
	if (exprhdl.HasOuterRefs(0))
	{
		return GPOS_NEW(mp)
			CRewindabilitySpec(CRewindabilitySpec::ErtRescannable,
							   CRewindabilitySpec::EmhtNoMotion);
	}
	else
	{
		return GPOS_NEW(mp) CRewindabilitySpec(
			CRewindabilitySpec::ErtNone, CRewindabilitySpec::EmhtNoMotion);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::PcteRequired
//
//	@doc:
//		Compute required CTE map of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<CCTEReq *>
CPhysicalSort::PcteRequired(CMemoryPool *,		  //mp,
							CExpressionHandle &,  //exprhdl,
							gpos::pointer<CCTEReq *> pcter,
							ULONG
#ifdef GPOS_DEBUG
								child_index
#endif
							,
							CDrvdPropArray *,  //pdrgpdpCtxt,
							ULONG			   //ulOptReq
) const
{
	GPOS_ASSERT(0 == child_index);
	return PcterPushThru(pcter);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::FProvidesReqdCols
//
//	@doc:
//		Check if required columns are included in output columns
//
//---------------------------------------------------------------------------
BOOL
CPhysicalSort::FProvidesReqdCols(CExpressionHandle &exprhdl,
								 gpos::pointer<CColRefSet *> pcrsRequired,
								 ULONG	// ulOptReq
) const
{
	return FUnaryProvidesReqdCols(exprhdl, pcrsRequired);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::PosDerive
//
//	@doc:
//		Derive sort order
//
//---------------------------------------------------------------------------
gpos::owner<COrderSpec *>
CPhysicalSort::PosDerive(CMemoryPool *,		  // mp
						 CExpressionHandle &  // exprhdl
) const
{
	m_pos->AddRef();
	return m_pos;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::PdsDerive
//
//	@doc:
//		Derive distribution
//
//---------------------------------------------------------------------------
gpos::owner<CDistributionSpec *>
CPhysicalSort::PdsDerive(CMemoryPool *,	 // mp
						 CExpressionHandle &exprhdl) const
{
	return PdsDerivePassThruOuter(exprhdl);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::PrsDerive
//
//	@doc:
//		Derive rewindability
//
//---------------------------------------------------------------------------
gpos::owner<CRewindabilitySpec *>
CPhysicalSort::PrsDerive(CMemoryPool *mp,
						 CExpressionHandle &  // exprhdl
) const
{
	// rewindability of output is always true
	return GPOS_NEW(mp) CRewindabilitySpec(CRewindabilitySpec::ErtMarkRestore,
										   CRewindabilitySpec::EmhtNoMotion);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::EpetOrder
//
//	@doc:
//		Return the enforcing type for order property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalSort::EpetOrder(CExpressionHandle &,  // exprhdl
						 gpos::pointer<const CEnfdOrder *> peo) const
{
	GPOS_ASSERT(nullptr != peo);
	GPOS_ASSERT(!peo->PosRequired()->IsEmpty());

	if (peo->FCompatible(m_pos))
	{
		// required order is already established by sort operator
		return CEnfdProp::EpetUnnecessary;
	}

	// required order is incompatible with the order established by the
	// sort operator, prohibit adding another sort operator on top
	return CEnfdProp::EpetProhibited;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::EpetDistribution
//
//	@doc:
//		Return the enforcing type for distribution property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalSort::EpetDistribution(CExpressionHandle & /*exprhdl*/,
								gpos::pointer<const CEnfdDistribution *>
#ifdef GPOS_DEBUG
									ped
#endif	// GPOS_DEBUG
) const
{
	GPOS_ASSERT(nullptr != ped);

	// distribution enforcers have already been added
	return CEnfdProp::EpetUnnecessary;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::EpetRewindability
//
//	@doc:
//		Return the enforcing type for rewindability property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalSort::EpetRewindability(
	CExpressionHandle &,					   // exprhdl
	gpos::pointer<const CEnfdRewindability *>  // per
) const
{
	// no need for enforcing rewindability on output
	return CEnfdProp::EpetUnnecessary;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalSort::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CPhysicalSort::OsPrint(IOstream &os) const
{
	os << SzId() << "  ";
	return Pos()->OsPrint(os);
}

// EOF
