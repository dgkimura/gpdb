//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalConstTableGet.cpp
//
//	@doc:
//		Implementation of physical const table get operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalConstTableGet.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CCTEMap.h"
#include "gpopt/base/CDistributionSpecUniversal.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"


using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalConstTableGet::CPhysicalConstTableGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalConstTableGet::CPhysicalConstTableGet(
	CMemoryPool *mp, gpos::Ref<CColumnDescriptorArray> pdrgpcoldesc,
	gpos::Ref<IDatum2dArray> pdrgpdrgpdatum,
	gpos::Ref<CColRefArray> pdrgpcrOutput)
	: CPhysical(mp),
	  m_pdrgpcoldesc(std::move(pdrgpcoldesc)),
	  m_pdrgpdrgpdatum(std::move(pdrgpdrgpdatum)),
	  m_pdrgpcrOutput(std::move(pdrgpcrOutput))
{
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalConstTableGet::~CPhysicalConstTableGet
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPhysicalConstTableGet::~CPhysicalConstTableGet()
{
	;
	;
	;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalConstTableGet::Matches
//
//	@doc:
//		Match operators
//
//---------------------------------------------------------------------------
BOOL
CPhysicalConstTableGet::Matches(COperator *pop) const
{
	if (Eopid() == pop->Eopid())
	{
		CPhysicalConstTableGet *popCTG =
			gpos::dyn_cast<CPhysicalConstTableGet>(pop);
		return m_pdrgpcoldesc == popCTG->Pdrgpcoldesc() &&
			   m_pdrgpdrgpdatum == popCTG->Pdrgpdrgpdatum() &&
			   m_pdrgpcrOutput == popCTG->PdrgpcrOutput();
	}

	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalConstTableGet::PcrsRequired
//
//	@doc:
//		Compute required columns of the n-th child;
//		we only compute required columns for the relational child;
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CPhysicalConstTableGet::PcrsRequired(CMemoryPool *,		   // mp,
									 CExpressionHandle &,  // exprhdl,
									 CColRefSet *,		   // pcrsRequired,
									 ULONG,				   // child_index,
									 CDrvdPropArray *,	   // pdrgpdpCtxt
									 ULONG				   // ulOptReq
)
{
	GPOS_ASSERT(!"CPhysicalConstTableGet has no children");
	return nullptr;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalConstTableGet::PosRequired
//
//	@doc:
//		Compute required sort order of the n-th child
//
//---------------------------------------------------------------------------
gpos::Ref<COrderSpec>
CPhysicalConstTableGet::PosRequired(CMemoryPool *,		  // mp,
									CExpressionHandle &,  // exprhdl,
									COrderSpec *,		  // posRequired,
									ULONG,				  // child_index,
									CDrvdPropArray *,	  // pdrgpdpCtxt
									ULONG				  // ulOptReq
) const
{
	GPOS_ASSERT(!"CPhysicalConstTableGet has no children");
	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalConstTableGet::PdsRequired
//
//	@doc:
//		Compute required distribution of the n-th child
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpec>
CPhysicalConstTableGet::PdsRequired(CMemoryPool *,		  // mp,
									CExpressionHandle &,  // exprhdl,
									CDistributionSpec *,  // pdsRequired,
									ULONG,				  //child_index
									CDrvdPropArray *,	  // pdrgpdpCtxt
									ULONG				  // ulOptReq
) const
{
	GPOS_ASSERT(!"CPhysicalConstTableGet has no children");
	return nullptr;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalConstTableGet::PrsRequired
//
//	@doc:
//		Compute required rewindability of the n-th child
//
//---------------------------------------------------------------------------
gpos::Ref<CRewindabilitySpec>
CPhysicalConstTableGet::PrsRequired(CMemoryPool *,		   // mp,
									CExpressionHandle &,   // exprhdl,
									CRewindabilitySpec *,  // prsRequired,
									ULONG,				   // child_index,
									CDrvdPropArray *,	   // pdrgpdpCtxt
									ULONG				   // ulOptReq
) const
{
	GPOS_ASSERT(!"CPhysicalConstTableGet has no children");
	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalConstTableGet::PcteRequired
//
//	@doc:
//		Compute required CTE map of the n-th child
//
//---------------------------------------------------------------------------
gpos::Ref<CCTEReq>
CPhysicalConstTableGet::PcteRequired(CMemoryPool *,		   //mp,
									 CExpressionHandle &,  //exprhdl,
									 CCTEReq *,			   //pcter,
									 ULONG,				   //child_index,
									 CDrvdPropArray *,	   //pdrgpdpCtxt,
									 ULONG				   //ulOptReq
) const
{
	GPOS_ASSERT(!"CPhysicalConstTableGet has no children");
	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalConstTableGet::FProvidesReqdCols
//
//	@doc:
//		Check if required columns are included in output columns
//
//---------------------------------------------------------------------------
BOOL
CPhysicalConstTableGet::FProvidesReqdCols(CExpressionHandle &,	// exprhdl,
										  CColRefSet *pcrsRequired,
										  ULONG	 // ulOptReq
) const
{
	GPOS_ASSERT(nullptr != pcrsRequired);

	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrs->Include(m_pdrgpcrOutput.get());

	BOOL result = pcrs->ContainsAll(pcrsRequired);

	;

	return result;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalConstTableGet::PosDerive
//
//	@doc:
//		Derive sort order
//
//---------------------------------------------------------------------------
gpos::Ref<COrderSpec>
CPhysicalConstTableGet::PosDerive(CMemoryPool *mp,
								  CExpressionHandle &  // exprhdl
) const
{
	return GPOS_NEW(mp) COrderSpec(mp);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalConstTableGet::PdsDerive
//
//	@doc:
//		Derive distribution
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpec>
CPhysicalConstTableGet::PdsDerive(CMemoryPool *mp,
								  CExpressionHandle &  // exprhdl
) const
{
	return GPOS_NEW(mp) CDistributionSpecUniversal();
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalConstTableGet::PrsDerive
//
//	@doc:
//		Derive rewindability
//
//---------------------------------------------------------------------------
gpos::Ref<CRewindabilitySpec>
CPhysicalConstTableGet::PrsDerive(CMemoryPool *mp,
								  CExpressionHandle &  // exprhdl
) const
{
	return GPOS_NEW(mp) CRewindabilitySpec(CRewindabilitySpec::ErtMarkRestore,
										   CRewindabilitySpec::EmhtNoMotion);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalConstTableGet::PcmDerive
//
//	@doc:
//		Derive cte map
//
//---------------------------------------------------------------------------
gpos::Ref<CCTEMap>
CPhysicalConstTableGet::PcmDerive(CMemoryPool *mp,
								  CExpressionHandle &  //exprhdl
) const
{
	return GPOS_NEW(mp) CCTEMap(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalConstTableGet::EpetOrder
//
//	@doc:
//		Return the enforcing type for order property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalConstTableGet::EpetOrder(CExpressionHandle &,	// exprhdl
								  const CEnfdOrder *
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
//		CPhysicalConstTableGet::EpetRewindability
//
//	@doc:
//		Return the enforcing type for rewindability property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalConstTableGet::EpetRewindability(CExpressionHandle &,		  // exprhdl
										  const CEnfdRewindability *  // per
) const
{
	// rewindability is already provided
	return CEnfdProp::EpetUnnecessary;
}

// print values in const table
IOstream &
CPhysicalConstTableGet::OsPrint(IOstream &os) const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}
	else
	{
		os << SzId() << " ";
		os << "Columns: [";
		CUtils::OsPrintDrgPcr(os, m_pdrgpcrOutput.get());
		os << "] ";
		os << "Values: [";
		for (ULONG ulA = 0; ulA < m_pdrgpdrgpdatum->Size(); ulA++)
		{
			if (0 < ulA)
			{
				os << "; ";
			}
			os << "(";
			IDatumArray *pdrgpdatum = (*m_pdrgpdrgpdatum)[ulA].get();

			const ULONG length = pdrgpdatum->Size();
			for (ULONG ulB = 0; ulB < length; ulB++)
			{
				IDatum *datum = (*pdrgpdatum)[ulB].get();
				datum->OsPrint(os);

				if (ulB < length - 1)
				{
					os << ", ";
				}
			}
			os << ")";
		}
		os << "]";
	}

	return os;
}


// EOF
