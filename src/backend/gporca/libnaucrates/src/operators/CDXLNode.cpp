//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLNode.cpp
//
//	@doc:
//		Implementation of DXL nodes
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLNode.h"

#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLDirectDispatchInfo.h"
#include "naucrates/dxl/operators/CDXLOperator.h"

using namespace gpos;
using namespace gpdxl;


//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::CDXLNode
//
//	@doc:
//		Constructs a DXL node with unspecified operator
//
//---------------------------------------------------------------------------
CDXLNode::CDXLNode(CMemoryPool *mp)
	: m_dxl_op(nullptr),
	  m_dxl_properties(nullptr),
	  m_direct_dispatch_info(nullptr)
{
	m_dxl_array = GPOS_NEW(mp) CDXLNodeArray(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::CDXLNode
//
//	@doc:
//		Constructs a DXL node with given operator
//
//---------------------------------------------------------------------------
CDXLNode::CDXLNode(CMemoryPool *mp, gpos::Ref<CDXLOperator> dxl_op)
	: m_dxl_op(std::move(dxl_op)),
	  m_dxl_properties(nullptr),
	  m_direct_dispatch_info(nullptr)
{
	GPOS_ASSERT(nullptr != m_dxl_op);
	m_dxl_array = GPOS_NEW(mp) CDXLNodeArray(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::CDXLNode
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLNode::CDXLNode(CMemoryPool *mp, gpos::Ref<CDXLOperator> dxl_op,
				   gpos::Ref<CDXLNode> child_dxlnode)
	: m_dxl_op(std::move(dxl_op)),
	  m_dxl_properties(nullptr),
	  m_dxl_array(nullptr),
	  m_direct_dispatch_info(nullptr)
{
	GPOS_ASSERT(nullptr != m_dxl_op);
	GPOS_ASSERT(nullptr != child_dxlnode);

	m_dxl_array = GPOS_NEW(mp) CDXLNodeArray(mp);
	m_dxl_array->Append(std::move(child_dxlnode));
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::CDXLNode
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLNode::CDXLNode(CMemoryPool *mp, gpos::Ref<CDXLOperator> dxl_op,
				   gpos::Ref<CDXLNode> first_child_dxlnode,
				   gpos::Ref<CDXLNode> second_child_dxlnode)
	: m_dxl_op(std::move(dxl_op)),
	  m_dxl_properties(nullptr),
	  m_dxl_array(nullptr),
	  m_direct_dispatch_info(nullptr)
{
	GPOS_ASSERT(nullptr != m_dxl_op);
	GPOS_ASSERT(nullptr != first_child_dxlnode);
	GPOS_ASSERT(nullptr != second_child_dxlnode);

	m_dxl_array = GPOS_NEW(mp) CDXLNodeArray(mp);
	m_dxl_array->Append(std::move(first_child_dxlnode));
	m_dxl_array->Append(std::move(second_child_dxlnode));
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::CDXLNode
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLNode::CDXLNode(CMemoryPool *mp, gpos::Ref<CDXLOperator> dxl_op,
				   gpos::Ref<CDXLNode> first_child_dxlnode,
				   gpos::Ref<CDXLNode> second_child_dxlnode,
				   gpos::Ref<CDXLNode> third_child_dxlnode)
	: m_dxl_op(std::move(dxl_op)),
	  m_dxl_properties(nullptr),
	  m_dxl_array(nullptr),
	  m_direct_dispatch_info(nullptr)
{
	GPOS_ASSERT(nullptr != m_dxl_op);
	GPOS_ASSERT(nullptr != first_child_dxlnode);
	GPOS_ASSERT(nullptr != second_child_dxlnode);
	GPOS_ASSERT(nullptr != third_child_dxlnode);

	m_dxl_array = GPOS_NEW(mp) CDXLNodeArray(mp);
	m_dxl_array->Append(std::move(first_child_dxlnode));
	m_dxl_array->Append(std::move(second_child_dxlnode));
	m_dxl_array->Append(std::move(third_child_dxlnode));
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::CDXLNode
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLNode::CDXLNode(gpos::Ref<CDXLOperator> dxl_op,
				   gpos::Ref<CDXLNodeArray> dxl_array)
	: m_dxl_op(std::move(dxl_op)),
	  m_dxl_properties(nullptr),
	  m_dxl_array(std::move(dxl_array)),
	  m_direct_dispatch_info(nullptr)
{
	GPOS_ASSERT(nullptr != m_dxl_op);
	GPOS_ASSERT(nullptr != m_dxl_array);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::~CDXLNode
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLNode::~CDXLNode()
{
	;
	;
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::AddChild
//
//	@doc:
//		Adds a child to the DXL node's list of children
//
//---------------------------------------------------------------------------
void
CDXLNode::AddChild(gpos::Ref<CDXLNode> child_dxlnode)
{
	GPOS_ASSERT(nullptr != m_dxl_array);
	GPOS_ASSERT(nullptr != child_dxlnode);

	m_dxl_array->Append(std::move(child_dxlnode));
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::ReplaceChild
//
//	@doc:
//		Replaces a child of the DXL node with a new one
//
//---------------------------------------------------------------------------
void
CDXLNode::ReplaceChild(ULONG pos, gpos::Ref<CDXLNode> child_dxlnode)
{
	GPOS_ASSERT(nullptr != m_dxl_array);
	GPOS_ASSERT(nullptr != child_dxlnode);

	m_dxl_array->Replace(pos, std::move(child_dxlnode));
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::SetOperator
//
//	@doc:
//		Sets the operator at that DXL node
//
//---------------------------------------------------------------------------
void
CDXLNode::SetOperator(gpos::Ref<CDXLOperator> dxl_op)
{
	GPOS_ASSERT(nullptr == m_dxl_op);
	m_dxl_op = dxl_op;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::SerializeToDXL
//
//	@doc:
//		Serializes the node in DXL format
//
//---------------------------------------------------------------------------
void
CDXLNode::SerializeToDXL(CXMLSerializer *xml_serializer) const
{
	if (nullptr != m_dxl_op)
	{
		m_dxl_op->SerializeToDXL(xml_serializer, this);
	}

	if (nullptr != m_direct_dispatch_info &&
		0 < m_direct_dispatch_info->GetDispatchIdentifierDatumArray()->Size())
	{
		m_direct_dispatch_info->Serialize(xml_serializer);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::SerializeChildrenToDXL
//
//	@doc:
//		Serializes the node's children in DXL format
//
//---------------------------------------------------------------------------
void
CDXLNode::SerializeChildrenToDXL(CXMLSerializer *xml_serializer) const
{
	// serialize children nodes
	const ULONG arity = Arity();
	for (ULONG idx = 0; idx < arity; idx++)
	{
		GPOS_CHECK_ABORT;

		CDXLNode *child_dxlnode = (*m_dxl_array)[idx].get();
		child_dxlnode->SerializeToDXL(xml_serializer);

		GPOS_CHECK_ABORT;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::SetProperties
//
//	@doc:
//		Set operator properties
//
//---------------------------------------------------------------------------
void
CDXLNode::SetProperties(gpos::Ref<CDXLProperties> dxl_properties)
{
	// allow setting properties only once
	GPOS_ASSERT(nullptr == m_dxl_properties);
	m_dxl_properties = dxl_properties;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::SetDirectDispatchInfo
//
//	@doc:
//		Set direct dispatch info
//
//---------------------------------------------------------------------------
void
CDXLNode::SetDirectDispatchInfo(
	gpos::Ref<CDXLDirectDispatchInfo> dxl_direct_dispatch_info)
{
	// allow setting direct dispatch info only once
	GPOS_ASSERT(nullptr == m_direct_dispatch_info);
	GPOS_ASSERT(nullptr != dxl_direct_dispatch_info);
	m_direct_dispatch_info = dxl_direct_dispatch_info;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::SerializePropertiesToDXL
//
//	@doc:
//		Serialize properties in DXL format
//
//---------------------------------------------------------------------------
void
CDXLNode::SerializePropertiesToDXL(CXMLSerializer *xml_serializer) const
{
	m_dxl_properties->SerializePropertiesToDXL(xml_serializer);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLNode::AssertValid
//
//	@doc:
//		Checks whether node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLNode::AssertValid(BOOL validate_children) const
{
	if (!validate_children)
	{
		return;
	}

	const ULONG arity = Arity();
	for (ULONG idx = 0; idx < arity; idx++)
	{
		CDXLNode *child_dxlnode = (*this)[idx];
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode,
												  validate_children);
	}
}
#endif	// GPOS_DEBUG



// EOF
