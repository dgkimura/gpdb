//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CSearchStage.cpp
//
//	@doc:
//		Implementation of optimizer search stage
//---------------------------------------------------------------------------

#include "gpopt/search/CSearchStage.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CCostContext.h"
#include "gpopt/xforms/CXformFactory.h"

using namespace gpopt;
using namespace gpos;

FORCE_GENERATE_DBGSTR(CSearchStage);

//---------------------------------------------------------------------------
//	@function:
//		CSearchStage::CSearchStage
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CSearchStage::CSearchStage(gpos::owner<CXformSet *> xform_set,
						   ULONG ulTimeThreshold, CCost costThreshold)
	: m_xforms(xform_set),
	  m_time_threshold(ulTimeThreshold),
	  m_cost_threshold(costThreshold),
	  m_pexprBest(nullptr),
	  m_costBest(GPOPT_INVALID_COST)
{
	GPOS_ASSERT(nullptr != m_xforms);
	GPOS_ASSERT(0 < m_xforms->Size());

	// include all implementation rules in any search strategy
	m_xforms->Union(CXformFactory::Pxff()->PxfsImplementation());

	if (GPOS_FTRACE(EopttracePrintOptimizationStatistics))
	{
		m_timer.Restart();
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CSearchStage::~CSearchStage
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CSearchStage::~CSearchStage()
{
	m_xforms->Release();
	CRefCount::SafeRelease(m_pexprBest);
}


//---------------------------------------------------------------------------
//	@function:
//		CSearchStage::OsPrint
//
//	@doc:
//		Print job
//
//---------------------------------------------------------------------------
IOstream &
CSearchStage::OsPrint(IOstream &os) const
{
	os << "Search Stage" << std::endl
	   << "\ttime threshold: " << m_time_threshold
	   << ", cost threshold:" << m_cost_threshold
	   << ", best plan found: " << std::endl;

	if (nullptr != m_pexprBest)
	{
		os << *m_pexprBest;
	}

	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CSearchStage::SetBestExpr
//
//	@doc:
//		Set best plan found at the end of search stage
//
//---------------------------------------------------------------------------
void
CSearchStage::SetBestExpr(CExpression *pexpr)
{
	GPOS_ASSERT_IMP(nullptr != pexpr, pexpr->Pop()->FPhysical());

	m_pexprBest = pexpr;
	if (nullptr != m_pexprBest)
	{
		m_costBest = m_pexprBest->Cost();
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CSearchStage::PdrgpssDefault
//
//	@doc:
//		Generate default search strategy;
//		one stage with all xforms and no time/cost thresholds
//
//---------------------------------------------------------------------------
gpos::owner<CSearchStageArray *>
CSearchStage::PdrgpssDefault(CMemoryPool *mp)
{
	gpos::owner<CXformSet *> xform_set = GPOS_NEW(mp) CXformSet(mp);
	xform_set->Union(CXformFactory::Pxff()->PxfsExploration());
	gpos::owner<CSearchStageArray *> search_stage_array =
		GPOS_NEW(mp) CSearchStageArray(mp);

	search_stage_array->Append(GPOS_NEW(mp) CSearchStage(std::move(xform_set)));

	return search_stage_array;
}

// EOF
