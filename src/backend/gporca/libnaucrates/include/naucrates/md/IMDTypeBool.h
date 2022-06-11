//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		IMDTypeBool.h
//
//	@doc:
//		Interface for BOOL types in the metadata cache
//---------------------------------------------------------------------------



#ifndef GPMD_IMDTypeBool_H
#define GPMD_IMDTypeBool_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/base/IDatumBool.h"
#include "naucrates/dxl/gpdb_types.h"
#include "naucrates/md/IMDType.h"

namespace gpmd
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		IMDTypeBool
//
//	@doc:
//		Interface for BOOL types in the metadata cache
//
//---------------------------------------------------------------------------
class IMDTypeBool : public IMDType
{
public:
	// type id
	static ETypeInfo
	GetTypeInfo()
	{
		return EtiBool;
	}

	ETypeInfo
	GetDatumType() const override
	{
		return IMDTypeBool::GetTypeInfo();
	}

	// factory function for BOOL datums
	virtual gpos::Ref<IDatumBool> CreateBoolDatum(CMemoryPool *mp, BOOL value,
												  BOOL is_null) const = 0;
};

}  // namespace gpmd

#endif	// !GPMD_IMDTypeBool_H

// EOF
