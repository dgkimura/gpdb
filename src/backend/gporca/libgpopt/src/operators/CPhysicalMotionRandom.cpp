//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalMotionRandom.cpp
//
//	@doc:
//		Implementation of random motion operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalMotionRandom.h"

#include "gpos/base.h"

#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRandom::CPhysicalMotionRandom
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalMotionRandom::CPhysicalMotionRandom(
	CMemoryPool *mp, gpos::Ref<CDistributionSpecRandom> pdsRandom)
	: CPhysicalMotion(mp), m_pdsRandom(pdsRandom)
{
	GPOS_ASSERT(nullptr != pdsRandom);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRandom::~CPhysicalMotionRandom
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPhysicalMotionRandom::~CPhysicalMotionRandom()
{
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRandom::Matches
//
//	@doc:
//		Match operators
//
//---------------------------------------------------------------------------
BOOL
CPhysicalMotionRandom::Matches(gpos::Ref<COperator> pop) const
{
	return Eopid() == pop->Eopid();
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRandom::PcrsRequired
//
//	@doc:
//		Compute required columns of the n-th child;
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CPhysicalMotionRandom::PcrsRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
									gpos::Ref<CColRefSet> pcrsRequired,
									ULONG child_index,
									gpos::Ref<CDrvdPropArray>,	// pdrgpdpCtxt
									ULONG						// ulOptReq
)
{
	GPOS_ASSERT(0 == child_index);

	return PcrsChildReqd(mp, exprhdl, pcrsRequired, child_index,
						 gpos::ulong_max);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRandom::FProvidesReqdCols
//
//	@doc:
//		Check if required columns are included in output columns
//
//---------------------------------------------------------------------------
BOOL
CPhysicalMotionRandom::FProvidesReqdCols(CExpressionHandle &exprhdl,
										 gpos::Ref<CColRefSet> pcrsRequired,
										 ULONG	// ulOptReq
) const
{
	return FUnaryProvidesReqdCols(exprhdl, pcrsRequired);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRandom::EpetOrder
//
//	@doc:
//		Return the enforcing type for order property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalMotionRandom::EpetOrder(CExpressionHandle &,  // exprhdl
								 const gpos::Ref<CEnfdOrder>
#ifdef GPOS_DEBUG
									 peo
#endif	// GPOS_DEBUG
) const
{
	GPOS_ASSERT(nullptr != peo);
	GPOS_ASSERT(!peo->PosRequired()->IsEmpty());

	return CEnfdProp::EpetRequired;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRandom::PosRequired
//
//	@doc:
//		Compute required sort order of the n-th child
//
//---------------------------------------------------------------------------
gpos::Ref<COrderSpec>
CPhysicalMotionRandom::PosRequired(CMemoryPool *mp,
								   CExpressionHandle &,	   //exprhdl,
								   gpos::Ref<COrderSpec>,  //posInput,
								   ULONG
#ifdef GPOS_DEBUG
									   child_index
#endif	// GPOS_DEBUG
								   ,
								   gpos::Ref<CDrvdPropArray>,  // pdrgpdpCtxt
								   ULONG					   // ulOptReq
) const
{
	GPOS_ASSERT(0 == child_index);

	return GPOS_NEW(mp) COrderSpec(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRandom::PosDerive
//
//	@doc:
//		Derive sort order
//
//---------------------------------------------------------------------------
gpos::Ref<COrderSpec>
CPhysicalMotionRandom::PosDerive(CMemoryPool *mp,
								 CExpressionHandle &  // exprhdl
) const
{
	return GPOS_NEW(mp) COrderSpec(mp);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRandom::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CPhysicalMotionRandom::OsPrint(IOstream &os) const
{
	os << SzId();

	return os;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalMotionRandom::PopConvert
//
//	@doc:
//		Conversion function
//
//---------------------------------------------------------------------------
gpos::Ref<CPhysicalMotionRandom>
CPhysicalMotionRandom::PopConvert(gpos::Ref<COperator> pop)
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(EopPhysicalMotionRandom == pop->Eopid());

	return gpos::dyn_cast<CPhysicalMotionRandom>(pop);
}

// EOF
