//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalHashJoin.cpp
//
//	@doc:
//		Implementation of hash join operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalHashJoin.h"

#include "gpos/base.h"

#include "gpopt/base/CCastUtils.h"
#include "gpopt/base/CDistributionSpecHashed.h"
#include "gpopt/base/CDistributionSpecNonSingleton.h"
#include "gpopt/base/CDistributionSpecReplicated.h"
#include "gpopt/base/CDistributionSpecSingleton.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/operators/CScalarIdent.h"

using namespace gpopt;

// number of non-redistribute requests created by hash join
#define GPOPT_NON_HASH_DIST_REQUESTS 3

// maximum number of redistribute requests on single hash join keys
#define GPOPT_MAX_HASH_DIST_REQUESTS 6

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::CPhysicalHashJoin
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalHashJoin::CPhysicalHashJoin(
	CMemoryPool *mp, gpos::Ref<CExpressionArray> pdrgpexprOuterKeys,
	gpos::Ref<CExpressionArray> pdrgpexprInnerKeys,
	gpos::Ref<IMdIdArray> hash_opfamilies, BOOL is_null_aware,
	CXform::EXformId origin_xform)
	: CPhysicalJoin(mp, origin_xform),
	  m_pdrgpexprOuterKeys(pdrgpexprOuterKeys),
	  m_pdrgpexprInnerKeys(pdrgpexprInnerKeys),
	  m_hash_opfamilies(nullptr),
	  m_is_null_aware(is_null_aware),
	  m_pdrgpdsRedistributeRequests(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pdrgpexprOuterKeys);
	GPOS_ASSERT(nullptr != pdrgpexprInnerKeys);
	GPOS_ASSERT(pdrgpexprOuterKeys->Size() == pdrgpexprInnerKeys->Size());

	if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution))
	{
		GPOS_ASSERT(nullptr != hash_opfamilies);
		m_hash_opfamilies = hash_opfamilies;
		GPOS_ASSERT(pdrgpexprOuterKeys->Size() == m_hash_opfamilies->Size());
	}

	CreateOptRequests(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::~CPhysicalHashJoin
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPhysicalHashJoin::~CPhysicalHashJoin()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::CreateHashRedistributeRequests
//
//	@doc:
//		Create the set of redistribute requests to send to first
//		hash join child
//
//---------------------------------------------------------------------------
void
CPhysicalHashJoin::CreateHashRedistributeRequests(CMemoryPool *mp)
{
	GPOS_ASSERT(nullptr == m_pdrgpdsRedistributeRequests);
	GPOS_ASSERT(nullptr != m_pdrgpexprOuterKeys);
	GPOS_ASSERT(nullptr != m_pdrgpexprInnerKeys);

	gpos::Ref<CExpressionArray> pdrgpexpr = nullptr;
	if (EceoRightToLeft == Eceo())
	{
		pdrgpexpr = m_pdrgpexprInnerKeys;
	}
	else
	{
		pdrgpexpr = m_pdrgpexprOuterKeys;
	}

	m_pdrgpdsRedistributeRequests = GPOS_NEW(mp) CDistributionSpecArray(mp);
	const ULONG ulExprs =
		std::min((ULONG) GPOPT_MAX_HASH_DIST_REQUESTS, pdrgpexpr->Size());
	if (1 < ulExprs)
	{
		for (ULONG ul = 0; ul < ulExprs; ul++)
		{
			gpos::Ref<CExpressionArray> pdrgpexprCurrent =
				GPOS_NEW(mp) CExpressionArray(mp);
			gpos::Ref<CExpression> pexpr = (*pdrgpexpr)[ul];

			pdrgpexprCurrent->Append(pexpr);

			gpos::Ref<IMdIdArray> opfamilies = nullptr;
			if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution))
			{
				GPOS_ASSERT(nullptr != m_hash_opfamilies);
				opfamilies = GPOS_NEW(mp) IMdIdArray(mp);
				gpos::Ref<IMDId> opfamily = (*m_hash_opfamilies)[ul];

				opfamilies->Append(opfamily);
			}

			// add a separate request for each hash join key

			gpos::Ref<CDistributionSpecHashed> pdshashedCurrent =
				GPOS_NEW(mp) CDistributionSpecHashed(
					pdrgpexprCurrent, true /* fNullsCollocated */, opfamilies);
			m_pdrgpdsRedistributeRequests->Append(pdshashedCurrent);
		}
	}
	// add a request that contains all hash join keys

	if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution))
	{
		GPOS_ASSERT(nullptr != m_hash_opfamilies);
	}
	gpos::Ref<CDistributionSpecHashed> pdshashed =
		GPOS_NEW(mp) CDistributionSpecHashed(
			pdrgpexpr, true /* fNullsCollocated */, m_hash_opfamilies);
	m_pdrgpdsRedistributeRequests->Append(pdshashed);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::PosRequired
//
//	@doc:
//		Compute required sort order of the n-th child
//
//---------------------------------------------------------------------------
gpos::Ref<COrderSpec>
CPhysicalHashJoin::PosRequired(CMemoryPool *mp,
							   CExpressionHandle &,	   //exprhdl
							   gpos::Ref<COrderSpec>,  // posInput,
							   ULONG
#ifdef GPOS_DEBUG
								   child_index
#endif	// GPOS_DEBUG
							   ,
							   gpos::Ref<CDrvdPropArray>,  // pdrgpdpCtxt
							   ULONG					   // ulOptReq
) const
{
	GPOS_ASSERT(
		child_index < 2 &&
		"Required sort order can be computed on the relational child only");

	// hash join does not have order requirements to both children, and it
	// does not preserve any sort order
	return GPOS_NEW(mp) COrderSpec(mp);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::PrsRequired
//
//	@doc:
//		Compute required rewindability of the n-th child
//
//---------------------------------------------------------------------------
gpos::Ref<CRewindabilitySpec>
CPhysicalHashJoin::PrsRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
							   gpos::Ref<CRewindabilitySpec> prsRequired,
							   ULONG child_index,
							   gpos::Ref<CDrvdPropArray>,  // pdrgpdpCtxt
							   ULONG					   // ulOptReq
) const
{
	GPOS_ASSERT(
		child_index < 2 &&
		"Required rewindability can be computed on the relational child only");

	if (1 == child_index)
	{
		// If the inner child contains outer references, and the required
		// rewindability is not ErtNone, we must ensure that the inner subtree is
		// at least rescannable, even though a Hash op on the inner side
		// materialized the subtree results.
		if (exprhdl.HasOuterRefs(1) &&
			(prsRequired->Ert() == CRewindabilitySpec::ErtRescannable ||
			 prsRequired->Ert() == CRewindabilitySpec::ErtRewindable))
		{
			return GPOS_NEW(mp) CRewindabilitySpec(
				CRewindabilitySpec::ErtRescannable, prsRequired->Emht());
		}
		// Otherwise, the inner Hash op will take care of materializing the
		// subtree, so no rewindability type is required
		else
		{
			return GPOS_NEW(mp) CRewindabilitySpec(CRewindabilitySpec::ErtNone,
												   prsRequired->Emht());
		}
	}

	// pass through requirements to outer child
	return PrsPassThru(mp, exprhdl, prsRequired, 0 /*child_index*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::PdsMatch
//
//	@doc:
//		Compute a distribution matching the distribution delivered by
//		given child
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpec>
CPhysicalHashJoin::PdsMatch(CMemoryPool *mp, gpos::Ref<CDistributionSpec> pds,
							ULONG ulSourceChildIndex) const
{
	GPOS_ASSERT(nullptr != pds);

	EChildExecOrder eceo = Eceo();

	// check the type of distribution delivered by first (inner) child
	switch (pds->Edt())
	{
		case CDistributionSpec::EdtUniversal:
			// first child is universal, request second child to execute on a single host to avoid duplicates
			return GPOS_NEW(mp) CDistributionSpecSingleton();

		case CDistributionSpec::EdtSingleton:
		case CDistributionSpec::EdtStrictSingleton:
			// require second child to provide a matching singleton distribution
			return PdssMatching(mp,
								CDistributionSpecSingleton::PdssConvert(pds));

		case CDistributionSpec::EdtHashed:
			// require second child to provide a matching hashed distribution
			return PdshashedMatching(mp,
									 CDistributionSpecHashed::PdsConvert(pds),
									 ulSourceChildIndex);

		default:
			GPOS_ASSERT(CDistributionSpec::EdtStrictReplicated == pds->Edt() ||
						CDistributionSpec::EdtTaintedReplicated == pds->Edt());

			if (EceoRightToLeft == eceo)
			{
				GPOS_ASSERT(1 == ulSourceChildIndex);

				// inner child is replicated, for ROJ outer must be executed on a single (non-coordinator) segment to avoid duplicates
				if (this->Eopid() == EopPhysicalRightOuterHashJoin)
				{
					return GPOS_NEW(mp) CDistributionSpecSingleton(
						CDistributionSpecSingleton::EstSegment);
				}
				// inner child is replicated, request outer child to have non-singleton distribution
				return GPOS_NEW(mp) CDistributionSpecNonSingleton();
			}

			GPOS_ASSERT(0 == ulSourceChildIndex);

			// outer child is replicated, replicate inner child too in order to preserve correctness of semi-join
			return GPOS_NEW(mp) CDistributionSpecReplicated(
				CDistributionSpec::EdtStrictReplicated);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::PdshashedMatching
//
//	@doc:
//		Compute a hashed distribution matching the given distribution
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpecHashed>
CPhysicalHashJoin::PdshashedMatching(
	CMemoryPool *mp, gpos::Ref<CDistributionSpecHashed> pdshashed,
	ULONG
		ulSourceChild  // index of child that delivered the given hashed distribution
) const
{
	GPOS_ASSERT(2 > ulSourceChild);

	gpos::Ref<CExpressionArray> pdrgpexprSource = m_pdrgpexprOuterKeys;
	gpos::Ref<CExpressionArray> pdrgpexprTarget = m_pdrgpexprInnerKeys;
	if (1 == ulSourceChild)
	{
		pdrgpexprSource = m_pdrgpexprInnerKeys;
		pdrgpexprTarget = m_pdrgpexprOuterKeys;
	}

	const gpos::Ref<CExpressionArray> pdrgpexprDist = pdshashed->Pdrgpexpr();
	const ULONG ulDlvrdSize = pdrgpexprDist->Size();
	const ULONG ulSourceSize = pdrgpexprSource->Size();

	// construct an array of target key expressions matching source key expressions
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	gpos::Ref<CExpressionArrays> all_equiv_exprs =
		pdshashed->HashSpecEquivExprs();
	gpos::Ref<IMdIdArray> opfamilies = nullptr;

	if (GPOS_FTRACE(EopttraceConsiderOpfamiliesForDistribution))
	{
		opfamilies = GPOS_NEW(mp) IMdIdArray(mp);
	}

	for (ULONG ulDlvrdIdx = 0; ulDlvrdIdx < ulDlvrdSize; ulDlvrdIdx++)
	{
		gpos::Ref<CExpression> pexprDlvrd = (*pdrgpexprDist)[ulDlvrdIdx];
		gpos::Ref<CExpressionArray> equiv_distribution_exprs = nullptr;
		if (nullptr != all_equiv_exprs && all_equiv_exprs->Size() > 0)
		{
			equiv_distribution_exprs = (*all_equiv_exprs)[ulDlvrdIdx];
		}
		for (ULONG idx = 0; idx < ulSourceSize; idx++)
		{
			BOOL fSuccess = false;
			gpos::Ref<CExpression> source_expr = (*pdrgpexprSource)[idx];
			fSuccess = CUtils::Equals(pexprDlvrd, source_expr);
			if (!fSuccess)
			{
				// if failed to find a equal match in the source distribution expr
				// array, check the equivalent exprs to find a match
				fSuccess =
					CUtils::Contains(equiv_distribution_exprs, source_expr);
			}
			if (fSuccess)
			{
				// TODO: 02/21/2012 - ; source column may be mapped to multiple
				// target columns (e.g. i=j and i=k);
				// in this case, we need to generate multiple optimization requests to the target child
				gpos::Ref<CExpression> pexprTarget = (*pdrgpexprTarget)[idx];

				pdrgpexpr->Append(pexprTarget);

				if (nullptr != opfamilies)
				{
					GPOS_ASSERT(nullptr != m_hash_opfamilies);
					gpos::Ref<IMDId> opfamily = (*m_hash_opfamilies)[idx];

					opfamilies->Append(opfamily);
				}
				break;
			}
		}
	}
	// check if we failed to compute required distribution
	if (pdrgpexpr->Size() != ulDlvrdSize)
	{
		if (nullptr != pdshashed->PdshashedEquiv())
		{
			// try again using the equivalent hashed distribution
			return PdshashedMatching(mp, pdshashed->PdshashedEquiv(),
									 ulSourceChild);
		}
	}
	if (pdrgpexpr->Size() != ulDlvrdSize)
	{
		// it should never happen, but instead of creating wrong spec, raise an exception
		GPOS_RAISE(
			CException::ExmaInvalid, CException::ExmiInvalid,
			GPOS_WSZ_LIT("Unable to create matching hashed distribution."));
	}

	// nulls colocated for inner hash joins, but not colocated in outer hash joins
	BOOL fNullsColocated = true;

	if (!m_is_null_aware &&
		(COperator::EopPhysicalLeftOuterHashJoin == Eopid() ||
		 COperator::EopPhysicalRightOuterHashJoin == Eopid()))
	{
		fNullsColocated = false;
	}

	return GPOS_NEW(mp)
		CDistributionSpecHashed(pdrgpexpr, fNullsColocated, opfamilies);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::PdsRequiredSingleton
//
//	@doc:
//		Create (singleton, singleton) optimization request
//
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpec>
CPhysicalHashJoin::PdsRequiredSingleton(
	CMemoryPool *mp,
	CExpressionHandle &,		   // exprhdl
	gpos::Ref<CDistributionSpec>,  // pdsInput
	ULONG child_index, gpos::Ref<CDrvdPropArray> pdrgpdpCtxt) const
{
	if (FFirstChildToOptimize(child_index))
	{
		// require first child to be singleton
		return GPOS_NEW(mp) CDistributionSpecSingleton();
	}

	// require a matching distribution from second child
	GPOS_ASSERT(nullptr != pdrgpdpCtxt);
	gpos::Ref<CDistributionSpec> pdsFirst =
		CDrvdPropPlan::Pdpplan((*pdrgpdpCtxt)[0])->Pds();
	GPOS_ASSERT(nullptr != pdsFirst);

	if (CDistributionSpec::EdtUniversal == pdsFirst->Edt() ||
		CDistributionSpec::EdtTaintedReplicated == pdsFirst->Edt())
	{
		// first child is universal, request second child to execute on a single host to avoid duplicates
		return GPOS_NEW(mp) CDistributionSpecSingleton();
	}

	if (COptCtxt::PoctxtFromTLS()->OptimizeDMLQueryWithSingletonSegment() &&
		CDistributionSpec::EdtStrictReplicated == pdsFirst->Edt())
	{
		// For a DML query that can be optimized by enforcing a non-coordinator gather motion,
		// we request singleton-segment distribution on the outer child. If the outer child
		// is replicated, no enforcer gets added; in which case pdsFirst is EdtStrictReplicated.
		// Hence handle this scenario here and require a singleton-segment on the
		// inner child to produce a singleton execution alternavtive for the HJ.
		return GPOS_NEW(mp)
			CDistributionSpecSingleton(CDistributionSpecSingleton::EstSegment);
	}

	GPOS_ASSERT(CDistributionSpec::EdtSingleton == pdsFirst->Edt() ||
				CDistributionSpec::EdtStrictSingleton == pdsFirst->Edt());

	// require second child to have matching singleton distribution
	return CPhysical::PdssMatching(
		mp, CDistributionSpecSingleton::PdssConvert(pdsFirst));
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::PdsRequiredReplicate
//
//	@doc:
//		Create (hashed/non-singleton, broadcast) optimization request
//
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpec>
CPhysicalHashJoin::PdsRequiredReplicate(CMemoryPool *mp,
										CExpressionHandle &exprhdl,
										gpos::Ref<CDistributionSpec> pdsInput,
										ULONG child_index,
										gpos::Ref<CDrvdPropArray> pdrgpdpCtxt,
										ULONG ulOptReq,
										gpos::Ref<CReqdPropPlan> prppInput)
{
	EChildExecOrder eceo = Eceo();
	if (EceoLeftToRight == eceo)
	{
		// if optimization order is left to right, fall-back to implementation of parent Join operator
		gpos::Ref<CEnfdDistribution> ped = CPhysicalJoin::Ped(
			mp, exprhdl, prppInput, child_index, pdrgpdpCtxt, ulOptReq);
		gpos::Ref<CDistributionSpec> pds = ped->PdsRequired();


		return pds;
	}
	GPOS_ASSERT(EceoRightToLeft == eceo);

	if (1 == child_index)
	{
		// require inner child to be replicated
		return GPOS_NEW(mp)
			CDistributionSpecReplicated(CDistributionSpec::EdtReplicated);
	}
	GPOS_ASSERT(0 == child_index);

	// require a matching distribution from outer child
	gpos::Ref<CDistributionSpec> pdsInner =
		CDrvdPropPlan::Pdpplan((*pdrgpdpCtxt)[0])->Pds();
	GPOS_ASSERT(nullptr != pdsInner);

	if (CDistributionSpec::EdtUniversal == pdsInner->Edt())
	{
		// first child is universal, request second child to execute on a single host to avoid duplicates
		return GPOS_NEW(mp) CDistributionSpecSingleton();
	}

	if (ulOptReq == m_pdrgpdsRedistributeRequests->Size() &&
		CDistributionSpec::EdtHashed == pdsInput->Edt())
	{
		// attempt to propagate hashed request to child
		gpos::Ref<CDistributionSpecHashed> pdshashed = PdshashedPassThru(
			mp, exprhdl, CDistributionSpecHashed::PdsConvert(pdsInput),
			child_index, pdrgpdpCtxt, ulOptReq);
		if (nullptr != pdshashed)
		{
			return pdshashed;
		}
	}

	// otherwise, require second child to deliver non-singleton distribution
	GPOS_ASSERT(CDistributionSpec::EdtStrictReplicated == pdsInner->Edt() ||
				CDistributionSpec::EdtTaintedReplicated == pdsInner->Edt());
	return GPOS_NEW(mp) CDistributionSpecNonSingleton();
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::PdshashedPassThru
//
//	@doc:
//		Create a child hashed distribution request based on input hashed
//		distribution,
//		return NULL if no such request can be created
//
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpecHashed>
CPhysicalHashJoin::PdshashedPassThru(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::Ref<CDistributionSpecHashed> pdshashedInput,
	ULONG,						// child_index
	gpos::Ref<CDrvdPropArray>,	// pdrgpdpCtxt
	ULONG ulOptReq GPOS_UNUSED)
{
	GPOS_ASSERT(nullptr != pdshashedInput);

	if (!GPOS_FTRACE(EopttraceEnableRedistributeBroadcastHashJoin))
	{
		// this option is disabled
		return nullptr;
	}

	// since incoming request is hashed, we attempt here to propagate this request to outer child
	gpos::Ref<CColRefSet> pcrsOuterOutput =
		exprhdl.DeriveOutputColumns(0 /*child_index*/);
	gpos::Ref<CExpressionArray> pdrgpexprIncomingRequest =
		pdshashedInput->Pdrgpexpr();
	gpos::Ref<CColRefSet> pcrsAllUsed =
		CUtils::PcrsExtractColumns(mp, pdrgpexprIncomingRequest);
	BOOL fSubset = pcrsOuterOutput->ContainsAll(pcrsAllUsed);
	BOOL fDisjoint = pcrsOuterOutput->IsDisjoint(pcrsAllUsed);

	if (fSubset)
	{
		// incoming request uses columns from outer child only, pass it through
		// but create a copy
		gpos::Ref<CDistributionSpecHashed> pdsHashedRequired =
			pdshashedInput->Copy(mp);
		return pdsHashedRequired;
	}

	if (!fDisjoint)
	{
		// incoming request intersects with columns from outer child,
		// we restrict the request to outer child columns only, then we pass it through
		gpos::Ref<CExpressionArray> pdrgpexprChildRequest =
			GPOS_NEW(mp) CExpressionArray(mp);
		const ULONG size = pdrgpexprIncomingRequest->Size();
		for (ULONG ul = 0; ul < size; ul++)
		{
			gpos::Ref<CExpression> pexpr = (*pdrgpexprIncomingRequest)[ul];
			gpos::Ref<CColRefSet> pcrsUsed = pexpr->DeriveUsedColumns();
			if (pcrsOuterOutput->ContainsAll(pcrsUsed))
			{
				// hashed expression uses columns from outer child only, add it to request

				pdrgpexprChildRequest->Append(pexpr);
			}
		}
		GPOS_ASSERT(0 < pdrgpexprChildRequest->Size());

		gpos::Ref<CDistributionSpecHashed> pdshashed =
			GPOS_NEW(mp) CDistributionSpecHashed(
				pdrgpexprChildRequest, pdshashedInput->FNullsColocated());

		// since the other child of the join is replicated, we need to enforce hashed-distribution across segments here
		pdshashed->MarkUnsatisfiableBySingleton();

		return pdshashed;
	}

	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::PdsRequiredRedistribute
//
//	@doc:
//		Compute (redistribute, redistribute) optimization request
//
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpec>
CPhysicalHashJoin::PdsRequiredRedistribute(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::Ref<CDistributionSpec>,  // pdsInput
	ULONG child_index, gpos::Ref<CDrvdPropArray> pdrgpdpCtxt,
	ULONG ulOptReq) const
{
	if (FFirstChildToOptimize(child_index))
	{
		// require first child to provide a hashed distribution,
		return PdshashedRequired(mp, child_index, ulOptReq);
	}

	// find the distribution delivered by first child
	gpos::Ref<CDistributionSpec> pdsFirst =
		CDrvdPropPlan::Pdpplan((*pdrgpdpCtxt)[0])->Pds();
	GPOS_ASSERT(nullptr != pdsFirst);

	gpos::Ref<CDistributionSpec> pdsInputForMatch = nullptr;
	if (pdsFirst->Edt() == CDistributionSpec::EdtHashed)
	{
		// we need to create a matching required spec based on the derived distribution spec from
		// the first child. Since that does not contain the m_equiv_hash_exprs (as they are not populated
		// for derived specs), so compute that here.
		gpos::Ref<CDistributionSpecHashed> pdsHashedFirstChild =
			CDistributionSpecHashed::PdsConvert(pdsFirst);
		gpos::Ref<CDistributionSpecHashed> pdsHashed =
			pdsHashedFirstChild->Copy(mp);
		pdsHashed->ComputeEquivHashExprs(mp, exprhdl);
		pdsInputForMatch = pdsHashed;
	}
	else
	{
		pdsInputForMatch = pdsFirst;
	}

	// find the index of the first child
	ULONG ulFirstChild = 0;
	if (EceoRightToLeft == Eceo())
	{
		ulFirstChild = 1;
	}

	// return a matching distribution request for the second child
	gpos::Ref<CDistributionSpec> pdsMatch =
		PdsMatch(mp, pdsInputForMatch, ulFirstChild);
	if (pdsFirst->Edt() == CDistributionSpec::EdtHashed)
	{
		// if the input spec was created as a copy, release it
	}
	return pdsMatch;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::PdsRequired
//
//	@doc:
//		Compute required distribution of the n-th child;
//		this function creates three optimization requests to join children:
//		Req(1 to N) (redistribute, redistribute), where we request the first hash join child
//			to be distributed on single hash join keys separately, as well as the set
//			of all hash join keys,
//			the second hash join child is always required to match the distribution returned
//			by first child
// 		Req(N + 1) (hashed, broadcast)
// 		Req(N + 2) (non-singleton, broadcast)
// 		Req(N + 3) (singleton, singleton)
//
//		we always check the distribution delivered by the first child (as
//		given by child optimization order), and then match the delivered
//		distribution on the second child
//
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpec>
CPhysicalHashJoin::PdsRequired(
	CMemoryPool *mp GPOS_UNUSED, CExpressionHandle &exprhdl GPOS_UNUSED,
	gpos::Ref<CDistributionSpec> pdsInput GPOS_UNUSED,
	ULONG child_index GPOS_UNUSED,
	gpos::Ref<CDrvdPropArray> pdrgpdpCtxt GPOS_UNUSED,
	ULONG ulOptReq
		GPOS_UNUSED	 // identifies which optimization request should be created
) const
{
	GPOS_RAISE(
		CException::ExmaInvalid, CException::ExmiInvalid,
		GPOS_WSZ_LIT("PdsRequired should not be called for CPhysicalHashJoin"));
	return nullptr;
}

gpos::Ref<CEnfdDistribution>
CPhysicalHashJoin::Ped(CMemoryPool *mp, CExpressionHandle &exprhdl,
					   gpos::Ref<CReqdPropPlan> prppInput, ULONG child_index,
					   gpos::Ref<CDrvdPropArray> pdrgpdpCtxt, ULONG ulOptReq)
{
	GPOS_ASSERT(2 > child_index);
	GPOS_ASSERT(ulOptReq < UlDistrRequests());

	CEnfdDistribution::EDistributionMatching dmatch =
		Edm(prppInput, child_index, pdrgpdpCtxt, ulOptReq);
	gpos::Ref<CDistributionSpec> const pdsInput =
		prppInput->Ped()->PdsRequired();

	// if expression has to execute on a single host then we need a gather
	if (exprhdl.NeedsSingletonExecution())
	{
		return GPOS_NEW(mp) CEnfdDistribution(
			PdsRequireSingleton(mp, exprhdl, pdsInput, child_index), dmatch);
	}

	if (exprhdl.HasOuterRefs())
	{
		if (CDistributionSpec::EdtSingleton == pdsInput->Edt() ||
			CDistributionSpec::EdtStrictReplicated == pdsInput->Edt())
		{
			return GPOS_NEW(mp) CEnfdDistribution(
				PdsPassThru(mp, exprhdl, pdsInput, child_index), dmatch);
		}
		return GPOS_NEW(mp)
			CEnfdDistribution(GPOS_NEW(mp) CDistributionSpecReplicated(
								  CDistributionSpec::EdtStrictReplicated),
							  dmatch);
	}

	const ULONG ulHashDistributeRequests =
		m_pdrgpdsRedistributeRequests->Size();
	if (ulOptReq < ulHashDistributeRequests)
	{
		// requests 1 .. N are (redistribute, redistribute)
		gpos::Ref<CDistributionSpec> pds = PdsRequiredRedistribute(
			mp, exprhdl, pdsInput, child_index, pdrgpdpCtxt, ulOptReq);
		if (CDistributionSpec::EdtHashed == pds->Edt())
		{
			gpos::Ref<CDistributionSpecHashed> pdsHashed =
				CDistributionSpecHashed::PdsConvert(pds);
			pdsHashed->ComputeEquivHashExprs(mp, exprhdl);
		}
		return GPOS_NEW(mp) CEnfdDistribution(pds, dmatch);
	}

	if (ulOptReq == ulHashDistributeRequests ||
		ulOptReq == ulHashDistributeRequests + 1)
	{
		// requests N+1, N+2 are (hashed/non-singleton, replicate)

		gpos::Ref<CDistributionSpec> pds =
			PdsRequiredReplicate(mp, exprhdl, pdsInput, child_index,
								 pdrgpdpCtxt, ulOptReq, prppInput);
		if (CDistributionSpec::EdtHashed == pds->Edt())
		{
			gpos::Ref<CDistributionSpecHashed> pdsHashed =
				CDistributionSpecHashed::PdsConvert(pds);
			pdsHashed->ComputeEquivHashExprs(mp, exprhdl);
		}
		return GPOS_NEW(mp) CEnfdDistribution(pds, dmatch);
	}

	GPOS_ASSERT(ulOptReq == ulHashDistributeRequests + 2);

	// requests N+3 is (singleton, singleton)

	return GPOS_NEW(mp) CEnfdDistribution(
		PdsRequiredSingleton(mp, exprhdl, pdsInput, child_index, pdrgpdpCtxt),
		dmatch);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::PdshashedRequired
//
//	@doc:
//		Compute required hashed distribution of the n-th child
//
//---------------------------------------------------------------------------
gpos::Ref<CDistributionSpecHashed>
CPhysicalHashJoin::PdshashedRequired(CMemoryPool *,	 // mp
									 ULONG,			 // child_index
									 ULONG ulReqIndex) const
{
	GPOS_ASSERT(ulReqIndex < m_pdrgpdsRedistributeRequests->Size());
	gpos::Ref<CDistributionSpec> pds =
		(*m_pdrgpdsRedistributeRequests)[ulReqIndex];


	return CDistributionSpecHashed::PdsConvert(pds);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::EpetOrder
//
//	@doc:
//		Return the enforcing type for order property based on this operator;
//
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalHashJoin::EpetOrder(CExpressionHandle &,  // exprhdl
							 const gpos::Ref<CEnfdOrder>
#ifdef GPOS_DEBUG
								 peo
#endif	// GPOS_DEBUG
) const
{
	GPOS_ASSERT(nullptr != peo);
	GPOS_ASSERT(!peo->PosRequired()->IsEmpty());

	// hash join is not order-preserving;
	// any order requirements have to be enforced on top
	return CEnfdProp::EpetRequired;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::FNullableInnerHashKeys
//
//	@doc:
//		Check whether the hash keys from one child are nullable. pcrsNotNull must
//		be all the "not null" columns coming from that child
//
//---------------------------------------------------------------------------
BOOL
CPhysicalHashJoin::FNullableHashKeys(gpos::Ref<CColRefSet> pcrsNotNull,
									 BOOL fInner) const
{
	ULONG ulHashKeys = 0;
	if (fInner)
	{
		ulHashKeys = m_pdrgpexprInnerKeys->Size();
	}
	else
	{
		ulHashKeys = m_pdrgpexprOuterKeys->Size();
	}

	for (ULONG ul = 0; ul < ulHashKeys; ul++)
	{
		if (FNullableHashKey(ul, pcrsNotNull, fInner))
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalHashJoin::FNullableHashKey
//
//	@doc:
//		Check whether a hash key is nullable
//
//---------------------------------------------------------------------------
BOOL
CPhysicalHashJoin::FNullableHashKey(ULONG ulKey,
									gpos::Ref<CColRefSet> pcrsNotNull,
									BOOL fInner) const
{
	gpos::Ref<COperator> pop = nullptr;
	if (fInner)
	{
		pop = (*m_pdrgpexprInnerKeys)[ulKey]->Pop();
	}
	else
	{
		pop = (*m_pdrgpexprOuterKeys)[ulKey]->Pop();
	}
	EOperatorId op_id = pop->Eopid();

	if (COperator::EopScalarIdent == op_id)
	{
		const gpos::Ref<CColRef> colref = CScalarIdent::PopConvert(pop)->Pcr();
		return (!pcrsNotNull->FMember(colref));
	}

	if (COperator::EopScalarConst == op_id)
	{
		return CScalarConst::PopConvert(pop)->GetDatum()->IsNull();
	}

	// be conservative for all other scalar expressions where we cannot easily
	// determine nullability
	return true;
}

void
CPhysicalHashJoin::CreateOptRequests(CMemoryPool *mp)
{
	CreateHashRedistributeRequests(mp);

	// given an optimization context, HJN creates three optimization requests
	// to enforce distribution of its children:
	// Req(1 to N) (redistribute, redistribute), where we request the first hash join child
	//		to be distributed on single hash join keys separately, as well as the set
	//		of all hash join keys,
	//		the second hash join child is always required to match the distribution returned
	//		by first child
	// Req(N + 1) (hashed, broadcast)
	// Req(N + 2) (non-singleton, broadcast)
	// Req(N + 3) (singleton, singleton)

	ULONG ulDistrReqs =
		GPOPT_NON_HASH_DIST_REQUESTS + m_pdrgpdsRedistributeRequests->Size();
	SetDistrRequests(ulDistrReqs);

	// With DP enabled, there are several (max 10 controlled by macro)
	// alternatives generated for a join tree and during optimization of those
	// alternatives expressions PS is inserted in almost all the groups possibly.
	// However, if DP is turned off, i.e in query or greedy join order,
	// PS must be inserted in the group with DTS else in some cases HJ plan
	// cannot be created. So, to ensure pushing PS without DPE 2 partition
	// propagation request are required if DP is disabled.
	//    Req 0 => Push PS with considering DPE possibility
	//    Req 1 => Push PS without considering DPE possibility
	// Ex case: select * from non_part_tbl1 t1, part_tbl t2, non_part_tbl2 t3
	// where t1.b = t2.b and t2.b = t3.b;
	// Note: b is the partitioned column for part_tbl. If DP is turned off, HJ
	// will not be created for the above query if we send only 1 request.
	// Also, increasing the number of request increases the optimization time, so
	// set 2 only when needed.
	//
	// There are also cases where greedy does generate a better plan
	// without DPE. This adds some overhead (<10%)to optimization time in
	// some cases, but can create better alternatives to DPE, so
	// we also generate this additional request for expressions that originated
	// from CXformExpandNAryJoinGreedy.
	if ((GPOPT_FDISABLED_XFORM(CXform::ExfExpandNAryJoinDP) &&
		 GPOPT_FDISABLED_XFORM(CXform::ExfExpandNAryJoinDPv2)) ||
		dynamic_cast<CPhysicalJoin *>(this)->OriginXform() ==
			CXform::ExfExpandNAryJoinGreedy)
	{
		SetPartPropagateRequests(2);
	}
	else
	{
		SetPartPropagateRequests(1);
	}
}

gpos::Ref<CExpression>
CPhysicalHashJoin::PexprJoinPredOnPartKeys(
	CMemoryPool *mp, gpos::Ref<CExpression> pexprScalar,
	gpos::Ref<CPartKeysArray> pdrgppartkeys,
	gpos::Ref<CColRefSet> pcrsAllowedRefs) const
{
	GPOS_ASSERT(nullptr != pcrsAllowedRefs);

	gpos::Ref<CExpression> pexprPred = nullptr;
	for (ULONG ulKey = 0; nullptr == pexprPred && ulKey < pdrgppartkeys->Size();
		 ulKey++)
	{
		// get partition key
		gpos::Ref<CColRef2dArray> pdrgpdrgpcrPartKeys =
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
CPhysicalHashJoin::PppsRequiredForJoins(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::Ref<CPartitionPropagationSpec> pppsRequired, ULONG child_index,
	gpos::Ref<CDrvdPropArray> pdrgpdpCtxt, ULONG ulOptReq) const
{
	GPOS_ASSERT(nullptr != pppsRequired);
	GPOS_ASSERT(nullptr != pdrgpdpCtxt);

	gpos::Ref<CExpression> pexprScalar =
		exprhdl.PexprScalarExactChild(2 /*child_index*/);

	gpos::Ref<CColRefSet> pcrsOutputInner = exprhdl.DeriveOutputColumns(1);

	gpos::Ref<CPartitionPropagationSpec> pps_result;
	if (ulOptReq == 0)
	{
		// DPE: create a new request
		pps_result = GPOS_NEW(mp) CPartitionPropagationSpec(mp);

		// Extract the partition info of the outer child.
		// Info in CPartInfo is added at the logical level. We add information
		// about consumers during that stage. During the physical implementation,
		// for every consumer, we check if we have to insert a consumer/propagator
		// in PppsRequired()
		gpos::Ref<CPartInfo> part_info_outer = exprhdl.DerivePartitionInfo(0);

		// Extracting the information of existing partition table consumers.
		// For every consumer(Dynamic Table Scan, identified by scan-id),
		// if PppsRequired() is called for inner child, we can add a propagator.
		// if PppsRequired() is called for outer child, we can add a consumer.
		for (ULONG ul = 0; ul < part_info_outer->UlConsumers(); ++ul)
		{
			ULONG scan_id = part_info_outer->ScanId(ul);
			gpos::Ref<IMDId> rel_mdid = part_info_outer->GetRelMdId(ul);
			gpos::Ref<CPartKeysArray> part_keys_array =
				part_info_outer->Pdrgppartkeys(ul);

			gpos::Ref<CExpression> pexprCmp =
				PexprJoinPredOnPartKeys(mp, pexprScalar, part_keys_array,
										pcrsOutputInner /* pcrsAllowedRefs*/);

			// If we don't have predicate on partition keys, then partition
			// elimination won't work, so we don't add a Consumer or Propagator
			if (pexprCmp == nullptr)
			{
				continue;
			}

			// For outer child(index=0), we insert Consumer, if a Partition
			// Selector exist in the inner child for a scan-id.
			if (child_index == 0)
			{
				// For the inner child, we extract the derived PPS.
				gpos::Ref<CPartitionPropagationSpec> pps_inner =
					CDrvdPropPlan::Pdpplan((*pdrgpdpCtxt)[0])->Ppps();

				// In the derived plan properties of the inner child,
				// we check if a partition selector exist for the given scan-id
				// If found, we insert a corresponding 'Consumer' in the outer child
				gpos::Ref<CBitSet> selector_ids =
					GPOS_NEW(mp) CBitSet(mp, *pps_inner->SelectorIds(scan_id));

				// For the identified 'partition selector' we insert a consumer.
				// This will a form part of our required properties, i.e. for the
				// given 'partition selector', we require a 'Consumer'
				pps_result->Insert(scan_id,
								   CPartitionPropagationSpec::EpptConsumer,
								   rel_mdid, selector_ids, nullptr /* expr */);
			}
			else
			{
				// For inner child (index=1), we insert a propagator given that
				// we have predicate on the partition keys
				GPOS_ASSERT(child_index == 1);
				pps_result->Insert(scan_id,
								   CPartitionPropagationSpec::EpptPropagator,
								   rel_mdid, nullptr, pexprCmp);
			}
		}

		// Now for the input 'pppsRequired' & 'child_index' we check if any
		// other consumer is required to be added in the pps_result.
		// 1. We prepare a list of allowed scan-ids for the input child. These scan-ids
		// we defined at the logical level.
		// 2. For each of the scan-ids, that exist in pppsRequired, we check if
		// they exist in 'allowed list' and are of type Consumer.
		// 3. For all such scan-ids, we update our computed required props in pps_result
		// Thus based we have computed the required properties for the operator based
		// on the input from higher level(using input pppsRequired) and our own
		// operator specific rules (as in the for loop above)
		gpos::Ref<CBitSet> allowed_scan_ids = GPOS_NEW(mp) CBitSet(mp);
		gpos::Ref<CPartInfo> part_info =
			exprhdl.DerivePartitionInfo(child_index);
		for (ULONG ul = 0; ul < part_info->UlConsumers(); ++ul)
		{
			ULONG scan_id = part_info->ScanId(ul);
			allowed_scan_ids->ExchangeSet(scan_id);
		}
		pps_result->InsertAllowedConsumers(pppsRequired, allowed_scan_ids);
	}
	else
	{
		// No DPE: pass through requests
		pps_result = CPhysical::PppsRequired(
			mp, exprhdl, pppsRequired, child_index, pdrgpdpCtxt, ulOptReq);
	}
	return pps_result;
}

// In the following function, we are generating the Derived property :
// "Partition Propagation Spec" of the join.
// Since Property derivation takes place in a bottom-up fashion, this operator
// derives the information from its child and passes it up. In this function
// we are focussing only on the "Partition Propagation Spec" of the children
gpos::Ref<CPartitionPropagationSpec>
CPhysicalHashJoin::PppsDeriveForJoins(CMemoryPool *mp,
									  CExpressionHandle &exprhdl) const
{
	gpos::Ref<CPartitionPropagationSpec> pps_outer = exprhdl.Pdpplan(0)->Ppps();
	gpos::Ref<CPartitionPropagationSpec> pps_inner = exprhdl.Pdpplan(1)->Ppps();

	gpos::Ref<CPartitionPropagationSpec> pps_result =
		GPOS_NEW(mp) CPartitionPropagationSpec(mp);
	pps_result->InsertAll(pps_outer);
	pps_result->InsertAllResolve(pps_inner);

	return pps_result;
}
// EOF
