//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalUnary.h
//
//	@doc:
//		Base class of logical unary operators
//---------------------------------------------------------------------------
#ifndef GPOS_CLogicalUnary_H
#define GPOS_CLogicalUnary_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogical.h"
#include "naucrates/base/IDatum.h"

namespace gpopt
{
using namespace gpnaucrates;

//---------------------------------------------------------------------------
//	@class:
//		CLogicalUnary
//
//	@doc:
//		Base class of logical unary operators
//
//---------------------------------------------------------------------------
class CLogicalUnary : public CLogical
{
private:
protected:
	// derive statistics for projection operators
	gpos::owner<IStatistics *> PstatsDeriveProject(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<UlongToIDatumMap *> phmuldatum = nullptr) const;

public:
	CLogicalUnary(const CLogicalUnary &) = delete;

	// ctor
	explicit CLogicalUnary(CMemoryPool *mp) : CLogical(mp)
	{
	}

	// dtor
	~CLogicalUnary() override = default;

	// match function
	BOOL Matches(gpos::pointer<COperator *> pop) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return true;
	}

	// return a copy of the operator with remapped columns
	gpos::owner<COperator *>
	PopCopyWithRemappedColumns(
		CMemoryPool *,						//mp,
		gpos::pointer<UlongToColRefMap *>,	//colref_mapping,
		BOOL								//must_exist
		) override
	{
		return PopCopyDefault();
	}

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive not nullable output columns
	gpos::owner<CColRefSet *>
	DeriveNotNullColumns(CMemoryPool *,	 // mp
						 CExpressionHandle &exprhdl) const override
	{
		// TODO,  03/18/2012, derive nullability of columns computed by scalar child
		return PcrsDeriveNotNullPassThruOuter(exprhdl);
	}

	// derive partition consumer info
	gpos::owner<CPartInfo *>
	DerivePartitionInfo(CMemoryPool *mp,
						CExpressionHandle &exprhdl) const override
	{
		return PpartinfoDeriveCombine(mp, exprhdl);
	}

	// derive function properties
	gpos::owner<CFunctionProp *>
	DeriveFunctionProperties(CMemoryPool *mp,
							 CExpressionHandle &exprhdl) const override
	{
		return PfpDeriveFromScalar(mp, exprhdl);
	}

	//-------------------------------------------------------------------------------------
	// Derived Stats
	//-------------------------------------------------------------------------------------

	// promise level for stat derivation
	EStatPromise Esp(CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Required Relational Properties
	//-------------------------------------------------------------------------------------

	// compute required stat columns of the n-th child
	gpos::owner<CColRefSet *>
	PcrsStat(CMemoryPool *mp, CExpressionHandle &exprhdl,
			 gpos::pointer<CColRefSet *> pcrsInput,
			 ULONG child_index) const override
	{
		return PcrsReqdChildStats(mp, exprhdl, pcrsInput,
								  exprhdl.DeriveUsedColumns(1), child_index);
	}

};	// class CLogicalUnary

}  // namespace gpopt

#endif	// !GPOS_CLogicalUnary_H

// EOF
