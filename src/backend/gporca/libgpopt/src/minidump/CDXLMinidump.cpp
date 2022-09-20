//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CDXLMinidump.cpp
//
//	@doc:
//		Implementation of DXL-based minidump object
//---------------------------------------------------------------------------

#include "gpopt/minidump/CDXLMinidump.h"

#include "gpos/common/CBitSet.h"
#include "gpos/common/owner.h"

#include "gpopt/engine/CEnumeratorConfig.h"
#include "gpopt/engine/CStatisticsConfig.h"
#include "gpopt/optimizer/COptimizerConfig.h"
#include "naucrates/dxl/operators/CDXLNode.h"

using namespace gpos;
using namespace gpdxl;
using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::CDXLMinidump
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLMinidump::CDXLMinidump(gpos::Ref<CBitSet> pbs,
						   gpos::Ref<COptimizerConfig> optimizer_config,
						   gpos::Ref<CDXLNode> query,
						   gpos::Ref<CDXLNodeArray> query_output_dxlnode_array,
						   gpos::Ref<CDXLNodeArray> cte_producers,
						   gpos::Ref<CDXLNode> pdxlnPlan,
						   gpos::Ref<IMDCacheObjectArray> mdcache_obj_array,
						   gpos::Ref<CSystemIdArray> pdrgpsysid, ULLONG plan_id,
						   ULLONG plan_space_size)
	: m_pbs(std::move(pbs)),
	  m_optimizer_config(std::move(optimizer_config)),
	  m_query_dxl_root(std::move(query)),
	  m_query_output(std::move(query_output_dxlnode_array)),
	  m_cte_producers(std::move(cte_producers)),
	  m_plan_dxl_root(std::move(pdxlnPlan)),
	  m_mdid_cached_obj_array(std::move(mdcache_obj_array)),
	  m_system_id_array(std::move(pdrgpsysid)),
	  m_plan_id(plan_id),
	  m_plan_space_size(plan_space_size)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::~CDXLMinidump
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLMinidump::~CDXLMinidump()
{
	// some of the structures may be NULL as they are not included in the minidump
	;
	;
	;
	;
	;
	;
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::Pbs
//
//	@doc:
//		Traceflags
//
//---------------------------------------------------------------------------
const CBitSet *
CDXLMinidump::Pbs() const
{
	return m_pbs.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::GetQueryDXLRoot
//
//	@doc:
//		Query object
//
//---------------------------------------------------------------------------
const CDXLNode *
CDXLMinidump::GetQueryDXLRoot() const
{
	return m_query_dxl_root.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::PdrgpdxlnQueryOutput
//
//	@doc:
//		Query output columns
//
//---------------------------------------------------------------------------
const CDXLNodeArray *
CDXLMinidump::PdrgpdxlnQueryOutput() const
{
	return m_query_output.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::GetCTEProducerDXLArray
//
//	@doc:
//		CTE list
//
//---------------------------------------------------------------------------
const CDXLNodeArray *
CDXLMinidump::GetCTEProducerDXLArray() const
{
	return m_cte_producers.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::PdxlnPlan
//
//	@doc:
//		Query object
//
//---------------------------------------------------------------------------
const CDXLNode *
CDXLMinidump::PdxlnPlan() const
{
	return m_plan_dxl_root.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::GetMdIdCachedObjArray
//
//	@doc:
//		Metadata objects
//
//---------------------------------------------------------------------------
const IMDCacheObjectArray *
CDXLMinidump::GetMdIdCachedObjArray() const
{
	return m_mdid_cached_obj_array.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::GetSysidPtrArray
//
//	@doc:
//		Metadata source system ids
//
//---------------------------------------------------------------------------
const CSystemIdArray *
CDXLMinidump::GetSysidPtrArray() const
{
	return m_system_id_array.get();
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::GetPlanId
//
//	@doc:
//		Returns plan id
//
//---------------------------------------------------------------------------
ULLONG
CDXLMinidump::GetPlanId() const
{
	return m_plan_id;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::GetPlanId
//
//	@doc:
//		Returns plan space size
//
//---------------------------------------------------------------------------
ULLONG
CDXLMinidump::GetPlanSpaceSize() const
{
	return m_plan_space_size;
}


// EOF
