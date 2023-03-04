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
