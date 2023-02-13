//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CPhysicalJoin.h
//
//	@doc:
//		Physical join base class
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalJoin_H
#define GPOPT_CPhysicalJoin_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpec.h"
#include "gpopt/operators/CPhysical.h"
#include "gpopt/xforms/CXform.h"

namespace gpopt
{
// fwd declarations
class CDistributionSpecSingleton;

//---------------------------------------------------------------------------
//	@class:
//		CPhysicalJoin
//
//	@doc:
//		Inner nested-loops join operator
//
//---------------------------------------------------------------------------
class CPhysicalJoin : public CPhysical
{
private:
	// private copy ctor
	CPhysicalJoin(const CPhysicalJoin &);

	// check whether the child being processed is the child that has the part consumer
	static BOOL FProcessingChildWithPartConsumer(BOOL fOuterPartConsumerTest,
												 ULONG ulChildIndexToTestFirst,
												 ULONG ulChildIndexToTestSecond,
												 ULONG child_index);

	// xform that join order originated from
	CXform::EXformId m_origin_xform;

protected:
	// ctor
	explicit CPhysicalJoin(CMemoryPool *mp,
						   CXform::EXformId origin_xform = CXform::ExfSentinel);

	// dtor
	~CPhysicalJoin() override = default;

	// helper to check if given child index correspond to first child to be optimized
	BOOL FFirstChildToOptimize(ULONG child_index) const;

	// helper to compute required distribution of correlated join's children
	gpos::Ref<CEnfdDistribution> PedCorrelatedJoin(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CReqdPropPlan> prppInput, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq);

	// helper to compute required rewindability of correlated join's children
	static gpos::Ref<CRewindabilitySpec> PrsRequiredCorrelatedJoin(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		CRewindabilitySpec *prsRequired, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq);

	// helper for propagating required sort order to outer child
	static gpos::Ref<COrderSpec> PosPropagateToOuter(CMemoryPool *mp,
													 CExpressionHandle &exprhdl,
													 COrderSpec *posRequired);

	// helper for checking if required sort columns come from outer child
	static BOOL FSortColsInOuterChild(CMemoryPool *mp,
									  CExpressionHandle &exprhdl,
									  COrderSpec *pos);

	// helper for checking if the outer input of a binary join operator
	// includes the required columns
	static BOOL FOuterProvidesReqdCols(CExpressionHandle &exprhdl,
									   gpos::Ref<CColRefSet> pcrsRequired);


	// Do each of the given predicate children use columns from a different
	// join child?
	static BOOL FPredKeysSeparated(gpos::Ref<CExpression> pexprOuter,
								   gpos::Ref<CExpression> pexprInner,
								   gpos::Ref<CExpression> pexprPredOuter,
								   gpos::Ref<CExpression> pexprPredInner);

	static gpos::Ref<CEnfdDistribution> PedInnerHashedFromOuterHashed(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		CEnfdDistribution::EDistributionMatching dmatch,
		gpos::Ref<CDrvdProp> outerDrvdProp);

public:
	// match function
	BOOL Matches(gpos::Ref<COperator> pop) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return true;
	}

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required output columns of the n-th child
	gpos::Ref<CColRefSet> PcrsRequired(CMemoryPool *mp,
									   CExpressionHandle &exprhdl,
									   gpos::Ref<CColRefSet> pcrsRequired,
									   ULONG child_index,
									   CDrvdPropArray *pdrgpdpCtxt,
									   ULONG ulOptReq) override;

	// compute required ctes of the n-th child
	gpos::Ref<CCTEReq> PcteRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
									gpos::Ref<CCTEReq> pcter, ULONG child_index,
									CDrvdPropArray *pdrgpdpCtxt,
									ULONG ulOptReq) const override;

	// compute required distribution of the n-th child
	gpos::Ref<CDistributionSpec> PdsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CDistributionSpec> pdsRequired, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const override;

	gpos::Ref<CEnfdDistribution> Ped(CMemoryPool *mp,
									 CExpressionHandle &exprhdl,
									 gpos::Ref<CReqdPropPlan> prppInput,
									 ULONG child_index,
									 CDrvdPropArray *pdrgpdpCtxt,
									 ULONG ulDistrReq) override;

	// check if required columns are included in output columns
	BOOL FProvidesReqdCols(CExpressionHandle &exprhdl,
						   gpos::Ref<CColRefSet> pcrsRequired,
						   ULONG ulOptReq) const override;

	// distribution matching type
	CEnfdDistribution::EDistributionMatching Edm(
		gpos::Ref<CReqdPropPlan> prppInput, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) override;

	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive sort order from outer child
	gpos::Ref<COrderSpec>
	PosDerive(CMemoryPool *,  // mp
			  CExpressionHandle &exprhdl) const override
	{
		return PosDerivePassThruOuter(exprhdl);
	}

	// derive distribution
	gpos::Ref<CDistributionSpec> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive rewindability
	gpos::Ref<CRewindabilitySpec> PrsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;


	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return rewindability property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetRewindability(
		CExpressionHandle &exprhdl,
		const CEnfdRewindability *per) const override;

	// return true if operator passes through stats obtained from children,
	// this is used when computing stats during costing
	BOOL
	FPassThruStats() const override
	{
		return false;
	}

	CXform::EXformId
	OriginXform()
	{
		return m_origin_xform;
	}
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// is given predicate hash-join compatible
	static BOOL FHashJoinCompatible(gpos::Ref<CExpression> pexprPred,
									gpos::Ref<CExpression> pexprOuter,
									gpos::Ref<CExpression> pexprInner);

	// is given predicate merge-join compatible
	static BOOL FMergeJoinCompatible(gpos::Ref<CExpression> pexprPred,
									 gpos::Ref<CExpression> pexprOuter,
									 gpos::Ref<CExpression> pexprInner);

	// return number of distribution requests for correlated join
	static ULONG UlDistrRequestsForCorrelatedJoin();

	static void AlignJoinKeyOuterInner(gpos::Ref<CExpression> pexprConjunct,
									   gpos::Ref<CExpression> pexprOuter,
									   gpos::Ref<CExpression> pexprInner,
									   gpos::Ref<CExpression> *ppexprKeyOuter,
									   gpos::Ref<CExpression> *ppexprKeyInner,
									   gpos::Ref<IMDId> *mdid_scop);

	static CRewindabilitySpec *PrsRequiredForNLJoinOuterChild(
		CMemoryPool *pmp, CExpressionHandle &exprhdl,
		CRewindabilitySpec *prsRequired);

};	// class CPhysicalJoin

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalJoin_H

// EOF
