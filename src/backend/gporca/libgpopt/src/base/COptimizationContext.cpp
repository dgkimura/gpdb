//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		COptimizationContext.cpp
//
//	@doc:
//		Implementation of optimization context
//---------------------------------------------------------------------------

#include "gpopt/base/COptimizationContext.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/error/CAutoTrace.h"

#include "gpopt/base/CEnfdOrder.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/operators/CPhysicalAgg.h"
#include "gpopt/operators/CPhysicalCTEProducer.h"
#include "gpopt/operators/CPhysicalMotion.h"
#include "gpopt/operators/CPhysicalNLJoin.h"
#include "gpopt/operators/CPhysicalSort.h"
#include "gpopt/search/CGroupExpression.h"
#include "gpopt/xforms/CXformUtils.h"


using namespace gpopt;

FORCE_GENERATE_DBGSTR(COptimizationContext);

// invalid optimization context
const COptimizationContext COptimizationContext::m_ocInvalid;

// invalid optimization context pointer
const OPTCTXT_PTR COptimizationContext::m_pocInvalid = nullptr;



//---------------------------------------------------------------------------
//	@function:
//		COptimizationContext::~COptimizationContext
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
COptimizationContext::~COptimizationContext()
{
	CRefCount::SafeRelease(m_prpp);
	CRefCount::SafeRelease(m_prprel);
	CRefCount::SafeRelease(m_pdrgpstatCtxt);
}


//---------------------------------------------------------------------------
//	@function:
//		COptimizationContext::PgexprBest
//
//	@doc:
//		Best group expression accessor
//
//---------------------------------------------------------------------------
CGroupExpression *
COptimizationContext::PgexprBest() const
{
	if (nullptr == m_pccBest)
	{
		return nullptr;
	}

	return m_pccBest->Pgexpr();
}


//---------------------------------------------------------------------------
//	@function:
//		COptimizationContext::SetBest
//
//	@doc:
//		 Set best cost context
//
//---------------------------------------------------------------------------
void
COptimizationContext::SetBest(gpos::pointer<CCostContext *> pcc)
{
	GPOS_ASSERT(nullptr != pcc);

	m_pccBest = pcc;

	gpos::pointer<COperator *> pop = pcc->Pgexpr()->Pop();
	if (CUtils::FPhysicalAgg(pop) &&
		gpos::dyn_cast<CPhysicalAgg>(pop)->FMultiStage())
	{
		m_fHasMultiStageAggPlan = true;
	}
}


//---------------------------------------------------------------------------
//	@function:
//		COptimizationContext::Matches
//
//	@doc:
//		Match against another context
//
//---------------------------------------------------------------------------
BOOL
COptimizationContext::Matches(
	gpos::pointer<const COptimizationContext *> poc) const
{
	GPOS_ASSERT(nullptr != poc);

	if (m_pgroup != poc->Pgroup() ||
		m_ulSearchStageIndex != poc->UlSearchStageIndex())
	{
		return false;
	}

	gpos::pointer<CReqdPropPlan *> prppFst = this->Prpp();
	gpos::pointer<CReqdPropPlan *> prppSnd = poc->Prpp();

	// make sure we are not comparing to invalid context
	if (nullptr == prppFst || nullptr == prppSnd)
	{
		return nullptr == prppFst && nullptr == prppSnd;
	}

	return prppFst->Equals(prppSnd);
}


//---------------------------------------------------------------------------
//	@function:
//		COptimizationContext::FEqualForStats
//
//	@doc:
//		Equality function used for computing stats during costing
//
//---------------------------------------------------------------------------
BOOL
COptimizationContext::FEqualForStats(
	gpos::pointer<const COptimizationContext *> pocLeft,
	gpos::pointer<const COptimizationContext *> pocRight)
{
	GPOS_ASSERT(nullptr != pocLeft);
	GPOS_ASSERT(nullptr != pocRight);

	return pocLeft->GetReqdRelationalProps()->PcrsStat()->Equals(
			   pocRight->GetReqdRelationalProps()->PcrsStat()) &&
		   pocLeft->Pdrgpstat()->Equals(pocRight->Pdrgpstat());
}


//---------------------------------------------------------------------------
//	@function:
//		COptimizationContext::FOptimize
//
//	@doc:
//		Return true if given group expression should be optimized under
//		given context
//
//---------------------------------------------------------------------------
BOOL
COptimizationContext::FOptimize(CMemoryPool *mp,
								gpos::pointer<CGroupExpression *> pgexprParent,
								gpos::pointer<CGroupExpression *> pgexprChild,
								gpos::pointer<COptimizationContext *> pocChild,
								ULONG ulSearchStages)
{
	gpos::pointer<COperator *> pop = pgexprChild->Pop();

	if (CUtils::FPhysicalMotion(pop))
	{
		return FOptimizeMotion(mp, pgexprParent, pgexprChild, pocChild,
							   ulSearchStages);
	}

	if (COperator::EopPhysicalSort == pop->Eopid())
	{
		return FOptimizeSort(mp, pgexprParent, pgexprChild, pocChild,
							 ulSearchStages);
	}

	if (CUtils::FPhysicalAgg(pop))
	{
		return FOptimizeAgg(mp, pgexprParent, pgexprChild, pocChild,
							ulSearchStages);
	}

	if (CUtils::FNLJoin(pop))
	{
		return FOptimizeNLJoin(mp, pgexprParent, pgexprChild, pocChild,
							   ulSearchStages);
	}

	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		COptimizationContext::FEqualIds
//
//	@doc:
//		Compare array of optimization contexts based on context ids
//
//---------------------------------------------------------------------------
BOOL
COptimizationContext::FEqualContextIds(
	gpos::pointer<COptimizationContextArray *> pdrgpocFst,
	gpos::pointer<COptimizationContextArray *> pdrgpocSnd)
{
	if (nullptr == pdrgpocFst || nullptr == pdrgpocSnd)
	{
		return (nullptr == pdrgpocFst && nullptr == pdrgpocSnd);
	}

	const ULONG ulCtxts = pdrgpocFst->Size();
	if (ulCtxts != pdrgpocSnd->Size())
	{
		return false;
	}

	BOOL fEqual = true;
	for (ULONG ul = 0; fEqual && ul < ulCtxts; ul++)
	{
		fEqual = (*pdrgpocFst)[ul]->Id() == (*pdrgpocSnd)[ul]->Id();
	}

	return fEqual;
}


//---------------------------------------------------------------------------
//	@function:
//		COptimizationContext::FOptimizeMotion
//
//	@doc:
//		Check if a Motion node should be optimized for the given context
//
//---------------------------------------------------------------------------
BOOL
COptimizationContext::FOptimizeMotion(
	CMemoryPool *,						// mp
	gpos::pointer<CGroupExpression *>,	// pgexprParent
	gpos::pointer<CGroupExpression *> pgexprMotion,
	gpos::pointer<COptimizationContext *> poc,
	ULONG  // ulSearchStages
)
{
	GPOS_ASSERT(nullptr != pgexprMotion);
	GPOS_ASSERT(nullptr != poc);
	GPOS_ASSERT(CUtils::FPhysicalMotion(pgexprMotion->Pop()));

	gpos::pointer<CPhysicalMotion *> pop =
		gpos::dyn_cast<CPhysicalMotion>(pgexprMotion->Pop());

	return poc->Prpp()->Ped()->FCompatible(pop->Pds());
}


//---------------------------------------------------------------------------
//	@function:
//		COptimizationContext::FOptimizeSort
//
//	@doc:
//		Check if a Sort node should be optimized for the given context
//
//---------------------------------------------------------------------------
BOOL
COptimizationContext::FOptimizeSort(
	CMemoryPool *,						// mp
	gpos::pointer<CGroupExpression *>,	// pgexprParent
	gpos::pointer<CGroupExpression *> pgexprSort,
	gpos::pointer<COptimizationContext *> poc,
	ULONG  // ulSearchStages
)
{
	GPOS_ASSERT(nullptr != pgexprSort);
	GPOS_ASSERT(nullptr != poc);
	GPOS_ASSERT(COperator::EopPhysicalSort == pgexprSort->Pop()->Eopid());

	gpos::pointer<CPhysicalSort *> pop =
		gpos::dyn_cast<CPhysicalSort>(pgexprSort->Pop());

	return poc->Prpp()->Peo()->FCompatible(
		const_cast<COrderSpec *>(pop->Pos()));
}


//---------------------------------------------------------------------------
//	@function:
//		COptimizationContext::FOptimizeAgg
//
//	@doc:
//		Check if Agg node should be optimized for the given context
//
//---------------------------------------------------------------------------
BOOL
COptimizationContext::FOptimizeAgg(
	CMemoryPool *mp,
	gpos::pointer<CGroupExpression *>,	// pgexprParent
	gpos::pointer<CGroupExpression *> pgexprAgg,
	gpos::pointer<COptimizationContext *> poc, ULONG ulSearchStages)
{
	GPOS_ASSERT(nullptr != pgexprAgg);
	GPOS_ASSERT(nullptr != poc);
	GPOS_ASSERT(CUtils::FPhysicalAgg(pgexprAgg->Pop()));
	GPOS_ASSERT(0 < ulSearchStages);

	if (GPOS_FTRACE(EopttraceForceExpandedMDQAs))
	{
		BOOL fHasMultipleDistinctAggs =
			gpos::dyn_cast<CDrvdPropScalar>((*pgexprAgg)[1]->Pdp())
				->HasMultipleDistinctAggs();
		if (fHasMultipleDistinctAggs)
		{
			// do not optimize plans with MDQAs, since preference is for plans with expanded MDQAs
			return false;
		}
	}

	if (!GPOS_FTRACE(EopttraceForceMultiStageAgg))
	{
		// no preference for multi-stage agg, we always proceed with optimization
		return true;
	}

	// otherwise, we need to avoid optimizing node unless it is a multi-stage agg
	gpos::pointer<COptimizationContext *> pocFound =
		pgexprAgg->Pgroup()->PocLookupBest(mp, ulSearchStages, poc->Prpp());
	if (nullptr != pocFound && pocFound->FHasMultiStageAggPlan())
	{
		// context already has a multi-stage agg plan, optimize child only if it is also a multi-stage agg
		return gpos::dyn_cast<CPhysicalAgg>(pgexprAgg->Pop())->FMultiStage();
	}

	// child context has no plan yet, return true
	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalNLJoin::FOptimizeNLJoin
//
//	@doc:
//		Check if NL join node should be optimized for the given context
//
//---------------------------------------------------------------------------
BOOL
COptimizationContext::FOptimizeNLJoin(
	CMemoryPool *mp,
	gpos::pointer<CGroupExpression *>,	// pgexprParent
	gpos::pointer<CGroupExpression *> pgexprJoin,
	gpos::pointer<COptimizationContext *> poc,
	ULONG  // ulSearchStages
)
{
	GPOS_ASSERT(nullptr != pgexprJoin);
	GPOS_ASSERT(nullptr != poc);
	GPOS_ASSERT(CUtils::FNLJoin(pgexprJoin->Pop()));

	gpos::pointer<COperator *> pop = pgexprJoin->Pop();
	if (!CUtils::FCorrelatedNLJoin(pop))
	{
		return true;
	}

	// for correlated join, the requested columns must be covered by outer child
	// columns and columns to be generated from inner child
	gpos::pointer<CPhysicalNLJoin *> popNLJoin =
		gpos::dyn_cast<CPhysicalNLJoin>(pop);
	gpos::owner<CColRefSet *> pcrs =
		GPOS_NEW(mp) CColRefSet(mp, popNLJoin->PdrgPcrInner());
	gpos::pointer<CColRefSet *> pcrsOuterChild =
		gpos::dyn_cast<CDrvdPropRelational>((*pgexprJoin)[0]->Pdp())
			->GetOutputColumns();
	pcrs->Include(pcrsOuterChild);
	BOOL fIncluded = pcrs->ContainsAll(poc->Prpp()->PcrsRequired());
	pcrs->Release();

	return fIncluded;
}


//---------------------------------------------------------------------------
//	@function:
//		COptimizationContext::PrppCTEProducer
//
//	@doc:
//		Compute required properties to CTE producer based on plan properties
//		of CTE consumer
//
//---------------------------------------------------------------------------
gpos::owner<CReqdPropPlan *>
COptimizationContext::PrppCTEProducer(CMemoryPool *mp,
									  gpos::pointer<COptimizationContext *> poc,
									  ULONG ulSearchStages)
{
	GPOS_ASSERT(nullptr != poc);
	GPOS_ASSERT(nullptr != poc->PccBest());

	gpos::pointer<CCostContext *> pccBest = poc->PccBest();
	gpos::pointer<CGroupExpression *> pgexpr = pccBest->Pgexpr();
	BOOL fOptimizeCTESequence =
		(COperator::EopPhysicalSequence == pgexpr->Pop()->Eopid() &&
		 (*pgexpr)[0]->FHasCTEProducer());

	if (!fOptimizeCTESequence)
	{
		// best group expression is not a CTE sequence
		return nullptr;
	}

	gpos::pointer<COptimizationContext *> pocProducer =
		(*pgexpr)[0]->PocLookupBest(mp, ulSearchStages,
									(*pccBest->Pdrgpoc())[0]->Prpp());
	if (nullptr == pocProducer)
	{
		return nullptr;
	}

	gpos::pointer<CCostContext *> pccProducer = pocProducer->PccBest();
	if (nullptr == pccProducer)
	{
		return nullptr;
	}
	gpos::pointer<COptimizationContext *> pocConsumer =
		(*pgexpr)[1]->PocLookupBest(mp, ulSearchStages,
									(*pccBest->Pdrgpoc())[1]->Prpp());
	if (nullptr == pocConsumer)
	{
		return nullptr;
	}

	gpos::pointer<CCostContext *> pccConsumer = pocConsumer->PccBest();
	if (nullptr == pccConsumer)
	{
		return nullptr;
	}

	gpos::pointer<CColRefSet *> pcrsInnerOutput =
		gpos::dyn_cast<CDrvdPropRelational>((*pgexpr)[1]->Pdp())
			->GetOutputColumns();
	gpos::pointer<CPhysicalCTEProducer *> popProducer =
		gpos::dyn_cast<CPhysicalCTEProducer>(pccProducer->Pgexpr()->Pop());
	gpos::owner<UlongToColRefMap *> colref_mapping =
		COptCtxt::PoctxtFromTLS()->Pcteinfo()->PhmulcrConsumerToProducer(
			mp, popProducer->UlCTEId(), pcrsInnerOutput,
			popProducer->Pdrgpcr());
	gpos::owner<CReqdPropPlan *> prppProducer = CReqdPropPlan::PrppRemapForCTE(
		mp, pocProducer->Prpp(), pccConsumer->Pdpplan(), colref_mapping);
	colref_mapping->Release();

	if (prppProducer->Equals(pocProducer->Prpp()))
	{
		prppProducer->Release();

		return nullptr;
	}

	return prppProducer;
}


//---------------------------------------------------------------------------
//	@function:
//		COptimizationContext::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
COptimizationContext::OsPrint(IOstream &os) const
{
	return OsPrintWithPrefix(os, "");
}

IOstream &
COptimizationContext::OsPrintWithPrefix(IOstream &os,
										const CHAR *szPrefix) const
{
	os << szPrefix << m_id << " (stage " << m_ulSearchStageIndex << "): ("
	   << *m_prpp << ") => Best Expr:";
	if (nullptr != PgexprBest())
	{
		os << PgexprBest()->Id();
	}
	os << std::endl;

	return os;
}

// EOF
