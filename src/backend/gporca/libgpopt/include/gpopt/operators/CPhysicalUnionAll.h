//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.

#ifndef GPOPT_CPhysicalUnionAll_H
#define GPOPT_CPhysicalUnionAll_H

#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CDistributionSpecHashed.h"
#include "gpopt/operators/COperator.h"
#include "gpopt/operators/CPhysical.h"


namespace gpopt
{
class CPhysicalUnionAll : public CPhysical
{
private:
	// output column array
	gpos::owner<CColRefArray *> const m_pdrgpcrOutput;

	// input column array
	gpos::owner<CColRef2dArray *> const m_pdrgpdrgpcrInput;

	// set representation of input columns
	gpos::owner<CColRefSetArray *> m_pdrgpcrsInput;

	// array of child hashed distributions -- used locally for distribution derivation
	gpos::owner<CDistributionSpecArray *> m_pdrgpds;

	void PopulateDistrSpecs(CMemoryPool *mp,
							gpos::pointer<CColRefArray *> pdrgpcrOutput,
							gpos::pointer<CColRef2dArray *> pdrgpdrgpcrInput);

	// map given array of scalar ident expressions to positions of UnionAll input columns in the given child;
	gpos::owner<ULongPtrArray *> PdrgpulMap(
		CMemoryPool *mp, gpos::pointer<CExpressionArray *> pdrgpexpr,
		ULONG child_index) const;

	// map given ColRefSet, expressed in terms of outputs,
	// into an equivalent ColRefSet, expressed in terms
	// of input number n
	gpos::owner<CColRefSet *> MapOutputColRefsToInput(
		CMemoryPool *mp, gpos::pointer<CColRefSet *> out_col_refs,
		ULONG child_index);

	// derive hashed distribution from child operators
	gpos::owner<CDistributionSpecHashed *> PdshashedDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const;

	// derive strict random distribution spec if all the children of the parallel union all
	// node derive strict random; derive null spec otherwise
	static gpos::owner<CDistributionSpecRandom *>
	PdsStrictRandomParallelUnionAllChildren(CMemoryPool *mp,
											CExpressionHandle &expr_handle);

	// compute output hashed distribution matching the outer child's hashed distribution
	gpos::owner<CDistributionSpecHashed *> PdsMatching(
		CMemoryPool *mp,
		gpos::pointer<const ULongPtrArray *> pdrgpulOuter) const;

	// derive output distribution based on child distribution
	static CDistributionSpec *PdsDeriveFromChildren(CMemoryPool *mp,
													CExpressionHandle &exprhdl);

protected:
	// compute required hashed distribution of the n-th child
	gpos::owner<CDistributionSpecHashed *> PdshashedPassThru(
		CMemoryPool *mp,
		gpos::pointer<CDistributionSpecHashed *> pdshashedRequired,
		ULONG child_index) const;

public:
	CPhysicalUnionAll(CMemoryPool *mp,
					  gpos::owner<CColRefArray *> pdrgpcrOutput,
					  gpos::owner<CColRef2dArray *> pdrgpdrgpcrInput);

	~CPhysicalUnionAll() override;

	// match function
	BOOL Matches(gpos::pointer<COperator *>) const override;

	// ident accessors
	EOperatorId Eopid() const override = 0;

	const CHAR *SzId() const override = 0;

	// sensitivity to order of inputs
	BOOL FInputOrderSensitive() const override;

	// accessor of output column array
	gpos::pointer<CColRefArray *> PdrgpcrOutput() const;

	// accessor of input column array
	gpos::pointer<CColRef2dArray *> PdrgpdrgpcrInput() const;

	// return true if operator passes through stats obtained from children,
	// this is used when computing stats during costing
	BOOL FPassThruStats() const override;

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required output columns of the n-th child
	gpos::owner<CColRefSet *> PcrsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CColRefSet *> pcrsRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt, ULONG ulOptReq) override;

	// compute required ctes of the n-th child
	gpos::owner<CCTEReq *> PcteRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CCTEReq *> pcter, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// compute required sort order of the n-th child
	gpos::owner<COrderSpec *> PosRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<COrderSpec *> posRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// compute required rewindability of the n-th child
	gpos::owner<CRewindabilitySpec *> PrsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CRewindabilitySpec *> prsRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// conversion function
	static gpos::cast_func<CPhysicalUnionAll *> PopConvert(COperator *pop);


	// check if required columns are included in output columns
	BOOL FProvidesReqdCols(CExpressionHandle &exprhdl,
						   gpos::pointer<CColRefSet *> pcrsRequired,
						   ULONG ulOptReq) const override;

	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive sort order
	gpos::owner<COrderSpec *> PosDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive distribution
	gpos::owner<CDistributionSpec *> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive rewindability
	gpos::owner<CRewindabilitySpec *> PrsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------


	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl,
		gpos::pointer<const CEnfdOrder *> peo) const override;

	// return rewindability property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetRewindability(
		CExpressionHandle &,					   // exprhdl
		gpos::pointer<const CEnfdRewindability *>  // per
	) const override;
};
}  // namespace gpopt

#endif
