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

#include "gpopt/base/CDistributionSpecHashed.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarCmp.h"

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
	CMemoryPool *mp, CExpressionArray *pdrgpexprOuterKeys,
	CExpressionArray *pdrgpexprInnerKeys, IMdIdArray *hash_opfamilies,
	BOOL is_null_aware, CXform::EXformId origin_xform)
	: CPhysicalHashJoin(mp, pdrgpexprOuterKeys, pdrgpexprInnerKeys,
						hash_opfamilies, is_null_aware, origin_xform)
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
	pdshashedMatching->Pdrgpexpr()->AddRef();
	pdshashed->AddRef();
	if (nullptr != pdshashedMatching->Opfamilies())
	{
		pdshashedMatching->Opfamilies()->AddRef();
	}
	gpos::Ref<CDistributionSpecHashed> pdsHashedMatchingEquivalents =
		GPOS_NEW(mp) CDistributionSpecHashed(
			pdshashedMatching->Pdrgpexpr(),
			pdshashedMatching->FNullsColocated(),
			pdshashed,	// matching distribution spec is equivalent to passed distribution spec
			pdshashedMatching->Opfamilies());
	pdshashedMatching->Release();
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
		CDistributionSpecHashed::PdsConvert(pdsOuter);
	gpos::Ref<CDistributionSpecHashed> pdshashedInner =
		CDistributionSpecHashed::PdsConvert(pdsInner);

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
			CDistributionSpecHashed::PdsConvert(pdsInner);
		if (pdshashedInner->IsCoveredBy(PdrgpexprInnerKeys()))
		{
			// inner child is hashed on a subset of inner hashkeys,
			// return a hashed distribution equivalent to a matching outer distribution
			return PdshashedCreateMatching(mp, pdshashedInner,
										   1 /*ulSourceChild*/);
		}
	}

	// otherwise, pass-through inner distribution
	pdsInner->AddRef();
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
		CDistributionSpecHashed::PdsConvert(pdsOuter);
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
		pdsInner->AddRef();
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
	pdsOuter->AddRef();
	return pdsOuter;
}

gpos::Ref<CPartitionPropagationSpec>
CPhysicalInnerHashJoin::PppsRequired(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::Ref<CPartitionPropagationSpec> pppsRequired, ULONG child_index,
	CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const
{
	return PppsRequiredForJoins(mp, exprhdl, pppsRequired, child_index,
								pdrgpdpCtxt, ulOptReq);
}

gpos::Ref<CPartitionPropagationSpec>
CPhysicalInnerHashJoin::PppsDerive(CMemoryPool *mp,
								   CExpressionHandle &exprhdl) const
{
	return PppsDeriveForJoins(mp, exprhdl);
}

// EOF
