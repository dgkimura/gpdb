//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		IMDExtStats.h
//
//	@doc:
//		Interface for column stats
//---------------------------------------------------------------------------



#ifndef GPMD_IMDExtStats_H
#define GPMD_IMDExtStats_H

#include "gpos/base.h"

#include "naucrates/md/IMDCacheObject.h"

namespace gpmd
{
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@class:
//		IMDExtStats
//
//	@doc:
//		Interface for column stats
//
//---------------------------------------------------------------------------
class IMDExtStats : public IMDCacheObject
{
public:
	// object type
	Emdtype
	MDType() const override
	{
		return EmdtExtStats;
	}
};
}  // namespace gpmd


#endif	// !GPMD_IMDExtStats_H

// EOF
