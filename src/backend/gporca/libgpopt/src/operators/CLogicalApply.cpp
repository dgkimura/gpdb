//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalApply.cpp
//
//	@doc:
//		Implementation of apply operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalApply.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CExpressionHandle.h"


using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalApply::CLogicalApply
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalApply::CLogicalApply(CMemoryPool *mp)
	: CLogical(mp),
	  m_pdrgpcrInner(nullptr),
	  m_eopidOriginSubq(COperator::EopSentinel)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalApply::CLogicalApply
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalApply::CLogicalApply(CMemoryPool *mp,
							 gpos::Ref<CColRefArray> pdrgpcrInner,
							 EOperatorId eopidOriginSubq)
	: CLogical(mp),
	  m_pdrgpcrInner(std::move(pdrgpcrInner)),
	  m_eopidOriginSubq(eopidOriginSubq)
{
	GPOS_ASSERT(nullptr != m_pdrgpcrInner);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalApply::~CLogicalApply
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalApply::~CLogicalApply()
{
	;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalApply::PcrsStat
//
//	@doc:
//		Compute required stat columns of the n-th child
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CLogicalApply::PcrsStat(CMemoryPool *mp, CExpressionHandle &exprhdl,
						CColRefSet *pcrsInput, ULONG child_index) const
{
	GPOS_ASSERT(3 == exprhdl.Arity());

	gpos::Ref<CColRefSet> pcrsUsed = GPOS_NEW(mp) CColRefSet(mp);
	// add columns used by scalar child
	pcrsUsed->Union(exprhdl.DeriveUsedColumns(2));

	if (0 == child_index)
	{
		// add outer references coming from inner child
		pcrsUsed->Union(exprhdl.DeriveOuterReferences(1));
	}

	gpos::Ref<CColRefSet> pcrsStat =
		PcrsReqdChildStats(mp, exprhdl, pcrsInput, pcrsUsed.get(), child_index);
	;

	return pcrsStat;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalApply::Matches
//
//	@doc:
//		Match function
//
//---------------------------------------------------------------------------
BOOL
CLogicalApply::Matches(COperator *pop) const
{
	if (pop->Eopid() == Eopid())
	{
		CColRefArray *pdrgpcrInner =
			gpos::dyn_cast<CLogicalApply>(pop)->PdrgPcrInner();
		if (nullptr == m_pdrgpcrInner || nullptr == pdrgpcrInner)
		{
			return (nullptr == m_pdrgpcrInner && nullptr == pdrgpcrInner);
		}

		return m_pdrgpcrInner->Equals(pdrgpcrInner);
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalApply::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalApply::OsPrint(IOstream &os) const
{
	os << this->SzId();
	if (nullptr != m_pdrgpcrInner)
	{
		os << " (Reqd Inner Cols: ";
		(void) CUtils::OsPrintDrgPcr(os, m_pdrgpcrInner.get());
		os << ")";
	}

	return os;
}


// EOF
