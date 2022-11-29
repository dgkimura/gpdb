//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLExtStats.cpp
//
//	@doc:
//		Implementation of the class for representing column stats in DXL
//---------------------------------------------------------------------------


#include "naucrates/md/CDXLExtStats.h"

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/statistics/CStatistics.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CDXLExtStats::CDXLExtStats
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLExtStats::CDXLExtStats(CMemoryPool *mp, CMDIdExtStats *mdid_ext_stats,
						   CDXLExtStatArray *ext_stats)
	: m_mp(mp), m_mdid_ext_stats(mdid_ext_stats), m_ext_stats(ext_stats)
{
	GPOS_ASSERT(mdid_ext_stats->IsValid());
	m_dxl_str = CDXLUtils::SerializeMDObj(
		m_mp, this, false /*fSerializeHeader*/, false /*indentation*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLExtStats::~CDXLExtStats
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLExtStats::~CDXLExtStats()
{
	GPOS_DELETE(m_dxl_str);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLExtStats::MDId
//
//	@doc:
//		Returns the metadata id of this column stats object
//
//---------------------------------------------------------------------------
IMDId *
CDXLExtStats::MDId() const
{
	return m_mdid_ext_stats;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLExtStats::Mdname
//
//	@doc:
//		Returns the name of this column
//
//---------------------------------------------------------------------------
CMDName
CDXLExtStats::Mdname() const
{
	return CMDName(GPOS_NEW(m_mp) CWStringConst(GPOS_WSZ_LIT("ExtStat")));
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLExtStats::GetMDName
//
//	@doc:
//		Returns the DXL string for this object
//
//---------------------------------------------------------------------------
const CWStringDynamic *
CDXLExtStats::GetStrRepr() const
{
	return m_dxl_str;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLExtStats::Serialize
//
//	@doc:
//		Serialize column stats in DXL format
//
//---------------------------------------------------------------------------
void
CDXLExtStats::Serialize(CXMLSerializer *xml_serializer) const
{
	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenExtendedStats));

	for (ULONG i = 0; i < m_ext_stats->Size(); i++)
	{
		(*m_ext_stats)[i]->Serialize(xml_serializer);
	}

	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenExtendedStats));
}



#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLExtStats::DebugPrint
//
//	@doc:
//		Dbug print of the column stats object
//
//---------------------------------------------------------------------------
void
CDXLExtStats::DebugPrint(IOstream &os) const
{
	os << "Extended stats id: ";
	MDId()->OsPrint(os);
	os << std::endl;
}

#endif	// GPOS_DEBUG

// EOF
