//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.

#include "gpopt/minidump/CMetadataAccessorFactory.h"

#include "gpos/common/CAutoRef.h"
#include "gpos/common/owner.h"

#include "gpopt/mdcache/CMDCache.h"
#include "naucrates/md/CMDProviderMemory.h"

namespace gpopt
{
CMetadataAccessorFactory::CMetadataAccessorFactory(CMemoryPool *mp,
												   CDXLMinidump *pdxlmd,
												   const CHAR *file_name)
{
	// set up MD providers
	gpos::Ref<CMDProviderMemory> apmdp(GPOS_NEW(mp)
										   CMDProviderMemory(mp, file_name));
	const CSystemIdArray *pdrgpsysid = pdxlmd->GetSysidPtrArray();
	gpos::Ref<CMDProviderArray> apdrgpmdp(GPOS_NEW(mp) CMDProviderArray(mp));

	// ensure there is at least ONE system id
	;
	apdrgpmdp->Append(apmdp.get());

	for (ULONG ul = 1; ul < pdrgpsysid->Size(); ul++)
	{
		;
		apdrgpmdp->Append(apmdp.get());
	}

	m_apmda = GPOS_NEW(mp) CMDAccessor(
		mp, CMDCache::Pcache(), pdxlmd->GetSysidPtrArray(), apdrgpmdp.get());
}

CMDAccessor *
CMetadataAccessorFactory::Pmda()
{
	return m_apmda.Value();
}
}  // namespace gpopt
