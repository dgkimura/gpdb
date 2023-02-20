//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CLogicalJoin.h
//
//	@doc:
//		Base class of all logical join operators
//---------------------------------------------------------------------------
#ifndef GPOS_CLogicalJoin_H
#define GPOS_CLogicalJoin_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalJoin
//
//	@doc:
//		join operator
//
//---------------------------------------------------------------------------
class CLogicalJoin : public CLogical
{
private:
	// xform that join originated from
	CXform::EXformId m_origin_xform;

protected:
	// ctor
	explicit CLogicalJoin(CMemoryPool *mp,
						  CXform::EXformId origin_xform = CXform::ExfSentinel);

	// dtor
	~CLogicalJoin() override = default;

public:
	CLogicalJoin(const CLogicalJoin &) = delete;

	// match function
	BOOL Matches(gpos::Ref<COperator> pop) const override;


	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return true;
	}

	// return a copy of the operator with remapped columns
	gpos::Ref<COperator>
	PopCopyWithRemappedColumns(CMemoryPool *,				 //mp,
							   gpos::Ref<UlongToColRefMap>,	 //colref_mapping,
							   BOOL							 //must_exist
							   ) override
	{
		return PopCopyDefault();
	}

	// conversion function
	static gpos::Ref<CLogicalJoin>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);

		return gpos::dyn_cast<CLogicalJoin>(pop);
	}

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::Ref<CColRefSet>
	DeriveOutputColumns(CMemoryPool *mp, CExpressionHandle &exprhdl) override
	{
		return PcrsDeriveOutputCombineLogical(mp, exprhdl);
	}

	// derive partition consumer info
	gpos::Ref<CPartInfo>
	DerivePartitionInfo(CMemoryPool *mp,
						CExpressionHandle &exprhdl) const override
	{
		return PpartinfoDeriveCombine(mp, exprhdl);
	}


	// derive keys
	gpos::Ref<CKeyCollection>
	DeriveKeyCollection(CMemoryPool *mp,
						CExpressionHandle &exprhdl) const override
	{
		return PkcCombineKeys(mp, exprhdl);
	}

	// derive function properties
	gpos::Ref<CFunctionProp>
	DeriveFunctionProperties(CMemoryPool *mp,
							 CExpressionHandle &exprhdl) const override
	{
		return PfpDeriveFromScalar(mp, exprhdl);
	}

	//-------------------------------------------------------------------------------------
	// Derived Stats
	//-------------------------------------------------------------------------------------

	// promise level for stat derivation
	EStatPromise
	Esp(CExpressionHandle &exprhdl) const override
	{
		// no stat derivation on Join trees with subqueries
		if (exprhdl.DeriveHasSubquery(exprhdl.Arity() - 1))
		{
			return EspLow;
		}

		if (nullptr != exprhdl.Pgexpr() &&
			exprhdl.Pgexpr()->ExfidOrigin() == CXform::ExfExpandNAryJoin)
		{
			return EspMedium;
		}

		return EspHigh;
	}

	// derive statistics
	gpos::Ref<IStatistics> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<IStatisticsArray> stats_ctxt) const override;

	//-------------------------------------------------------------------------------------
	// Required Relational Properties
	//-------------------------------------------------------------------------------------

	// compute required stat columns of the n-th child
	gpos::Ref<CColRefSet>
	PcrsStat(CMemoryPool *mp, CExpressionHandle &exprhdl,
			 gpos::Ref<CColRefSet> pcrsInput, ULONG child_index) const override
	{
		const ULONG arity = exprhdl.Arity();

		return PcrsReqdChildStats(mp, exprhdl, pcrsInput,
								  exprhdl.DeriveUsedColumns(arity - 1),
								  child_index);
	}

	// return true if operator can select a subset of input tuples based on some predicate
	BOOL
	FSelectionOp() const override
	{
		return true;
	}

	CXform::EXformId
	OriginXform()
	{
		return m_origin_xform;
	}


};	// class CLogicalJoin

}  // namespace gpopt


#endif	// !GPOS_CLogicalJoin_H

// EOF
