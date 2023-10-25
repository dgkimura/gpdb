//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.
//
//	@filename:
//		CParseHandlerPlanHint.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing hint
//		configuration
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerPlanHint.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/xml/dxltokens.h"

using namespace gpdxl;
using namespace gpopt;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPlanHint::CParseHandlerPlanHint
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerPlanHint::CParseHandlerPlanHint(
	CMemoryPool *mp, CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root)
	: CParseHandlerBase(mp, parse_handler_mgr, parse_handler_root),
	  m_hint(nullptr)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPlanHint::~CParseHandlerPlanHint
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerPlanHint::~CParseHandlerPlanHint()
{
	CRefCount::SafeRelease(m_hint);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPlanHint::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerPlanHint::StartElement(
	const XMLCh *const element_uri GPOS_UNUSED,
	const XMLCh *const element_local_name,
	const XMLCh *const element_qname GPOS_UNUSED, const Attributes &attrs)
{
	if (0 ==
		XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPlanHint),
								 element_local_name))
	{
		m_hint = GPOS_NEW(m_mp) CPlanHint(m_mp);
	}
	else if (0 == XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenScanHint),
					  element_local_name))
	{
		CWStringBase *alias = CDXLOperatorFactory::ExtractConvertAttrValueToStr(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenAlias,
			EdxltokenPlanHint);
		COperator::EOperatorId opid = static_cast<COperator::EOperatorId>(
			CDXLOperatorFactory::ExtractConvertAttrValueToUlong(
				m_parse_handler_mgr->GetDXLMemoryManager(), attrs,
				EdxltokenOpName, EdxltokenPlanHint));
		BOOL isnot = CDXLOperatorFactory::ExtractConvertAttrValueToBool(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs,
			EdxltokenScalarBoolNot, EdxltokenPlanHint);

		const XMLCh *attr_val_xml = CDXLOperatorFactory::ExtractAttrValue(
			attrs, EdxltokenIndexName, EdxltokenPlanHint);
		StringPtrArray *index_names = nullptr;
		if (nullptr != attr_val_xml)
		{
			index_names = CDXLOperatorFactory::ExtractConvertStrsToArray(
				m_parse_handler_mgr->GetDXLMemoryManager(), attr_val_xml);
		}

		m_hint->AddHint(GPOS_NEW(m_mp)
							CScanHint(m_mp, alias, index_names, opid, isnot));
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPlanHint::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerPlanHint::EndElement(const XMLCh *const,  // element_uri,
								  const XMLCh *const element_local_name,
								  const XMLCh *const  // element_qname
)
{
	if (0 ==
		XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPlanHint),
								 element_local_name))
	{
		// deactivate handler
		m_parse_handler_mgr->DeactivateHandler();
	}
	else if (0 != XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenHint),
					  element_local_name) &&
			 0 != XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenScanHint),
					  element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(
			m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag,
				   str->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPlanHint::GetParseHandlerType
//
//	@doc:
//		Return the type of the parse handler.
//
//---------------------------------------------------------------------------
EDxlParseHandlerType
CParseHandlerPlanHint::GetParseHandlerType() const
{
	return EdxlphPlanHint;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPlanHint::GetPlanHint
//
//	@doc:
//		Returns the hint configuration
//
//---------------------------------------------------------------------------
CPlanHint *
CParseHandlerPlanHint::GetPlanHint() const
{
	return m_hint;
}

// EOF
