//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.

#include "gpopt/base/CColConstraintsArrayMapper.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CConstraint.h"

using namespace gpopt;

gpos::Ref<CConstraintArray>
CColConstraintsArrayMapper::PdrgPcnstrLookup(CColRef *colref)
{
	const BOOL fExclusive = true;
	return CConstraint::PdrgpcnstrOnColumn(m_mp, m_pdrgpcnstr.get(), colref,
										   fExclusive);
}

CColConstraintsArrayMapper::CColConstraintsArrayMapper(
	gpos::CMemoryPool *mp, gpos::Ref<CConstraintArray> pdrgpcnstr)
	: m_mp(mp), m_pdrgpcnstr(std::move(pdrgpcnstr))
{
}

CColConstraintsArrayMapper::~CColConstraintsArrayMapper()
{
	;
}
