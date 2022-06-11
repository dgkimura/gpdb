//	Greenplum Database
//	Copyright (C) 2016 VMware, Inc. or its affiliates.

#include "gpopt/base/CColConstraintsHashMapper.h"

#include "gpos/common/CAutoRef.h"
#include "gpos/common/owner.h"

using namespace gpopt;

gpos::owner<CConstraintArray *>
CColConstraintsHashMapper::PdrgPcnstrLookup(CColRef *colref)
{
	gpos::owner<CConstraintArray *> pdrgpcnstrCol =
		m_phmColConstr->Find(colref);
	pdrgpcnstrCol->AddRef();
	return pdrgpcnstrCol;
}

// mapping between columns and single column constraints in array of constraints
static gpos::owner<ColRefToConstraintArrayMap *>
PhmcolconstrSingleColConstr(CMemoryPool *mp,
							gpos::pointer<const CConstraintArray *> drgPcnstr)
{
	gpos::owner<ColRefToConstraintArrayMap *> phmcolconstr =
		GPOS_NEW(mp) ColRefToConstraintArrayMap(mp);

	const ULONG length = drgPcnstr->Size();

	for (ULONG ul = 0; ul < length; ul++)
	{
		CConstraint *pcnstrChild = (*drgPcnstr)[ul];
		gpos::pointer<CColRefSet *> pcrs = pcnstrChild->PcrsUsed();

		if (1 == pcrs->Size())
		{
			CColRef *colref = pcrs->PcrFirst();
			gpos::owner<CConstraintArray *> pcnstrMapped =
				phmcolconstr->Find(colref);
			if (nullptr == pcnstrMapped)
			{
				pcnstrMapped = GPOS_NEW(mp) CConstraintArray(mp);
				phmcolconstr->Insert(colref, pcnstrMapped);
			}
			pcnstrChild->AddRef();
			pcnstrMapped->Append(pcnstrChild);
		}
	}

	return phmcolconstr;
}

CColConstraintsHashMapper::CColConstraintsHashMapper(
	CMemoryPool *mp, gpos::pointer<CConstraintArray *> pdrgpcnstr)
	: m_phmColConstr(PhmcolconstrSingleColConstr(mp, pdrgpcnstr))
{
}

CColConstraintsHashMapper::~CColConstraintsHashMapper()
{
	m_phmColConstr->Release();
}
