//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLMinidump.h
//
//	@doc:
//		DXL-based minidump structure
//---------------------------------------------------------------------------
#ifndef GPOPT_CDXLMinidump_H
#define GPOPT_CDXLMinidump_H

#include "gpos/base.h"
#include "gpos/common/CBitSet.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLNode.h"


using namespace gpos;
using namespace gpdxl;

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CDXLMinidump
//
//	@doc:
//		DXL-based minidump
//
//---------------------------------------------------------------------------
class CDXLMinidump
{
private:
	// traceflags
	gpos::owner<CBitSet *> m_pbs;

	// optimizer configuration
	gpos::owner<COptimizerConfig *> m_optimizer_config;

	// DXL query tree
	gpos::owner<CDXLNode *> m_query_dxl_root;

	// Array of DXL nodes that represent the query output
	gpos::owner<CDXLNodeArray *> m_query_output;

	// Array of DXL nodes that represent the CTE producers
	gpos::owner<CDXLNodeArray *> m_cte_producers;

	// DXL plan
	gpos::owner<CDXLNode *> m_plan_dxl_root;

	// metadata objects
	gpos::owner<IMDCacheObjectArray *> m_mdid_cached_obj_array;

	// source system ids
	gpos::owner<CSystemIdArray *> m_system_id_array;

	// plan Id
	ULLONG m_plan_id;

	// plan space size
	ULLONG m_plan_space_size;

public:
	CDXLMinidump(const CDXLMinidump &) = delete;

	// ctor
	CDXLMinidump(gpos::owner<CBitSet *> pbs,
				 gpos::owner<COptimizerConfig *> optimizer_config,
				 gpos::owner<CDXLNode *> query,
				 gpos::owner<CDXLNodeArray *> query_output_dxlnode_array,
				 gpos::owner<CDXLNodeArray *> cte_producers,
				 gpos::owner<CDXLNode *> pdxlnPlan,
				 gpos::owner<IMDCacheObjectArray *> mdcache_obj_array,
				 gpos::owner<CSystemIdArray *> pdrgpsysid, ULLONG plan_id,
				 ULLONG plan_space_size);

	// dtor
	~CDXLMinidump();

	// traceflags
	gpos::pointer<const CBitSet *> Pbs() const;

	// optimizer configuration
	gpos::pointer<COptimizerConfig *>
	GetOptimizerConfig() const
	{
		return m_optimizer_config;
	}

	// query object
	gpos::pointer<const CDXLNode *> GetQueryDXLRoot() const;

	// query output columns
	gpos::pointer<const CDXLNodeArray *> PdrgpdxlnQueryOutput() const;

	// CTE list
	gpos::pointer<const CDXLNodeArray *> GetCTEProducerDXLArray() const;

	// plan
	gpos::pointer<const CDXLNode *> PdxlnPlan() const;

	// metadata objects
	gpos::pointer<const IMDCacheObjectArray *> GetMdIdCachedObjArray() const;

	// source system ids
	gpos::pointer<const CSystemIdArray *> GetSysidPtrArray() const;

	// return plan id
	ULLONG GetPlanId() const;

	// return plan space size
	ULLONG GetPlanSpaceSize() const;

};	// class CDXLMinidump
}  // namespace gpopt

#endif	// !GPOPT_CDXLMinidump_H

// EOF
