#ifndef __DEX_UTIL_H__
#define __DEX_UTIL_H__

#include <assert.h>

#include "Types.h"
#include "alog.h"

#define DEX_MAGIC       "dex\n"

#define DEX_MAGIC_VERS  "036\0"

#define DEX_MAGIC_VERS_API_13  "035\0"

#define DEX_OPT_MAGIC   "dey\n"
#define DEX_OPT_MAGIC_VERS  "036\0"

#define DEX_DEP_MAGIC   "deps"

#define VALID_METHOD_ID 0xffffffff

enum { kSHA1DigestLen = 20,
       kSHA1DigestOutputLen = kSHA1DigestLen * 2 + 1
     };

struct DexOptHeader {
    u1  magic[8];

    u4  dexOffset;
    u4  dexLength;
    u4  depsOffset;
    u4  depsLength;
    u4  optOffset;
    u4  optLength;

    u4  flags;
    u4  checksum;

};

struct DexClassLookup {
    int     size;
    int     numEntries;
    struct {
        u4      classDescriptorHash;
        int     classDescriptorOffset;
        int     classDefOffset;
    } table[1];
};

struct DexHeader {
    u1  magic[8];
    u4  checksum;
    u1  signature[kSHA1DigestLen];
    u4  fileSize;
    u4  headerSize;
    u4  endianTag;
    u4  linkSize;
    u4  linkOff;
    u4  mapOff;
    u4  stringIdsSize;
    u4  stringIdsOff;
    u4  typeIdsSize;
    u4  typeIdsOff;
    u4  protoIdsSize;
    u4  protoIdsOff;
    u4  fieldIdsSize;
    u4  fieldIdsOff;
    u4  methodIdsSize;
    u4  methodIdsOff;
    u4  classDefsSize;
    u4  classDefsOff;
    u4  dataSize;
    u4  dataOff;
};

enum {
    kDexTypeHeaderItem               = 0x0000,
    kDexTypeStringIdItem             = 0x0001,
    kDexTypeTypeIdItem               = 0x0002,
    kDexTypeProtoIdItem              = 0x0003,
    kDexTypeFieldIdItem              = 0x0004,
    kDexTypeMethodIdItem             = 0x0005,
    kDexTypeClassDefItem             = 0x0006,
    kDexTypeMapList                  = 0x1000,
    kDexTypeTypeList                 = 0x1001,
    kDexTypeAnnotationSetRefList     = 0x1002,
    kDexTypeAnnotationSetItem        = 0x1003,
    kDexTypeClassDataItem            = 0x2000,
    kDexTypeCodeItem                 = 0x2001,
    kDexTypeStringDataItem           = 0x2002,
    kDexTypeDebugInfoItem            = 0x2003,
    kDexTypeAnnotationItem           = 0x2004,
    kDexTypeEncodedArrayItem         = 0x2005,
    kDexTypeAnnotationsDirectoryItem = 0x2006,
};

enum {
    ACC_PUBLIC       = 0x00000001,
    ACC_PRIVATE      = 0x00000002,
    ACC_PROTECTED    = 0x00000004,
    ACC_STATIC       = 0x00000008,
    ACC_FINAL        = 0x00000010,
    ACC_SYNCHRONIZED = 0x00000020,
    ACC_SUPER        = 0x00000020,
    ACC_VOLATILE     = 0x00000040,
    ACC_BRIDGE       = 0x00000040,
    ACC_TRANSIENT    = 0x00000080,
    ACC_VARARGS      = 0x00000080,
    ACC_NATIVE       = 0x00000100,
    ACC_INTERFACE    = 0x00000200,
    ACC_ABSTRACT     = 0x00000400,
    ACC_STRICT       = 0x00000800,
    ACC_SYNTHETIC    = 0x00001000,
    ACC_ANNOTATION   = 0x00002000,
    ACC_ENUM         = 0x00004000,
    ACC_CONSTRUCTOR  = 0x00010000,
    ACC_DECLARED_SYNCHRONIZED =
        0x00020000,
    ACC_CLASS_MASK =
        (ACC_PUBLIC | ACC_FINAL | ACC_INTERFACE | ACC_ABSTRACT
         | ACC_SYNTHETIC | ACC_ANNOTATION | ACC_ENUM),
    ACC_INNER_CLASS_MASK =
        (ACC_CLASS_MASK | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC),
    ACC_FIELD_MASK =
        (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL
         | ACC_VOLATILE | ACC_TRANSIENT | ACC_SYNTHETIC | ACC_ENUM),
    ACC_METHOD_MASK =
        (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL
         | ACC_SYNCHRONIZED | ACC_BRIDGE | ACC_VARARGS | ACC_NATIVE
         | ACC_ABSTRACT | ACC_STRICT | ACC_SYNTHETIC | ACC_CONSTRUCTOR
         | ACC_DECLARED_SYNCHRONIZED),
};

struct DexMapItem {
    u2 type;
    u2 unused;
    u4 size;
    u4 offset;
};

struct DexMapList {
    u4  size;
    DexMapItem list[1];
};

struct DexStringId {
    u4 stringDataOff;
};

struct DexTypeId {
    u4  descriptorIdx;
};

struct DexFieldId {
    u2  classIdx;
    u2  typeIdx;
    u4  nameIdx;
};

struct DexMethodId {
    u2  classIdx;
    u2  protoIdx;
    u4  nameIdx;
};

struct DexProtoId {
    u4  shortyIdx;
    u4  returnTypeIdx;
    u4  parametersOff;
};

struct DexClassDef {
    u4  classIdx;
    u4  accessFlags;
    u4  superclassIdx;
    u4  interfacesOff;
    u4  sourceFileIdx;
    u4  annotationsOff;
    u4  classDataOff;
    u4  staticValuesOff;
};

struct DexCode {
    u2  registersSize;
    u2  insSize;
    u2  outsSize;
    u2  triesSize;
    u4  debugInfoOff;
    u4  insnsSize;
    u2  insns[1];

};

struct DexTry {
    u4  startAddr;
    u2  insnCount;
    u2  handlerOff;
};

struct DexLink {
    u1  bleargh;
};

struct DexTypeItem {
    u2  typeIdx;
};

struct DexTypeList {
    u4  size;
    DexTypeItem list[1];
};

struct DexAnnotationsDirectoryItem {
    u4  classAnnotationsOff;
    u4  fieldsSize;
    u4  methodsSize;
    u4  parametersSize;

};

struct DexFieldAnnotationsItem {
    u4  fieldIdx;
    u4  annotationsOff;
};

struct DexMethodAnnotationsItem {
    u4  methodIdx;
    u4  annotationsOff;
};

struct DexParameterAnnotationsItem {
    u4  methodIdx;
    u4  annotationsOff;
};

struct DexAnnotationSetRefItem {
    u4  annotationsOff;
};

struct DexAnnotationSetRefList {
    u4  size;
    DexAnnotationSetRefItem list[1];
};

struct DexAnnotationSetItem {
    u4  size;
    u4  entries[1];
};

struct DexClassDataHeader {
    u4 staticFieldsSize;
    u4 instanceFieldsSize;
    u4 directMethodsSize;
    u4 virtualMethodsSize;
};

struct DexField {
    u4 fieldIdx;
    u4 accessFlags;
};

struct DexMethod {
    u4 methodIdx;
    u4 accessFlags;
    u4 codeOff;
};

struct DexClassData {
    DexClassDataHeader header;
    DexField          *staticFields;
    DexField          *instanceFields;
    DexMethod         *directMethods;
    DexMethod         *virtualMethods;
};

struct PaddingData {
    u4 methodIdx;
    u4 protoIdx;
    u4 codeOff;
    u4 insnsSize;
    const u1 *codePtr;
};

struct DexFile {

    const DexOptHeader *pOptHeader;

    const DexHeader    *pHeader;
    const DexStringId  *pStringIds;
    const DexTypeId    *pTypeIds;
    const DexFieldId   *pFieldIds;
    const DexMethodId  *pMethodIds;
    const DexProtoId   *pProtoIds;
    const DexClassDef  *pClassDefs;
    const DexLink      *pLinkData;

    const DexClassLookup *pClassLookup;
    const void         *pRegisterMapPool;

    const u1           *baseAddr;

    int                 overhead;

};
struct DexFile_Amazon {

    const DexOptHeader *pOptHeader;

    const DexHeader    *pHeader;
    const DexStringId  *pStringIds;
    const DexTypeId    *pTypeIds;
    const DexFieldId   *pFieldIds;
    const DexMethodId  *pMethodIds;

    const DexStringId  *pStringIds2;
    const DexProtoId   *pProtoIds;
    const DexClassDef  *pClassDefs;
    const DexFieldId   *pFieldIds2;
    const DexMethodId  *pMethodIds2;
    const DexLink      *pLinkData;

    const DexClassLookup *pClassLookup;
    const void         *pRegisterMapPool;

    const u1           *baseAddr;

    u4                  fieldSize;
    u4                  methodsSize;
    u4                  stringSize;

};

typedef struct DexIndexMap {
    const u2 *classMap;
    u4  classFullCount;
    u4  classReducedCount;
    const u2 *methodMap;
    u4  methodFullCount;
    u4  methodReducedCount;
    const u2 *fieldMap;
    u4  fieldFullCount;
    u4  fieldReducedCount;
    const u2 *stringMap;
    u4  stringFullCount;
    u4  stringReducedCount;
} DexIndexMap;

struct DexFile2X {

    const DexOptHeader *pOptHeader;

    const DexHeader    *pHeader;
    const DexStringId  *pStringIds;
    const DexTypeId    *pTypeIds;
    const DexFieldId   *pFieldIds;
    const DexMethodId  *pMethodIds;
    const DexProtoId   *pProtoIds;
    const DexClassDef  *pClassDefs;
    const DexLink      *pLinkData;

    const DexClassLookup *pClassLookup;
    DexIndexMap         indexMap;
    const void         *pRegisterMapPool;

    const u1           *baseAddr;

    int                 overhead;

};

class UpdateClassDataFilter {

public:
    virtual ~UpdateClassDataFilter() {}
    virtual bool onMethod(u4 classId, u4 methodId, u4 methodIndex, DexMethod &dexMethod) = 0;
};

class DexUtil {
public:
    DexUtil(const u1 *addr);

public:
    static bool isDex(const u1 *addr);
    static bool isOptDex(const u1 *addr);
    static u4 getNativeCountInDexClassData(DexClassData *classDataItem);

public:
    const u1 *base() {
        return mAddr;
    }

    u4 fileSize() {
        return mHeader->fileSize;
    }

    u4 getTypeIdsSize() {
        return mHeader->typeIdsSize;
    }

    const DexHeader *header() {
        return mHeader;
    }

    u4 classCount() {
        return mHeader->classDefsSize;
    }

    u4 methodCount() {
        return 0;
    }

    u4 checksum() {
        return mHeader->checksum;
    }

    u4 classDescriptorHash(const char *str);

    bool hasNative();
    int findClassIndex(const char *name);
    u4 getMethodCount(u4 classIdx);
    const char *getMethodName(u4 classIdx, u4 methodIndex);

    void calcMethodHash(u4 methodIdx, char *hashStr);

    DexClassLookup *dexCreateClassLookup();
    void classLookupAdd(DexClassLookup *pLookup, int stringOff, int classDefOff, int *pNumProbes);
    void *dexFileSetupBasicPointers(u1 *data, bool is2x);
    DexClassData *dexReadAndVerifyClassData(const u1 **pData, const u1 *pLimit);

    const DexClassDef *dexGetClassDef(u4 idx) {
        const DexClassDef *classDef = (const DexClassDef *)(mAddr + mHeader->classDefsOff);
        return &classDef[idx];
    }

    const DexTypeId *dexGetTypeId(u4 idx) {
        assert(idx < mHeader->typeIdsSize);
        const DexTypeId *pTypeIds = (const DexTypeId *)(mAddr + mHeader->typeIdsOff);
        return &pTypeIds[idx];
    }

    const char *dexStringByTypeIdx(u4 idx) {
        const DexTypeId *typeId = dexGetTypeId(idx);
        return dexStringById(typeId->descriptorIdx);
    }

    const char *dexGetStringData(const DexStringId *pStringId) {
        const u1 *ptr = mAddr + pStringId->stringDataOff;

        while (*(ptr++) > 0x7f)  ;
        return (const char *) ptr;
    }

    const DexStringId *dexGetStringId(u4 idx) {
        assert(idx < mHeader->stringIdsSize);
        const DexStringId *pStringIds = (const DexStringId *)(mAddr + mHeader->stringIdsOff);
        return &pStringIds[idx];
    }

    const DexStringId *getDexStringIdByTypeIdx(u4 idx) {
        const DexTypeId *typeId = dexGetTypeId(idx);
        return dexGetStringId(typeId->descriptorIdx);
    }

    const char *dexStringById(u4 idx) {
        const DexStringId *pStringId = dexGetStringId(idx);
        return dexGetStringData(pStringId);
    }

    const DexTypeItem *dexGetTypeItem(const DexTypeList *pList, u4 idx) {
        assert(idx < pList->size);
        return &pList->list[idx];
    }

    const DexMethodId *dexGetMethodId(u4 idx) {
        assert(idx < mHeader->methodIdsSize);
        const DexMethodId *pMethodIds = (const DexMethodId *) (mAddr + mHeader->methodIdsOff);
        return &pMethodIds[idx];
    }

    const DexProtoId *dexGetProtoId(u4 idx) {
        const DexProtoId *pProtoIds = (const DexProtoId *) (mAddr + mHeader->protoIdsOff);
        return &pProtoIds[idx];
    }

    const DexTypeList *dexGetProtoParameters(const DexProtoId *pProtoId) {
        if (pProtoId->parametersOff == 0)
            return NULL;
        return (const DexTypeList *)(mAddr + pProtoId->parametersOff);
    }

    const u1 *dexGetClassData(const DexClassDef &classDef) const {
        if (classDef.classDataOff == 0)
            return NULL;
        else
            return mAddr + classDef.classDataOff;
    }

private:
    const u1  *mAddr;

    const DexOptHeader *mOptHeader;
    const DexHeader *mHeader;
};

#endif

