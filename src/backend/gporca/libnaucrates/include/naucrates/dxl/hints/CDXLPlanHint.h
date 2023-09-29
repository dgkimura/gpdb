//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (c) 2023 VMware, Inc. or its affiliates. All Rights Reserved.
//
//	@filename:
//		CDXLPlanHint.h
//---------------------------------------------------------------------------



#ifndef GPDXL_CDXLPlanHintiptor_H
#define GPDXL_CDXLPlanHintiptor_H

#include "gpos/base.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

namespace gpdxl
{
//---------------------------------------------------------------------------
//	@class:
//		CDXLPlanHint
//---------------------------------------------------------------------------
class CDXLPlanHint : public CRefCount
{
private:
	// identifier of query to which current table belongs.
	// This field is used for assigning current table entry with
	// target one within DML operation. If descriptor doesn't point
	// to the target (result) relation it has value UNASSIGNED_QUERYID
	ULONG m_assigned_query_id_for_target_rel;

	void SerializeMDId(CXMLSerializer *xml_serializer) const;

public:
	CDXLPlanHint(const CDXLPlanHint &) = delete;

	// ctor/dtor
	CDXLPlanHint();

	~CDXLPlanHint() override;

	// hint arity
	ULONG Arity() const;

	// serialize to dxl format
	void SerializeToDXL(CXMLSerializer *xml_serializer) const;
};
}  // namespace gpdxl


#endif	// !GPDXL_CDXLHintiptor_H

// EOF
