//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalMotionRoutedDistribute.cpp
//
//	@doc:
//		Implementation of routed distribute motion operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalMotionRoutedDistribute.h"

#include "gpos/base.h"

#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRoutedDistribute::CPhysicalMotionRoutedDistribute
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalMotionRoutedDistribute::CPhysicalMotionRoutedDistribute(
	CMemoryPool *mp, gpos::Ref<CDistributionSpecRouted> pdsRouted)
	: CPhysicalMotion(mp), m_pdsRouted(pdsRouted), m_pcrsRequiredLocal(nullptr)
{
	GPOS_ASSERT(nullptr != pdsRouted);

	m_pcrsRequiredLocal = GPOS_NEW(mp) CColRefSet(mp);

	// include segment id column
	m_pcrsRequiredLocal->Include(m_pdsRouted->Pcr());
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRoutedDistribute::~CPhysicalMotionRoutedDistribute
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPhysicalMotionRoutedDistribute::~CPhysicalMotionRoutedDistribute()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRoutedDistribute::Matches
//
//	@doc:
//		Match operators
//
//---------------------------------------------------------------------------
BOOL
CPhysicalMotionRoutedDistribute::Matches(gpos::Ref<COperator> pop) const
{
	if (Eopid() != pop->Eopid())
	{
		return false;
	}

	gpos::Ref<CPhysicalMotionRoutedDistribute> popRoutedDistribute =
		CPhysicalMotionRoutedDistribute::PopConvert(pop);

	return m_pdsRouted->Matches(popRoutedDistribute->m_pdsRouted);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRoutedDistribute::PcrsRequired
//
//	@doc:
//		Compute required columns of the n-th child;
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CPhysicalMotionRoutedDistribute::PcrsRequired(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::Ref<CColRefSet> pcrsRequired, ULONG child_index,
	gpos::Ref<CDrvdPropArray>,	// pdrgpdpCtxt
	ULONG						// ulOptReq
)
{
	GPOS_ASSERT(0 == child_index);

	gpos::Ref<CColRefSet> pcrs =
		GPOS_NEW(mp) CColRefSet(mp, *m_pcrsRequiredLocal);
	pcrs->Union(pcrsRequired);

	gpos::Ref<CColRefSet> pcrsChildReqd =
		PcrsChildReqd(mp, exprhdl, pcrs, child_index, gpos::ulong_max);


	return pcrsChildReqd;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRoutedDistribute::FProvidesReqdCols
//
//	@doc:
//		Check if required columns are included in output columns
//
//---------------------------------------------------------------------------
BOOL
CPhysicalMotionRoutedDistribute::FProvidesReqdCols(
	CExpressionHandle &exprhdl, gpos::Ref<CColRefSet> pcrsRequired,
	ULONG  // ulOptReq
) const
{
	return FUnaryProvidesReqdCols(exprhdl, pcrsRequired);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRoutedDistribute::EpetOrder
//
//	@doc:
//		Return the enforcing type for order property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalMotionRoutedDistribute::EpetOrder(CExpressionHandle &,	 // exprhdl
										   const gpos::Ref<CEnfdOrder>	// peo
) const
{
	return CEnfdProp::EpetRequired;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRoutedDistribute::PosRequired
//
//	@doc:
//		Compute required sort order of the n-th child
//
//---------------------------------------------------------------------------
gpos::Ref<COrderSpec>
CPhysicalMotionRoutedDistribute::PosRequired(
	CMemoryPool *mp,
	CExpressionHandle &,	// exprhdl
	gpos::Ref<COrderSpec>,	//posInput
	ULONG
#ifdef GPOS_DEBUG
		child_index
#endif	// GPOS_DEBUG
	,
	gpos::Ref<CDrvdPropArray>,	// pdrgpdpCtxt
	ULONG						// ulOptReq
) const
{
	GPOS_ASSERT(0 == child_index);

	return GPOS_NEW(mp) COrderSpec(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRoutedDistribute::PosDerive
//
//	@doc:
//		Derive sort order
//
//---------------------------------------------------------------------------
gpos::Ref<COrderSpec>
CPhysicalMotionRoutedDistribute::PosDerive(CMemoryPool *mp,
										   CExpressionHandle &	// exprhdl
) const
{
	return GPOS_NEW(mp) COrderSpec(mp);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRoutedDistribute::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CPhysicalMotionRoutedDistribute::OsPrint(IOstream &os) const
{
	os << SzId() << " ";

	return m_pdsRouted->OsPrint(os);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRoutedDistribute::PopConvert
//
//	@doc:
//		Conversion function
//
//---------------------------------------------------------------------------
gpos::Ref<CPhysicalMotionRoutedDistribute>
CPhysicalMotionRoutedDistribute::PopConvert(gpos::Ref<COperator> pop)
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(EopPhysicalMotionRoutedDistribute == pop->Eopid());

	return gpos::dyn_cast<CPhysicalMotionRoutedDistribute>(pop);
}

// EOF
