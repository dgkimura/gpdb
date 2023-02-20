//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CPhysicalInnerHashJoin.cpp
//
//	@doc:
//		Implementation of inner hash join operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalInnerHashJoin.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpecHashed.h"
#include "gpopt/base/CDrvdPropCtxtPlan.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/search/CGroupExpression.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalInnerHashJoin::CPhysicalInnerHashJoin
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalInnerHashJoin::CPhysicalInnerHashJoin(
	CMemoryPool *mp, gpos::Ref<CExpressionArray> pdrgpexprOuterKeys,
	gpos::Ref<CExpressionArray> pdrgpexprInnerKeys,
	gpos::Ref<IMdIdArray> hash_opfamilies, CXform::EXformId origin_xform)
	: CPhysicalHashJoin(mp, std::move(pdrgpexprOuterKeys),
						std::move(pdrgpexprInnerKeys),
						std::move(hash_opfamilies), origin_xform)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalInnerHashJoin::~CPhysicalInnerHashJoin
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPhysicalInnerHashJoin::~CPhysicalInnerHashJoin() = default;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalInnerHashJoin::PdshashedCreateMatching
//
//	@doc:
//		Helper function for creating a matching hashed distribution
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpecHashed>
CPhysicalInnerHashJoin::PdshashedCreateMatching(
	CMemoryPool *mp, gpos::Ref<CDistributionSpecHashed> pdshashed,
	ULONG
		ulSourceChild  // index of child that delivered the given hashed distribution
) const
{
	GPOS_ASSERT(nullptr != pdshashed);

	gpos::Ref<CDistributionSpecHashed> pdshashedMatching =
		PdshashedMatching(mp, pdshashed, ulSourceChild);

	// create a new spec with input and the output spec as equivalents, as you don't want to lose
	// the already existing equivalent specs of pdshashed
	// NB: The matching spec is added at the beginning.
	;
	;
	if (nullptr != pdshashedMatching->Opfamilies())
	{
		;
	}
	gpos::Ref<CDistributionSpecHashed> pdsHashedMatchingEquivalents =
		GPOS_NEW(mp) CDistributionSpecHashed(
			pdshashedMatching->Pdrgpexpr(),
			pdshashedMatching->FNullsColocated(),
			pdshashed,	// matching distribution spec is equivalent to passed distribution spec
			pdshashedMatching->Opfamilies());
	;
	return pdsHashedMatchingEquivalents;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalInnerHashJoin::PdsDeriveFromHashedChildren
//
//	@doc:
//		Derive hash join distribution from hashed children;
//		return nullptr if derivation failed
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpec>
CPhysicalInnerHashJoin::PdsDeriveFromHashedChildren(
	CMemoryPool *mp, gpos::Ref<CDistributionSpec> pdsOuter,
	gpos::Ref<CDistributionSpec> pdsInner) const
{
	GPOS_ASSERT(nullptr != pdsOuter);
	GPOS_ASSERT(nullptr != pdsInner);

	gpos::Ref<CDistributionSpecHashed> pdshashedOuter =
		gpos::dyn_cast<CDistributionSpecHashed>(pdsOuter);
	gpos::Ref<CDistributionSpecHashed> pdshashedInner =
		gpos::dyn_cast<CDistributionSpecHashed>(pdsInner);

	if (pdshashedOuter->IsCoveredBy(PdrgpexprOuterKeys()) &&
		pdshashedInner->IsCoveredBy(PdrgpexprInnerKeys()))
	{
		// if both sides are hashed on subsets of hash join keys, join's output can be
		// seen as distributed on outer spec or (equivalently) on inner spec,
		// so create a new spec and mark outer and inner as equivalent
		gpos::Ref<CDistributionSpecHashed> combined_hashed_spec =
			pdshashedOuter->Combine(mp, pdshashedInner);
		return combined_hashed_spec;
	}

	return nullptr;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalInnerHashJoin::PdsDeriveFromReplicatedOuter
//
//	@doc:
//		Derive hash join distribution from a replicated outer child;
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpec>
CPhysicalInnerHashJoin::PdsDeriveFromReplicatedOuter(
	CMemoryPool *mp,
	gpos::Ref<CDistributionSpec>
#ifdef GPOS_DEBUG
		pdsOuter
#endif	// GPOS_DEBUG
	,
	gpos::Ref<CDistributionSpec> pdsInner) const
{
	GPOS_ASSERT(nullptr != pdsOuter);
	GPOS_ASSERT(nullptr != pdsInner);
	GPOS_ASSERT(CDistributionSpec::EdtStrictReplicated == pdsOuter->Edt());

	// if outer child is replicated, join results distribution is defined by inner child
	if (CDistributionSpec::EdtHashed == pdsInner->Edt())
	{
		gpos::Ref<CDistributionSpecHashed> pdshashedInner =
			gpos::dyn_cast<CDistributionSpecHashed>(pdsInner);
		if (pdshashedInner->IsCoveredBy(PdrgpexprInnerKeys()))
		{
			// inner child is hashed on a subset of inner hashkeys,
			// return a hashed distribution equivalent to a matching outer distribution
			return PdshashedCreateMatching(mp, pdshashedInner,
										   1 /*ulSourceChild*/);
		}
	}

	// otherwise, pass-through inner distribution
	;
	return pdsInner;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalInnerHashJoin::PdsDeriveFromHashedOuter
//
//	@doc:
//		Derive hash join distribution from a hashed outer child;
//		return nullptr if derivation failed
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpec>
CPhysicalInnerHashJoin::PdsDeriveFromHashedOuter(
	CMemoryPool *mp, gpos::Ref<CDistributionSpec> pdsOuter,
	gpos::Ref<CDistributionSpec>
#ifdef GPOS_DEBUG
		pdsInner
#endif	// GPOS_DEBUG
) const
{
	GPOS_ASSERT(nullptr != pdsOuter);
	GPOS_ASSERT(nullptr != pdsInner);

	GPOS_ASSERT(CDistributionSpec::EdtHashed == pdsOuter->Edt());

	gpos::Ref<CDistributionSpecHashed> pdshashedOuter =
		gpos::dyn_cast<CDistributionSpecHashed>(pdsOuter);
	if (pdshashedOuter->IsCoveredBy(PdrgpexprOuterKeys()))
	{
		// outer child is hashed on a subset of outer hashkeys,
		// return a hashed distribution equivalent to a matching outer distribution
		return PdshashedCreateMatching(mp, pdshashedOuter, 0 /*ulSourceChild*/);
	}

	return nullptr;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalInnerHashJoin::PdsDerive
//
//	@doc:
//		Derive distribution
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpec>
CPhysicalInnerHashJoin::PdsDerive(CMemoryPool *mp,
								  CExpressionHandle &exprhdl) const
{
	gpos::Ref<CDistributionSpec> pdsOuter =
		exprhdl.Pdpplan(0 /*child_index*/)->Pds();
	gpos::Ref<CDistributionSpec> pdsInner =
		exprhdl.Pdpplan(1 /*child_index*/)->Pds();

	if (CDistributionSpec::EdtUniversal == pdsOuter->Edt())
	{
		// if outer is universal, pass through inner distribution
		;
		return pdsInner;
	}

	if (CDistributionSpec::EdtHashed == pdsOuter->Edt() &&
		CDistributionSpec::EdtHashed == pdsInner->Edt())
	{
		gpos::Ref<CDistributionSpec> pdsDerived =
			PdsDeriveFromHashedChildren(mp, pdsOuter, pdsInner);
		if (nullptr != pdsDerived)
		{
			return pdsDerived;
		}
	}

	if (CDistributionSpec::EdtStrictReplicated == pdsOuter->Edt())
	{
		return PdsDeriveFromReplicatedOuter(mp, pdsOuter, pdsInner);
	}

	if (CDistributionSpec::EdtHashed == pdsOuter->Edt())
	{
		gpos::Ref<CDistributionSpec> pdsDerived =
			PdsDeriveFromHashedOuter(mp, pdsOuter, pdsInner);
		if (nullptr != pdsDerived)
		{
			return pdsDerived;
		}
	}

	// otherwise, pass through outer distribution
	;
	return pdsOuter;
}

gpos::Ref<CExpression>
PexprJoinPredOnPartKeys(CMemoryPool *mp, gpos::Ref<CExpression> pexprScalar,
						CPartKeysArray *pdrgppartkeys,
						gpos::Ref<CColRefSet> pcrsAllowedRefs)
{
	GPOS_ASSERT(nullptr != pcrsAllowedRefs);

	gpos::Ref<CExpression> pexprPred = nullptr;
	for (ULONG ulKey = 0; nullptr == pexprPred && ulKey < pdrgppartkeys->Size();
		 ulKey++)
	{
		// get partition key
		CColRef2dArray *pdrgpdrgpcrPartKeys =
			(*pdrgppartkeys)[ulKey]->Pdrgpdrgpcr();

		// try to generate a request with dynamic partition selection
		pexprPred = CPredicateUtils::PexprExtractPredicatesOnPartKeys(
			mp, pexprScalar, pdrgpdrgpcrPartKeys, pcrsAllowedRefs,
			true  // fUseConstraints
		);
	}

	return pexprPred;
}

gpos::Ref<CPartitionPropagationSpec>
CPhysicalInnerHashJoin::PppsRequired(CMemoryPool *mp,
									 CExpressionHandle &exprhdl,
									 CPartitionPropagationSpec *pppsRequired,
									 ULONG child_index,
									 gpos::Ref<CDrvdPropArray> pdrgpdpCtxt,
									 ULONG ulOptReq) const
{
	GPOS_ASSERT(nullptr != pppsRequired);
	GPOS_ASSERT(nullptr != pdrgpdpCtxt);

	gpos::Ref<CExpression> pexprScalar =
		exprhdl.PexprScalarExactChild(2 /*child_index*/);

	// gpos::Ref<CColRefSet> pcrsOutputOuter = exprhdl.DeriveOutputColumns(0);
	gpos::Ref<CColRefSet> pcrsOutputInner = exprhdl.DeriveOutputColumns(1);

	// gpos::Ref<CPartInfo> part_info_outer = exprhdl.DerivePartitionInfo(0);
	// gpos::Ref<CPartInfo> part_info_inner = exprhdl.DerivePartitionInfo(1);

	gpos::Ref<CPartitionPropagationSpec> pps_result;
	if (ulOptReq == 0)
	{
		// DPE: create a new request
		pps_result = GPOS_NEW(mp) CPartitionPropagationSpec(mp);
		gpos::Ref<CPartInfo> part_info_outer = exprhdl.DerivePartitionInfo(0);
		for (ULONG ul = 0; ul < part_info_outer->UlConsumers(); ++ul)
		{
			ULONG scan_id = part_info_outer->ScanId(ul);
			gpos::Ref<IMDId> rel_mdid = part_info_outer->GetRelMdId(ul);
			CPartKeysArray *part_keys_array =
				part_info_outer->Pdrgppartkeys(ul);

			gpos::Ref<CExpression> pexprCmp =
				PexprJoinPredOnPartKeys(mp, pexprScalar, part_keys_array,
										pcrsOutputInner /* pcrsAllowedRefs*/);
			if (pexprCmp == nullptr)
			{
				continue;
			}

			if (child_index == 0)
			{
				CPartitionPropagationSpec *pps_inner =
					gpos::dyn_cast<CDrvdPropPlan>((*pdrgpdpCtxt)[0])->Ppps();

				gpos::Ref<CBitSet> selector_ids =
					GPOS_NEW(mp) CBitSet(mp, *pps_inner->SelectorIds(scan_id));
				pps_result->Insert(
					scan_id, CPartitionPropagationSpec::EpptConsumer, rel_mdid,
					selector_ids.get(), nullptr /* expr */);
				;
			}
			else
			{
				GPOS_ASSERT(child_index == 1);
				pps_result->Insert(scan_id,
								   CPartitionPropagationSpec::EpptPropagator,
								   rel_mdid, nullptr, pexprCmp.get());
			};
		}

		gpos::Ref<CBitSet> allowed_scan_ids = GPOS_NEW(mp) CBitSet(mp);
		gpos::Ref<CPartInfo> part_info =
			exprhdl.DerivePartitionInfo(child_index);
		for (ULONG ul = 0; ul < part_info->UlConsumers(); ++ul)
		{
			ULONG scan_id = part_info->ScanId(ul);
			allowed_scan_ids->ExchangeSet(scan_id);
		}

		pps_result->InsertAllowedConsumers(pppsRequired,
										   allowed_scan_ids.get());
		;
	}
	else
	{
		// No DPE: pass through requests
		pps_result = CPhysical::PppsRequired(
			mp, exprhdl, pppsRequired, child_index, pdrgpdpCtxt, ulOptReq);
	}
	return pps_result;
}

gpos::Ref<CPartitionPropagationSpec>
CPhysicalInnerHashJoin::PppsDerive(CMemoryPool *mp,
								   CExpressionHandle &exprhdl) const
{
	CPartitionPropagationSpec *pps_outer = exprhdl.Pdpplan(0)->Ppps();
	CPartitionPropagationSpec *pps_inner = exprhdl.Pdpplan(1)->Ppps();

	gpos::Ref<CPartitionPropagationSpec> pps_result =
		GPOS_NEW(mp) CPartitionPropagationSpec(mp);
	pps_result->InsertAll(pps_outer);
	pps_result->InsertAllResolve(pps_inner);

	return pps_result;
}

// EOF
