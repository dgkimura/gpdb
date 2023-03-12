//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CCTEInfo.h
//
//	@doc:
//		Information about CTEs in a query
//---------------------------------------------------------------------------
#ifndef GPOPT_CCTEInfo_H
#define GPOPT_CCTEInfo_H

#include "gpos/base.h"
#include "gpos/common/CHashMap.h"
#include "gpos/common/CStack.h"

#include "gpopt/base/CCTEReq.h"
#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColumnFactory.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
// fwd declarations
class CLogicalCTEConsumer;

//---------------------------------------------------------------------------
//	@class:
//		CCTEInfo
//
//	@doc:
//		Global information about common table expressions (CTEs) including:
//		- the expression tree that defines each CTE
//		- the number of consumers created by the optimizer
//		- a mapping from consumer columns to producer columns
//
//---------------------------------------------------------------------------
class CCTEInfo : public CRefCount<CCTEInfo>
{
private:
	//-------------------------------------------------------------------
	//	@struct:
	//		SConsumerCounter
	//
	//	@doc:
	//		Representation of the number of consumers of a given CTE inside
	// 		a specific context (e.g. inside the main query, inside another CTE, etc.)
	//
	//-------------------------------------------------------------------
	struct SConsumerCounter : public CRefCount<SConsumerCounter>
	{
	private:
		// consumer ID
		ULONG m_ulCTEId;

		// number of occurrences
		ULONG m_ulCount;

	public:
		// ctor
		explicit SConsumerCounter(ULONG ulCTEId)
			: m_ulCTEId(ulCTEId), m_ulCount(1)
		{
		}

		// consumer ID
		ULONG
		UlCTEId() const
		{
			return m_ulCTEId;
		}

		// number of consumers
		ULONG
		UlCount() const
		{
			return m_ulCount;
		}

		// increment number of consumers
		void
		Increment()
		{
			m_ulCount++;
		}
	};

	// hash map mapping ULONG -> SConsumerCounter
	using UlongToConsumerCounterMap =
		gpos::UnorderedMap<ULONG, gpos::Ref<SConsumerCounter>>;

	// map iterator
	using UlongToConsumerCounterMapIter =
		gpos::UnorderedMap<ULONG, gpos::Ref<SConsumerCounter>>::LegacyIterator;

	// hash map mapping ULONG -> UlongToConsumerCounterMap: maps from CTE producer ID to all consumers inside this CTE
	using UlongToProducerConsumerMap =
		gpos::UnorderedMap<ULONG, gpos::Ref<UlongToConsumerCounterMap>>;

	//-------------------------------------------------------------------
	//	@struct:
	//		CCTEInfoEntry
	//
	//	@doc:
	//		A single entry for CTEInfo, representing a single CTE producer
	//
	//-------------------------------------------------------------------
	class CCTEInfoEntry : public CRefCount<CCTEInfoEntry>
	{
	private:
		// memory pool
		CMemoryPool *m_mp;

		// logical producer expression
		gpos::Ref<CExpression> m_pexprCTEProducer;

		// map columns of all created consumers of current CTE to their positions in consumer output
		gpos::Ref<ColRefToUlongMap> m_phmcrulConsumers;

		// is this CTE used
		BOOL m_fUsed;

	public:
		// ctors
		CCTEInfoEntry(CMemoryPool *mp, gpos::Ref<CExpression> pexprCTEProducer);
		CCTEInfoEntry(CMemoryPool *mp, gpos::Ref<CExpression> pexprCTEProducer,
					  BOOL fUsed);

		// dtor
		~CCTEInfoEntry() override;

		// CTE expression
		gpos::Ref<CExpression>
		Pexpr() const
		{
			return m_pexprCTEProducer;
		}

		// is this CTE used?
		BOOL
		FUsed() const
		{
			return m_fUsed;
		}

		// CTE id
		ULONG UlCTEId() const;

		// mark CTE as unused
		void
		MarkUnused()
		{
			m_fUsed = false;
		}

		// add given columns to consumers column map
		void AddConsumerCols(gpos::Ref<CColRefArray> colref_array);

		// return position of given consumer column in consumer output
		ULONG UlConsumerColPos(gpos::Ref<CColRef> colref);

	};	//class CCTEInfoEntry

	// hash maps mapping ULONG -> CCTEInfoEntry
	using UlongToCTEInfoEntryMap =
		gpos::UnorderedMap<ULONG, gpos::Ref<CCTEInfoEntry>>;

	// map iterator
	using UlongToCTEInfoEntryMapIter =
		gpos::UnorderedMap<ULONG, gpos::Ref<CCTEInfoEntry>>::LegacyIterator;

	// memory pool
	CMemoryPool *m_mp;

	// mapping from cte producer id -> cte info entry
	gpos::Ref<UlongToCTEInfoEntryMap> m_phmulcteinfoentry;

	// next available CTE Id
	ULONG m_ulNextCTEId;

	// whether or not to inline CTE consumers
	BOOL m_fEnableInlining;

	// consumers inside each cte/main query
	gpos::Ref<UlongToProducerConsumerMap> m_phmulprodconsmap;

	// initialize default statistics for a given CTE Producer
	void InitDefaultStats(gpos::Ref<CExpression> pexprCTEProducer);

	// preprocess CTE producer expression
	gpos::Ref<CExpression> PexprPreprocessCTEProducer(
		const gpos::Ref<CExpression> pexprCTEProducer);

	// number of consumers of given CTE inside a given parent
	ULONG UlConsumersInParent(ULONG ulConsumerId, ULONG ulParentId) const;

	// find all CTE consumers inside given parent, and push them to the given stack
	void FindConsumersInParent(ULONG ulParentId,
							   gpos::Ref<CBitSet> pbsUnusedConsumers,
							   CStack<ULONG> *pstack);

public:
	CCTEInfo(const CCTEInfo &) = delete;

	// ctor
	explicit CCTEInfo(CMemoryPool *mp);

	//dtor
	~CCTEInfo() override;

	// logical cte producer with given id
	gpos::Ref<CExpression> PexprCTEProducer(ULONG ulCTEId) const;

	// number of CTE consumers of given CTE
	ULONG UlConsumers(ULONG ulCTEId) const;

	// check if given CTE is used
	BOOL FUsed(ULONG ulCTEId) const;

	// increment number of CTE consumers
	void IncrementConsumers(ULONG ulConsumerId,
							ULONG ulParentCTEId = gpos::ulong_max);

	// add cte producer to hashmap
	void AddCTEProducer(gpos::Ref<CExpression> pexprCTEProducer);

	// replace cte producer with given expression
	void ReplaceCTEProducer(gpos::Ref<CExpression> pexprCTEProducer);

	// next available CTE id
	ULONG
	next_id()
	{
		return m_ulNextCTEId++;
	}

	// derive the statistics on the CTE producer
	void DeriveProducerStats(
		gpos::Ref<CLogicalCTEConsumer> popConsumer,	 // CTE Consumer operator
		gpos::Ref<CColRefSet>
			pcrsStat  // required stat columns on the CTE Consumer
	);

	// return a CTE requirement with all the producers as optional
	gpos::Ref<CCTEReq> PcterProducers(CMemoryPool *mp) const;

	// return an array of all stored CTE expressions
	gpos::Ref<CExpressionArray> PdrgPexpr(CMemoryPool *mp) const;

	// disable CTE inlining
	void
	DisableInlining()
	{
		m_fEnableInlining = false;
	}

	// whether or not to inline CTE consumers
	BOOL
	FEnableInlining() const
	{
		return m_fEnableInlining;
	}

	// mark unused CTEs
	void MarkUnusedCTEs();

	// walk the producer expressions and add the mapping between computed column
	// and their corresponding used column(s)
	void MapComputedToUsedCols(CColumnFactory *col_factory) const;

	// add given columns to consumers column map
	void AddConsumerCols(ULONG ulCTEId, gpos::Ref<CColRefArray> colref_array);

	// return position of given consumer column in consumer output
	ULONG UlConsumerColPos(ULONG ulCTEId, gpos::Ref<CColRef> colref);

	// return a map from Id's of consumer columns in the given column set to their corresponding producer columns
	gpos::Ref<UlongToColRefMap> PhmulcrConsumerToProducer(
		CMemoryPool *mp, ULONG ulCTEId, gpos::Ref<CColRefSet> pcrs,
		gpos::Ref<CColRefArray> pdrgpcrProducer);

};	// CCTEInfo
}  // namespace gpopt

#endif	// !GPOPT_CCTEInfo_H

// EOF
