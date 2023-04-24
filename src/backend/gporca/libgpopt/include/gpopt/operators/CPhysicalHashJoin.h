//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalHashJoin.h
//
//	@doc:
//		Base hash join operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalHashJoin_H
#define GPOPT_CPhysicalHashJoin_H

#include "gpos/base.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CPhysicalJoin.h"

namespace gpopt
{
// fwd declarations
class CDistributionSpecHashed;

//---------------------------------------------------------------------------
//	@class:
//		CPhysicalHashJoin
//
//	@doc:
//		Base hash join operator
//
//---------------------------------------------------------------------------
class CPhysicalHashJoin : public CPhysicalJoin
{
private:
	// the array of expressions from the outer relation
	// that are extracted from the hashing condition
	CExpressionArray *m_pdrgpexprOuterKeys;

	// the array of expressions from the inner relation
	// that are extracted from the hashing condition
	CExpressionArray *m_pdrgpexprInnerKeys;

	// Hash op families of the operators used in the join conditions
	IMdIdArray *m_hash_opfamilies;

	// if the join condition is null-aware
	// true by default, and false if the join condition doesn't contain
	// any INDF predicates
	BOOL m_is_null_aware;

	// array redistribute request sent to the first hash join child
	CDistributionSpecArray *m_pdrgpdsRedistributeRequests;

	// compute a distribution matching the distribution delivered by given child
	gpos::Ref<CDistributionSpec> PdsMatch(CMemoryPool *mp,
										  gpos::Ref<CDistributionSpec> pds,
										  ULONG ulSourceChildIndex) const;

protected:
	// compute required hashed distribution from the n-th child
	gpos::Ref<CDistributionSpecHashed> PdshashedRequired(
		CMemoryPool *mp, ULONG child_index, ULONG ulReqIndex) const;
	// create (redistribute, redistribute) optimization request
	gpos::Ref<CDistributionSpec> PdsRequiredRedistribute(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CDistributionSpec> pdsInput, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const;

	// create the set of redistribute requests to send to first hash join child
	void CreateHashRedistributeRequests(CMemoryPool *mp);

private:
	// create (non-singleton, replicate) optimization request
	gpos::Ref<CDistributionSpec> PdsRequiredReplicate(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CDistributionSpec> pdsInput, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq,
		gpos::Ref<CReqdPropPlan> prppInput);

	// create a child hashed distribution request based on input hashed distribution,
	// return NULL if no such request can be created
	static gpos::Ref<CDistributionSpecHashed> PdshashedPassThru(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CDistributionSpecHashed> pdshashedInput, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq);

	// check whether a hash key is nullable
	BOOL FNullableHashKey(ULONG ulKey, gpos::Ref<CColRefSet> pcrsNotNullInner,
						  BOOL fInner) const;

protected:
	// helper for computing a hashed distribution matching the given distribution
	gpos::Ref<CDistributionSpecHashed> PdshashedMatching(
		CMemoryPool *mp, gpos::Ref<CDistributionSpecHashed> pdshashed,
		ULONG ulSourceChild) const;

	// create (singleton, singleton) optimization request
	gpos::Ref<CDistributionSpec> PdsRequiredSingleton(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CDistributionSpec> pdsInput, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt) const;

	// check whether the hash keys from one child are nullable
	BOOL FNullableHashKeys(gpos::Ref<CColRefSet> pcrsNotNullInner,
						   BOOL fInner) const;

	ULONG
	NumDistrReq() const
	{
		return m_pdrgpdsRedistributeRequests->Size();
	}

	// create optimization requests
	virtual void CreateOptRequests(CMemoryPool *mp);

	gpos::Ref<CPartitionPropagationSpec> PppsRequiredForJoins(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CPartitionPropagationSpec> pppsRequired, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const;

	gpos::Ref<CExpression> PexprJoinPredOnPartKeys(
		CMemoryPool *mp, gpos::Ref<CExpression> pexprScalar,
		CPartKeysArray *pdrgppartkeys,
		gpos::Ref<CColRefSet> pcrsAllowedRefs) const;

	gpos::Ref<CPartitionPropagationSpec> PppsDeriveForJoins(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const;

public:
	CPhysicalHashJoin(const CPhysicalHashJoin &) = delete;

	// ctor
	CPhysicalHashJoin(CMemoryPool *mp, CExpressionArray *pdrgpexprOuterKeys,
					  CExpressionArray *pdrgpexprInnerKeys,
					  IMdIdArray *hash_opfamilies, BOOL is_null_aware = true,
					  CXform::EXformId origin_xform = CXform::ExfSentinel);

	// dtor
	~CPhysicalHashJoin() override;

	// inner keys
	const CExpressionArray *
	PdrgpexprInnerKeys() const
	{
		return m_pdrgpexprInnerKeys;
	}

	// outer keys
	const CExpressionArray *
	PdrgpexprOuterKeys() const
	{
		return m_pdrgpexprOuterKeys;
	}

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required sort order of the n-th child
	gpos::Ref<COrderSpec> PosRequired(CMemoryPool *mp,
									  CExpressionHandle &exprhdl,
									  gpos::Ref<COrderSpec> posInput,
									  ULONG child_index,
									  CDrvdPropArray *pdrgpdpCtxt,
									  ULONG ulOptReq) const override;

	// compute required rewindability of the n-th child
	gpos::Ref<CRewindabilitySpec> PrsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CRewindabilitySpec> prsRequired, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const override;

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

	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive sort order
	gpos::Ref<COrderSpec>
	PosDerive(CMemoryPool *mp,
			  CExpressionHandle &  // exprhdl
	) const override
	{
		// hash join is not order-preserving
		return GPOS_NEW(mp) COrderSpec(mp);
	}

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl,
		const gpos::Ref<CEnfdOrder> peo) const override;


	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// execution order of children
	EChildExecOrder
	Eceo() const override
	{
		// TODO - ; 01/06/2014
		// obtain this property by through MD abstraction layer, similar to scalar properties

		// hash join in GPDB executes its inner (right) child first,
		// the optimization order of hash join children follows the execution order
		return EceoRightToLeft;
	}

	// conversion function
	static gpos::Ref<CPhysicalHashJoin>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(CUtils::FHashJoin(pop));

		return dynamic_cast<gpos::Ref<CPhysicalHashJoin> >(pop);
	}

};	// class CPhysicalHashJoin

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalHashJoin_H

// EOF
