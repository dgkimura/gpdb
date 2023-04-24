//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.

#include "gpopt/base/CColConstraintsArrayMapper.h"

#include "gpopt/base/CConstraint.h"

using namespace gpopt;

gpos::Ref<CConstraintArray>
CColConstraintsArrayMapper::PdrgPcnstrLookup(gpos::Ref<CColRef> colref)
{
	const BOOL fExclusive = true;
	return CConstraint::PdrgpcnstrOnColumn(m_mp, m_pdrgpcnstr, colref,
										   fExclusive);
}

CColConstraintsArrayMapper::CColConstraintsArrayMapper(
	gpos::CMemoryPool *mp, gpos::Ref<CConstraintArray> pdrgpcnstr)
	: m_mp(mp), m_pdrgpcnstr(pdrgpcnstr)
{
}

CColConstraintsArrayMapper::~CColConstraintsArrayMapper()
{
}
