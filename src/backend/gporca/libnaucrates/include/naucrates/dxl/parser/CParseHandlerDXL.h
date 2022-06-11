//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerDXL.h
//
//	@doc:
//		SAX parse handler class for parsing a DXL document
//---------------------------------------------------------------------------

#ifndef GPDXL_CParseHandlerDXL_H
#define GPDXL_CParseHandlerDXL_H

#include "gpos/base.h"
#include "gpos/common/CBitSet.h"
#include "gpos/common/owner.h"

#include "gpopt/cost/ICostModelParams.h"
#include "gpopt/search/CSearchStage.h"
#include "naucrates/dxl/parser/CParseHandlerBase.h"
#include "naucrates/md/CDXLStatsDerivedRelation.h"

namespace gpdxl
{
using namespace gpos;
using namespace gpmd;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@class:
//		CParseHandlerDXL
//
//	@doc:
//		Parse handler for DXL documents.
//		Starting point for all other parse handlers
//
//---------------------------------------------------------------------------
class CParseHandlerDXL : public CParseHandlerBase
{
private:
	// traceflags
	gpos::owner<CBitSet *> m_trace_flags_bitset;

	// optimizer config
	gpos::owner<COptimizerConfig *> m_optimizer_config;

	// MD request
	gpos::owner<CMDRequest *> m_mdrequest;

	// the root of the parsed DXL query
	gpos::owner<CDXLNode *> m_query_dxl_root;

	// list of query output columns
	gpos::owner<CDXLNodeArray *> m_output_colums_dxl_array;

	// list of CTE producers
	gpos::owner<CDXLNodeArray *> m_cte_producers;

	// the root of the parsed DXL plan
	gpos::owner<CDXLNode *> m_plan_dxl_root;

	// list of parsed metadata objects
	gpos::owner<IMDCacheObjectArray *> m_mdid_cached_obj_array;

	// list of parsed metadata ids
	gpos::owner<IMdIdArray *> m_mdid_array;

	// the root of the parsed scalar expression
	gpos::owner<CDXLNode *> m_scalar_expr_dxl;

	// list of source system ids
	gpos::owner<CSystemIdArray *> m_system_id_array;

	// list of parsed statistics objects
	gpos::owner<CDXLStatsDerivedRelationArray *> m_dxl_stats_derived_rel_array;

	// search strategy
	gpos::owner<CSearchStageArray *> m_search_stage_array;

	// plan Id
	ULLONG m_plan_id;

	// plan space size
	ULLONG m_plan_space_size;

	// cost model params
	gpos::owner<ICostModelParams *> m_cost_model_params;

	// process the start of an element
	void StartElement(
		const XMLCh *const element_uri,			// URI of element's namespace
		const XMLCh *const element_local_name,	// local part of element's name
		const XMLCh *const element_qname,		// element's qname
		const Attributes &attr					// element's attributes
		) override;

	// process the end of an element
	void EndElement(
		const XMLCh *const element_uri,			// URI of element's namespace
		const XMLCh *const element_local_name,	// local part of element's name
		const XMLCh *const element_qname		// element's qname
		) override;

	// extract traceflags
	void ExtractTraceFlags(CParseHandlerBase *parse_handler_base);

	// extract optimizer config
	void ExtractOptimizerConfig(CParseHandlerBase *parse_handler_base);

	// extract a physical plan
	void ExtractDXLPlan(CParseHandlerBase *parse_handler_base);

	// extract metadata objects
	void ExtractMetadataObjects(CParseHandlerBase *parse_handler_base);

	// extract statistics
	void ExtractStats(CParseHandlerBase *parse_handler_base);

	// extract DXL query
	void ExtractDXLQuery(CParseHandlerBase *parse_handler_base);

	// extract mdids of requested objects
	void ExtractMDRequest(CParseHandlerBase *parse_handler_base);

	// extract search strategy
	void ExtractSearchStrategy(CParseHandlerBase *parse_handler_base);

	// extract cost params
	void ExtractCostParams(CParseHandlerBase *parse_handler_base);

	// extract a top level scalar expression
	void ExtractScalarExpr(CParseHandlerBase *parse_handler_base);

	// check if given element name is valid for starting DXL document
	static BOOL IsValidStartElement(const XMLCh *const element_name);

public:
	CParseHandlerDXL(const CParseHandlerDXL &) = delete;

	// ctor
	CParseHandlerDXL(CMemoryPool *mp, CParseHandlerManager *parse_handler_mgr);

	//dtor
	~CParseHandlerDXL() override;

	// traceflag bitset
	gpos::pointer<CBitSet *> Pbs() const;

	// optimizer config
	gpos::pointer<COptimizerConfig *> GetOptimizerConfig() const;

	// returns the root of the parsed DXL query
	gpos::pointer<CDXLNode *> GetQueryDXLRoot() const;

	// returns the list of query output columns
	gpos::pointer<CDXLNodeArray *> GetOutputColumnsDXLArray() const;

	// returns the list of CTE producers
	gpos::pointer<CDXLNodeArray *> GetCTEProducerDXLArray() const;

	// returns the root of the parsed DXL plan
	gpos::pointer<CDXLNode *> PdxlnPlan() const;

	// return the list of parsed metadata objects
	gpos::pointer<IMDCacheObjectArray *> GetMdIdCachedObjArray() const;

	// return the list of parsed metadata ids
	gpos::pointer<IMdIdArray *> GetMdIdArray() const;

	// return the MD request object
	gpos::pointer<CMDRequest *> GetMiniDumper() const;

	// return the root of the parsed scalar expression
	gpos::pointer<CDXLNode *> GetScalarExprDXLRoot() const;

	// return the list of parsed source system id objects
	gpos::pointer<CSystemIdArray *> GetSysidPtrArray() const;

	// return the list of statistics objects
	gpos::pointer<CDXLStatsDerivedRelationArray *> GetStatsDerivedRelDXLArray()
		const;

	// return search strategy
	gpos::pointer<CSearchStageArray *> GetSearchStageArray() const;

	// return plan id
	ULLONG GetPlanId() const;

	// return plan space size
	ULLONG GetPlanSpaceSize() const;

	// return cost params
	gpos::pointer<ICostModelParams *> GetCostModelParams() const;

	// process the end of the document
	void endDocument() override;
};
}  // namespace gpdxl

#endif	// !GPDXL_CParseHandlerDXL_H

// EOF
