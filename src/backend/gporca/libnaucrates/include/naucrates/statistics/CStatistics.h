//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		CStatistics.h
//
//	@doc:
//		Statistics implementation over 1D histograms
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_CStatistics_H
#define GPNAUCRATES_CStatistics_H

#include "gpos/base.h"
#include "gpos/common/CBitSet.h"
#include "gpos/common/owner.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/statistics/CHistogram.h"
#include "naucrates/statistics/CStatsPredArrayCmp.h"
#include "naucrates/statistics/CStatsPredConj.h"
#include "naucrates/statistics/CStatsPredDisj.h"
#include "naucrates/statistics/CStatsPredLike.h"
#include "naucrates/statistics/CStatsPredUnsupported.h"
#include "naucrates/statistics/CUpperBoundNDVs.h"
#include "naucrates/statistics/IStatistics.h"

namespace gpopt
{
class CStatisticsConfig;
class CColumnFactory;
}  // namespace gpopt

namespace gpnaucrates
{
using namespace gpos;
using namespace gpdxl;
using namespace gpmd;
using namespace gpopt;

// hash maps ULONG -> array of ULONGs
using UlongToUlongPtrArrayMap =
	CHashMap<ULONG, ULongPtrArray, gpos::HashValue<ULONG>, gpos::Equals<ULONG>,
			 CleanupDelete<ULONG>, CleanupRelease<ULongPtrArray>>;

// iterator
using UlongToUlongPtrArrayMapIter =
	CHashMapIter<ULONG, ULongPtrArray, gpos::HashValue<ULONG>,
				 gpos::Equals<ULONG>, CleanupDelete<ULONG>,
				 CleanupRelease<ULongPtrArray>>;

//---------------------------------------------------------------------------
//	@class:
//		CStatistics
//
//	@doc:
//		Abstract statistics API
//---------------------------------------------------------------------------
class CStatistics : public IStatistics, public DbgPrintMixin<CStatistics>
{
public:
	// method used to compute for columns of each source it corresponding
	// the cardinality upper bound
	enum ECardBoundingMethod
	{
		EcbmOutputCard =
			0,	// use the estimated output cardinality as the upper bound cardinality of the source
		EcbmInputSourceMaxCard,	 // use the upper bound cardinality of the source in the input statistics object
		EcbmMin,  // use the minimum of the above two cardinality estimates

		EcbmSentinel
	};

	// helper method to copy stats on columns that are not excluded by bitset
	void AddNotExcludedHistograms(
		CMemoryPool *mp, gpos::pointer<CBitSet *> excluded_cols,
		gpos::pointer<UlongToHistogramMap *> col_histogram_mapping) const;

private:
	// hashmap from column ids to histograms
	gpos::owner<UlongToHistogramMap *> m_colid_histogram_mapping;

	// hashmap from column id to width
	gpos::owner<UlongToDoubleMap *> m_colid_width_mapping;

	// number of rows
	CDouble m_rows;

	// the risk to have errors in cardinality estimation; it goes from 1 to
	// infinity, where 1 is no risk when going from the leaves to the root of the
	// plan, operators that generate joins, selections and groups increment the risk
	ULONG m_stats_estimation_risk;

	// flag to indicate if input relation is empty
	BOOL m_empty;

	// number of blocks in the relation (not always up to-to-date)
	ULONG m_relpages;

	// number of all-visible blocks in the relation (not always up-to-date)
	ULONG m_relallvisible;

	// statistics could be computed using predicates with external parameters (outer
	// references), this is the total number of external parameters' values
	CDouble m_num_rebinds;

	// number of predicates applied
	ULONG m_num_predicates;

	// statistics configuration
	gpos::pointer<CStatisticsConfig *> m_stats_conf;

	// array of upper bound of ndv per source;
	// source can be one of the following operators: like Get, Group By, and Project
	gpos::owner<CUpperBoundNDVPtrArray *> m_src_upper_bound_NDVs;

	// the default value for operators that have no cardinality estimation risk
	static const ULONG no_card_est_risk_default_val;

	// helper method to add histograms where the column ids have been remapped
	static void AddHistogramsWithRemap(
		CMemoryPool *mp, gpos::pointer<UlongToHistogramMap *> src_histograms,
		gpos::pointer<UlongToHistogramMap *> dest_histograms,
		gpos::pointer<UlongToColRefMap *> colref_mapping, BOOL must_exist);

	// helper method to add width information where the column ids have been
	// remapped
	static void AddWidthInfoWithRemap(
		CMemoryPool *mp, gpos::pointer<UlongToDoubleMap *> src_width,
		gpos::pointer<UlongToDoubleMap *> dest_width,
		gpos::pointer<UlongToColRefMap *> colref_mapping, BOOL must_exist);

public:
	CStatistics &operator=(CStatistics &) = delete;

	CStatistics(const CStatistics &) = delete;

	// ctor
	CStatistics(CMemoryPool *mp,
				gpos::owner<UlongToHistogramMap *> col_histogram_mapping,
				gpos::owner<UlongToDoubleMap *> colid_width_mapping,
				CDouble rows, BOOL is_empty, ULONG num_predicates = 0);

	CStatistics(CMemoryPool *mp,
				gpos::owner<UlongToHistogramMap *> col_histogram_mapping,
				gpos::owner<UlongToDoubleMap *> colid_width_mapping,
				CDouble rows, BOOL is_empty, ULONG relpages,
				ULONG relallvisible);


	// dtor
	~CStatistics() override;

	virtual gpos::owner<UlongToDoubleMap *> CopyWidths(CMemoryPool *mp) const;

	virtual void CopyWidthsInto(
		CMemoryPool *mp,
		gpos::pointer<UlongToDoubleMap *> colid_width_mapping) const;

	virtual gpos::owner<UlongToHistogramMap *> CopyHistograms(
		CMemoryPool *mp) const;

	// actual number of rows
	CDouble Rows() const override;

	ULONG
	RelPages() const override
	{
		return m_relpages;
	}

	ULONG
	RelAllVisible() const override
	{
		return m_relallvisible;
	}

	// number of rebinds
	CDouble
	NumRebinds() const override
	{
		return m_num_rebinds;
	}

	// skew estimate for given column
	CDouble GetSkew(ULONG colid) const override;

	// what is the width in bytes of set of column id's
	CDouble Width(gpos::pointer<ULongPtrArray *> colids) const override;

	// what is the width in bytes of set of column references
	CDouble Width(CMemoryPool *mp,
				  gpos::pointer<CColRefSet *> colrefs) const override;

	// what is the width in bytes
	CDouble Width() const override;

	// is statistics on an empty input
	BOOL
	IsEmpty() const override
	{
		return m_empty;
	}

	// look up the histogram of a particular column
	virtual const CHistogram *
	GetHistogram(ULONG colid) const
	{
		return m_colid_histogram_mapping->Find(&colid);
	}

	// look up the number of distinct values of a particular column
	CDouble GetNDVs(const CColRef *colref) override;

	// look up the width of a particular column
	virtual const CDouble *GetWidth(ULONG colid) const;

	// the risk of errors in cardinality estimation
	ULONG
	StatsEstimationRisk() const override
	{
		return m_stats_estimation_risk;
	}

	// update the risk of errors in cardinality estimation
	void
	SetStatsEstimationRisk(ULONG risk) override
	{
		m_stats_estimation_risk = risk;
	}

	// inner join with another stats structure
	IStatistics *CalcInnerJoinStats(
		CMemoryPool *mp, gpos::pointer<const IStatistics *> other_stats,
		CStatsPredJoinArray *join_preds_stats) const override;

	// LOJ with another stats structure
	IStatistics *CalcLOJoinStats(
		CMemoryPool *mp, gpos::pointer<const IStatistics *> other_stats,
		CStatsPredJoinArray *join_preds_stats) const override;

	// left anti semi join with another stats structure
	IStatistics *CalcLASJoinStats(
		CMemoryPool *mp, gpos::pointer<const IStatistics *> other_stats,
		CStatsPredJoinArray *join_preds_stats,
		BOOL
			DoIgnoreLASJHistComputation	 // except for the case of LOJ cardinality estimation this flag is always
		// "true" since LASJ stats computation is very aggressive
	) const override;

	// semi join stats computation
	IStatistics *CalcLSJoinStats(
		CMemoryPool *mp, gpos::pointer<const IStatistics *> inner_side_stats,
		CStatsPredJoinArray *join_preds_stats) const override;

	// return required props associated with stats object
	gpos::owner<CReqdPropRelational *> GetReqdRelationalProps(
		CMemoryPool *mp) const override;

	// append given stats to current object
	void AppendStats(CMemoryPool *mp,
					 gpos::pointer<IStatistics *> stats) override;

	// set number of rebinds
	void
	SetRebinds(CDouble num_rebinds) override
	{
		GPOS_ASSERT(0.0 < num_rebinds);

		m_num_rebinds = num_rebinds;
	}

	// copy stats
	gpos::owner<IStatistics *> CopyStats(CMemoryPool *mp) const override;

	// return a copy of this stats object scaled by a given factor
	gpos::owner<IStatistics *> ScaleStats(CMemoryPool *mp,
										  CDouble factor) const override;

	// copy stats with remapped column id
	gpos::owner<IStatistics *> CopyStatsWithRemap(
		CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
		BOOL must_exist) const override;

	// return the set of column references we have stats for
	gpos::owner<CColRefSet *> GetColRefSet(CMemoryPool *mp) const override;

	// generate the DXL representation of the statistics object
	gpos::owner<CDXLStatsDerivedRelation *> GetDxlStatsDrvdRelation(
		CMemoryPool *mp, CMDAccessor *md_accessor) const override;

	// print function
	IOstream &OsPrint(IOstream &os) const override;

	// add upper bound of source cardinality
	virtual void AddCardUpperBound(CUpperBoundNDVs *upper_bound_NDVs);

	// return the upper bound of the number of distinct values for a given column
	virtual CDouble GetColUpperBoundNDVs(const CColRef *colref);

	// return the index of the array of upper bound ndvs to which column reference belongs
	virtual ULONG GetIndexUpperBoundNDVs(const CColRef *colref);

	// return the column identifiers of all columns statistics maintained
	virtual gpos::owner<ULongPtrArray *> GetColIdsWithStats(
		CMemoryPool *mp) const;

	ULONG
	GetNumberOfPredicates() const override
	{
		return m_num_predicates;
	}

	gpos::pointer<CStatisticsConfig *>
	GetStatsConfig() const
	{
		return m_stats_conf;
	}

	gpos::pointer<CUpperBoundNDVPtrArray *>
	GetUpperBoundNDVs() const
	{
		return m_src_upper_bound_NDVs;
	}
	// create an empty statistics object
	static gpos::owner<CStatistics *>
	MakeEmptyStats(CMemoryPool *mp)
	{
		gpos::owner<ULongPtrArray *> colids = GPOS_NEW(mp) ULongPtrArray(mp);
		gpos::owner<CStatistics *> stats =
			MakeDummyStats(mp, colids, DefaultRelationRows);

		// clean up
		colids->Release();

		return stats;
	}

	// conversion function
	static gpos::cast_func<CStatistics *>
	CastStats(IStatistics *pstats)
	{
		GPOS_ASSERT(nullptr != pstats);
		return dynamic_cast<CStatistics *>(pstats);
	}

	// create a dummy statistics object
	static gpos::owner<CStatistics *> MakeDummyStats(
		CMemoryPool *mp, gpos::pointer<ULongPtrArray *> colids, CDouble rows);

	// create a dummy statistics object
	static gpos::owner<CStatistics *> MakeDummyStats(
		CMemoryPool *mp, gpos::pointer<ULongPtrArray *> col_histogram_mapping,
		gpos::pointer<ULongPtrArray *> colid_width_mapping, CDouble rows);

	// default column width
	static const CDouble DefaultColumnWidth;

	// default number of rows in relation
	static const CDouble DefaultRelationRows;

	// minimum number of rows in relation
	static const CDouble MinRows;

	// epsilon
	static const CDouble Epsilon;

	// default number of distinct values
	static const CDouble DefaultDistinctValues;

	// check if the input statistics from join statistics computation empty
	static BOOL IsEmptyJoin(gpos::pointer<const CStatistics *> outer_stats,
							gpos::pointer<const CStatistics *> inner_side_stats,
							BOOL IsLASJ);

	// add upper bound ndvs information for a given set of columns
	static void CreateAndInsertUpperBoundNDVs(
		CMemoryPool *mp, gpos::pointer<CStatistics *> stats,
		gpos::pointer<ULongPtrArray *> colids, CDouble rows);

	// cap the total number of distinct values (NDV) in buckets to the number of rows
	static void CapNDVs(
		CDouble rows,
		gpos::pointer<UlongToHistogramMap *> col_histogram_mapping);
};	// class CStatistics

}  // namespace gpnaucrates

#endif	// !GPNAUCRATES_CStatistics_H

// EOF
