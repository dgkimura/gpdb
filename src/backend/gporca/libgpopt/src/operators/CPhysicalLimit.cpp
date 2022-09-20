//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CPhysicalLimit.cpp
//
//	@doc:
//		Implementation of limit operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalLimit.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpecAny.h"
#include "gpopt/base/CDistributionSpecReplicated.h"
#include "gpopt/base/CDistributionSpecSingleton.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"


using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLimit::CPhysicalLimit
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalLimit::CPhysicalLimit(CMemoryPool *mp, gpos::owner<COrderSpec *> pos,
							   BOOL fGlobal, BOOL fHasCount,
							   BOOL fTopLimitUnderDML)
	: CPhysical(mp),
	  m_pos(std::move(pos)),
	  m_fGlobal(fGlobal),
	  m_fHasCount(fHasCount),
	  m_top_limit_under_dml(fTopLimitUnderDML),
	  m_pcrsSort(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != m_pos);

	m_pcrsSort = m_pos->PcrsUsed(mp);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLimit::~CPhysicalLimit
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPhysicalLimit::~CPhysicalLimit()
{
	m_pos->Release();
	m_pcrsSort->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLimit::Matches
//
//	@doc:
//		Match operators
//
//---------------------------------------------------------------------------
BOOL
CPhysicalLimit::Matches(gpos::pointer<COperator *> pop) const
{
	if (pop->Eopid() == Eopid())
	{
		gpos::pointer<CPhysicalLimit *> popLimit =
			gpos::dyn_cast<CPhysicalLimit>(pop);

		if (popLimit->FGlobal() == m_fGlobal &&
			popLimit->FHasCount() == m_fHasCount)
		{
			// match if order specs match
			return m_pos->Matches(popLimit->m_pos);
		}
	}

	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLimit::PcrsRequired
//
//	@doc:
//		Columns required by Limit's relational child
//
//---------------------------------------------------------------------------
gpos::owner<CColRefSet *>
CPhysicalLimit::PcrsRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
							 gpos::pointer<CColRefSet *> pcrsRequired,
							 ULONG child_index,
							 gpos::pointer<CDrvdPropArray *>,  // pdrgpdpCtxt
							 ULONG							   // ulOptReq
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
//		CPhysicalLimit::PosRequired
//
//	@doc:
//		Compute required sort order of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<COrderSpec *>
CPhysicalLimit::PosRequired(CMemoryPool *,				  // mp
							CExpressionHandle &,		  // exprhdl
							gpos::pointer<COrderSpec *>,  // posInput
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

	// limit requires its internal order spec to be satisfied by its child;
	// an input required order to the limit operator is always enforced on
	// top of limit
	m_pos->AddRef();

	return m_pos;
}

gpos::owner<CDistributionSpec *>
CPhysicalLimit::PdsRequired(CMemoryPool *, CExpressionHandle &,
							gpos::pointer<CDistributionSpec *>, ULONG,
							gpos::pointer<CDrvdPropArray *>, ULONG) const
{
	// FIXME: this method will (and should) _never_ be called
	// sweep through all 38 overrides of PdsRequired and switch to Ped()
	GPOS_RAISE(
		CException::ExmaInvalid, CException::ExmiInvalid,
		GPOS_WSZ_LIT("PdsRequired should not be called for CPhysicalLimit"));
	return nullptr;
}

gpos::owner<CEnfdDistribution *>
CPhysicalLimit::Ped(CMemoryPool *mp, CExpressionHandle &exprhdl,
					gpos::pointer<CReqdPropPlan *> prppInput, ULONG child_index,
					gpos::pointer<CDrvdPropArray *>,  // pdrgpdpCtxt
					ULONG							  // ulDistrReq
)
{
	GPOS_ASSERT(0 == child_index);

	gpos::pointer<CDistributionSpec *> const pdsInput =
		prppInput->Ped()->PdsRequired();

	if (FGlobal())
	{
		// TODO:  - Mar 19, 2012; Cleanup: move this check to the caller
		if (exprhdl.HasOuterRefs())
		{
			return GPOS_NEW(mp) CEnfdDistribution(
				PdsPassThru(mp, exprhdl, pdsInput, child_index),
				CEnfdDistribution::EdmSatisfy);
		}

		gpos::pointer<CExpression *> pexprOffset =
			exprhdl.PexprScalarExactChild(1 /*child_index*/,
										  true /*error_on_null_return*/);
		if (!m_fHasCount && CUtils::FScalarConstIntZero(pexprOffset))
		{
			// pass through input distribution if it has no count nor offset and is not
			// a singleton
			if (CDistributionSpec::EdtSingleton != pdsInput->Edt() &&
				CDistributionSpec::EdtStrictSingleton != pdsInput->Edt())
			{
				return GPOS_NEW(mp) CEnfdDistribution(
					PdsPassThru(mp, exprhdl, pdsInput, child_index),
					CEnfdDistribution::EdmSatisfy);
			}

			return GPOS_NEW(mp) CEnfdDistribution(
				GPOS_NEW(mp) CDistributionSpecAny(this->Eopid()),
				CEnfdDistribution::EdmSatisfy);
		}
		if (CDistributionSpec::EdtSingleton == pdsInput->Edt())
		{
			// pass through input distribution if it is a singleton (and it has count or offset)
			return GPOS_NEW(mp) CEnfdDistribution(
				PdsPassThru(mp, exprhdl, pdsInput, child_index),
				CEnfdDistribution::EdmSatisfy);
		}

		// otherwise, require a singleton explicitly
		return GPOS_NEW(mp)
			CEnfdDistribution(GPOS_NEW(mp) CDistributionSpecSingleton(),
							  CEnfdDistribution::EdmSatisfy);
	}

	// if expression has to execute on a single host then we need a gather
	if (exprhdl.NeedsSingletonExecution())
	{
		return GPOS_NEW(mp) CEnfdDistribution(
			PdsRequireSingleton(mp, exprhdl, pdsInput, child_index),
			CEnfdDistribution::EdmSatisfy);
	}

	// no local limits are generated if there are outer references, so if this
	// is a local limit, there should be no outer references
	GPOS_ASSERT(0 == exprhdl.DeriveOuterReferences()->Size());

	// for local limit, we impose no distribution requirements
	return GPOS_NEW(mp)
		CEnfdDistribution(GPOS_NEW(mp) CDistributionSpecAny(this->Eopid()),
						  CEnfdDistribution::EdmSatisfy);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLimit::PrsRequired
//
//	@doc:
//		Compute required rewindability of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<CRewindabilitySpec *>
CPhysicalLimit::PrsRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
							gpos::pointer<CRewindabilitySpec *> prsRequired,
							ULONG child_index,
							gpos::pointer<CDrvdPropArray *>,  // pdrgpdpCtxt
							ULONG							  // ulOptReq
) const
{
	GPOS_ASSERT(0 == child_index);

	if (exprhdl.HasOuterRefs())
	{
		// If the Limit op or its subtree contains an outer ref, then it must
		// request rewindability with a motion hazard (a Blocking Spool) from its
		// subtree. Otherwise, if a streaming Spool is added to the subtree, it will
		// only return tuples it materialized in its first execution (i.e with the
		// first value of the outer ref) for every re-execution. This can produce
		// wrong results.
		// E.g select *, (select 1 from generate_series(1, 10) limit t1.a) from t1;
		return GPOS_NEW(mp) CRewindabilitySpec(prsRequired->Ert(),
											   CRewindabilitySpec::EmhtMotion);
	}

	return PrsPassThru(mp, exprhdl, prsRequired, child_index);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLimit::PcteRequired
//
//	@doc:
//		Compute required CTE map of the n-th child
//
//---------------------------------------------------------------------------
gpos::owner<CCTEReq *>
CPhysicalLimit::PcteRequired(CMemoryPool *,		   //mp,
							 CExpressionHandle &,  //exprhdl,
							 gpos::pointer<CCTEReq *> pcter,
							 ULONG
#ifdef GPOS_DEBUG
								 child_index
#endif
							 ,
							 gpos::pointer<CDrvdPropArray *>,  //pdrgpdpCtxt,
							 ULONG							   //ulOptReq
) const
{
	GPOS_ASSERT(0 == child_index);
	return PcterPushThru(pcter);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLimit::FProvidesReqdCols
//
//	@doc:
//		Check if required columns are included in output columns
//
//---------------------------------------------------------------------------
BOOL
CPhysicalLimit::FProvidesReqdCols(CExpressionHandle &exprhdl,
								  gpos::pointer<CColRefSet *> pcrsRequired,
								  ULONG	 // ulOptReq
) const
{
	return FUnaryProvidesReqdCols(exprhdl, pcrsRequired);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLimit::PosDerive
//
//	@doc:
//		Derive sort order
//
//---------------------------------------------------------------------------
gpos::owner<COrderSpec *>
CPhysicalLimit::PosDerive(CMemoryPool *,	   // mp
						  CExpressionHandle &  // exprhdl
) const
{
	m_pos->AddRef();

	return m_pos;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLimit::PdsDerive
//
//	@doc:
//		Derive distribution
//
//---------------------------------------------------------------------------
gpos::owner<CDistributionSpec *>
CPhysicalLimit::PdsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl) const
{
	gpos::pointer<CDistributionSpec *> pdsOuter = exprhdl.Pdpplan(0)->Pds();

	if (CDistributionSpec::EdtStrictReplicated == pdsOuter->Edt())
	{
		// Limit functions can give unstable results and therefore cannot
		// guarantee strictly replicated data. For example,
		//
		//   SELECT * FROM foo WHERE a<>1 LIMIT 1;
		//
		// In this case, if the child was replicated, we can no longer
		// guarantee that property and must now derive tainted replicated.
		return GPOS_NEW(mp) CDistributionSpecReplicated(
			CDistributionSpec::EdtTaintedReplicated);
	}
	else
	{
		pdsOuter->AddRef();
		return pdsOuter;
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLimit::PrsDerive
//
//	@doc:
//		Derive rewindability
//
//---------------------------------------------------------------------------
gpos::owner<CRewindabilitySpec *>
CPhysicalLimit::PrsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl) const
{
	return PrsDerivePassThruOuter(mp, exprhdl);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLimit::EpetOrder
//
//	@doc:
//		Return the enforcing type for order property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalLimit::EpetOrder(CExpressionHandle &,	// exprhdl
						  gpos::pointer<const CEnfdOrder *> peo) const
{
	GPOS_ASSERT(nullptr != peo);
	GPOS_ASSERT(!peo->PosRequired()->IsEmpty());

	if (peo->FCompatible(m_pos))
	{
		// required order will be established by the limit operator
		return CEnfdProp::EpetUnnecessary;
	}

	// required order will be enforced on limit's output
	return CEnfdProp::EpetRequired;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLimit::EpetDistribution
//
//	@doc:
//		Return the enforcing type for distribution property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalLimit::EpetDistribution(
	CExpressionHandle &exprhdl,
	gpos::pointer<const CEnfdDistribution *> ped) const
{
	GPOS_ASSERT(nullptr != ped);

	// get distribution delivered by the limit node
	gpos::pointer<CDistributionSpec *> pds =
		gpos::dyn_cast<CDrvdPropPlan>(exprhdl.Pdp())->Pds();

	if (ped->FCompatible(pds))
	{
		if (m_fGlobal)
		{
			return CEnfdProp::EpetUnnecessary;
		}

		// prohibit the plan if local limit already delivers the enforced distribution, since
		// otherwise we would create two limits with no intermediate motion operators
		return CEnfdProp::EpetProhibited;
	}

	return CEnfdProp::EpetRequired;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLimit::EpetRewindability
//
//	@doc:
//		Return the enforcing type for rewindability property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalLimit::EpetRewindability(
	CExpressionHandle &,					   // exprhdl
	gpos::pointer<const CEnfdRewindability *>  // per
) const
{
	// rewindability is preserved on operator's output
	return CEnfdProp::EpetOptional;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalLimit::OsPrint
//
//	@doc:
//		Print
//
//---------------------------------------------------------------------------
IOstream &
CPhysicalLimit::OsPrint(IOstream &os) const
{
	os << SzId() << " " << (*m_pos) << " " << (m_fGlobal ? "global" : "local");

	return os;
}



// EOF
