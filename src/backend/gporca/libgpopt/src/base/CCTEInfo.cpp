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
CCTEInfo::CCTEInfoEntry::CCTEInfoEntry(
	CMemoryPool *mp, gpos::owner<CExpression *> pexprCTEProducer)
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
CCTEInfo::CCTEInfoEntry::CCTEInfoEntry(
	CMemoryPool *mp, gpos::owner<CExpression *> pexprCTEProducer, BOOL fUsed)
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
	m_pexprCTEProducer->Release();
	m_phmcrulConsumers->Release();
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
CCTEInfo::CCTEInfoEntry::AddConsumerCols(
	gpos::pointer<CColRefArray *> colref_array)
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
	CRefCount::SafeRelease(m_phmulcteinfoentry);
	CRefCount::SafeRelease(m_phmulprodconsmap);
}


//---------------------------------------------------------------------------
//	@function:
//		CCTEInfo::PreprocessCTEProducer
//
//	@doc:
//		Preprocess CTE producer expression
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CCTEInfo::PexprPreprocessCTEProducer(
	gpos::pointer<const CExpression *> pexprCTEProducer)
{
	GPOS_ASSERT(nullptr != pexprCTEProducer);

	gpos::pointer<CExpression *> pexprProducerChild = (*pexprCTEProducer)[0];

	// get cte output cols for preprocessing use
	gpos::pointer<CColRefSet *> pcrsOutput =
		gpos::dyn_cast<CLogicalCTEProducer>(pexprCTEProducer->Pop())
			->DeriveOutputColumns();

	gpos::owner<CExpression *> pexprChildPreprocessed =
		CExpressionPreprocessor::PexprPreprocess(m_mp, pexprProducerChild,
												 pcrsOutput);

	gpos::owner<COperator *> pop = pexprCTEProducer->Pop();
	pop->AddRef();

	gpos::owner<CExpression *> pexprProducerPreprocessed = GPOS_NEW(m_mp)
		CExpression(m_mp, std::move(pop), std::move(pexprChildPreprocessed));

	pexprProducerPreprocessed->ResetStats();
	InitDefaultStats(pexprProducerPreprocessed);

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
CCTEInfo::AddCTEProducer(gpos::pointer<CExpression *> pexprCTEProducer)
{
	gpos::owner<CExpression *> pexprProducerToAdd =
		PexprPreprocessCTEProducer(pexprCTEProducer);

	gpos::pointer<COperator *> pop = pexprCTEProducer->Pop();
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
CCTEInfo::ReplaceCTEProducer(gpos::pointer<CExpression *> pexprCTEProducer)
{
	gpos::pointer<COperator *> pop = pexprCTEProducer->Pop();
	ULONG ulCTEId = gpos::dyn_cast<CLogicalCTEProducer>(pop)->UlCTEId();

	gpos::pointer<CCTEInfoEntry *> pcteinfoentry =
		m_phmulcteinfoentry->Find(&ulCTEId);
	GPOS_ASSERT(nullptr != pcteinfoentry);

#ifdef GPOS_DBUG
	CExpression *pexprCTEProducerOld = pcteinfoentry->Pexpr();
	COperator *popCTEProducerOld = pexprCTEProducerOld->Pop();
	GPOS_ASSERT(ulCTEId ==
				CLogicalCTEProducer::PopConvert(popCTEProducerOld)->UlCTEId());
#endif	// GPOS_DEBUG

	gpos::owner<CExpression *> pexprCTEProducerNew =
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
CCTEInfo::InitDefaultStats(gpos::pointer<CExpression *> pexprCTEProducer)
{
	// Generate statistics with empty requirement. This handles cases when
	// the CTE is a N-Ary join that will require statistics calculation

	gpos::owner<CReqdPropRelational *> prprel =
		GPOS_NEW(m_mp) CReqdPropRelational(GPOS_NEW(m_mp) CColRefSet(m_mp));
	(void) pexprCTEProducer->PstatsDerive(prprel, nullptr /* stats_ctxt */);

	// cleanup
	prprel->Release();
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
CCTEInfo::DeriveProducerStats(gpos::pointer<CLogicalCTEConsumer *> popConsumer,
							  gpos::pointer<CColRefSet *> pcrsStat)
{
	const ULONG ulCTEId = popConsumer->UlCTEId();

	gpos::pointer<CCTEInfoEntry *> pcteinfoentry =
		m_phmulcteinfoentry->Find(&ulCTEId);
	GPOS_ASSERT(nullptr != pcteinfoentry);

	gpos::pointer<CExpression *> pexprCTEProducer = pcteinfoentry->Pexpr();

	// Given the subset of CTE consumer columns needed for statistics derivation,
	// compute its corresponding set of columns in the CTE Producer
	gpos::owner<CColRefSet *> pcrsCTEProducer =
		CUtils::PcrsCTEProducerColumns(m_mp, pcrsStat, popConsumer);
	GPOS_ASSERT(pcrsStat->Size() == pcrsCTEProducer->Size());

	gpos::owner<CReqdPropRelational *> prprel =
		GPOS_NEW(m_mp) CReqdPropRelational(std::move(pcrsCTEProducer));
	(void) pexprCTEProducer->PstatsDerive(prprel, nullptr /* stats_ctxt */);

	// cleanup
	prprel->Release();
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
	gpos::pointer<const CCTEInfoEntry *> pcteinfoentry =
		m_phmulcteinfoentry->Find(&ulCTEId);
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
	gpos::pointer<const UlongToConsumerCounterMap *> phmulconsumermap =
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
	UlongToCTEInfoEntryMapIter hmulei(m_phmulcteinfoentry);
	while (hmulei.Advance())
	{
		gpos::pointer<const CCTEInfoEntry *> pcteinfoentry = hmulei.Value();
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
	gpos::pointer<CCTEInfoEntry *> pcteinfoentry =
		m_phmulcteinfoentry->Find(&ulCTEId);
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
	gpos::owner<UlongToConsumerCounterMap *> phmulconsumermap =
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
gpos::owner<CCTEReq *>
CCTEInfo::PcterProducers(CMemoryPool *mp) const
{
	gpos::owner<CCTEReq *> pcter = GPOS_NEW(mp) CCTEReq(mp);

	UlongToCTEInfoEntryMapIter hmulei(m_phmulcteinfoentry);
	while (hmulei.Advance())
	{
		gpos::pointer<const CCTEInfoEntry *> pcteinfoentry = hmulei.Value();
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
gpos::owner<CExpressionArray *>
CCTEInfo::PdrgPexpr(CMemoryPool *mp) const
{
	gpos::owner<CExpressionArray *> pdrgpexpr =
		GPOS_NEW(mp) CExpressionArray(mp);
	UlongToCTEInfoEntryMapIter hmulei(m_phmulcteinfoentry);
	while (hmulei.Advance())
	{
		gpos::owner<CExpression *> pexpr = hmulei.Value()->Pexpr();
		pexpr->AddRef();
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
	UlongToCTEInfoEntryMapIter hmulei(m_phmulcteinfoentry);
	while (hmulei.Advance())
	{
		gpos::pointer<CExpression *> pexprProducer = hmulei.Value()->Pexpr();
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
CCTEInfo::AddConsumerCols(ULONG ulCTEId,
						  gpos::pointer<CColRefArray *> colref_array)
{
	GPOS_ASSERT(nullptr != colref_array);

	gpos::pointer<CCTEInfoEntry *> pcteinfoentry =
		m_phmulcteinfoentry->Find(&ulCTEId);
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

	gpos::pointer<CCTEInfoEntry *> pcteinfoentry =
		m_phmulcteinfoentry->Find(&ulCTEId);
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
CCTEInfo::FindConsumersInParent(ULONG ulParentId,
								gpos::pointer<CBitSet *> pbsUnusedConsumers,
								CStack<ULONG> *pstack)
{
	gpos::pointer<UlongToConsumerCounterMap *> phmulconsumermap =
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
	gpos::owner<CBitSet *> pbsUnusedConsumers = GPOS_NEW(m_mp) CBitSet(m_mp);

	// start with all CTEs
	UlongToCTEInfoEntryMapIter hmulei(m_phmulcteinfoentry);
	while (hmulei.Advance())
	{
		gpos::pointer<const CCTEInfoEntry *> pcteinfoentry = hmulei.Value();
		pbsUnusedConsumers->ExchangeSet(pcteinfoentry->UlCTEId());
	}

	// start with the main query and find out which CTEs are used there
	CStack<ULONG> stack(m_mp);
	FindConsumersInParent(gpos::ulong_max, pbsUnusedConsumers, &stack);

	// repeatedly find CTEs that are used in these CTEs
	while (!stack.IsEmpty())
	{
		// get one CTE id from list, and see which consumers are inside this CTE
		ULONG *pulCTEId = stack.Pop();
		FindConsumersInParent(*pulCTEId, pbsUnusedConsumers, &stack);
		GPOS_DELETE(pulCTEId);
	}

	// now the only CTEs remaining in the bitset are the unused ones. mark them as such
	UlongToCTEInfoEntryMapIter hmulei2(m_phmulcteinfoentry);
	while (hmulei2.Advance())
	{
		gpos::pointer<CCTEInfoEntry *> pcteinfoentry =
			const_cast<CCTEInfoEntry *>(hmulei2.Value());
		if (pbsUnusedConsumers->Get(pcteinfoentry->UlCTEId()))
		{
			pcteinfoentry->MarkUnused();
		}
	}

	pbsUnusedConsumers->Release();
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
gpos::owner<UlongToColRefMap *>
CCTEInfo::PhmulcrConsumerToProducer(
	CMemoryPool *mp, ULONG ulCTEId,
	gpos::pointer<CColRefSet *> pcrs,			   // set of columns to check
	gpos::pointer<CColRefArray *> pdrgpcrProducer  // producer columns
)
{
	GPOS_ASSERT(nullptr != pcrs);
	GPOS_ASSERT(nullptr != pdrgpcrProducer);

	gpos::owner<UlongToColRefMap *> colref_mapping =
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
