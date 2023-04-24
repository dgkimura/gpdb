//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CPartInfo.h
//
//	@doc:
//		Derived partition information at the logical level
//---------------------------------------------------------------------------
#ifndef GPOPT_CPartInfo_H
#define GPOPT_CPartInfo_H

#include "gpos/base.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CPartKeys.h"

// fwd decl
namespace gpmd
{
class IMDId;
}

namespace gpopt
{
using namespace gpos;
using namespace gpmd;

// fwd decl
class CPartConstraint;

//---------------------------------------------------------------------------
//	@class:
//		CPartInfo
//
//	@doc:
//		Derived partition information at the logical level
//
//---------------------------------------------------------------------------
class CPartInfo : public CRefCount<CPartInfo>, public DbgPrintMixin<CPartInfo>
{
private:
	//---------------------------------------------------------------------------
	//	@class:
	//		CPartInfoEntry
	//
	//	@doc:
	//		A single entry of the CPartInfo
	//
	//---------------------------------------------------------------------------
	class CPartInfoEntry : public CRefCount<CPartInfoEntry>,
						   public DbgPrintMixin<CPartInfoEntry>
	{
	private:
		// scan id
		ULONG m_scan_id;

		// partition table mdid
		gpos::Ref<IMDId> m_mdid;

		// partition keys
		gpos::Ref<CPartKeysArray> m_pdrgppartkeys;

	public:
		CPartInfoEntry(const CPartInfoEntry &) = delete;

		// ctor
		CPartInfoEntry(ULONG scan_id, gpos::Ref<IMDId> mdid,
					   gpos::Ref<CPartKeysArray> pdrgppartkeys);

		// dtor
		~CPartInfoEntry() override;

		// scan id
		virtual ULONG
		ScanId() const
		{
			return m_scan_id;
		}

		// create a copy of the current object, and add a set of remapped
		// part keys to this entry, using the existing keys and the given hashmap
		gpos::Ref<CPartInfoEntry> PpartinfoentryAddRemappedKeys(
			CMemoryPool *mp, gpos::Ref<CColRefSet> pcrs,
			gpos::Ref<UlongToColRefMap> colref_mapping);

		// mdid of partition table
		virtual gpos::Ref<IMDId>
		MDId() const
		{
			return m_mdid;
		}

		// partition keys of partition table
		virtual gpos::Ref<CPartKeysArray>
		Pdrgppartkeys() const
		{
			return m_pdrgppartkeys;
		}

		// print function
		IOstream &OsPrint(IOstream &os) const;

		// copy part info entry into given memory pool
		gpos::Ref<CPartInfoEntry> PpartinfoentryCopy(CMemoryPool *mp) const;

	};	// CPartInfoEntry

	using CPartInfoEntryArray = gpos::Vector<gpos::Ref<CPartInfoEntry>>;

	// partition table consumers
	gpos::Ref<CPartInfoEntryArray> m_pdrgppartentries;

	// private ctor
	explicit CPartInfo(gpos::Ref<CPartInfoEntryArray> pdrgppartentries);

public:
	CPartInfo(const CPartInfo &) = delete;

	// ctor
	explicit CPartInfo(CMemoryPool *mp);

	// dtor
	~CPartInfo() override;

	// number of part table consumers
	ULONG
	UlConsumers() const
	{
		return m_pdrgppartentries->Size();
	}

	// add part table consumer
	void AddPartConsumer(CMemoryPool *mp, ULONG scan_id, gpos::Ref<IMDId> mdid,
						 gpos::Ref<CColRef2dArray> pdrgpdrgpcrPart);

	// scan id of the entry at the given position
	ULONG ScanId(ULONG ulPos) const;

	// relation mdid of the entry at the given position
	gpos::Ref<IMDId> GetRelMdId(ULONG ulPos) const;

	// part keys of the entry at the given position
	gpos::Ref<CPartKeysArray> Pdrgppartkeys(ULONG ulPos) const;

	// check if part info contains given scan id
	BOOL FContainsScanId(ULONG scan_id) const;

	// part keys of the entry with the given scan id
	gpos::Ref<CPartKeysArray> PdrgppartkeysByScanId(ULONG scan_id) const;

	// return a new part info object with an additional set of remapped keys
	gpos::Ref<CPartInfo> PpartinfoWithRemappedKeys(
		CMemoryPool *mp, gpos::Ref<CColRefArray> pdrgpcrSrc,
		gpos::Ref<CColRefArray> pdrgpcrDest) const;

	// print
	IOstream &OsPrint(IOstream &) const;

	// combine two part info objects
	static gpos::Ref<CPartInfo> PpartinfoCombine(
		CMemoryPool *mp, gpos::Ref<CPartInfo> ppartinfoFst,
		gpos::Ref<CPartInfo> ppartinfoSnd);

};	// CPartInfo

// shorthand for printing
inline IOstream &
operator<<(IOstream &os, CPartInfo &partinfo)
{
	return partinfo.OsPrint(os);
}
}  // namespace gpopt

#endif	// !GPOPT_CPartInfo_H

// EOF
