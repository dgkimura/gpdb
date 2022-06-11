//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalRowTrigger.cpp
//
//	@doc:
//		Implementation of Physical row-level trigger operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalRowTrigger.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpecAny.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalRowTrigger::CPhysicalRowTrigger
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalRowTrigger::CPhysicalRowTrigger(CMemoryPool *mp,
										 gpos::owner<IMDId *> rel_mdid,
										 INT type,
										 gpos::owner<CColRefArray *> pdrgpcrOld,
										 gpos::owner<CColRefArray *> pdrgpcrNew)
	: CPhysical(mp),
	  m_rel_mdid(rel_mdid),
	  m_type(type),
	  m_pdrgpcrOld(pdrgpcrOld),
	  m_pdrgpcrNew(pdrgpcrNew),
	  m_pcrsRequiredLocal(nullptr)
{
	GPOS_ASSERT(m_rel_mdid->IsValid());
	GPOS_ASSERT(0 != type);
	GPOS_ASSERT(nullptr != m_pdrgpcrNew || nullptr != m_pdrgpcrOld);
	GPOS_ASSERT_IMP(nullptr != m_pdrgpcrNew && nullptr != m_pdrgpcrOld,
					m_pdrgpcrNew->Size() == m_pdrgpcrOld->Size());

	m_pcrsRequiredLocal = GPOS_NEW(mp) CColRefSet(mp);
	if (nullptr != m_pdrgpcrOld)
	{
		m_pcrsRequiredLocal->Include(m_pdrgpcrOld);
	}

	if (nullptr != m_pdrgpcrNew)
	{
		m_pcrsRequiredLocal->Include(m_pdrgpcrNew);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalRowTrigger::~CPhysicalRowTrigger
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPhysicalRowTrigger::~CPhysicalRowTrigger()
{
	m_rel_mdid->Release();
	CRefCount::SafeRelease(m_pdrgpcrOld);
	CRefCount::SafeRelease(m_pdrgpcrNew);
	m_pcrsRequiredLocal->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalRowTrigger::PosRequired
//
//	@doc:
//		Compute required sort columns of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<COrderSpec *>
CPhysicalRowTrigger::PosRequired(
	CMemoryPool *mp,
	CExpressionHandle &,		  //exprhdl,
	gpos::pointer<COrderSpec *>,  //posRequired,
	ULONG
#ifdef GPOS_DEBUG
		child_index
#endif
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
//		CPhysicalRowTrigger::PosDerive
//
//	@doc:
//		Derive sort order
//
//---------------------------------------------------------------------------
gpos::owner<COrderSpec *>
CPhysicalRowTrigger::PosDerive(CMemoryPool *mp,
							   CExpressionHandle &	//exprhdl
) const
{
	return GPOS_NEW(mp) COrderSpec(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalRowTrigger::EpetOrder
//
//	@doc:
//		Return the enforcing type for order property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalRowTrigger::EpetOrder(CExpressionHandle &,	 // exprhdl
							   gpos::pointer<const CEnfdOrder *>
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
//		CPhysicalRowTrigger::PcrsRequired
//
//	@doc:
//		Compute required columns of the n-th child;
//		we only compute required columns for the relational child;
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CPhysicalRowTrigger::PcrsRequired(
	CMemoryPool *mp,
	CExpressionHandle &,  // exprhdl,
	gpos::pointer<CColRefSet *> pcrsRequired,
	ULONG
#ifdef GPOS_DEBUG
		child_index
#endif	// GPOS_DEBUG
	,
	gpos::pointer<CDrvdPropArray *>,  // pdrgpdpCtxt
	ULONG							  // ulOptReq
)
{
	GPOS_ASSERT(0 == child_index);

	gpos::owner<CColRefSet *> pcrs =
		GPOS_NEW(mp) CColRefSet(mp, *m_pcrsRequiredLocal);
	pcrs->Union(pcrsRequired);

	return pcrs;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalRowTrigger::PdsRequired
//
//	@doc:
//		Compute required distribution of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<CDistributionSpec *>
CPhysicalRowTrigger::PdsRequired(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::pointer<CDistributionSpec *> pdsInput, ULONG child_index,
	gpos::pointer<CDrvdPropArray *>,  // pdrgpdpCtxt
	ULONG							  // ulOptReq
) const
{
	GPOS_ASSERT(0 == child_index);

	// if expression has to execute on a single host then we need a gather
	if (exprhdl.NeedsSingletonExecution())
	{
		return PdsRequireSingleton(mp, exprhdl, pdsInput, child_index);
	}

	return GPOS_NEW(mp) CDistributionSpecAny(this->Eopid());
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalRowTrigger::PrsRequired
//
//	@doc:
//		Compute required rewindability of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<CRewindabilitySpec *>
CPhysicalRowTrigger::PrsRequired(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::pointer<CRewindabilitySpec *> prsRequired, ULONG child_index,
	gpos::pointer<CDrvdPropArray *>,  // pdrgpdpCtxt
	ULONG							  // ulOptReq
) const
{
	GPOS_ASSERT(0 == child_index);

	return PrsPassThru(mp, exprhdl, prsRequired, child_index);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalRowTrigger::PcteRequired
//
//	@doc:
//		Compute required CTE map of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<CCTEReq *>
CPhysicalRowTrigger::PcteRequired(CMemoryPool *,		//mp,
								  CExpressionHandle &,	//exprhdl,
								  gpos::pointer<CCTEReq *> pcter,
								  ULONG
#ifdef GPOS_DEBUG
									  child_index
#endif
								  ,
								  CDrvdPropArray *,	 //pdrgpdpCtxt,
								  ULONG				 //ulOptReq
) const
{
	GPOS_ASSERT(0 == child_index);
	return PcterPushThru(pcter);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalRowTrigger::FProvidesReqdCols
//
//	@doc:
//		Check if required columns are included in output columns
//
//---------------------------------------------------------------------------
BOOL
CPhysicalRowTrigger::FProvidesReqdCols(CExpressionHandle &exprhdl,
									   gpos::pointer<CColRefSet *> pcrsRequired,
									   ULONG  // ulOptReq
) const
{
	return FUnaryProvidesReqdCols(exprhdl, pcrsRequired);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalRowTrigger::PdsDerive
//
//	@doc:
//		Derive distribution
//
//---------------------------------------------------------------------------
gpos::owner<CDistributionSpec *>
CPhysicalRowTrigger::PdsDerive(CMemoryPool *,  // mp
							   CExpressionHandle &exprhdl) const
{
	return PdsDerivePassThruOuter(exprhdl);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalRowTrigger::PrsDerive
//
//	@doc:
//		Derive rewindability
//
//---------------------------------------------------------------------------
gpos::owner<CRewindabilitySpec *>
CPhysicalRowTrigger::PrsDerive(CMemoryPool *mp,
							   CExpressionHandle &exprhdl) const
{
	return PrsDerivePassThruOuter(mp, exprhdl);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalRowTrigger::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CPhysicalRowTrigger::HashValue() const
{
	ULONG ulHash =
		gpos::CombineHashes(COperator::HashValue(), m_rel_mdid->HashValue());
	ulHash = gpos::CombineHashes(ulHash, gpos::HashValue<INT>(&m_type));

	if (nullptr != m_pdrgpcrOld)
	{
		ulHash =
			gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcrOld));
	}

	if (nullptr != m_pdrgpcrNew)
	{
		ulHash =
			gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcrNew));
	}

	return ulHash;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalRowTrigger::Matches
//
//	@doc:
//		Match operator
//
//---------------------------------------------------------------------------
BOOL
CPhysicalRowTrigger::Matches(gpos::pointer<COperator *> pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	gpos::pointer<CPhysicalRowTrigger *> popRowTrigger =
		gpos::dyn_cast<CPhysicalRowTrigger>(pop);

	gpos::pointer<CColRefArray *> pdrgpcrOld = popRowTrigger->PdrgpcrOld();
	gpos::pointer<CColRefArray *> pdrgpcrNew = popRowTrigger->PdrgpcrNew();

	return m_rel_mdid->Equals(popRowTrigger->GetRelMdId()) &&
		   m_type == popRowTrigger->GetType() &&
		   CUtils::Equals(m_pdrgpcrOld, pdrgpcrOld) &&
		   CUtils::Equals(m_pdrgpcrNew, pdrgpcrNew);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalRowTrigger::EpetRewindability
//
//	@doc:
//		Return the enforcing type for rewindability property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalRowTrigger::EpetRewindability(
	CExpressionHandle &exprhdl,
	gpos::pointer<const CEnfdRewindability *> per) const
{
	gpos::pointer<CRewindabilitySpec *> prs =
		gpos::dyn_cast<CDrvdPropPlan>(exprhdl.Pdp())->Prs();
	if (per->FCompatible(prs))
	{
		// required rewindability is already provided
		return CEnfdProp::EpetUnnecessary;
	}

	// always force spool to be on top of trigger
	return CEnfdProp::EpetRequired;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalRowTrigger::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CPhysicalRowTrigger::OsPrint(IOstream &os) const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}

	os << SzId() << " (Type: " << m_type << ")";

	if (nullptr != m_pdrgpcrOld)
	{
		os << ", Old Columns: [";
		CUtils::OsPrintDrgPcr(os, m_pdrgpcrOld);
		os << "]";
	}

	if (nullptr != m_pdrgpcrNew)
	{
		os << ", New Columns: [";
		CUtils::OsPrintDrgPcr(os, m_pdrgpcrNew);
		os << "]";
	}

	return os;
}


// EOF
