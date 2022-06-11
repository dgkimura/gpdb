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
#include "gpos/common/owner.h"

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
	CMemoryPool *mp, gpos::owner<CDistributionSpecRouted *> pdsRouted)
	: CPhysicalMotion(mp),
	  m_pdsRouted(std::move(pdsRouted)),
	  m_pcrsRequiredLocal(nullptr)
{
	GPOS_ASSERT(nullptr != m_pdsRouted);

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
	m_pdsRouted->Release();
	m_pcrsRequiredLocal->Release();
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
CPhysicalMotionRoutedDistribute::Matches(gpos::pointer<COperator *> pop) const
{
	if (Eopid() != pop->Eopid())
	{
		return false;
	}

	gpos::pointer<CPhysicalMotionRoutedDistribute *> popRoutedDistribute =
		gpos::dyn_cast<CPhysicalMotionRoutedDistribute>(pop);

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
gpos::owner<CColRefSet *>
CPhysicalMotionRoutedDistribute::PcrsRequired(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::pointer<CColRefSet *> pcrsRequired, ULONG child_index,
	gpos::pointer<CDrvdPropArray *>,  // pdrgpdpCtxt
	ULONG							  // ulOptReq
)
{
	GPOS_ASSERT(0 == child_index);

	gpos::owner<CColRefSet *> pcrs =
		GPOS_NEW(mp) CColRefSet(mp, *m_pcrsRequiredLocal);
	pcrs->Union(pcrsRequired);

	gpos::owner<CColRefSet *> pcrsChildReqd =
		PcrsChildReqd(mp, exprhdl, pcrs, child_index, gpos::ulong_max);
	pcrs->Release();

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
	CExpressionHandle &exprhdl, gpos::pointer<CColRefSet *> pcrsRequired,
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
CPhysicalMotionRoutedDistribute::EpetOrder(
	CExpressionHandle &,			   // exprhdl
	gpos::pointer<const CEnfdOrder *>  // peo
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
gpos::owner<COrderSpec *>
CPhysicalMotionRoutedDistribute::PosRequired(
	CMemoryPool *mp,
	CExpressionHandle &,		  // exprhdl
	gpos::pointer<COrderSpec *>,  //posInput
	ULONG
#ifdef GPOS_DEBUG
		child_index
#endif	// GPOS_DEBUG
	,
	gpos::pointer<CDrvdPropArray *>,  // pdrgpdpCtxt
	ULONG							  // ulOptReq
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
gpos::owner<COrderSpec *>
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
gpos::cast_func<CPhysicalMotionRoutedDistribute *>
CPhysicalMotionRoutedDistribute::PopConvert(COperator *pop)
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(EopPhysicalMotionRoutedDistribute == pop->Eopid());

	return dynamic_cast<CPhysicalMotionRoutedDistribute *>(pop);
}

// EOF
