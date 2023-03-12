//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLUtils.h
//
//	@doc:
//		Entry point for parsing and serializing DXL documents.
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLUtils_H
#define GPDXL_CDXLUtils_H

#include <xercesc/util/XMLString.hpp>

#include "gpos/base.h"
#include "gpos/common/CAutoP.h"
#include "gpos/io/IOstream.h"
#include "gpos/string/CWStringDynamic.h"

#include "gpopt/optimizer/COptimizerConfig.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/CMDRequest.h"
#include "naucrates/md/IMDCacheObject.h"
#include "naucrates/statistics/CStatistics.h"

namespace gpopt
{
class CEnumeratorConfig;
class CStatisticsConfig;
}  // namespace gpopt

namespace gpdxl
{
using namespace gpos;
using namespace gpmd;
using namespace gpnaucrates;

XERCES_CPP_NAMESPACE_USE

// fwd decl
class CParseHandlerDXL;
class CDXLMemoryManager;
class CQueryToDXLResult;

using CStatisticsArray = gpos::Vector<gpos::Ref<CStatistics>>;

//---------------------------------------------------------------------------
//	@class:
//		CDXLUtils
//
//	@doc:
//		Entry point for parsing and serializing DXL documents.
//
//---------------------------------------------------------------------------
class CDXLUtils
{
private:
	// same as above but with a wide string parameter for the DXL document
	static CParseHandlerDXL *GetParseHandlerForDXLString(
		CMemoryPool *, const CWStringBase *dxl_string,
		const CHAR *xsd_file_path);



public:
	// helper functions for serializing DXL document header and footer, respectively
	static void SerializeHeader(CMemoryPool *, CXMLSerializer *);
	static void SerializeFooter(CXMLSerializer *);
	// helper routine which initializes and starts the xerces parser,
	// and returns the top-level parse handler which can be used to
	// retrieve the parsed elements
	static CParseHandlerDXL *GetParseHandlerForDXLString(
		CMemoryPool *, const CHAR *dxl_string, const CHAR *xsd_file_path);

	// same as above but with DXL file name specified instead of the file contents
	static CParseHandlerDXL *GetParseHandlerForDXLFile(
		CMemoryPool *, const CHAR *dxl_filename, const CHAR *xsd_file_path);

	// parse a DXL document containing a DXL plan
	static gpos::Ref<CDXLNode> GetPlanDXLNode(CMemoryPool *,
											  const CHAR *dxl_string,
											  const CHAR *xsd_file_path,
											  ULLONG *plan_id,
											  ULLONG *plan_space_size);

	// parse a DXL document representing a query
	// to return the DXL tree representing the query and
	// a DXL tree representing the query output
	static CQueryToDXLResult *ParseQueryToQueryDXLTree(
		CMemoryPool *, const CHAR *dxl_string, const CHAR *xsd_file_path);

	// parse a DXL document containing a scalar expression
	static gpos::Ref<CDXLNode> ParseDXLToScalarExprDXLNode(
		CMemoryPool *, const CHAR *dxl_string, const CHAR *xsd_file_path);

	// parse an MD request
	static gpos::Ref<CMDRequest> ParseDXLToMDRequest(CMemoryPool *mp,
													 const CHAR *dxl_string,
													 const CHAR *xsd_file_path);

	// parse a list of mdids from a MD request message
	static gpos::Ref<CMDRequest> ParseDXLToMDRequest(CMemoryPool *mp,
													 const WCHAR *dxl_string,
													 const CHAR *xsd_file_path);

	// parse optimizer config DXL
	static gpos::Ref<COptimizerConfig> ParseDXLToOptimizerConfig(
		CMemoryPool *mp, const CHAR *dxl_string, const CHAR *xsd_file_path);

	static gpos::Ref<IMDCacheObjectArray> ParseDXLToIMDObjectArray(
		CMemoryPool *, const CHAR *dxl_string, const CHAR *xsd_file_path);

	static gpos::Ref<IMDCacheObjectArray> ParseDXLToIMDObjectArray(
		CMemoryPool *, const CWStringBase *dxl_string,
		const CHAR *xsd_file_path);

	// parse mdid from a metadata document
	static gpos::Ref<IMDId> ParseDXLToMDId(CMemoryPool *,
										   const CWStringBase *dxl_string,
										   const CHAR *xsd_file_path);

	static gpos::Ref<IMDCacheObject> ParseDXLToIMDIdCacheObj(
		CMemoryPool *, const CWStringBase *dxl_string,
		const CHAR *xsd_file_path);

	// parse statistics object from the statistics document
	static CDXLStatsDerivedRelationArray *ParseDXLToStatsDerivedRelArray(
		CMemoryPool *, const CHAR *dxl_string, const CHAR *xsd_file_path);

	// parse statistics object from the statistics document
	static CDXLStatsDerivedRelationArray *ParseDXLToStatsDerivedRelArray(
		CMemoryPool *, const CWStringBase *dxl_string,
		const CHAR *xsd_file_path);

	// translate the dxl statistics object to optimizer statistics object
	static CStatisticsArray *ParseDXLToOptimizerStatisticObjArray(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		CDXLStatsDerivedRelationArray *dxl_derived_rel_stats_array);

	// extract the array of optimizer buckets from the dxl representation of
	// dxl buckets in the dxl derived column statistics object
	static gpos::Ref<CBucketArray> ParseDXLToBucketsArray(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::Ref<CDXLStatsDerivedColumn> dxl_derived_col_stats);

	// serialize a DXL query tree into DXL Document
	static void SerializeQuery(
		CMemoryPool *mp, IOstream &os, const gpos::Ref<CDXLNode> dxl_query_node,
		const gpos::Ref<CDXLNodeArray> query_output_dxlnode_array,
		const gpos::Ref<CDXLNodeArray> cte_producers,
		BOOL serialize_document_header_footer, BOOL indentation);

	// serialize a ULLONG value
	static CWStringDynamic *SerializeULLONG(CMemoryPool *mp, ULLONG value);

	// serialize a plan into DXL
	static void SerializePlan(CMemoryPool *mp, IOstream &os,
							  const gpos::Ref<CDXLNode> node, ULLONG plan_id,
							  ULLONG plan_space_size,
							  BOOL serialize_document_header_footer,
							  BOOL indentation);

	static CWStringDynamic *SerializeStatistics(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		const CStatisticsArray *statistics_array, BOOL serialize_header_footer,
		BOOL indentation);

	// serialize statistics objects into DXL and write to stream
	static void SerializeStatistics(CMemoryPool *mp, CMDAccessor *md_accessor,
									const CStatisticsArray *statistics_array,
									IOstream &os,
									BOOL serialize_document_header_footer,
									BOOL indentation);

	// serialize metadata objects into DXL and write to stream
	static void SerializeMetadata(
		CMemoryPool *mp, const gpos::Ref<IMDCacheObjectArray> imd_obj_array,
		IOstream &os, BOOL serialize_document_header_footer, BOOL indentation);

	// serialize metadata ids into a MD request message
	static void SerializeMDRequest(CMemoryPool *mp,
								   gpos::Ref<CMDRequest> md_request,
								   IOstream &os,
								   BOOL serialize_document_header_footer,
								   BOOL indentation);

	// serialize a list of metadata objects into DXL
	static CWStringDynamic *SerializeMetadata(
		CMemoryPool *, const gpos::Ref<IMDCacheObjectArray>,
		BOOL serialize_document_header_footer, BOOL indentation);

	// serialize a metadata id into DXL
	static CWStringDynamic *SerializeMetadata(
		CMemoryPool *mp, const gpos::Ref<IMDId> mdid,
		BOOL serialize_document_header_footer, BOOL indentation);

	// serialize sample plans
	static CWStringDynamic *SerializeSamplePlans(
		CMemoryPool *mp, gpos::Ref<CEnumeratorConfig> enumerator_cfg,
		BOOL indentation);

	// serialize cost distribution plans
	static CWStringDynamic *SerializeCostDistr(
		CMemoryPool *mp, gpos::Ref<CEnumeratorConfig> enumerator_cfg,
		BOOL indentation);

	// serialize a metadata object into DXL
	static CWStringDynamic *SerializeMDObj(
		CMemoryPool *, const gpos::Ref<IMDCacheObject>,
		BOOL serialize_document_header_footer, BOOL indentation);

	// serialize a scalar expression into DXL
	static CWStringDynamic *SerializeScalarExpr(
		CMemoryPool *mp, const gpos::Ref<CDXLNode> node,
		BOOL serialize_document_header_footer, BOOL indentation);

	// create a GPOS dynamic string from a Xerces XMLCh array
	static CWStringDynamic *CreateDynamicStringFromXMLChArray(
		CDXLMemoryManager *memory_manager, const XMLCh *);

	// create a GPOS string object from a base 64 encoded XML string
	static BYTE *CreateStringFrom64XMLStr(CDXLMemoryManager *memory_manager,
										  const XMLCh *xml_string,
										  ULONG *length);

	// create a GPOS dynamic string from a regular character array
	static CWStringDynamic *CreateDynamicStringFromCharArray(CMemoryPool *mp,
															 const CHAR *c);

	// create an MD name from a character array
	static CMDName *CreateMDNameFromCharArray(CMemoryPool *mp, const CHAR *c);

	// create an MD name from a Xerces character array
	static CMDName *CreateMDNameFromXMLChar(CDXLMemoryManager *memory_manager,
											const XMLCh *xml_string);

	// encode a byte array to a string
	static CWStringDynamic *EncodeByteArrayToString(CMemoryPool *mp,
													const BYTE *byte,
													ULONG length);

	// serialize a list of integers into a comma-separate string
	template <typename T, void (*CleanupFn)(T *)>
	static CWStringDynamic *Serialize(
		CMemoryPool *mp, const CDynamicPtrArray<T, CleanupFn> *arr);
	template <typename T, void (*CleanupFn)(T *)>
	static CWStringDynamic *Serialize(
		CMemoryPool *mp, const gpos::Ref<CDynamicPtrArray<T, CleanupFn>> arr);

	// serialize a list of lists of integers into a comma-separate string
	static CWStringDynamic *Serialize(CMemoryPool *mp,
									  const gpos::Ref<ULongPtr2dArray> pdrgpul);

	// serialize a list of chars into a comma-separate string
	static CWStringDynamic *SerializeToCommaSeparatedString(
		CMemoryPool *mp, const CharPtrArray *pdrgpsz);

	// decode a byte array from a string
	static BYTE *DecodeByteArrayFromString(CMemoryPool *mp,
										   const CWStringDynamic *dxl_string,
										   ULONG *length);

	static CHAR *Read(CMemoryPool *mp, const CHAR *filename);

	// create a multi-byte character string from a wide character string
	static CHAR *CreateMultiByteCharStringFromWCString(CMemoryPool *mp,
													   const WCHAR *wc_string);

	// serialize a double value in a string
	static CWStringDynamic *SerializeDouble(CDXLMemoryManager *memory_manager,
											CDouble value);

	// translate the optimizer datum from dxl datum object
	static gpos::Ref<IDatum> GetDatum(CMemoryPool *mp, CMDAccessor *md_accessor,
									  const gpos::Ref<CDXLDatum> dxl_datum);

#ifdef GPOS_DEBUG
	// debug print of the metadata relation
	static void DebugPrintMDIdArray(IOstream &os,
									gpos::Ref<IMdIdArray> mdid_array);
#endif
};

// serialize a list of integers into a comma-separate string
template <typename T, void (*CleanupFn)(T *)>
CWStringDynamic *
CDXLUtils::Serialize(
	CMemoryPool *mp,
	const gpos::Ref<CDynamicPtrArray<T, CleanupFn>> dynamic_ptr_array)
{
	return Serialize(mp, dynamic_ptr_array.get());
}
template <typename T, void (*CleanupFn)(T *)>
CWStringDynamic *
CDXLUtils::Serialize(CMemoryPool *mp,
					 const CDynamicPtrArray<T, CleanupFn> *dynamic_ptr_array)
{
	CAutoP<CWStringDynamic> string_var(GPOS_NEW(mp) CWStringDynamic(mp));

	if (nullptr == dynamic_ptr_array)
	{
		return string_var.Reset();
	}

	ULONG length = dynamic_ptr_array->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		T value = *((*dynamic_ptr_array)[ul]);
		if (ul == length - 1)
		{
			// last element: do not print a comma
			string_var->AppendFormat(GPOS_WSZ_LIT("%d"), value);
		}
		else
		{
			string_var->AppendFormat(
				GPOS_WSZ_LIT("%d%ls"), value,
				CDXLTokens::GetDXLTokenStr(EdxltokenComma)->GetBuffer());
		}
	}

	return string_var.Reset();
}

}  // namespace gpdxl

#endif	// GPDXL_CDXLUtils_H

// EOF
