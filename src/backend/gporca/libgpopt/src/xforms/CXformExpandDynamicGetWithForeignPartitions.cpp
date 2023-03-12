//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2023 VMware Inc.
//
//	@filename:
//		CXformExpandDynamicGetWithForeignPartitions.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformExpandDynamicGetWithForeignPartitions.h"

#include "gpos/base.h"

#include "gpopt/exception.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CLogicalDynamicForeignGet.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/xforms/CXformUtils.h"
#include "naucrates/md/CMDRelationGPDB.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformExpandDynamicGetWithForeignPartitions::CXformExpandDynamicGetWithForeignPartitions
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformExpandDynamicGetWithForeignPartitions::
	CXformExpandDynamicGetWithForeignPartitions(CMemoryPool *mp)
	: CXformExploration(
		  // pattern
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalDynamicGet(mp)))
{
}

CXform::EXformPromise
CXformExpandDynamicGetWithForeignPartitions::Exfp(
	CExpressionHandle &exprhdl) const
{
	gpos::Ref<CLogicalDynamicGet> popGet =
		CLogicalDynamicGet::PopConvert(exprhdl.Pop());
	if (popGet->ContainsForeignParts())
	{
		return CXform::ExfpHigh;
	}

	// No need to run this xform if the relation being scanned does not
	// contain foreign partitions
	return CXform::ExfpNone;
}


// Expands a dynamic get with foreign partitions into CLogicalDynamicForeignGet(s) and a CLogicalDynamicGet
// This xform separates the DynamicGet into non-foreign partitions and foreign partitions grouped by the
// foreign server. If there are only foreign scans of a single server OID, this xform will produce only 1
// CLogicalDynamicForeignGet. However, if there are multiple servers, it will produce a UNION of these
// CLogicalDynamicForeignGets. Additionally, if there are any non-foreign partitions, these will also be
// in a CLogicalDynamicGet that is in this UNION.

// The physical plan created after this xform and the corresponding logical->physical xforms will create the following from
// Dynamic get containing both foreign and non-foreign tables.
//    +--CPhysicalSerialUnionAll ]
//       |--CPhysicalDynamicTableScan "part" ("part"), Columns: ["a" (9), "b" (10), Scan Id: 1 Parts to scan: 5
//       +--CPhysicalDynamicForeignScan "part" ("part"), Columns: ["a" (18), "b" (19)] Scan Id: 1 Parts to scan: 3]

void
CXformExpandDynamicGetWithForeignPartitions::Transform(
	gpos::Ref<CXformContext> pxfctxt, gpos::Ref<CXformResult> pxfres,
	gpos::Ref<CExpression> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), SharedFromThis(), pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));
	GPOS_ASSERT(nullptr != pxfres);

	CMemoryPool *mp = pxfctxt->Pmp();
	gpos::Ref<CLogicalDynamicGet> popGet =
		CLogicalDynamicGet::PopConvert(pexpr->Pop());
	// only run this xform if it contains foreign partitions
	GPOS_ASSERT(popGet->ContainsForeignParts());

	gpos::Ref<IMdIdArray> foreign_server_mdids = popGet->ForeignServerMdIds();
	gpos::Ref<IMdIdArray> all_part_mdids = popGet->GetPartitionMdids();

	gpos::Ref<IMdIdArray> non_foreign_parts = GPOS_NEW(mp) IMdIdArray(mp);
	// create map from server-> (array of part mdids)
	SForeignServerToIMdIdArrayMap *foreign_server_to_part_oid_array_map =
		GPOS_NEW(mp) SForeignServerToIMdIdArrayMap(mp);

	// iterate over all partitions. If it is not foreign, place in non_foreign_parts array,
	// otherwise place in foreign server map
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	for (ULONG ul = 0; ul < all_part_mdids->Size(); ++ul)
	{
		gpos::Ref<IMDId> foreign_server_mdid = (*foreign_server_mdids)[ul];

		gpos::Ref<IMDId> partMdid = (*all_part_mdids)[ul];

		// if partition is not foreign, add to non foreign array
		if (!foreign_server_mdid->IsValid())
		{
			non_foreign_parts->Append(partMdid);
		}
		else
		{
			// this is a foreign partition. However, we need to separate it by the foreign server mdid,
			// as each server can have a different distribution derivation
			// (some foreign tables can only be executed on segments, others only the coordinator)
			// place these in a map from server->array of foreign partitions
			const gpos::Ref<IMDRelation> pmdrel =
				md_accessor->RetrieveRel(partMdid);
			BOOL is_master_only = (gpmd::CMDRelationGPDB::EreldistrMasterOnly ==
								   pmdrel->GetRelDistribution());

			OID foreign_server_oid =
				CMDIdGPDB::CastMdid(foreign_server_mdid)->Oid();
			SForeignServer foreign_server_lookup = {foreign_server_oid,
													is_master_only};
			const gpos::Ref<IMdIdArray> foreign_server =
				foreign_server_to_part_oid_array_map->Find(
					&foreign_server_lookup);
			if (nullptr == foreign_server)
			{
				// create array for foreign server and insert
				gpos::Ref<IMdIdArray> part_oid_array =
					GPOS_NEW(mp) IMdIdArray(mp);
				part_oid_array->Append(partMdid);
				BOOL fres GPOS_ASSERTS_ONLY =
					foreign_server_to_part_oid_array_map->Insert(
						GPOS_NEW(mp)
							SForeignServer{foreign_server_oid, is_master_only},
						part_oid_array);
				GPOS_ASSERT(fres);
			}
			else
			{
				// array for foreign server already exists in map, just append
				foreign_server->Append(partMdid);
			}
		}
	}

	// By this point we have an array of non-foreign parts and a map from foreign_server->(arry of parts)
	// Now we can create the DynamicGet operators and union them if necessary.
	// We need a union if there is any non-foreign part, or multiple different servers
	gpos::Ref<CExpressionArray> expressionsForUnion =
		GPOS_NEW(mp) CExpressionArray(mp);
	gpos::Ref<CColRef2dArray> inputColArrayForUnion =
		GPOS_NEW(mp) CColRef2dArray(mp);

	// Create a regular dynamic from the non-foreign partitions and add to union all expression array
	ULONG non_foreign_parts_size = non_foreign_parts->Size();
	if (non_foreign_parts_size > 0)
	{
		CName *new_alias = GPOS_NEW(mp) CName(mp, popGet->Name());
		// This will be null if no static partitioning was performed
		if (popGet->GetPartitionConstraintsDisj())
		{
		}
		gpos::Ref<CColRefArray> pdrgpcrNew =
			CUtils::PdrgpcrCopy(mp, popGet->PdrgpcrOutput());
		gpos::Ref<CLogicalDynamicGet> nonForeignDynamicGet =
			GPOS_NEW(mp) CLogicalDynamicGet(
				mp, new_alias, popGet->Ptabdesc(), popGet->ScanId(), pdrgpcrNew,
				popGet->PdrgpdrgpcrPart(), non_foreign_parts,
				popGet->GetPartitionConstraintsDisj(), popGet->FStaticPruned(),
				GPOS_NEW(mp) IMdIdArray(mp) /* foreign_server_mdids */);
		gpos::Ref<CExpression> pexprNonForeignDynamicGet =
			GPOS_NEW(mp) CExpression(mp, nonForeignDynamicGet);

		// addref for use in union all

		// add to union all arrays
		inputColArrayForUnion->Append(nonForeignDynamicGet->PdrgpcrOutput());
		expressionsForUnion->Append(pexprNonForeignDynamicGet);
	}
	else
	{
	}

	// loop over each key in the map, create a DynamicForeignGet for each
	// foreign server using the mdid array
	SForeignServerToIMdIdArrayMapIter map_iter(
		foreign_server_to_part_oid_array_map);
	BOOL no_union_all = foreign_server_to_part_oid_array_map->Size() == 1 &&
						non_foreign_parts_size == 0;
	while (map_iter.Advance())
	{
		gpos::Ref<SForeignServer> foreign_server = map_iter.Key();
		gpos::Ref<IMdIdArray> part_oid_array = map_iter.Value();



		CName *new_alias = GPOS_NEW(mp) CName(mp, popGet->Name());

		gpos::Ref<CColRefArray> pdrgpcrNew;
		// Only generate new colrefs if we're creating a union all, otherwise we can use existing output colrefs
		if (no_union_all)
		{
			pdrgpcrNew = popGet->PdrgpcrOutput();
		}
		else
		{
			pdrgpcrNew = CUtils::PdrgpcrCopy(mp, popGet->PdrgpcrOutput());
		}
		gpos::Ref<CLogicalDynamicForeignGet> dynamicForeignGet = GPOS_NEW(mp)
			CLogicalDynamicForeignGet(mp, new_alias, popGet->Ptabdesc(),
									  popGet->ScanId(), pdrgpcrNew,
									  popGet->PdrgpdrgpcrPart(), part_oid_array,
									  foreign_server->m_foreign_server_oid,
									  foreign_server->m_is_master_only);
		gpos::Ref<CExpression> pexprDynamicForeignGet =
			GPOS_NEW(mp) CExpression(mp, dynamicForeignGet);

		if (no_union_all)
		{
			// no union needed, just return the dynamicForeignGet


			pxfres->Add(pexprDynamicForeignGet);

			return;
		}
		else
		{
			inputColArrayForUnion->Append(dynamicForeignGet->PdrgpcrOutput());
			expressionsForUnion->Append(pexprDynamicForeignGet);
		}
	}



	// Create a UNION ALL node above the gets

	gpos::Ref<CExpression> pexprResult = GPOS_NEW(mp)
		CExpression(mp,
					GPOS_NEW(mp) CLogicalUnionAll(mp, popGet->PdrgpcrOutput(),
												  inputColArrayForUnion),
					expressionsForUnion);
	// add alternative to transformation result
	pxfres->Add(pexprResult);
}


// EOF
