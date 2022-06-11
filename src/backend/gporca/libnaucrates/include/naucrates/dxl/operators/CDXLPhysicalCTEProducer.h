//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalCTEProducer.h
//
//	@doc:
//		Class for representing DXL physical CTE producer operators
//---------------------------------------------------------------------------
#ifndef GPDXL_CDXLPhysicalCTEProducer_H
#define GPDXL_CDXLPhysicalCTEProducer_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLPhysical.h"

namespace gpdxl
{
//---------------------------------------------------------------------------
//	@class:
//		CDXLPhysicalCTEProducer
//
//	@doc:
//		Class for representing DXL physical CTE producers
//
//---------------------------------------------------------------------------
class CDXLPhysicalCTEProducer : public CDXLPhysical
{
private:
	// cte id
	ULONG m_id;

	// output column ids
	gpos::Ref<ULongPtrArray> m_output_colids_array;

public:
	CDXLPhysicalCTEProducer(CDXLPhysicalCTEProducer &) = delete;

	// ctor
	CDXLPhysicalCTEProducer(CMemoryPool *mp, ULONG id,
							gpos::Ref<ULongPtrArray> output_colids_array);

	// dtor
	~CDXLPhysicalCTEProducer() override;

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

	ULongPtrArray *
	GetOutputColIdsArray() const
	{
		return m_output_colids_array.get();
	}

	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						const CDXLNode *dxlnode) const override;

#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(const CDXLNode *dxlnode,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG

	// conversion function
	static CDXLPhysicalCTEProducer *
	Cast(CDXLOperator *dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopPhysicalCTEProducer == dxl_op->GetDXLOperator());
		return dynamic_cast<CDXLPhysicalCTEProducer *>(dxl_op);
	}
};
}  // namespace gpdxl
#endif	// !GPDXL_CDXLPhysicalCTEProducer_H

// EOF
