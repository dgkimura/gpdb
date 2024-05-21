//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 VMware, Inc. or its affiliates.
//
//	@filename:
//		COptimizerConfig.cpp
//
//	@doc:
//		Implementation of configuration used by the optimizer
//---------------------------------------------------------------------------

#include "gpopt/optimizer/COptimizerConfig.h"

#include "gpos/base.h"
#include "gpos/common/CBitSetIter.h"
#include "gpos/string/CWStringDynamic.h"

#include "gpopt/cost/ICostModel.h"
#include "naucrates/dxl/CCostModelConfigSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		COptimizerConfig::COptimizerConfig
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
COptimizerConfig::COptimizerConfig(
	CEnumeratorConfig *pec, CStatisticsConfig *stats_config,
	CCTEConfig *pcteconf, ICostModel *cost_model, CHint *phint,
	CPlanHint *pplanhint, StringToStringArrayMap *alias_table_names_map,
	CWindowOids *pwindowoids)
	: m_enumerator_cfg(pec),
	  m_stats_conf(stats_config),
	  m_cte_conf(pcteconf),
	  m_cost_model(cost_model),
	  m_hint(phint),
	  m_plan_hint(pplanhint),
	  m_alias_table_names_map(alias_table_names_map),
	  m_window_oids(pwindowoids)
{
	GPOS_ASSERT(nullptr != pec);
	GPOS_ASSERT(nullptr != stats_config);
	GPOS_ASSERT(nullptr != pcteconf);
	GPOS_ASSERT(nullptr != m_cost_model);
	GPOS_ASSERT(nullptr != phint);
	GPOS_ASSERT(nullptr != m_window_oids);
}

//---------------------------------------------------------------------------
//	@function:
//		COptimizerConfig::~COptimizerConfig
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
COptimizerConfig::~COptimizerConfig()
{
	m_enumerator_cfg->Release();
	m_stats_conf->Release();
	m_cte_conf->Release();
	m_cost_model->Release();
	CRefCount::SafeRelease(m_plan_hint);
	CRefCount::SafeRelease(m_alias_table_names_map);
	m_hint->Release();
	m_window_oids->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		COptimizerConfig::PocDefault
//
//	@doc:
//		Default optimizer configuration
//
//---------------------------------------------------------------------------
COptimizerConfig *
COptimizerConfig::PoconfDefault(CMemoryPool *mp)
{
	return GPOS_NEW(mp) COptimizerConfig(
		GPOS_NEW(mp) CEnumeratorConfig(mp, 0 /*plan_id*/, 0 /*ullSamples*/),
		CStatisticsConfig::PstatsconfDefault(mp),
		CCTEConfig::PcteconfDefault(mp), ICostModel::PcmDefault(mp),
		CHint::PhintDefault(mp), nullptr /* pplanhint */,
		nullptr /* alias_table_names_map */, CWindowOids::GetWindowOids(mp));
}

//---------------------------------------------------------------------------
//	@function:
//		COptimizerConfig::PocDefault
//
//	@doc:
//		Default optimizer configuration with the given cost model
//
//---------------------------------------------------------------------------
COptimizerConfig *
COptimizerConfig::PoconfDefault(CMemoryPool *mp, ICostModel *pcm)
{
	GPOS_ASSERT(nullptr != pcm);

	return GPOS_NEW(mp) COptimizerConfig(
		GPOS_NEW(mp) CEnumeratorConfig(mp, 0 /*plan_id*/, 0 /*ullSamples*/),
		CStatisticsConfig::PstatsconfDefault(mp),
		CCTEConfig::PcteconfDefault(mp), pcm, CHint::PhintDefault(mp),
		nullptr /* pplanhint */, nullptr /* alias_table_names_map */,
		CWindowOids::GetWindowOids(mp));
}

//---------------------------------------------------------------------------
//	@function:
//		COptimizerConfig::SerializeAliasTablesMap
//
//	@doc:
//		Serialize the alias tables map. Example format:
//
//		<dxl:OptimizerConfig>
//		  ...
//		  <dxl:SubqueryAliasMap>
//		    <dxl:Item Name="q" Value="t1,t2"/>
//		    <dxl:Item Name="qq" Value="t3"/>
//		  </dxl:SubqueryAliasMap>
//		  ...
//		</dxl:OptimizerConfig>
//
//---------------------------------------------------------------------------
void
COptimizerConfig::SerializeAliasTablesMap(CMemoryPool *mp,
										  CXMLSerializer *xml_serializer) const
{
	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenSubqueryAliasMap));

	if (nullptr != m_alias_table_names_map)
	{
		StringToStringArrayMapIter hmiter(m_alias_table_names_map);
		while (hmiter.Advance())
		{
			xml_serializer->OpenElement(
				CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
				CDXLTokens::GetDXLTokenStr(EdxltokenItem));

			const CWStringBase *alias = hmiter.Key();

			xml_serializer->AddAttribute(
				CDXLTokens::GetDXLTokenStr(EdxltokenName), alias);

			StringPtrArray *table_names =
				const_cast<StringPtrArray *>(hmiter.Value());
			CWStringDynamic *names =
				CDXLUtils::SerializeToCommaSeparatedString(mp, table_names);

			xml_serializer->AddAttribute(
				CDXLTokens::GetDXLTokenStr(EdxltokenValue), names);

			GPOS_DELETE(names);

			xml_serializer->CloseElement(
				CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
				CDXLTokens::GetDXLTokenStr(EdxltokenItem));
		}
	}

	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenSubqueryAliasMap));
}

//---------------------------------------------------------------------------
//	@function:
//		COptimizerConfig::Serialize
//
//	@doc:
//		Serialize optimizer configuration
//
//---------------------------------------------------------------------------
void
COptimizerConfig::Serialize(CMemoryPool *mp, CXMLSerializer *xml_serializer,
							CBitSet *pbsTrace) const
{
	GPOS_ASSERT(nullptr != xml_serializer);
	GPOS_ASSERT(nullptr != pbsTrace);

	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenOptimizerConfig));

	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenEnumeratorConfig));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenPlanId),
								 m_enumerator_cfg->GetPlanId());
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenPlanSamples),
		m_enumerator_cfg->GetPlanId());
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenCostThreshold),
		m_enumerator_cfg->GetPlanId());
	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenEnumeratorConfig));

	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenStatisticsConfig));
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenDampingFactorFilter),
		m_stats_conf->DDampingFactorFilter());
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenDampingFactorJoin),
		m_stats_conf->DDampingFactorJoin());
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenDampingFactorGroupBy),
		m_stats_conf->DDampingFactorGroupBy());
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenMaxStatsBuckets),
		m_stats_conf->UlMaxStatsBuckets());
	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenStatisticsConfig));

	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenCTEConfig));
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenCTEInliningCutoff),
		m_cte_conf->UlCTEInliningCutoff());
	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenCTEConfig));

	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenWindowOids));
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenOidRowNumber),
		m_window_oids->OidRowNumber());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenOidRank),
								 m_window_oids->OidRank());
	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenWindowOids));

	CCostModelConfigSerializer cmcSerializer(m_cost_model);
	cmcSerializer.Serialize(*xml_serializer);

	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenHint));
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(
			EdxltokenJoinArityForAssociativityCommutativity),
		m_hint->UlJoinArityForAssociativityCommutativity());
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenArrayExpansionThreshold),
		m_hint->UlArrayExpansionThreshold());
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenJoinOrderDPThreshold),
		m_hint->UlJoinOrderDPLimit());
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenBroadcastThreshold),
		m_hint->UlBroadcastThreshold());
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenEnforceConstraintsOnDML),
		m_hint->FEnforceConstraintsOnDML());
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenPushGroupByBelowSetopThreshold),
		m_hint->UlPushGroupByBelowSetopThreshold());
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenXformBindThreshold),
		m_hint->UlXformBindThreshold());
	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(gpdxl::EdxltokenSkewFactor),
		m_hint->UlSkewFactor());
	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenHint));

	SerializeAliasTablesMap(mp, xml_serializer);

	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenPlanHint));

	if (nullptr != GetPlanHint())
	{
		GetPlanHint()->Serialize(xml_serializer);
	}

	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenPlanHint));

	// Serialize traceflags represented in bitset into stream
	gpos::CBitSetIter bsi(*pbsTrace);
	CWStringDynamic wsTraceFlags(mp);
	for (ULONG ul = 0; bsi.Advance(); ul++)
	{
		if (0 < ul)
		{
			wsTraceFlags.AppendCharArray(",");
		}

		wsTraceFlags.AppendFormat(GPOS_WSZ_LIT("%d"), bsi.Bit());
	}

	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenTraceFlags));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenValue),
								 &wsTraceFlags);
	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenTraceFlags));

	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenOptimizerConfig));
}

// EOF
