//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CParseHandlerMDType.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing metadata for
//		GPDB types.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMDType.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CDrvdPropCtxtPlan.h"
#include "gpopt/search/CGroupExpression.h"
#include "naucrates/dxl/gpdb_types.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/md/CMDTypeBoolGPDB.h"
#include "naucrates/md/CMDTypeGenericGPDB.h"
#include "naucrates/md/CMDTypeInt2GPDB.h"
#include "naucrates/md/CMDTypeInt4GPDB.h"
#include "naucrates/md/CMDTypeInt8GPDB.h"
#include "naucrates/md/CMDTypeOidGPDB.h"

using namespace gpdxl;
using namespace gpmd;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDType::CParseHandlerMDType
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerMDType::CParseHandlerMDType(
	CMemoryPool *mp, CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root)
	: CParseHandlerMetadataObject(mp, parse_handler_mgr, parse_handler_root),
	  m_mdid(nullptr),
	  m_distr_opfamily(nullptr),
	  m_legacy_distr_opfamily(nullptr),
	  m_mdname(nullptr),
	  m_mdid_eq_op(nullptr),
	  m_mdid_neq_op(nullptr),
	  m_mdid_lt_op(nullptr),
	  m_mdid_lteq_op(nullptr),
	  m_mdid_gt_op(nullptr),
	  m_mdid_gteq_op(nullptr),
	  m_mdid_cmp_op(nullptr),
	  m_mdid_min_op(nullptr),
	  m_mdid_max_op(nullptr),
	  m_mdid_avg_op(nullptr),
	  m_mdid_sum_op(nullptr),
	  m_mdid_count_op(nullptr),
	  m_is_hashable(false),
	  m_is_composite(false),
	  m_is_text_related(false),
	  m_mdid_base_rel(nullptr),
	  m_mdid_array_type(nullptr)
{
	// default: no aggregates for type
	m_mdid_min_op = GPOS_NEW(mp) CMDIdGPDB(0);
	m_mdid_max_op = GPOS_NEW(mp) CMDIdGPDB(0);
	m_mdid_avg_op = GPOS_NEW(mp) CMDIdGPDB(0);
	m_mdid_sum_op = GPOS_NEW(mp) CMDIdGPDB(0);
	m_mdid_count_op = GPOS_NEW(mp) CMDIdGPDB(0);
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDType::~CParseHandlerMDType
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerMDType::~CParseHandlerMDType()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDType::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDType::StartElement(const XMLCh *const,  // element_uri,
								  const XMLCh *const element_local_name,
								  const XMLCh *const,  // element_qname
								  const Attributes &attrs)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenMDType),
									  element_local_name))
	{
		// parse metadata id info
		m_mdid = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDType);

		if (!IsBuiltInType(m_mdid.get()))
		{
			// parse type name
			const XMLCh *xmlszTypeName = CDXLOperatorFactory::ExtractAttrValue(
				attrs, EdxltokenName, EdxltokenMDType);

			CWStringDynamic *pstrTypeName =
				CDXLUtils::CreateDynamicStringFromXMLChArray(
					m_parse_handler_mgr->GetDXLMemoryManager(), xmlszTypeName);

			// create a copy of the string in the CMDName constructor
			m_mdname = GPOS_NEW(m_mp) CMDName(m_mp, pstrTypeName);
			GPOS_DELETE(pstrTypeName);

			// parse if type is redistributable
			m_is_redistributable =
				CDXLOperatorFactory::ExtractConvertAttrValueToBool(
					m_parse_handler_mgr->GetDXLMemoryManager(), attrs,
					EdxltokenMDTypeRedistributable, EdxltokenMDType);

			// parse if type is passed by value
			m_type_passed_by_value =
				CDXLOperatorFactory::ExtractConvertAttrValueToBool(
					m_parse_handler_mgr->GetDXLMemoryManager(), attrs,
					EdxltokenMDTypeByValue, EdxltokenMDType);

			// parse if type is hashable
			m_is_hashable = CDXLOperatorFactory::ExtractConvertAttrValueToBool(
				m_parse_handler_mgr->GetDXLMemoryManager(), attrs,
				EdxltokenMDTypeHashable, EdxltokenMDType);

			// parse if type is merge joinable
			m_is_merge_joinable =
				CDXLOperatorFactory::ExtractConvertAttrValueToBool(
					m_parse_handler_mgr->GetDXLMemoryManager(), attrs,
					EdxltokenMDTypeMergeJoinable, EdxltokenMDType);

			// parse if type is composite
			const XMLCh *attribute_val_xml = attrs.getValue(
				CDXLTokens::XmlstrToken(EdxltokenMDTypeComposite));
			if (nullptr == attribute_val_xml)
			{
				m_is_composite = false;
			}
			else
			{
				m_is_composite = CDXLOperatorFactory::ConvertAttrValueToBool(
					m_parse_handler_mgr->GetDXLMemoryManager(),
					attribute_val_xml, EdxltokenMDTypeComposite,
					EdxltokenMDType);
			}

			if (m_is_composite)
			{
				// get base relation id
				m_mdid_base_rel =
					CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
						m_parse_handler_mgr->GetDXLMemoryManager(), attrs,
						EdxltokenMDTypeRelid, EdxltokenMDType);
			}

			// parse if type is fixed-length
			m_istype_fixed_Length =
				CDXLOperatorFactory::ExtractConvertAttrValueToBool(
					m_parse_handler_mgr->GetDXLMemoryManager(), attrs,
					EdxltokenMDTypeFixedLength, EdxltokenMDType);

			const XMLCh *xml_is_text_related = attrs.getValue(
				CDXLTokens::XmlstrToken(EdxltokenMDTypeIsTextRelated));
			if (nullptr == xml_is_text_related)
			{
				m_is_text_related = false;
			}
			else
			{
				m_is_text_related = CDXLOperatorFactory::ConvertAttrValueToBool(
					m_parse_handler_mgr->GetDXLMemoryManager(),
					xml_is_text_related, EdxltokenMDTypeIsTextRelated,
					EdxltokenMDType);
			}

			// get type length
			m_type_length = CDXLOperatorFactory::ExtractConvertAttrValueToInt(
				m_parse_handler_mgr->GetDXLMemoryManager(), attrs,
				EdxltokenMDTypeLength, EdxltokenMDType);
		}
	}
	else
	{
		ParseMdid(element_local_name, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDType::ParseMdid
//
//	@doc:
//		Parse the mdid
//
//---------------------------------------------------------------------------
void
CParseHandlerMDType::ParseMdid(const XMLCh *element_local_name,
							   const Attributes &attrs)
{
	if (0 ==
		XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenMDTypeEqOp),
								 element_local_name))
	{
		m_mdid_eq_op = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDTypeEqOp);
	}
	else if (0 == XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenMDTypeNEqOp),
					  element_local_name))
	{
		m_mdid_neq_op = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDTypeNEqOp);
	}
	else if (0 == XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenMDTypeLTOp),
					  element_local_name))
	{
		m_mdid_lt_op = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDTypeLTOp);
	}
	else if (0 == XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenMDTypeLEqOp),
					  element_local_name))
	{
		m_mdid_lteq_op = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDTypeLEqOp);
	}
	else if (0 == XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenMDTypeGTOp),
					  element_local_name))
	{
		m_mdid_gt_op = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDTypeGTOp);
	}
	else if (0 == XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenMDTypeGEqOp),
					  element_local_name))
	{
		m_mdid_gteq_op = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDTypeGEqOp);
	}
	else if (0 == XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenMDTypeCompOp),
					  element_local_name))
	{
		m_mdid_cmp_op = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDTypeCompOp);
	}
	else if (0 == XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenMDTypeArray),
					  element_local_name))
	{
		m_mdid_array_type = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDTypeArray);
	}
	else if (0 == XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenMDTypeAggMin),
					  element_local_name))
	{
		m_mdid_min_op = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDTypeAggMin);
	}
	else if (0 == XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenMDTypeAggMax),
					  element_local_name))
	{
		m_mdid_max_op = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDTypeAggMax);
	}
	else if (0 == XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenMDTypeAggAvg),
					  element_local_name))
	{
		m_mdid_avg_op = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDTypeAggAvg);
	}
	else if (0 == XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenMDTypeAggSum),
					  element_local_name))
	{
		m_mdid_sum_op = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDTypeAggSum);
	}
	else if (0 == XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenMDTypeAggCount),
					  element_local_name))
	{
		m_mdid_count_op = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDTypeAggCount);
	}
	else if (0 == XMLString::compareString(
					  CDXLTokens::XmlstrToken(EdxltokenMDTypeDistrOpfamily),
					  element_local_name))
	{
		m_distr_opfamily = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
			m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid,
			EdxltokenMDTypeDistrOpfamily);
	}
	else if (0 ==
			 XMLString::compareString(
				 CDXLTokens::XmlstrToken(EdxltokenMDTypeLegacyDistrOpfamily),
				 element_local_name))
	{
		m_legacy_distr_opfamily =
			CDXLOperatorFactory::ExtractConvertAttrValueToMdId(
				m_parse_handler_mgr->GetDXLMemoryManager(), attrs,
				EdxltokenMdid, EdxltokenMDTypeLegacyDistrOpfamily);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDType::IsBuiltInType
//
//	@doc:
//		Is this a built-in type
//
//---------------------------------------------------------------------------
BOOL
CParseHandlerMDType::IsBuiltInType(const gpos::Ref<IMDId> mdid)
{
	if (IMDId::EmdidGPDB != mdid->MdidType())
	{
		return false;
	}

	const gpos::Ref<CMDIdGPDB> mdidGPDB = CMDIdGPDB::CastMdid(mdid);

	switch (mdidGPDB->Oid())
	{
		case GPDB_INT2:
		case GPDB_INT4:
		case GPDB_INT8:
		case GPDB_BOOL:
		case GPDB_OID:
			return true;
		default:
			return false;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDType::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDType::EndElement(const XMLCh *const,	 // element_uri,
								const XMLCh *const element_local_name,
								const XMLCh *const	// element_qname
)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenMDType),
									  element_local_name))
	{
		// construct the MD type object from its part
		GPOS_ASSERT(m_mdid->IsValid());

		// TODO:  - Jan 30, 2012; add support for other types of mdids

		const gpos::Ref<CMDIdGPDB> pmdidGPDB =
			gpos::dyn_cast<CMDIdGPDB>(m_mdid.get());

		switch (pmdidGPDB->Oid())
		{
			case GPDB_INT2:
				m_imd_obj = GPOS_NEW(m_mp) CMDTypeInt2GPDB(m_mp);
				break;

			case GPDB_INT4:
				m_imd_obj = GPOS_NEW(m_mp) CMDTypeInt4GPDB(m_mp);
				break;

			case GPDB_INT8:
				m_imd_obj = GPOS_NEW(m_mp) CMDTypeInt8GPDB(m_mp);
				break;

			case GPDB_BOOL:
				m_imd_obj = GPOS_NEW(m_mp) CMDTypeBoolGPDB(m_mp);
				break;

			case GPDB_OID:
				m_imd_obj = GPOS_NEW(m_mp) CMDTypeOidGPDB(m_mp);
				break;

			default:

				ULONG length = 0;
				if (0 < m_type_length)
				{
					length = (ULONG) m_type_length;
				}
				m_imd_obj = GPOS_NEW(m_mp) CMDTypeGenericGPDB(
					m_mp, m_mdid, m_mdname, m_is_redistributable,
					m_istype_fixed_Length, length, m_type_passed_by_value,
					m_distr_opfamily, m_legacy_distr_opfamily, m_mdid_eq_op,
					m_mdid_neq_op, m_mdid_lt_op, m_mdid_lteq_op, m_mdid_gt_op,
					m_mdid_gteq_op, m_mdid_cmp_op, m_mdid_min_op, m_mdid_max_op,
					m_mdid_avg_op, m_mdid_sum_op, m_mdid_count_op,
					m_is_hashable, m_is_merge_joinable, m_is_composite,
					m_is_text_related, m_mdid_base_rel, m_mdid_array_type,
					m_type_length);
				break;
		}

		// deactivate handler
		m_parse_handler_mgr->DeactivateHandler();
	}
}

// EOF
