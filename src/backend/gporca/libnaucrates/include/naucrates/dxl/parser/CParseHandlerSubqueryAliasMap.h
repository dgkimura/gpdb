//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2024 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerSubqueryAliasMap.h
//
//	@doc:
//		SAX parse handler class for subquery alias mapping
//---------------------------------------------------------------------------

#ifndef GPDXL_CParseHandlerSubqueryAliasMap_H
#define GPDXL_CParseHandlerSubqueryAliasMap_H

#include "gpos/base.h"

#include "naucrates/dxl/parser/CParseHandlerBase.h"
#include "naucrates/md/CMDRelationGPDB.h"


namespace gpdxl
{
using namespace gpos;
using namespace gpmd;

XERCES_CPP_NAMESPACE_USE

using StringToStringArrayMap =
	CHashMap<CWStringBase, StringPtrArray, CWStringBase::HashValue,
			 CWStringBase::Equals, CleanupDelete<CWStringBase>,
			 CleanupRelease<StringPtrArray>>;

//---------------------------------------------------------------------------
//	@class:
//		CParseHandlerSubqueryAliasMap
//
//	@doc:
//		Parse handler for subquery alias mapping
//
//---------------------------------------------------------------------------
class CParseHandlerSubqueryAliasMap : public CParseHandlerBase
{
private:
	// list of columns
	StringToStringArrayMap *m_alias_table_names_map;

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

public:
	CParseHandlerSubqueryAliasMap(const CParseHandlerSubqueryAliasMap &) =
		delete;

	// ctor/dtor
	CParseHandlerSubqueryAliasMap(CMemoryPool *mp,
								  CParseHandlerManager *parse_handler_mgr,
								  CParseHandlerBase *parse_handler_root);

	~CParseHandlerSubqueryAliasMap() override;


	// returns the constructed columns list
	StringToStringArrayMap *GetAliasTableNamesMap();
};
}  // namespace gpdxl

#endif	// !GPDXL_CParseHandlerSubqueryAliasMap_H

// EOF
