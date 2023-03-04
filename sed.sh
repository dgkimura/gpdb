#git grep -l "public CRefCount" -- "*.h" -- "*.cpp" | xargs sed -ie 's/\([[:alpha:]]*\) : public CRefCount/\1 : public CRefCount<\1>/g' # only need one layer...
#git grep -l "CRefCount::SafeRelease" -- "*.h" -- "*.cpp" | xargs sed -ie 's/CRefCount::SafeRelease/SafeRelease/g'

# round 1, fight!
git grep 'public CRefCount' | awk -F ':' '{print $2}' | awk '{print $2}' > names.txt
git grep 'public CRefCount' | awk -F ':' '{print $2}' | awk '{print $2}' | xargs -I@ printf 'git grep -l "%s \*" -- "*.h" -- "*.cpp" | xargs sed -ie "s/\\b\([[:alpha:]]*::\)\\?%s \*/gpos::Ref<\\1%s> /g"\n' @ @ @ > converter.sh
git grep 'public CRefCount' | awk -F ':' '{print $2}' | awk '{print $2}' | xargs printf 'git grep -w "public %s"\n' > script.sh
# round 2, fight!
sh script.sh | awk -F ':' '{print $2}' | awk '{print $2}' > names2.txt
sh script.sh | awk -F ':' '{print $2}' | awk '{print $2}' | xargs -I@ printf 'git grep -l "%s \*" -- "*.h" -- "*.cpp" | xargs sed -ie "s/\\b\([[:alpha:]]*::\)\\?%s \*/gpos::Ref<\\1%s> /g"\n' @ @ @ > converter2.sh
sh script.sh | awk -F ':' '{print $2}' | awk '{print $2}' | xargs printf 'git grep -w "public %s"\n' > script2.sh
# round 3, fight!
sh script2.sh | awk -F ':' '{print $2}' | awk '{print $2}' > names3.txt
sh script2.sh | awk -F ':' '{print $2}' | awk '{print $2}' | xargs -I@ printf 'git grep -l "%s \*" -- "*.h" -- "*.cpp" | xargs sed -ie "s/\\b\([[:alpha:]]*::\)\\?%s \*/gpos::Ref<\\1%s> /g"\n' @ @ @ > converter3.sh
sh script2.sh | awk -F ':' '{print $2}' | awk '{print $2}' | xargs printf 'git grep -w "public %s"\n' > script3.sh
# round 4, fight!
sh script3.sh | awk -F ':' '{print $2}' | awk '{print $2}' > names4.txt
sh script3.sh | awk -F ':' '{print $2}' | awk '{print $2}' | xargs -I@ printf 'git grep -l "%s \*" -- "*.h" -- "*.cpp" | xargs sed -ie "s/\\b\([[:alpha:]]*::\)\\?%s \*/gpos::Ref<\\1%s> /g"\n' @ @ @ > converter4.sh
sh script3.sh | awk -F ':' '{print $2}' | awk '{print $2}' | xargs printf 'git grep -w "public %s"\n' > script4.sh
# round 5, fight!
sh script4.sh | awk -F ':' '{print $2}' | awk '{print $2}' > names5.txt
sh script4.sh | awk -F ':' '{print $2}' | awk '{print $2}' | xargs -I@ printf 'git grep -l "%s \*" -- "*.h" -- "*.cpp" | xargs sed -ie "s/\\b\([[:alpha:]]*::\)\\?%s \*/gpos::Ref<\\1%s> /g"\n' @ @ @ > converter5.sh
sh script4.sh | awk -F ':' '{print $2}' | awk '{print $2}' | xargs printf 'git grep -w "public %s"\n' > script5.sh
# round 6, fight!
sh script5.sh | awk -F ':' '{print $2}' | awk '{print $2}' > names6.txt
sh script5.sh | awk -F ':' '{print $2}' | awk '{print $2}' | xargs -I@ printf 'git grep -l "%s \*" -- "*.h" -- "*.cpp" | xargs sed -ie "s/\\b\([[:alpha:]]*::\)\\?%s \*/gpos::Ref<\\1%s> /g"\n' @ @ @ > converter6.sh
sh script5.sh | awk -F ':' '{print $2}' | awk '{print $2}' | xargs printf 'git grep -w "public %s"\n' > script6.sh
# round 7, fight!
sh script6.sh | awk -F ':' '{print $2}' | awk '{print $2}'
# finish 'em!

find . -name *.he | xargs rm
find . -name *.cppe | xargs rm

sh converter.sh
sh converter2.sh
sh converter3.sh
sh converter4.sh
sh converter5.sh
sh converter6.sh

CLANG_FORMAT=clang-format-11 ./src/tools/fmt fmt
git add -u
git commit -m "[auto convert] CRefCount * to gpos::Ref"

# post conversion fixups...
git grep -l "dynamic_cast<.*gpos::Ref<" -- "*.h" -- "*.cpp" | xargs sed -ie 's/dynamic_cast<\(.*\)[[:space:]]*gpos::Ref<\(.*\)[[:space:]]*>/gpos::dyn_cast<\2/g'
git grep -l 'AddRef()' -- '*.cpp' -- '*.h' -- ':!*CDynamicPtrArray.h' -- ':!*CRefCount.h' -- ':!*Ref.h' | xargs sed -ie 's/.*AddRef();//g'
git grep -l 'Release()' -- '*.cpp' -- '*.h' -- ':!*CDynamicPtrArray.h' -- ':!*CRefCount.h' -- ':!*Ref.h' | xargs sed -ie 's/.*Release();//g'
git grep -l "SafeRelease" -- "*.h" -- "*.cpp" | xargs sed -ie 's/SafeRelease.*;//g'
git grep -l 'GPOS_ASSERT(FPromising' -- "*.h" -- "*.cpp" | xargs sed -ie 's/GPOS_ASSERT(FPromising\(.*\), this/GPOS_ASSERT(FPromising\1, SharedFromThis()/'

ls src/backend/gporca/libgpdbcost/src/CCostModelGPDB.cpp | xargs sed -ie 's/\(return .*\)this/\1gpos::dyn_cast<CCostModelGPDB>(SharedFromThis())/'
ls src/backend/gporca/libgpdbcost/src/CCostModelGPDB.cpp | xargs sed -ie 's/\(#include <limits>\)/\1\n#include "gpos\/common\/Casting\.h"/g'
ls src/backend/gporca/libnaucrates/include/naucrates/dxl/operators/CDXLScalarConstValue.h | xargs sed -ie 's/\(#include \"gpos\/base\.h"\)/\1\n#include "gpos\/common\/Casting\.h"/g'
ls src/backend/gporca/libgpopt/include/gpopt/search/CGroupExpression.h | xargs sed -ie 's/this/SharedFromThis()/'
ls src/backend/gporca/libnaucrates/include/naucrates/base/IDatum.h | xargs sed -ie 's/(this);/(SharedFromThis());/'
ls src/backend/gporca/libgpopt/include/gpopt/base/COptimizationContext.h | xargs sed -ie 's/(this);/(SharedFromThis());/'
git grep -l 'CDynamicPtrArray<.*CleanupRelease>' -- '*.cpp' -- '*.h' | xargs sed -ie 's/CDynamicPtrArray<\(.*\), CleanupRelease>/gpos::Vector<gpos::Ref<\1>>/g'
git grep -l 'CDynamicPtrArray<CColRef.*, CleanupNULL>' -- '*.cpp' -- '*.h' | xargs sed -ie 's/CDynamicPtrArray<\(.*\), CleanupNULL>/gpos::Vector<gpos::Ref<\1>>/g'
ls src/backend/gporca/libgpopt/include/gpopt/mdcache/CMDAccessor.h | xargs sed -ie 's/dynamic_cast<const T \*>/gpos::dyn_cast<T>/'
ls src/backend/gporca/libgpopt/include/gpopt/mdcache/CMDAccessor.h | xargs sed -ie 's/const T \*/const gpos::Ref<T>/'
git grep -l "dyn_cast" src/backend/gporca/libnaucrates/include/naucrates/dxl/operators | xargs sed -ie 's/\(^namespace gpdxl\)/#include "gpos\/common\/Casting\.h"\n\1/g'
git grep -l "((gpos::Ref<"  -- "*.h" -- "*.cpp" | xargs sed -ie 's/((gpos::Ref<\([[:alpha:]]*\)>)/gpos::dyn_cast<\1>(/g'
git grep -l 'CConstraintArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/CConstraintArray \*/gpos::Ref<CConstraintArray> /g'
git grep -l 'CExpressionArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/CExpressionArray \*/gpos::Ref<CExpressionArray> /g'
git grep -l 'CFunctionalDependencyArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/CFunctionalDependencyArray \*/gpos::Ref<CFunctionalDependencyArray> /g'
git grep -l 'CPartInfoEntryArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/CPartInfoEntryArray \*/gpos::Ref<CPartInfoEntryArray> /g'
git grep -l 'CPartKeysArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/CPartKeysArray \*/gpos::Ref<CPartKeysArray> /g'
git grep -l 'CColRefArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/CColRefArray \*/gpos::Ref<CColRefArray> /g'
git grep -l 'CGroupArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/CGroupArray \*/gpos::Ref<CGroupArray> /g'
git grep -l 'IStatisticsArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/IStatisticsArray \*/gpos::Ref<IStatisticsArray> /g'
git grep -l 'CExpressionArrays \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/CExpressionArrays \*/gpos::Ref<CExpressionArrays> /g'
git grep -l 'ColRefToUlongMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/ColRefToUlongMap \*/gpos::Ref<ColRefToUlongMap> /g'
git grep -l 'IDatumArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/IDatumArray \*/gpos::Ref<IDatumArray> /g'
git grep -l 'UlongToIDatumMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/UlongToIDatumMap \*/gpos::Ref<UlongToIDatumMap> /g'
git grep -l 'CDXLDatumArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/CDXLDatumArray \*/gpos::Ref<CDXLDatumArray> /g'
git grep -l 'CColRefSetArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/CColRefSetArray \*/gpos::Ref<CColRefSetArray> /g'
git grep -l 'CColumnDescriptorArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/CColumnDescriptorArray \*/gpos::Ref<CColumnDescriptorArray> /g'
git grep -l 'IDatum2dArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/IDatum2dArray \*/gpos::Ref<IDatum2dArray> /g'
git grep -l 'UlongPtrArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/UlongPtrArray \*/gpos::Ref<UlongPtrArray> /g'
git grep -l 'CBitVector \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/CBitVector \*/gpos::Ref<CBitVector> /g'
git grep -l 'CBitSetLink \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CBitSetLink \*/gpos::Ref<\1CBitSetLink> /g'
git grep -l 'ExprToConstantMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?ExprToConstantMap \*/gpos::Ref<\1ExprToConstantMap> /g'
ls src/backend/gporca/libgpopt/src/operators/CExpression.cpp | xargs sed -ie 's/Attach(this)/Attach(SharedFromThis())/g'
git grep -l 'CRangeArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CRangeArray \*/gpos::Ref<\1CRangeArray> /g'
git grep -l 'ColRefToConstraintArrayMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?ColRefToConstraintArrayMap \*/gpos::Ref<\1ColRefToConstraintArrayMap> /g'
git grep -l 'IntToColRefMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?IntToColRefMap \*/gpos::Ref<\1IntToColRefMap> /g'
git grep -l 'UlongToColRefMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?UlongToColRefMap \*/gpos::Ref<\1UlongToColRefMap> /g'
git grep -l 'UlongToHistogramMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?UlongToHistogramMap \*/gpos::Ref<\1UlongToHistogramMap> /g'
git grep -l 'UlongToHistogramMapIter ' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\(UlongToHistogramMapIter .*(.*\));/\1.get());/g'
git grep -l 'UlongToDoubleMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?UlongToDoubleMap \*/gpos::Ref<\1UlongToDoubleMap> /g'
git grep -l 'UlongToDoubleMapIter ' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\(UlongToDoubleMapIter .*(.*\));/\1.get());/g'
git grep -l 'UlongToIntMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?UlongToIntMap \*/gpos::Ref<\1UlongToIntMap> /g'
git grep -l 'CUpperBoundNDVPtrArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CUpperBoundNDVPtrArray \*/gpos::Ref<\1CUpperBoundNDVPtrArray> /g'
git grep -l 'UlongToConsumerCounterMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?UlongToConsumerCounterMap \*/gpos::Ref<\1UlongToConsumerCounterMap> /g'
git grep -l 'CDistributionSpecArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CDistributionSpecArray \*/gpos::Ref<\1CDistributionSpecArray> /g'
git grep -l 'ULongPtrArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?ULongPtrArray \*/gpos::Ref<\1ULongPtrArray> /g'
git grep -l 'CStatsPredJoinArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CStatsPredJoinArray \*/gpos::Ref<\1CStatsPredJoinArray> /g'
git grep -l 'SJoinConditionArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?SJoinConditionArray \*/gpos::Ref<\1SJoinConditionArray> /g'
git grep -l 'CBucketArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CBucketArray \*/gpos::Ref<\1CBucketArray> /g'
git grep -l 'SGroupInfoArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?SGroupInfoArray \*/gpos::Ref<\1SGroupInfoArray> /g'
git grep -l 'CPointArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CPointArray \*/gpos::Ref<\1CPointArray> /g'
git grep -l 'IMdIdArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?IMdIdArray \*/gpos::Ref<\1IMdIdArray> /g'
git grep -l 'CMDColumnArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CMDColumnArray \*/gpos::Ref<\1CMDColumnArray> /g'
git grep -l 'ColRefToUlongMapArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?ColRefToUlongMapArray \*/gpos::Ref<\1ColRefToUlongMapArray> /g'
git grep -l 'CColRef2dArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CColRef2dArray \*/gpos::Ref<\1CColRef2dArray> /g'
git grep -l 'ExpressionToEdgeMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?ExpressionToEdgeMap \*/gpos::Ref<\1ExpressionToEdgeMap> /g'
git grep -l 'BitSetToGroupInfoMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?BitSetToGroupInfoMap \*/gpos::Ref<\1BitSetToGroupInfoMap> /g'
git grep -l 'CBitSetArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CBitSetArray \*/gpos::Ref<\1CBitSetArray> /g'
git grep -l 'CKHeap<SExpressionInfoArray, SExpressionInfo> \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CKHeap<SExpressionInfoArray, SExpressionInfo> \*/gpos::Ref<\1CKHeap<SExpressionInfoArray, SExpressionInfo>> /g'
git grep -l 'CStatsPredPtrArry \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CStatsPredPtrArry \*/gpos::Ref<\1CStatsPredPtrArry> /g'
git grep -l 'CDoubleArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CDoubleArray \*/gpos::Ref<\1CDoubleArray> /g'
git grep -l 'UlongToUlongPtrArrayMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?UlongToUlongPtrArrayMap \*/gpos::Ref<UlongToUlongPtrArrayMap> /g'
git grep -l 'CConstraintIntervalArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CConstraintIntervalArray \*/gpos::Ref<\1CConstraintIntervalArray> /g'
git grep -l 'CScalarConstArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CScalarConstArray \*/gpos::Ref<\1CScalarConstArray> /g'
git grep -l 'IntPtrArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?IntPtrArray \*/gpos::Ref<\1IntPtrArray> /g'
git grep -l 'ColRefToColRefSetMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?ColRefToColRefSetMap \*/gpos::Ref<\1ColRefToColRefSetMap> /g'
git grep -l 'ColRefToColRefMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?ColRefToColRefMap \*/gpos::Ref<\1ColRefToColRefMap> /g'
git grep -l 'CXformSet \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CXformSet \*/gpos::Ref<\1CXformSet> /g'
git grep -l 'UlongToSPartPropSpecInfoMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?UlongToSPartPropSpecInfoMap \*/gpos::Ref<\1UlongToSPartPropSpecInfoMap> /g'
git grep -l 'UlongToCTEReqEntryMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?UlongToCTEReqEntryMap \*/gpos::Ref<\1UlongToCTEReqEntryMap> /g'
git grep -l 'UlongToCTEMapEntryMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?UlongToCTEMapEntryMap \*/gpos::Ref<\1UlongToCTEMapEntryMap> /g'
git grep -l 'EsetStates \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?EsetStates \*/gpos::Ref<\1EsetStates> /g'
git grep -l 'EsetEvents \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?EsetEvents \*/gpos::Ref<\1EsetEvents> /g'
git grep -l 'CCostArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CCostArray \*/gpos::Ref<\1CCostArray> /g'
git grep -l 'UlongToDrvdPropPlanMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?UlongToDrvdPropPlanMap \*/gpos::Ref<\1UlongToDrvdPropPlanMap> /g'
git grep -l 'COptimizationContextArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?COptimizationContextArray \*/gpos::Ref<\1COptimizationContextArray> /g'
git grep -l 'CDrvdPropArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CDrvdPropArray \*/gpos::Ref<\1CDrvdPropArray> /g'
git grep -l 'ExprHashSet \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?ExprHashSet \*/gpos::Ref<\1ExprHashSet> /g'
git grep -l 'CMDNameArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CMDNameArray \*/gpos::Ref<\1CMDNameArray> /g'
git grep -l 'CDynamicPtrArray<T, CleanupNULL> \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CDynamicPtrArray<T, CleanupNULL> \*/gpos::Ref<\1CDynamicPtrArray<T, CleanupNULL>> /g'
git grep -l 'LinkMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?LinkMap \*/gpos::Ref<\1LinkMap> /g'
git grep -l 'OptCtxtToIStatisticsMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?OptCtxtToIStatisticsMap \*/gpos::Ref<\1OptCtxtToIStatisticsMap> /g'
git grep -l 'ReqdPropPlanToCostMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?ReqdPropPlanToCostMap \*/gpos::Ref<\1ReqdPropPlanToCostMap> /g'
git grep -l 'PartialPlanToCostMap \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?PartialPlanToCostMap \*/gpos::Ref<\1PartialPlanToCostMap> /g'
git grep -l 'CSearchStageArray \*' -- '*.cpp' -- '*.h' | xargs sed -ie 's/\b\([[:alpha:]]*::\)\?CSearchStageArray \*/gpos::Ref<\1CSearchStageArray> /g'

ls src/backend/gporca/libgpopt/src/search/CGroup.cpp | xargs sed -ie 's/CGroupProxy gp(this)/CGroupProxy gp(SharedFromThis())/'

# Look at destructor release's CJoinOrderDPv2 in main branch



CLANG_FORMAT=clang-format-11 ./src/tools/fmt fmt
git add -u
git commit -m "[auto convert] Post conversion fixups"

git cherry-pick refcount-sed-exec

# Ugh Hack!
# pushd src/backend/gporca/libnaucrates/src/base
# git grep -l '#include "gpos\/base\.h"' -- "*.cpp" | xargs sed -ie 's/#include "gpos\/base\.h"/#include "gpos\/base\.h"\n#include "gpopt\/search\/CGroupExpression.h"\n/g'
# popd
# git grep -l "SafeRelease(" -- "*.h" -- "*.cpp" | xargs sed -ie 's/SafeRelease(\([[:alpha:]_]*\))/SafeRelease(\1.get())/g'
# git grep -l 'using namespace gpopt;' -- "*.cpp" | xargs sed -ie 's/using namespace gpopt;/#include "gpopt\/search\/CGroupExpression.h"\nusing namespace gpopt;\n/'

#git grep 'using .* = CDynamicPtrArray'
#git grep -l 'using .* = CDynamicPtrArray' -- "*.h" -- "*.cpp" | xargs sed -ie 's/using \(.*\) = CDynamicPtrArray<\(.*\), \(.*\)>/using \1 = gpos::Vector<gpos::Ref<\2>>/g'

# remove from test directory
# git grep -l '#include "gpopt\/search\/CGroupExpression.h"' -- "*.cpp" | xargs sed -ie 's/#include "gpopt\/search\/CGroupExpression.h"\n//g'

# [patch]
#    diff --git a/src/backend/gporca/libgpos/include/gpos/base.h b/src/backend/gporca/libgpos/include/gpos/base.h
#    index c36b215fb0f..c3c1e1ff81a 100644
#    --- a/src/backend/gporca/libgpos/include/gpos/base.h
#    +++ b/src/backend/gporca/libgpos/include/gpos/base.h
#    @@ -19,6 +19,7 @@
#     #include "gpos/error/ILogger.h"
#     #include "gpos/memory/CMemoryPool.h"
#     #include "gpos/types.h"
#    +#include "gpos/common/Casting.h"


# [patch]
#    diff --git a/src/backend/gporca/libgpos/include/gpos/common/CDynamicPtrArray.h b/src/backend/gporca/libgpos/include/gpos/common/CDynamicPtrArray.h
#    index 15d4c5f313e..54d746a9fe3 100644
#    --- a/src/backend/gporca/libgpos/include/gpos/common/CDynamicPtrArray.h
#    +++ b/src/backend/gporca/libgpos/include/gpos/common/CDynamicPtrArray.h
#    @@ -12,6 +12,7 @@
#     #define GPOS_CDynamicPtrArray_H
#
#     #include "gpos/base.h"
#    +#include "gpos/common/Ref.h"
#     #include "gpos/common/CRefCount.h"
#     #include "gpos/common/clibwrapper.h"
#
#    @@ -187,6 +188,11 @@ public:
#
#            // append element to end of array
#            void
#    +       Append(gpos::Ref<T> elem)
#    +       {
#    +               Append(elem.get());
#    +       }
#    +       void
#            Append(T *elem)
#            {
#                    if (m_size == m_capacity)

#victory!
