//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalDynamicIndexOnlyScan.cpp
//
//	@doc:
//		Implementation of dynamic table scan operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalDynamicIndexOnlyScan.h"

#include "gpos/base.h"

#include "gpopt/base/CUtils.h"

using namespace gpopt;


CPartitionPropagationSpec *
CPhysicalDynamicIndexOnlyScan::PppsDerive(CMemoryPool *mp,
										  CExpressionHandle &) const
{
	CPartitionPropagationSpec *pps = GPOS_NEW(mp) CPartitionPropagationSpec(mp);
	pps->Insert(ScanId(), CPartitionPropagationSpec::EpptConsumer,
				Ptabdesc()->MDId(), nullptr, nullptr);

	return pps;
}

// EOF
