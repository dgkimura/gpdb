//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLExtStat.h
//
//	@doc:
//		Class representing column stats
//---------------------------------------------------------------------------

#ifndef GPMD_CDXLExtStat_H
#define GPMD_CDXLExtStat_H

#include <vector>

#include "gpos/base.h"
#include "gpos/common/CDouble.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CDXLStatsDependency.h"
#include "naucrates/md/CMDIdExtStats.h"
#include "naucrates/md/IMDExtStats.h"

namespace gpdxl
{
class CXMLSerializer;
}

namespace gpmd
{
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@class:
//		CDXLExtStat
//
//	@doc:
//		Class representing extended stat
//
//---------------------------------------------------------------------------
class CDXLExtStat : public CRefCount
{
private:
	// memory pool
	CMemoryPool *m_mp;

	// column name
	CMDName *m_mdname;

	std::vector<INT> m_stxkeys;

	CDXLStatsDependencyArray *m_dependencies;

	CWStringDynamic *SerializeStxKeys() const;

public:
	CDXLExtStat(const CDXLExtStat &) = delete;

	// ctor
	CDXLExtStat(CMemoryPool *mp, CMDName *mdname, std::vector<INT> stxkeys,
				CDXLStatsDependencyArray *dependencies);

	// dtor
	~CDXLExtStat() override;

	// relation name
	CMDName Mdname() const;

	// serialize column stats in DXL format
	void Serialize(gpdxl::CXMLSerializer *) const;

#ifdef GPOS_DEBUG
	// debug print of the column stats
	void DebugPrint(IOstream &os) const;
#endif
};

using CDXLExtStatArray = CDynamicPtrArray<CDXLExtStat, CleanupRelease>;

}  // namespace gpmd



#endif	// !GPMD_CDXLExtStat_H

// EOF
