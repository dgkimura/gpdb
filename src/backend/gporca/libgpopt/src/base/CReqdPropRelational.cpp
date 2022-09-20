//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 - 2011 EMC Corp.
//
//	@filename:
//		CReqdPropRelational.cpp
//
//	@doc:
//		Required relational properties;
//---------------------------------------------------------------------------

#include "gpopt/base/CReqdPropRelational.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/engine/CEngine.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogical.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CReqdPropRelational::CReqdPropRelational
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CReqdPropRelational::CReqdPropRelational()

	= default;

//---------------------------------------------------------------------------
//	@function:
//		CReqdPropRelational::CReqdPropRelational
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CReqdPropRelational::CReqdPropRelational(gpos::Ref<CColRefSet> pcrs)
	: m_pcrsStat(std::move(pcrs))
{
	GPOS_ASSERT(nullptr != m_pcrsStat);
}

//---------------------------------------------------------------------------
//	@function:
//		CReqdPropRelational::CReqdPropRelational
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CReqdPropRelational::CReqdPropRelational(gpos::Ref<CColRefSet> pcrs,
										 gpos::Ref<CExpression> pexprPartPred)
	: m_pcrsStat(std::move(pcrs)), m_pexprPartPred(std::move(pexprPartPred))
{
	GPOS_ASSERT(nullptr != m_pcrsStat);
	GPOS_ASSERT_IMP(nullptr != m_pexprPartPred,
					m_pexprPartPred->Pop()->FScalar());
}

//---------------------------------------------------------------------------
//	@function:
//		CReqdPropRelational::~CReqdPropRelational
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CReqdPropRelational::~CReqdPropRelational()
{
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CReqdPropRelational::Compute
//
//	@doc:
//		Compute required props
//
//---------------------------------------------------------------------------
void
CReqdPropRelational::Compute(CMemoryPool *mp, CExpressionHandle &exprhdl,
							 CReqdProp *prpInput, ULONG child_index,
							 CDrvdPropArray *,	// pdrgpdpCtxt
							 ULONG				// ulOptReq
)
{
	GPOS_CHECK_ABORT;

	CReqdPropRelational *prprelInput =
		gpos::dyn_cast<CReqdPropRelational>(prpInput);
	CLogical *popLogical = gpos::dyn_cast<CLogical>(exprhdl.Pop());

	m_pcrsStat =
		popLogical->PcrsStat(mp, exprhdl, prprelInput->PcrsStat(), child_index);
	m_pexprPartPred = popLogical->PexprPartPred(
		mp, exprhdl, prprelInput->PexprPartPred(), child_index);

	exprhdl.DeriveProducerStats(child_index, m_pcrsStat.get());
}

//---------------------------------------------------------------------------
//	@function:
//		CReqdPropRelational::GetReqdRelationalProps
//
//	@doc:
//		Short hand for conversion
//
//---------------------------------------------------------------------------
CReqdPropRelational *
CReqdPropRelational::GetReqdRelationalProps(CReqdProp *prp)
{
	return dynamic_cast<CReqdPropRelational *>(prp);
}


//---------------------------------------------------------------------------
//	@function:
//		CReqdPropRelational::PrprelDifference
//
//	@doc:
//		Return difference from given properties
//
//---------------------------------------------------------------------------
gpos::Ref<CReqdPropRelational>
CReqdPropRelational::PrprelDifference(CMemoryPool *mp,
									  CReqdPropRelational *prprel)
{
	GPOS_ASSERT(nullptr != prprel);

	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp);
	pcrs->Union(m_pcrsStat.get());
	pcrs->Difference(prprel->PcrsStat());

	return GPOS_NEW(mp) CReqdPropRelational(std::move(pcrs));
}


//---------------------------------------------------------------------------
//	@function:
//		CReqdPropRelational::IsEmpty
//
//	@doc:
//		Return true if property container is empty
//
//---------------------------------------------------------------------------
BOOL
CReqdPropRelational::IsEmpty() const
{
	return m_pcrsStat->Size() == 0;
}

//---------------------------------------------------------------------------
//	@function:
//		CReqdPropRelational::OsPrint
//
//	@doc:
//		Print function
//
//---------------------------------------------------------------------------
IOstream &
CReqdPropRelational::OsPrint(IOstream &os) const
{
	os << "req stat columns: [" << *m_pcrsStat << "]";
	if (nullptr != m_pexprPartPred)
	{
		os << ", partition predicate: " << *m_pexprPartPred;
	}

	return os;
}


// EOF
