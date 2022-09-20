//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 - 2011 EMC CORP.
//
//	@filename:
//		CReqdPropRelational.h
//
//	@doc:
//		Derived required relational properties
//---------------------------------------------------------------------------
#ifndef GPOPT_CReqdPropRelational_H
#define GPOPT_CReqdPropRelational_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CReqdProp.h"
#include "gpopt/operators/CExpression_Fwd.h"

namespace gpopt
{
using namespace gpos;

// forward declaration
class CExpressionHandle;

//---------------------------------------------------------------------------
//	@class:
//		CReqdPropRelational
//
//	@doc:
//		Required relational properties container.
//
//---------------------------------------------------------------------------
class CReqdPropRelational : public CReqdProp
{
private:
	// required stat columns
	gpos::Ref<CColRefSet> m_pcrsStat{nullptr};

	// predicate on partition key
	gpos::Ref<CExpression> m_pexprPartPred{nullptr};

public:
	CReqdPropRelational(const CReqdPropRelational &) = delete;

	// default ctor
	CReqdPropRelational();

	// ctor
	explicit CReqdPropRelational(gpos::Ref<CColRefSet> pcrs);

	// ctor
	CReqdPropRelational(gpos::Ref<CColRefSet> pcrs,
						gpos::Ref<CExpression> pexprPartPred);

	// dtor
	~CReqdPropRelational() override;

	// type of properties
	BOOL
	FRelational() const override
	{
		GPOS_ASSERT(!FPlan());
		return true;
	}

	// stat columns accessor
	CColRefSet *
	PcrsStat() const
	{
		return m_pcrsStat.get();
	}

	// partition predicate accessor
	CExpression *
	PexprPartPred() const
	{
		return m_pexprPartPred.get();
	}

	// required properties computation function
	void Compute(CMemoryPool *mp, CExpressionHandle &exprhdl,
				 CReqdProp *prpInput, ULONG child_index,
				 CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) override;

	// return difference from given properties
	gpos::Ref<CReqdPropRelational> PrprelDifference(
		CMemoryPool *mp, CReqdPropRelational *prprel);

	// return true if property container is empty
	BOOL IsEmpty() const;

	// shorthand for conversion
	static CReqdPropRelational *GetReqdRelationalProps(CReqdProp *prp);

	// print function
	IOstream &OsPrint(IOstream &os) const override;

};	// class CReqdPropRelational

}  // namespace gpopt


#endif	// !GPOPT_CReqdPropRelational_H

// EOF
