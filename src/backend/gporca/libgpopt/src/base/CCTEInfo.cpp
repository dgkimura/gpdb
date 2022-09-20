//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CCTEInfo.cpp
//
//	@doc:
//		Information about CTEs in a query
//---------------------------------------------------------------------------

#include "gpopt/base/CCTEInfo.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CCTEReq.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CQueryContext.h"
#include "gpopt/operators/CExpressionPreprocessor.h"
#include "gpopt/operators/CLogicalCTEConsumer.h"
#include "gpopt/operators/CLogicalCTEProducer.h"


using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::CCTEInfoEntry::CCTEInfoEntry
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CCTEInfo::CCTEInfoEntry::CCTEInfoEntry(CMemoryPool *mp,
									   gpos::Ref<CExpression> pexprCTEProducer)
	: m_mp(mp),
	  m_pexprCTEProducer(std::move(pexprCTEProducer)),
	  m_phmcrulConsumers(nullptr),
	  m_fUsed(true)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != m_pexprCTEProducer);

	m_phmcrulConsumers = GPOS_NEW(mp) ColRefToUlongMap(mp);
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::CCTEInfoEntry::CCTEInfoEntry
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CCTEInfo::CCTEInfoEntry::CCTEInfoEntry(CMemoryPool *mp,
									   gpos::Ref<CExpression> pexprCTEProducer,
									   BOOL fUsed)
	: m_mp(mp),
	  m_pexprCTEProducer(std::move(pexprCTEProducer)),
	  m_phmcrulConsumers(nullptr),
	  m_fUsed(fUsed)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != m_pexprCTEProducer);

	m_phmcrulConsumers = GPOS_NEW(mp) ColRefToUlongMap(mp);
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::CCTEInfoEntry::~CCTEInfoEntry
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CCTEInfo::CCTEInfoEntry::~CCTEInfoEntry()
{
	;
	;
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::CCTEInfoEntry::AddConsumerCols
//
//	@doc:
//		Add given columns to consumers column map
//
//---------------------------------------------------------------------------
void
CCTEInfo::CCTEInfoEntry::AddConsumerCols(CColRefArray *colref_array)
{
	GPOS_ASSERT(nullptr != colref_array);

	const ULONG size = colref_array->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		CColRef *colref = (*colref_array)[ul];
		if (nullptr == m_phmcrulConsumers->Find(colref))
		{
			BOOL fSuccess GPOS_ASSERTS_ONLY =
				m_phmcrulConsumers->Insert(colref, GPOS_NEW(m_mp) ULONG(ul));
			GPOS_ASSERT(fSuccess);
		}
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::CCTEInfoEntry::UlConsumerColPos
//
//	@doc:
//		Return position of given consumer column,
//		return gpos::ulong_max if column is not found in local map
//
//---------------------------------------------------------------------------
ULONG
CCTEInfo::CCTEInfoEntry::UlConsumerColPos(CColRef *colref)
{
	GPOS_ASSERT(nullptr != colref);

	ULONG *pul = m_phmcrulConsumers->Find(colref);
	if (nullptr == pul)
	{
		return gpos::ulong_max;
	}

	return *pul;
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::CCTEInfoEntry::UlCTEId
//
//	@doc:
//		CTE id
//
//---------------------------------------------------------------------------
ULONG
CCTEInfo::CCTEInfoEntry::UlCTEId() const
{
	return gpos::dyn_cast<CLogicalCTEProducer>(m_pexprCTEProducer->Pop())
		->UlCTEId();
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::CCTEInfo
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CCTEInfo::CCTEInfo(CMemoryPool *mp)
	: m_mp(mp),
	  m_phmulcteinfoentry(nullptr),
	  m_ulNextCTEId(0),
	  m_fEnableInlining(true)
{
	GPOS_ASSERT(nullptr != mp);
	m_phmulcteinfoentry = GPOS_NEW(m_mp) UlongToCTEInfoEntryMap(m_mp);
	m_phmulprodconsmap = GPOS_NEW(m_mp) UlongToProducerConsumerMap(m_mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::~CCTEInfo
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CCTEInfo::~CCTEInfo()
{
	;
	;
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::PreprocessCTEProducer
//
//	@doc:
//		Preprocess CTE producer expression
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CCTEInfo::PexprPreprocessCTEProducer(const CExpression *pexprCTEProducer)
{
	GPOS_ASSERT(nullptr != pexprCTEProducer);

	CExpression *pexprProducerChild = (*pexprCTEProducer)[0];

	// get cte output cols for preprocessing use
	CColRefSet *pcrsOutput =
		gpos::dyn_cast<CLogicalCTEProducer>(pexprCTEProducer->Pop())
			->DeriveOutputColumns();

	gpos::Ref<CExpression> pexprChildPreprocessed =
		CExpressionPreprocessor::PexprPreprocess(m_mp, pexprProducerChild,
												 pcrsOutput);

	gpos::Ref<COperator> pop = pexprCTEProducer->Pop();
	;

	gpos::Ref<CExpression> pexprProducerPreprocessed = GPOS_NEW(m_mp)
		CExpression(m_mp, std::move(pop), std::move(pexprChildPreprocessed));

	pexprProducerPreprocessed->ResetStats();
	InitDefaultStats(pexprProducerPreprocessed.get());

	return pexprProducerPreprocessed;
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::AddCTEProducer
//
//	@doc:
//		Add CTE producer to hashmap
//
//---------------------------------------------------------------------------
void
CCTEInfo::AddCTEProducer(CExpression *pexprCTEProducer)
{
	gpos::Ref<CExpression> pexprProducerToAdd =
		PexprPreprocessCTEProducer(pexprCTEProducer);

	COperator *pop = pexprCTEProducer->Pop();
	ULONG ulCTEId = gpos::dyn_cast<CLogicalCTEProducer>(pop)->UlCTEId();

	BOOL fInserted GPOS_ASSERTS_ONLY = m_phmulcteinfoentry->Insert(
		GPOS_NEW(m_mp) ULONG(ulCTEId),
		GPOS_NEW(m_mp) CCTEInfoEntry(m_mp, std::move(pexprProducerToAdd)));
	GPOS_ASSERT(fInserted);
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::ReplaceCTEProducer
//
//	@doc:
//		Replace cte producer with given expression
//
//---------------------------------------------------------------------------
void
CCTEInfo::ReplaceCTEProducer(CExpression *pexprCTEProducer)
{
	COperator *pop = pexprCTEProducer->Pop();
	ULONG ulCTEId = gpos::dyn_cast<CLogicalCTEProducer>(pop)->UlCTEId();

	CCTEInfoEntry *pcteinfoentry = m_phmulcteinfoentry->Find(&ulCTEId);
	GPOS_ASSERT(nullptr != pcteinfoentry);

#ifdef GPOS_DBUG
	CExpression *pexprCTEProducerOld = pcteinfoentry->Pexpr();
	COperator *popCTEProducerOld = pexprCTEProducerOld->Pop();
	GPOS_ASSERT(ulCTEId ==
				CLogicalCTEProducer::PopConvert(popCTEProducerOld)->UlCTEId());
#endif	// GPOS_DEBUG

	gpos::Ref<CExpression> pexprCTEProducerNew =
		PexprPreprocessCTEProducer(pexprCTEProducer);

	BOOL fReplaced GPOS_ASSERTS_ONLY = m_phmulcteinfoentry->Replace(
		&ulCTEId,
		GPOS_NEW(m_mp) CCTEInfoEntry(m_mp, std::move(pexprCTEProducerNew),
									 pcteinfoentry->FUsed()));
	GPOS_ASSERT(fReplaced);
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::InitDefaultStats
//
//	@doc:
//		Initialize default statistics for a given CTE Producer
//
//---------------------------------------------------------------------------
void
CCTEInfo::InitDefaultStats(CExpression *pexprCTEProducer)
{
	// Generate statistics with empty requirement. This handles cases when
	// the CTE is a N-Ary join that will require statistics calculation

	gpos::Ref<CReqdPropRelational> prprel =
		GPOS_NEW(m_mp) CReqdPropRelational(GPOS_NEW(m_mp) CColRefSet(m_mp));
	(void) pexprCTEProducer->PstatsDerive(prprel, nullptr /* stats_ctxt */);

	// cleanup
	;
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::DeriveProducerStats
//
//	@doc:
//		Derive the statistics on the CTE producer
//
//---------------------------------------------------------------------------
void
CCTEInfo::DeriveProducerStats(CLogicalCTEConsumer *popConsumer,
							  CColRefSet *pcrsStat)
{
	const ULONG ulCTEId = popConsumer->UlCTEId();

	CCTEInfoEntry *pcteinfoentry = m_phmulcteinfoentry->Find(&ulCTEId);
	GPOS_ASSERT(nullptr != pcteinfoentry);

	CExpression *pexprCTEProducer = pcteinfoentry->Pexpr();

	// Given the subset of CTE consumer columns needed for statistics derivation,
	// compute its corresponding set of columns in the CTE Producer
	gpos::Ref<CColRefSet> pcrsCTEProducer =
		CUtils::PcrsCTEProducerColumns(m_mp, pcrsStat, popConsumer);
	GPOS_ASSERT(pcrsStat->Size() == pcrsCTEProducer->Size());

	gpos::Ref<CReqdPropRelational> prprel =
		GPOS_NEW(m_mp) CReqdPropRelational(std::move(pcrsCTEProducer));
	(void) pexprCTEProducer->PstatsDerive(prprel, nullptr /* stats_ctxt */);

	// cleanup
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::PexprCTEProducer
//
//	@doc:
//		Return the logical cte producer with given id
//
//---------------------------------------------------------------------------
CExpression *
CCTEInfo::PexprCTEProducer(ULONG ulCTEId) const
{
	const CCTEInfoEntry *pcteinfoentry = m_phmulcteinfoentry->Find(&ulCTEId);
	GPOS_ASSERT(nullptr != pcteinfoentry);

	return pcteinfoentry->Pexpr();
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::UlConsumersInParent
//
//	@doc:
//		Number of consumers of given CTE inside a given parent
//
//---------------------------------------------------------------------------
ULONG
CCTEInfo::UlConsumersInParent(ULONG ulConsumerId, ULONG ulParentId) const
{
	// get map of given parent
	const UlongToConsumerCounterMap *phmulconsumermap =
		m_phmulprodconsmap->Find(&ulParentId);
	if (nullptr == phmulconsumermap)
	{
		return 0;
	}

	// find counter of given consumer inside this map
	const SConsumerCounter *pconsumercounter =
		phmulconsumermap->Find(&ulConsumerId);
	if (nullptr == pconsumercounter)
	{
		return 0;
	}

	return pconsumercounter->UlCount();
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::UlConsumers
//
//	@doc:
//		Return number of CTE consumers of given CTE
//
//---------------------------------------------------------------------------
ULONG
CCTEInfo::UlConsumers(ULONG ulCTEId) const
{
	// find consumers in main query
	ULONG ulConsumers = UlConsumersInParent(ulCTEId, gpos::ulong_max);

	// find consumers in other CTEs
	UlongToCTEInfoEntryMapIter hmulei(m_phmulcteinfoentry.get());
	while (hmulei.Advance())
	{
		const CCTEInfoEntry *pcteinfoentry = hmulei.Value();
		if (pcteinfoentry->FUsed())
		{
			ulConsumers +=
				UlConsumersInParent(ulCTEId, pcteinfoentry->UlCTEId());
		}
	}

	return ulConsumers;
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::FUsed
//
//	@doc:
//		Check if given CTE is used
//
//---------------------------------------------------------------------------
BOOL
CCTEInfo::FUsed(ULONG ulCTEId) const
{
	CCTEInfoEntry *pcteinfoentry = m_phmulcteinfoentry->Find(&ulCTEId);
	GPOS_ASSERT(nullptr != pcteinfoentry);
	return pcteinfoentry->FUsed();
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::IncrementConsumers
//
//	@doc:
//		Increment number of CTE consumers
//
//---------------------------------------------------------------------------
void
CCTEInfo::IncrementConsumers(ULONG ulConsumerId, ULONG ulParentCTEId)
{
	// get map of given parent
	gpos::Ref<UlongToConsumerCounterMap> phmulconsumermap =
		m_phmulprodconsmap->Find(&ulParentCTEId);
	if (nullptr == phmulconsumermap)
	{
		phmulconsumermap = GPOS_NEW(m_mp) UlongToConsumerCounterMap(m_mp);
		BOOL fInserted GPOS_ASSERTS_ONLY = m_phmulprodconsmap->Insert(
			GPOS_NEW(m_mp) ULONG(ulParentCTEId), phmulconsumermap);
		GPOS_ASSERT(fInserted);
	}

	// find counter of given consumer inside this map
	SConsumerCounter *pconsumercounter = phmulconsumermap->Find(&ulConsumerId);
	if (nullptr == pconsumercounter)
	{
		// no existing counter - start a new one
		BOOL fInserted GPOS_ASSERTS_ONLY = phmulconsumermap->Insert(
			GPOS_NEW(m_mp) ULONG(ulConsumerId),
			GPOS_NEW(m_mp) SConsumerCounter(ulConsumerId));
		GPOS_ASSERT(fInserted);
	}
	else
	{
		// counter already exists - just increment it
		pconsumercounter->Increment();
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::PcterProducers
//
//	@doc:
//		Return a CTE requirement with all the producers as optional
//
//---------------------------------------------------------------------------
gpos::Ref<CCTEReq>
CCTEInfo::PcterProducers(CMemoryPool *mp) const
{
	gpos::Ref<CCTEReq> pcter = GPOS_NEW(mp) CCTEReq(mp);

	UlongToCTEInfoEntryMapIter hmulei(m_phmulcteinfoentry.get());
	while (hmulei.Advance())
	{
		const CCTEInfoEntry *pcteinfoentry = hmulei.Value();
		pcter->Insert(pcteinfoentry->UlCTEId(), CCTEMap::EctProducer,
					  false /*fRequired*/, nullptr /*pdpplan*/);
	}

	return pcter;
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::PdrgPexpr
//
//	@doc:
//		Return an array of all stored CTE expressions
//
//---------------------------------------------------------------------------
gpos::Ref<CExpressionArray>
CCTEInfo::PdrgPexpr(CMemoryPool *mp) const
{
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	UlongToCTEInfoEntryMapIter hmulei(m_phmulcteinfoentry.get());
	while (hmulei.Advance())
	{
		gpos::Ref<CExpression> pexpr = hmulei.Value()->Pexpr();
		;
		pdrgpexpr->Append(pexpr);
	}

	return pdrgpexpr;
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::MapComputedToUsedCols
//
//	@doc:
//		Walk the producer expressions and add the mapping between computed
//		column and its used columns
//
//---------------------------------------------------------------------------
void
CCTEInfo::MapComputedToUsedCols(CColumnFactory *col_factory) const
{
	UlongToCTEInfoEntryMapIter hmulei(m_phmulcteinfoentry.get());
	while (hmulei.Advance())
	{
		CExpression *pexprProducer = hmulei.Value()->Pexpr();
		GPOS_ASSERT(nullptr != pexprProducer);
		CQueryContext::MapComputedToUsedCols(col_factory, pexprProducer);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::AddConsumerCols
//
//	@doc:
//		Add given columns to consumers column map
//
//---------------------------------------------------------------------------
void
CCTEInfo::AddConsumerCols(ULONG ulCTEId, CColRefArray *colref_array)
{
	GPOS_ASSERT(nullptr != colref_array);

	CCTEInfoEntry *pcteinfoentry = m_phmulcteinfoentry->Find(&ulCTEId);
	GPOS_ASSERT(nullptr != pcteinfoentry);

	pcteinfoentry->AddConsumerCols(colref_array);
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::UlConsumerColPos
//
//	@doc:
//		Return position of given consumer column in consumer output
//
//---------------------------------------------------------------------------
ULONG
CCTEInfo::UlConsumerColPos(ULONG ulCTEId, CColRef *colref)
{
	GPOS_ASSERT(nullptr != colref);

	CCTEInfoEntry *pcteinfoentry = m_phmulcteinfoentry->Find(&ulCTEId);
	GPOS_ASSERT(nullptr != pcteinfoentry);

	return pcteinfoentry->UlConsumerColPos(colref);
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::FindConsumersInParent
//
//	@doc:
//		Find all CTE consumers inside given parent, and push them to the given stack
//
//---------------------------------------------------------------------------
void
CCTEInfo::FindConsumersInParent(ULONG ulParentId, CBitSet *pbsUnusedConsumers,
								CStack<ULONG> *pstack)
{
	UlongToConsumerCounterMap *phmulconsumermap =
		m_phmulprodconsmap->Find(&ulParentId);
	if (nullptr == phmulconsumermap)
	{
		// no map found for given parent - there are no consumers inside it
		return;
	}

	UlongToConsumerCounterMapIter hmulci(phmulconsumermap);
	while (hmulci.Advance())
	{
		const SConsumerCounter *pconsumercounter = hmulci.Value();
		ULONG ulConsumerId = pconsumercounter->UlCTEId();
		if (pbsUnusedConsumers->Get(ulConsumerId))
		{
			pstack->Push(GPOS_NEW(m_mp) ULONG(ulConsumerId));
			pbsUnusedConsumers->ExchangeClear(ulConsumerId);
		}
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::MarkUnusedCTEs
//
//	@doc:
//		Mark unused CTEs
//
//---------------------------------------------------------------------------
void
CCTEInfo::MarkUnusedCTEs()
{
	gpos::Ref<CBitSet> pbsUnusedConsumers = GPOS_NEW(m_mp) CBitSet(m_mp);

	// start with all CTEs
	UlongToCTEInfoEntryMapIter hmulei(m_phmulcteinfoentry.get());
	while (hmulei.Advance())
	{
		const CCTEInfoEntry *pcteinfoentry = hmulei.Value();
		pbsUnusedConsumers->ExchangeSet(pcteinfoentry->UlCTEId());
	}

	// start with the main query and find out which CTEs are used there
	CStack<ULONG> stack(m_mp);
	FindConsumersInParent(gpos::ulong_max, pbsUnusedConsumers.get(), &stack);

	// repeatedly find CTEs that are used in these CTEs
	while (!stack.IsEmpty())
	{
		// get one CTE id from list, and see which consumers are inside this CTE
		ULONG *pulCTEId = stack.Pop();
		FindConsumersInParent(*pulCTEId, pbsUnusedConsumers.get(), &stack);
		GPOS_DELETE(pulCTEId);
	}

	// now the only CTEs remaining in the bitset are the unused ones. mark them as such
	UlongToCTEInfoEntryMapIter hmulei2(m_phmulcteinfoentry.get());
	while (hmulei2.Advance())
	{
		CCTEInfoEntry *pcteinfoentry =
			const_cast<CCTEInfoEntry *>(hmulei2.Value());
		if (pbsUnusedConsumers->Get(pcteinfoentry->UlCTEId()))
		{
			pcteinfoentry->MarkUnused();
		}
	}

	;
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::PhmulcrConsumerToProducer
//
//	@doc:
//		Return a map from Id's of consumer columns in the given column set
//		to their corresponding producer columns in the given column array
//
//---------------------------------------------------------------------------
gpos::Ref<UlongToColRefMap>
CCTEInfo::PhmulcrConsumerToProducer(
	CMemoryPool *mp, ULONG ulCTEId,
	CColRefSet *pcrs,			   // set of columns to check
	CColRefArray *pdrgpcrProducer  // producer columns
)
{
	GPOS_ASSERT(nullptr != pcrs);
	GPOS_ASSERT(nullptr != pdrgpcrProducer);

	gpos::Ref<UlongToColRefMap> colref_mapping =
		GPOS_NEW(mp) UlongToColRefMap(mp);

	CColRefSetIter crsi(*pcrs);
	while (crsi.Advance())
	{
		CColRef *colref = crsi.Pcr();
		ULONG ulPos = UlConsumerColPos(ulCTEId, colref);

		if (gpos::ulong_max != ulPos)
		{
			GPOS_ASSERT(ulPos < pdrgpcrProducer->Size());

			CColRef *pcrProducer = (*pdrgpcrProducer)[ulPos];
			BOOL fSuccess GPOS_ASSERTS_ONLY = colref_mapping->Insert(
				GPOS_NEW(mp) ULONG(colref->Id()), pcrProducer);
			GPOS_ASSERT(fSuccess);
		}
	}
	return colref_mapping;
}


// EOF
