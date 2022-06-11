//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalProperties.cpp
//
//	@doc:
//		Implementation of DXL physical operators
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLPhysicalProperties.h"

#include "gpos/common/owner.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalProperties::CDXLPhysicalProperties
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalProperties::CDXLPhysicalProperties(gpos::Ref<CDXLOperatorCost> cost)
	: CDXLProperties(), m_operator_cost_dxl(std::move(cost))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalProperties::~CDXLPhysicalProperties
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLPhysicalProperties::~CDXLPhysicalProperties()
{
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalProperties::SerializePropertiesToDXL
//
//	@doc:
//		Serialize operator properties in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalProperties::SerializePropertiesToDXL(
	CXMLSerializer *xml_serializer) const
{
	xml_serializer->OpenElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenProperties));

	m_operator_cost_dxl->SerializeToDXL(xml_serializer);
	SerializeStatsToDXL(xml_serializer);

	xml_serializer->CloseElement(
		CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
		CDXLTokens::GetDXLTokenStr(EdxltokenProperties));
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalProperties::MakeDXLOperatorCost
//
//	@doc:
//		Return cost of operator
//
//---------------------------------------------------------------------------
CDXLOperatorCost *
CDXLPhysicalProperties::GetDXLOperatorCost() const
{
	return m_operator_cost_dxl.get();
}

// EOF
