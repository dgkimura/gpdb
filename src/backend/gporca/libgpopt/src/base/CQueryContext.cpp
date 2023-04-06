//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CQueryContext.cpp
//
//	@doc:
//		Implementation of optimization context
//---------------------------------------------------------------------------

#include "gpopt/base/CQueryContext.h"

#include "gpos/base.h"
#include "gpos/error/CAutoTrace.h"

#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColumnFactory.h"
#include "gpopt/base/CDistributionSpecAny.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CLogicalLimit.h"

using namespace gpopt;

FORCE_GENERATE_DBGSTR(CQueryContext);

//---------------------------------------------------------------------------
//	@function:
//		CQueryContext::CQueryContext
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CQueryContext::CQueryContext(CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
							 gpos::Ref<CReqdPropPlan> prpp,
							 CColRefArray *colref_array,
							 CMDNameArray *pdrgpmdname, BOOL fDeriveStats)
	: m_prpp(prpp),
	  m_pdrgpcr(colref_array),
	  m_pdrgpcrSystemCols(nullptr),
	  m_pdrgpmdname(pdrgpmdname),
	  m_fDeriveStats(fDeriveStats)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != prpp);
	GPOS_ASSERT(nullptr != colref_array);
	GPOS_ASSERT(nullptr != pdrgpmdname);
	GPOS_ASSERT(colref_array->Size() == pdrgpmdname->Size());

#ifdef GPOS_DEBUG
	const ULONG ulReqdColumns = m_pdrgpcr->Size();
#endif	//GPOS_DEBUG

	// mark unused CTEs
	gpos::Ref<CCTEInfo> pcteinfo = COptCtxt::PoctxtFromTLS()->Pcteinfo();
	pcteinfo->MarkUnusedCTEs();

	gpos::Ref<CColRefSet> pcrsOutputAndOrderingCols =
		GPOS_NEW(mp) CColRefSet(mp);
	gpos::Ref<CColRefSet> pcrsOrderSpec =
		prpp->Peo()->PosRequired()->PcrsUsed(mp);

	pcrsOutputAndOrderingCols->Include(colref_array);
	pcrsOutputAndOrderingCols->Include(pcrsOrderSpec);
	pcrsOrderSpec->Release();

	m_pexpr = CExpressionPreprocessor::PexprPreprocess(
		mp, pexpr, pcrsOutputAndOrderingCols);

	pcrsOutputAndOrderingCols->Release();
	GPOS_ASSERT(m_pdrgpcr->Size() == ulReqdColumns);

	// collect required system columns
	SetSystemCols(mp);

	// collect CTE predicates and add them to CTE producer expressions
	CExpressionPreprocessor::AddPredsToCTEProducers(mp, m_pexpr);

	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();

	// create the mapping between the computed column, defined in the expression
	// and all CTEs, and its corresponding used columns
	MapComputedToUsedCols(col_factory, m_pexpr);
	pcteinfo->MapComputedToUsedCols(col_factory);
}


//---------------------------------------------------------------------------
//	@function:
//		CQueryContext::~CQueryContext
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CQueryContext::~CQueryContext()
{
	m_pexpr->Release();
	m_prpp->Release();
	m_pdrgpcr->Release();
	m_pdrgpmdname->Release();
	SafeRelease(m_pdrgpcrSystemCols);
}


//---------------------------------------------------------------------------
//	@function:
//		CQueryContext::PopTop
//
//	@doc:
// 		 Return top level operator in the given expression
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CQueryContext::PopTop(gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	// skip CTE anchors if any
	gpos::Ref<CExpression> pexprCurr = pexpr;
	while (COperator::EopLogicalCTEAnchor == pexprCurr->Pop()->Eopid())
	{
		pexprCurr = (*pexprCurr)[0];
		GPOS_ASSERT(nullptr != pexprCurr);
	}

	return pexprCurr->Pop();
}

//---------------------------------------------------------------------------
//	@function:
//		CQueryContext::SetReqdSystemCols
//
//	@doc:
// 		Collect system columns from output columns
//
//---------------------------------------------------------------------------
void
CQueryContext::SetSystemCols(CMemoryPool *mp)
{
	GPOS_ASSERT(nullptr == m_pdrgpcrSystemCols);
	GPOS_ASSERT(nullptr != m_pdrgpcr);

	m_pdrgpcrSystemCols = GPOS_NEW(mp) CColRefArray(mp);
	const ULONG ulReqdCols = m_pdrgpcr->Size();
	for (ULONG ul = 0; ul < ulReqdCols; ul++)
	{
		gpos::Ref<CColRef> colref = (*m_pdrgpcr)[ul];
		if (colref->IsSystemCol())
		{
			m_pdrgpcrSystemCols->Append(colref);
		}
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CQueryContext::PqcGenerate
//
//	@doc:
// 		Generate the query context for the given expression and array of
//		output column ref ids
//
//---------------------------------------------------------------------------
CQueryContext *
CQueryContext::PqcGenerate(CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
						   ULongPtrArray *pdrgpulQueryOutputColRefId,
						   CMDNameArray *pdrgpmdname, BOOL fDeriveStats)
{
	GPOS_ASSERT(nullptr != pexpr && nullptr != pdrgpulQueryOutputColRefId);

	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp);
	CColRefArray *colref_array = GPOS_NEW(mp) CColRefArray(mp);

	gpos::Ref<COptCtxt> poptctxt = COptCtxt::PoctxtFromTLS();
	CColumnFactory *col_factory = poptctxt->Pcf();
	GPOS_ASSERT(nullptr != col_factory);

	// Collect required column references (colref_array)
	const ULONG length = pdrgpulQueryOutputColRefId->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		ULONG *pul = (*pdrgpulQueryOutputColRefId)[ul];
		GPOS_ASSERT(nullptr != pul);

		gpos::Ref<CColRef> colref = col_factory->LookupColRef(*pul);
		GPOS_ASSERT(nullptr != colref);

		pcrs->Include(colref);
		colref_array->Append(colref);
	}

	// Collect required properties (prpp) at the top level:

	// By default no sort order requirement is added, unless the root operator in
	// the input logical expression is a LIMIT. This is because Orca always
	// attaches top level Sort to a LIMIT node.
	gpos::Ref<COrderSpec> pos = nullptr;
	gpos::Ref<CExpression> pexprResult = pexpr;
	gpos::Ref<COperator> popTop = PopTop(pexpr);
	if (COperator::EopLogicalLimit == popTop->Eopid())
	{
		// top level operator is a limit, copy order spec to query context
		pos = CLogicalLimit::PopConvert(popTop)->Pos();
		pos->AddRef();
	}
	else
	{
		// no order required
		pos = GPOS_NEW(mp) COrderSpec(mp);
	}

	gpos::Ref<CDistributionSpec> pds = nullptr;

	BOOL fDML = CUtils::FLogicalDML(pexpr->Pop());
	poptctxt->MarkDMLQuery(fDML);

	// DML commands do not have distribution requirement. Otherwise the
	// distribution requirement is Singleton.
	if (fDML)
	{
		pds = GPOS_NEW(mp) CDistributionSpecAny(COperator::EopSentinel);
	}
	else
	{
		pds = GPOS_NEW(mp)
			CDistributionSpecSingleton(CDistributionSpecSingleton::EstMaster);
	}

	// By default, no rewindability requirement needs to be satisfied at the top level
	gpos::Ref<CRewindabilitySpec> prs = GPOS_NEW(mp) CRewindabilitySpec(
		CRewindabilitySpec::ErtNone, CRewindabilitySpec::EmhtNoMotion);

	// No partition propagation required at the top
	gpos::Ref<CPartitionPropagationSpec> ppps =
		GPOS_NEW(mp) CPartitionPropagationSpec(mp);

	// Ensure order, distribution and rewindability meet 'satisfy' matching at the top level
	gpos::Ref<CEnfdOrder> peo =
		GPOS_NEW(mp) CEnfdOrder(pos, CEnfdOrder::EomSatisfy);
	gpos::Ref<CEnfdDistribution> ped =
		GPOS_NEW(mp) CEnfdDistribution(pds, CEnfdDistribution::EdmSatisfy);
	gpos::Ref<CEnfdRewindability> per =
		GPOS_NEW(mp) CEnfdRewindability(prs, CEnfdRewindability::ErmSatisfy);
	gpos::Ref<CEnfdPartitionPropagation> pepp = GPOS_NEW(mp)
		CEnfdPartitionPropagation(ppps, CEnfdPartitionPropagation::EppmSatisfy);

	// Required CTEs are obtained from the CTEInfo global information in the optimizer context
	gpos::Ref<CCTEReq> pcter = poptctxt->Pcteinfo()->PcterProducers(mp);

	// NB: Partition propagation requirements are not initialized here.  They are
	// constructed later based on derived relation properties (CPartInfo) by
	// CReqdPropPlan::InitReqdPartitionPropagation().

	gpos::Ref<CReqdPropPlan> prpp =
		GPOS_NEW(mp) CReqdPropPlan(pcrs, peo, ped, per, pepp, pcter);

	// Finally, create the CQueryContext
	pdrgpmdname->AddRef();
	return GPOS_NEW(mp) CQueryContext(mp, pexprResult, prpp, colref_array,
									  pdrgpmdname, fDeriveStats);
}

#ifdef GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CQueryContext::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CQueryContext::OsPrint(IOstream &os) const
{
	return os << *m_pexpr << std::endl << *m_prpp;
}
#endif	// GPOS_DEBUG


//---------------------------------------------------------------------------
//	@function:
//		CQueryContext::MapComputedToUsedCols
//
//	@doc:
//		Walk the expression and add the mapping between computed column
//		and its used columns
//
//---------------------------------------------------------------------------
void
CQueryContext::MapComputedToUsedCols(CColumnFactory *col_factory,
									 gpos::Ref<CExpression> pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	if (COperator::EopLogicalProject == pexpr->Pop()->Eopid())
	{
		gpos::Ref<CExpression> pexprPrL = (*pexpr)[1];

		const ULONG arity = pexprPrL->Arity();
		for (ULONG ul = 0; ul < arity; ul++)
		{
			gpos::Ref<CExpression> pexprPrEl = (*pexprPrL)[ul];
			col_factory->AddComputedToUsedColsMap(pexprPrEl);
		}
	}

	// process children
	const ULONG ulChildren = pexpr->Arity();
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		MapComputedToUsedCols(col_factory, (*pexpr)[ul]);
	}
}

// EOF
