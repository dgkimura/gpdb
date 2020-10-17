//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Greenplum, Inc.
//
//	@filename:
//		CPhysicalInnerIndexNLJoin.cpp
//
//	@doc:
//		Implementation of index inner nested-loops join operator
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/base/CDistributionSpecReplicated.h"
#include "gpopt/base/CDistributionSpecHashed.h"
#include "gpopt/base/CDistributionSpecNonSingleton.h"
#include "gpopt/base/CDistributionSpecAny.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPredicateUtils.h"


#include "gpopt/operators/CPhysicalInnerIndexNLJoin.h"



using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalInnerIndexNLJoin::CPhysicalInnerIndexNLJoin
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalInnerIndexNLJoin::CPhysicalInnerIndexNLJoin(CMemoryPool *mp,
													 CColRefArray *colref_array)
	: CPhysicalInnerNLJoin(mp), m_pdrgpcrOuterRefs(colref_array)
{
	GPOS_ASSERT(NULL != colref_array);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalInnerIndexNLJoin::~CPhysicalInnerIndexNLJoin
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPhysicalInnerIndexNLJoin::~CPhysicalInnerIndexNLJoin()
{
	m_pdrgpcrOuterRefs->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalInnerIndexNLJoin::Matches
//
//	@doc:
//		Match function
//
//---------------------------------------------------------------------------
BOOL
CPhysicalInnerIndexNLJoin::Matches(COperator *pop) const
{
	if (pop->Eopid() == Eopid())
	{
		return m_pdrgpcrOuterRefs->Equals(
			CPhysicalInnerIndexNLJoin::PopConvert(pop)->PdrgPcrOuterRefs());
	}

	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalInnerIndexNLJoin::PdsRequired
//
//	@doc:
//		Compute required distribution of the n-th child;
//
//---------------------------------------------------------------------------
CDistributionSpec *
CPhysicalInnerIndexNLJoin::PdsRequired(CMemoryPool *mp GPOS_UNUSED,
									   CExpressionHandle &exprhdl GPOS_UNUSED,
									   CDistributionSpec *,	 //pdsRequired,
									   ULONG child_index GPOS_UNUSED,
									   CDrvdPropArray *pdrgpdpCtxt GPOS_UNUSED,
									   ULONG  // ulOptReq
) const
{
	std::terminate();
	return nullptr;
}

CEnfdDistribution *
CPhysicalInnerIndexNLJoin::Ped(CMemoryPool *mp, CExpressionHandle &exprhdl,
							   CReqdPropPlan *prppInput GPOS_UNUSED,
							   ULONG child_index, CDrvdPropArray *pdrgpdpCtxt,
							   ULONG ulDistrReq GPOS_UNUSED)
{
	GPOS_ASSERT(2 > child_index);

	if (1 == child_index)
	{
		// inner (index-scan side) is requested for Any distribution,
		// we allow outer references on the inner child of the join since it needs
		// to refer to columns in join's outer child
		return GPOS_NEW(mp) CEnfdDistribution(
			GPOS_NEW(mp)
				CDistributionSpecAny(this->Eopid(), true /*fAllowOuterRefs*/),
			CEnfdDistribution::EdmSatisfy);
	}

	// we need to match distribution of inner
	CDistributionSpec *pdsInner =
		CDrvdPropPlan::Pdpplan((*pdrgpdpCtxt)[0])->Pds();
	CDistributionSpec::EDistributionType edtInner = pdsInner->Edt();
	if (CDistributionSpec::EdtSingleton == edtInner ||
		CDistributionSpec::EdtStrictSingleton == edtInner ||
		CDistributionSpec::EdtUniversal == edtInner)
	{
		if (FFirstChildToOptimize(child_index))
		{
			// use satisfaction for the first child to be optimized
			return GPOS_NEW(mp)
				CEnfdDistribution(GPOS_NEW(mp) CDistributionSpecSingleton(),
								  CEnfdDistribution::EdmSatisfy);
		}
		// enforce executing on a single host
		return GPOS_NEW(mp)
			CEnfdDistribution(GPOS_NEW(mp) CDistributionSpecSingleton(),
							  CEnfdDistribution::EdmExact);
	}

	if (CDistributionSpec::EdtHashed == edtInner)
	{
		// check if we could create an equivalent hashed distribution request to the inner child
		CDistributionSpecHashed *pdshashed =
			CDistributionSpecHashed::PdsConvert(pdsInner);
		CDistributionSpecHashed *pdshashedEquiv = pdshashed->PdshashedEquiv();

		// If the inner child is a IndexScan on a multi-key distributed index, it
		// may derive an incomplete equiv spec (see CPhysicalScan::PdsDerive()).
		// However, there is no point to using that here since there will be no
		// operator above this that can complete it.
		if (pdshashed->HasCompleteEquivSpec(mp))
		{
			// request hashed distribution from outer
			pdshashedEquiv->Pdrgpexpr()->AddRef();
			CDistributionSpecHashed *pdsHashedRequired = GPOS_NEW(mp)
				CDistributionSpecHashed(pdshashedEquiv->Pdrgpexpr(),
										pdshashedEquiv->FNullsColocated());
			pdsHashedRequired->ComputeEquivHashExprs(mp, exprhdl);

			return GPOS_NEW(mp) CEnfdDistribution(pdsHashedRequired,
												  CEnfdDistribution::EdmExact);
		}
	}

	// otherwise, require outer child to be replicated
	// The match type for this request has to be "Satisfy" since EdtReplicated
	// is required only property. Since a Broadcast motion will always
	// derive a EdtStrictReplicated distribution spec, it will never "Match"
	// the required distribution spec and hence will not be optimized.
	return GPOS_NEW(mp) CEnfdDistribution(
		GPOS_NEW(mp)
			CDistributionSpecReplicated(CDistributionSpec::EdtReplicated),
		CEnfdDistribution::EdmSatisfy);
}


// EOF
