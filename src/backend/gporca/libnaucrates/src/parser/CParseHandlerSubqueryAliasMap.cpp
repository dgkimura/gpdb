//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2024 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerSubqueryAliasMap.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing subquery
//		alias mapping.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerSubqueryAliasMap.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerMetadataColumn.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSubqueryAliasMap::CParseHandlerSubqueryAliasMap
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerSubqueryAliasMap::CParseHandlerSubqueryAliasMap(
	CMemoryPool *mp, CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root)
	: CParseHandlerBase(mp, parse_handler_mgr, parse_handler_root),
	  m_alias_table_names_map(nullptr)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSubqueryAliasMap::~CParseHandlerSubqueryAliasMap
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerSubqueryAliasMap::~CParseHandlerSubqueryAliasMap()
{
	CRefCount::SafeRelease(m_alias_table_names_map);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSubqueryAliasMap::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//		Example format:
//
//			 <dxl:OptimizerConfig>
//			   ...
//			   <dxl:SubqueryAliasMap>
//			     <dxl:Item Name="q" Value="t2,qq"/>
//			     <dxl:Item Name="qq" Value="t3"/>
//			   </dxl:SubqueryAliasMap>
//			   ...
//			 </dxl:OptimizerConfig>
//---------------------------------------------------------------------------
void
CParseHandlerSubqueryAliasMap::StartElement(
	const XMLCh *const element_uri GPOS_UNUSED,
	const XMLCh *const element_local_name,
	const XMLCh *const element_qname GPOS_UNUSED,
	const Attributes &attrs GPOS_UNUSED)
{
	if (0 == XMLString::compareString(
				 CDXLTokens::XmlstrToken(EdxltokenSubqueryAliasMap),
				 element_local_name))
	{
		// start of a columns' list
		GPOS_ASSERT(nullptr == m_alias_table_names_map);

		m_alias_table_names_map = GPOS_NEW(m_mp) StringToStringArrayMap(m_mp);
	}
	else if (0 ==
			 XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenItem),
									  element_local_name))
	{
		CWStringBase *alias = CDXLOperatorFactory::ExtractConvertAttrValueToStr(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenName,
			EdxltokenItem);

		const XMLCh *attr_val_xml_names = CDXLOperatorFactory::ExtractAttrValue(
			attrs, EdxltokenValue, EdxltokenItem);
		StringPtrArray *names = nullptr;
		if (nullptr != attr_val_xml_names)
		{
			names = CDXLOperatorFactory::ExtractConvertStrsToArray(
				m_parse_handler_mgr->GetDXLMemoryManager(), attr_val_xml_names);

			m_alias_table_names_map->Insert(alias, names);
		}
	}
	else
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(
			m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag,
				   str->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSubqueryAliasMap::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerSubqueryAliasMap::EndElement(const XMLCh *const,  // element_uri,
										  const XMLCh *const element_local_name,
										  const XMLCh *const  // element_qname
)
{
	if (0 == XMLString::compareString(
				 CDXLTokens::XmlstrToken(EdxltokenSubqueryAliasMap),
				 element_local_name))
	{
		// end of the columns' list
		GPOS_ASSERT(nullptr != m_alias_table_names_map);

		// deactivate handler
		m_parse_handler_mgr->DeactivateHandler();
	}
	else if (0 ==
			 XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenItem),
									  element_local_name))
	{
	}
	else
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(
			m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag,
				   str->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSubqueryAliasMap::GetMdColArray
//
//	@doc:
//		Return the constructed list of metadata columns
//
//---------------------------------------------------------------------------
StringToStringArrayMap *
CParseHandlerSubqueryAliasMap::GetAliasTableNamesMap()
{
	return m_alias_table_names_map;
}

// EOF
