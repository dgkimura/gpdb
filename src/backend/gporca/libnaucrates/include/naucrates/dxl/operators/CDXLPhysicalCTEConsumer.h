//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalCTEConsumer.h
//
//	@doc:
//		Class for representing DXL physical CTE Consumer operators
//---------------------------------------------------------------------------
#ifndef GPDXL_CDXLPhysicalCTEConsumer_H
#define GPDXL_CDXLPhysicalCTEConsumer_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/Casting.h"

#include "naucrates/dxl/operators/CDXLPhysical.h"
namespace gpdxl
{
//---------------------------------------------------------------------------
//	@class:
//		CDXLPhysicalCTEConsumer
//
//	@doc:
//		Class for representing DXL physical CTE Consumers
//
//---------------------------------------------------------------------------
class CDXLPhysicalCTEConsumer : public CDXLPhysical
{
private:
	// cte id
	ULONG m_id;

	// output column ids
	gpos::Ref<ULongPtrArray> m_output_colids_array;

public:
	CDXLPhysicalCTEConsumer(CDXLPhysicalCTEConsumer &) = delete;

	// ctor
	CDXLPhysicalCTEConsumer(CMemoryPool *mp, ULONG id,
							gpos::Ref<ULongPtrArray> output_colids_array);

	// dtor
	~CDXLPhysicalCTEConsumer() override;

	// operator type
	Edxlopid GetDXLOperator() const override;

	// operator name
	const CWStringConst *GetOpNameStr() const override;

	// cte identifier
	ULONG
	Id() const
	{
		return m_id;
	}

	gpos::Ref<ULongPtrArray>
	GetOutputColIdsArray() const
	{
		return m_output_colids_array;
	}

	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						const gpos::Ref<CDXLNode> dxlnode) const override;


#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(const gpos::Ref<CDXLNode> dxlnode,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG

	// conversion function
	static gpos::Ref<CDXLPhysicalCTEConsumer>
	Cast(gpos::Ref<CDXLOperator> dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopPhysicalCTEConsumer == dxl_op->GetDXLOperator());
		return gpos::dyn_cast<CDXLPhysicalCTEConsumer>(dxl_op);
	}
};
}  // namespace gpdxl
#endif	// !GPDXL_CDXLPhysicalCTEConsumer_H

// EOF
