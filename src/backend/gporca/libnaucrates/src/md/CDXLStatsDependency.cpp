//--------------------------------------------------------------------------- //	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLStatsDependency.cpp
//
//	@doc:
//		Implementation of the class for representing column stats in DXL
//---------------------------------------------------------------------------


#include "naucrates/md/CDXLStatsDependency.h"

#include "gpos/common/CAutoRef.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/statistics/CStatistics.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CDXLStatsDependency::CDXLStatsDependency
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLStatsDependency::CDXLStatsDependency(CMemoryPool *mp,
										 std::vector<ULONG> from_attno,
										 ULONG to_attno, CDouble degree)
	: m_mp(mp), m_from_attno(from_attno), m_to_attno(to_attno), m_degree(degree)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLStatsDependency::~CDXLStatsDependency
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLStatsDependency::~CDXLStatsDependency()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLStatsDependency::Serialize
//
//	@doc:
//		Serialize column stats in DXL format
//
//---------------------------------------------------------------------------
void
CDXLStatsDependency::Serialize(CXMLSerializer *xml_serializer) const
{
	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenStatsDependency));

	CWStringDynamic *relationship = GPOS_NEW(m_mp) CWStringDynamic(m_mp);
	relationship->AppendFormat(GPOS_WSZ_LIT("%d"), m_from_attno[0]);
	for (ULONG i = 1; i < m_from_attno.size(); i++)
	{
		relationship->AppendFormat(GPOS_WSZ_LIT(", %d"), m_from_attno[i]);
	}
	// Note '>' is escaped as '&gt;' in XML attribute value
	relationship->AppendFormat(GPOS_WSZ_LIT(" => %d"), m_to_attno);

	xml_serializer->AddAttribute(
		CDXLTokens::GetDXLTokenStr(EdxltokenRelationship), relationship);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenDegree),
								 m_degree);

	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenStatsDependency));
}



#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLStatsDependency::DebugPrint
//
//	@doc:
//		Dbug print of the column stats object
//
//---------------------------------------------------------------------------
void
CDXLStatsDependency::DebugPrint(IOstream &os) const
{
	os << "Extended Stats Dependency: ";
	os << m_from_attno[0];
	for (ULONG i = 1; i < m_from_attno.size(); i++)
	{
		os << ", " << m_from_attno[i];
	}
	os << " => " << m_to_attno << " Degree: " << m_degree << std::endl;
}

#endif	// GPOS_DEBUG

// EOF
