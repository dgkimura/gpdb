//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		IMDTypeInt8.h
//
//	@doc:
//		Interface for INT8 types in the metadata cache
//---------------------------------------------------------------------------

#ifndef GPMD_IMDTypeInt8_H
#define GPMD_IMDTypeInt8_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/base/IDatumInt8.h"
#include "naucrates/md/IMDType.h"

namespace gpmd
{
using namespace gpos;
using namespace gpnaucrates;


//---------------------------------------------------------------------------
//	@class:
//		IMDTypeInt8
//
//	@doc:
//		Interface for INT8 types in the metadata cache
//
//---------------------------------------------------------------------------
class IMDTypeInt8 : public IMDType
{
public:
	// type id
	static ETypeInfo
	GetTypeInfo()
	{
		return EtiInt8;
	}

	ETypeInfo
	GetDatumType() const override
	{
		return IMDTypeInt8::GetTypeInfo();
	}

	// factory function for INT8 datums
	virtual gpos::owner<IDatumInt8 *> CreateInt8Datum(CMemoryPool *mp,
													  LINT value,
													  BOOL is_null) const = 0;
};

}  // namespace gpmd

#endif	// !GPMD_IMDTypeInt8_H

// EOF
