//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLScalarSwitch.h
//
//	@doc:
//
//		Class for representing DXL Switch (corresponds to Case (expr) ...)
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarSwitch_H
#define GPDXL_CDXLScalarSwitch_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLScalar.h"
#include "naucrates/md/IMDId.h"

namespace gpdxl
{
using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@class:
//		CDXLScalarSwitch
//
//	@doc:
//		Class for representing DXL Switch
//
//---------------------------------------------------------------------------
class CDXLScalarSwitch : public CDXLScalar
{
private:
	// return type
	gpos::owner<IMDId *> m_mdid_type;

public:
	CDXLScalarSwitch(const CDXLScalarSwitch &) = delete;

	// ctor
	CDXLScalarSwitch(CMemoryPool *mp, IMDId *mdid_type);

	//dtor
	~CDXLScalarSwitch() override;

	// name of the operator
	const CWStringConst *GetOpNameStr() const override;

	// return type
	virtual gpos::pointer<IMDId *> MdidType() const;

	// DXL Operator ID
	Edxlopid GetDXLOperator() const override;

	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						gpos::pointer<const CDXLNode *> dxlnode) const override;

	// conversion function
	static gpos::cast_func<CDXLScalarSwitch *>
	Cast(CDXLOperator *dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopScalarSwitch == dxl_op->GetDXLOperator());

		return dynamic_cast<CDXLScalarSwitch *>(dxl_op);
	}

	// does the operator return a boolean result
	BOOL HasBoolResult(CMDAccessor *md_accessor) const override;

#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(gpos::pointer<const CDXLNode *> dxlnode,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG
};
}  // namespace gpdxl
#endif	// !GPDXL_CDXLScalarSwitch_H

// EOF
