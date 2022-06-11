//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CPartInfo.cpp
//
//	@doc:
//		Implementation of derived partition information at the logical level
//---------------------------------------------------------------------------

#include "gpopt/base/CPartInfo.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/error/CAutoTrace.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/metadata/CPartConstraint.h"

using namespace gpopt;

FORCE_GENERATE_DBGSTR(CPartInfo);
FORCE_GENERATE_DBGSTR(CPartInfo::CPartInfoEntry);

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::CPartInfoEntry::CPartInfoEntry
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPartInfo::CPartInfoEntry::CPartInfoEntry(
	ULONG scan_id, gpos::Ref<IMDId> mdid,
	gpos::Ref<CPartKeysArray> pdrgppartkeys)
	: m_scan_id(scan_id),
	  m_mdid(std::move(mdid)),
	  m_pdrgppartkeys(std::move(pdrgppartkeys))
{
	GPOS_ASSERT(m_mdid->IsValid());
	GPOS_ASSERT(m_pdrgppartkeys != nullptr);
	GPOS_ASSERT(0 < m_pdrgppartkeys->Size());
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::CPartInfoEntry::~CPartInfoEntry
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPartInfo::CPartInfoEntry::~CPartInfoEntry()
{
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::CPartInfoEntry::PpartinfoentryAddRemappedKeys
//
//	@doc:
//		Create a copy of the current object, and add a set of remapped
//		part keys to this entry, using the existing keys and the given hashmap
//
//---------------------------------------------------------------------------
gpos::Ref<CPartInfo::CPartInfoEntry>
CPartInfo::CPartInfoEntry::PpartinfoentryAddRemappedKeys(
	CMemoryPool *mp, CColRefSet *pcrs, UlongToColRefMap *colref_mapping)
{
	GPOS_ASSERT(nullptr != pcrs);
	GPOS_ASSERT(nullptr != colref_mapping);

	gpos::Ref<CPartKeysArray> pdrgppartkeys =
		CPartKeys::PdrgppartkeysCopy(mp, m_pdrgppartkeys.get());

	const ULONG size = m_pdrgppartkeys->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		CPartKeys *ppartkeys = (*m_pdrgppartkeys)[ul].get();

		if (ppartkeys->FOverlap(pcrs))
		{
			pdrgppartkeys->Append(
				ppartkeys->PpartkeysRemap(mp, colref_mapping));
			break;
		}
	}

	;

	return GPOS_NEW(mp)
		CPartInfoEntry(m_scan_id, m_mdid, std::move(pdrgppartkeys));
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::CPartInfoEntry::OsPrint
//
//	@doc:
//		Print functions
//
//---------------------------------------------------------------------------
IOstream &
CPartInfo::CPartInfoEntry::OsPrint(IOstream &os) const
{
	os << m_scan_id;

	return os;
}


//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::CPartInfoEntry::PpartinfoentryCopy
//
//	@doc:
//		Copy part info entry into given memory pool
//
//---------------------------------------------------------------------------
gpos::Ref<CPartInfo::CPartInfoEntry>
CPartInfo::CPartInfoEntry::PpartinfoentryCopy(CMemoryPool *mp) const
{
	gpos::Ref<IMDId> mdid = MDId();
	;

	// copy part keys
	gpos::Ref<CPartKeysArray> pdrgppartkeysCopy =
		CPartKeys::PdrgppartkeysCopy(mp, Pdrgppartkeys());

	// copy part constraint using empty remapping to get exact copy
	gpos::Ref<UlongToColRefMap> colref_mapping =
		GPOS_NEW(mp) UlongToColRefMap(mp);
	;

	return GPOS_NEW(mp)
		CPartInfoEntry(ScanId(), std::move(mdid), std::move(pdrgppartkeysCopy));
}


//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::CPartInfo
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPartInfo::CPartInfo(gpos::Ref<CPartInfoEntryArray> pdrgppartentries)
	: m_pdrgppartentries(std::move(pdrgppartentries))
{
	GPOS_ASSERT(nullptr != m_pdrgppartentries);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::CPartInfo
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPartInfo::CPartInfo(CMemoryPool *mp)
{
	m_pdrgppartentries = GPOS_NEW(mp) CPartInfoEntryArray(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::~CPartInfo
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPartInfo::~CPartInfo()
{
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::AddPartConsumer
//
//	@doc:
//		Add part table consumer
//
//---------------------------------------------------------------------------
void
CPartInfo::AddPartConsumer(CMemoryPool *mp, ULONG scan_id,
						   gpos::Ref<IMDId> mdid,
						   gpos::Ref<CColRef2dArray> pdrgpdrgpcrPart)
{
	gpos::Ref<CPartKeysArray> pdrgppartkeys = GPOS_NEW(mp) CPartKeysArray(mp);
	pdrgppartkeys->Append(GPOS_NEW(mp) CPartKeys(std::move(pdrgpdrgpcrPart)));

	m_pdrgppartentries->Append(GPOS_NEW(mp) CPartInfoEntry(
		scan_id, std::move(mdid), std::move(pdrgppartkeys)));
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::FContainsScanId
//
//	@doc:
//		Check if part info contains given scan id
//
//---------------------------------------------------------------------------
BOOL
CPartInfo::FContainsScanId(ULONG scan_id) const
{
	const ULONG size = m_pdrgppartentries->Size();

	for (ULONG ul = 0; ul < size; ul++)
	{
		CPartInfoEntry *ppartinfoentry = (*m_pdrgppartentries)[ul].get();
		if (scan_id == ppartinfoentry->ScanId())
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::ScanId
//
//	@doc:
//		Return scan id of the entry at the given position
//
//---------------------------------------------------------------------------
ULONG
CPartInfo::ScanId(ULONG ulPos) const
{
	return (*m_pdrgppartentries)[ulPos]->ScanId();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::GetRelMdId
//
//	@doc:
//		Return relation mdid of the entry at the given position
//
//---------------------------------------------------------------------------
IMDId *
CPartInfo::GetRelMdId(ULONG ulPos) const
{
	return (*m_pdrgppartentries)[ulPos]->MDId();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::Pdrgppartkeys
//
//	@doc:
//		Return part keys of the entry at the given position
//
//---------------------------------------------------------------------------
CPartKeysArray *
CPartInfo::Pdrgppartkeys(ULONG ulPos) const
{
	return (*m_pdrgppartentries)[ulPos]->Pdrgppartkeys();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::PdrgppartkeysByScanId
//
//	@doc:
//		Return part keys of the entry with the given scan id
//
//---------------------------------------------------------------------------
CPartKeysArray *
CPartInfo::PdrgppartkeysByScanId(ULONG scan_id) const
{
	const ULONG size = m_pdrgppartentries->Size();

	for (ULONG ul = 0; ul < size; ul++)
	{
		CPartInfoEntry *ppartinfoentry = (*m_pdrgppartentries)[ul].get();
		if (scan_id == ppartinfoentry->ScanId())
		{
			return ppartinfoentry->Pdrgppartkeys();
		}
	}

	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::PpartinfoWithRemappedKeys
//
//	@doc:
//		Return a new part info object with an additional set of remapped keys
//
//---------------------------------------------------------------------------
gpos::Ref<CPartInfo>
CPartInfo::PpartinfoWithRemappedKeys(CMemoryPool *mp, CColRefArray *pdrgpcrSrc,
									 CColRefArray *pdrgpcrDest) const
{
	GPOS_ASSERT(nullptr != pdrgpcrSrc);
	GPOS_ASSERT(nullptr != pdrgpcrDest);

	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp, pdrgpcrSrc);
	gpos::Ref<UlongToColRefMap> colref_mapping =
		CUtils::PhmulcrMapping(mp, pdrgpcrSrc, pdrgpcrDest);

	gpos::Ref<CPartInfoEntryArray> pdrgppartentries =
		GPOS_NEW(mp) CPartInfoEntryArray(mp);

	const ULONG size = m_pdrgppartentries->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		CPartInfoEntry *ppartinfoentry = (*m_pdrgppartentries)[ul].get();

		// if this entry has keys that overlap with the source columns then
		// add another set of keys to it using the destination columns
		gpos::Ref<CPartInfoEntry> ppartinfoentryNew =
			ppartinfoentry->PpartinfoentryAddRemappedKeys(mp, pcrs.get(),
														  colref_mapping);
		pdrgppartentries->Append(ppartinfoentryNew);
	}

	;
	;

	return GPOS_NEW(mp) CPartInfo(std::move(pdrgppartentries));
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::PpartinfoCombine
//
//	@doc:
//		Combine two part info objects
//
//---------------------------------------------------------------------------
gpos::Ref<CPartInfo>
CPartInfo::PpartinfoCombine(CMemoryPool *mp, CPartInfo *ppartinfoFst,
							CPartInfo *ppartinfoSnd)
{
	GPOS_ASSERT(nullptr != ppartinfoFst);
	GPOS_ASSERT(nullptr != ppartinfoSnd);

	gpos::Ref<CPartInfoEntryArray> pdrgppartentries =
		GPOS_NEW(mp) CPartInfoEntryArray(mp);

	// copy part entries from first part info object
	CUtils::AddRefAppend(pdrgppartentries.get(),
						 ppartinfoFst->m_pdrgppartentries.get());

	// copy part entries from second part info object, except those which already exist
	const ULONG length = ppartinfoSnd->m_pdrgppartentries->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		CPartInfoEntry *ppartinfoentry =
			(*(ppartinfoSnd->m_pdrgppartentries))[ul].get();
		CPartKeysArray *pdrgppartkeys =
			ppartinfoFst->PdrgppartkeysByScanId(ppartinfoentry->ScanId());

		if (nullptr != pdrgppartkeys)
		{
			// there is already an entry with the same scan id; need to add to it
			// the keys from the current entry
			gpos::Ref<CPartKeysArray> pdrgppartkeysCopy =
				CPartKeys::PdrgppartkeysCopy(mp,
											 ppartinfoentry->Pdrgppartkeys());
			CUtils::AddRefAppend(pdrgppartkeys, pdrgppartkeysCopy.get());
			;
		}
		else
		{
			gpos::Ref<CPartInfoEntry> ppartinfoentryCopy =
				ppartinfoentry->PpartinfoentryCopy(mp);
			pdrgppartentries->Append(ppartinfoentryCopy);
		}
	}

	return GPOS_NEW(mp) CPartInfo(std::move(pdrgppartentries));
}

//---------------------------------------------------------------------------
//	@function:
//		CPartInfo::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CPartInfo::OsPrint(IOstream &os) const
{
	const ULONG length = m_pdrgppartentries->Size();
	os << "Part Consumers: ";
	for (ULONG ul = 0; ul < length; ul++)
	{
		CPartInfoEntry *ppartinfoentry = (*m_pdrgppartentries)[ul].get();
		ppartinfoentry->OsPrint(os);

		// separator
		os << (ul == length - 1 ? "" : ", ");
	}

	os << ", Part Keys: ";
	for (ULONG ulCons = 0; ulCons < length; ulCons++)
	{
		CPartKeysArray *pdrgppartkeys = Pdrgppartkeys(ulCons);
		os << "(";
		const ULONG ulPartKeys = pdrgppartkeys->Size();
		;
		for (ULONG ulPartKey = 0; ulPartKey < ulPartKeys; ulPartKey++)
		{
			os << *(*pdrgppartkeys)[ulPartKey];
			os << (ulPartKey == ulPartKeys - 1 ? "" : ", ");
		}
		os << ")";
		os << (ulCons == length - 1 ? "" : ", ");
	}

	return os;
}

// EOF
