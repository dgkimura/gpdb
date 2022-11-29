//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLExtStat.cpp
//
//	@doc:
//		Implementation of the class for representing column stats in DXL
//---------------------------------------------------------------------------


#include "naucrates/md/CDXLExtStat.h"

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/statistics/CStatistics.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CDXLExtStat::CDXLExtStat
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLExtStat::CDXLExtStat(CMemoryPool *mp, CMDName *mdname,
						 CDXLStatsDependencyArray *dependencies)
	: m_mp(mp), m_mdname(mdname), m_dependencies(dependencies)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLExtStat::~CDXLExtStat
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLExtStat::~CDXLExtStat()
{
	GPOS_DELETE(m_mdname);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLExtStat::Mdname
//
//	@doc:
//		Returns the name of this column
//
//---------------------------------------------------------------------------
CMDName
CDXLExtStat::Mdname() const
{
	return *m_mdname;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLExtStat::Serialize
//
//	@doc:
//		Serialize column stats in DXL format
//
//---------------------------------------------------------------------------
void
CDXLExtStat::Serialize(CXMLSerializer *xml_serializer) const
{
	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenExtendedStat));

	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName),
								 Mdname().GetMDName());

	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenStatsDependencies));
	for (ULONG i = 0; i < m_dependencies->Size(); i++)
	{
		(*m_dependencies)[i]->Serialize(xml_serializer);
	}
	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenStatsDependencies));

	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenExtendedStat));
}



#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLExtStat::DebugPrint
//
//	@doc:
//		Dbug print of the column stats object
//
//---------------------------------------------------------------------------
void
CDXLExtStat::DebugPrint(IOstream &os) const
{
	os << "Column id: " << std::endl;

	os << "Statstic name: " << (Mdname()).GetMDName()->GetBuffer() << std::endl;
}

#endif	// GPOS_DEBUG

// EOF
