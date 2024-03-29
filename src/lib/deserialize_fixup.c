/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-typeinfo/chunk.h>
#include <swamp-typeinfo-serialize/deserialize.h>
#include <swamp-typeinfo-serialize/deserialize_internal.h>
#include <swamp-typeinfo/typeinfo.h>

#include <clog/clog.h>

static int fixupTypeRef(const SwtiType** type, const SwtiChunk* chunk)
{
    uintptr_t ptrValue = (uintptr_t)(*type);
    if (ptrValue >= 65535) {
        CLOG_ERROR("illegal ref")
        *type = 0;
        return -2;
    }
    if (ptrValue >= chunk->typeCount) {
        CLOG_ERROR("something is wrong here. index is more than number of entries. %I64u of %zu", ptrValue, chunk->typeCount)
        *type = 0;
        return -3;
    }
    *type = chunk->types[ptrValue];

    if (chunk->types[ptrValue]->index != ptrValue) {
        CLOG_ERROR("problem")
        *type = 0;
        return -4;
    }

    return 0;
}

static int fixupTypeRefs(const SwtiType** types, size_t count, const struct SwtiChunk* chunk)
{
    int error;

    for (size_t i = 0; i < count; ++i) {
        if ((error = fixupTypeRef(&types[i], chunk)) != 0) {
            return error;
        }
    }

    return 0;
}

static int fixupRecordType(SwtiRecordType* record, const struct SwtiChunk* chunk)
{
    int error;
    for (size_t i = 0; i < record->fieldCount; ++i) {
        SwtiRecordTypeField* mutableField = (SwtiRecordTypeField*) &record->fields[i];
        if ((error = fixupTypeRef(&mutableField->fieldType, chunk)) != 0) {
            return error;
        }
    }

    return 0;
}


static int fixupTupleType(SwtiTupleType* tuple, const struct SwtiChunk* chunk)
{
    int error;
    for (size_t i = 0; i < tuple->fieldCount; ++i) {
        SwtiRecordTypeField* mutableField = (SwtiRecordTypeField*) &tuple->fields[i];
        if ((error = fixupTypeRef(&mutableField->fieldType, chunk)) != 0) {
            return error;
        }
    }

    return 0;
}

static int fixupCustomVariantType(SwtiCustomTypeVariant* variant, const struct SwtiChunk* chunk)
{
    int error;

    if ((error = fixupTypeRef((const SwtiType**) &variant->inCustomType, chunk)) != 0) {
        return error;
    }
    if (variant->inCustomType->internal.type != SwtiTypeCustom) {
        return -4;
    }

    for (size_t i = 0; i < variant->paramCount; ++i) {
        SwtiCustomTypeVariantField * mutableField = (SwtiCustomTypeVariantField*) &variant->fields[i];
        if ((error = fixupTypeRef(&mutableField->fieldType, chunk)) != 0) {
            return error;
        }
    }

    return 0;
}

static int fixupGenerics(SwtiGenericParams* generics, const struct SwtiChunk* chunk)
{
    int error;
    for (size_t i = 0; i < generics->genericCount; ++i) {
        const SwtiType** mutableVariant = (const SwtiType **) &generics->genericTypes[i];
        if ((error = fixupTypeRef(mutableVariant, chunk)) != 0) {
            return error;
        }
        const SwtiType * genericType = generics->genericTypes[i];
        if (genericType->type == SwtiTypeCustomVariant) {
            return -46;
        }
    }

    return 0;
}


static int fixupCustomType(SwtiCustomType* custom, const struct SwtiChunk* chunk)
{
    int error;

    fixupGenerics(&custom->generic, chunk);

    for (size_t i = 0; i < custom->variantCount; ++i) {
        const SwtiType** mutableVariant = (const SwtiType **) &custom->variantTypes[i];
        if ((error = fixupTypeRef(mutableVariant, chunk)) != 0) {
            return error;
        }
        const SwtiCustomTypeVariant* variant = custom->variantTypes[i];

        if (variant->internal.type != SwtiTypeCustomVariant) {
            return -46;
        }
    }

    return 0;
}

static int fixupType(SwtiType* type, const SwtiChunk* chunk)
{
    switch (type->type) {
        case SwtiTypeCustom: {
            SwtiCustomType* custom = (SwtiCustomType*) type;
            return fixupCustomType(custom, chunk);
        }
        case SwtiTypeCustomVariant: {
            SwtiCustomTypeVariant * custom = (SwtiCustomTypeVariant*) type;
            return fixupCustomVariantType(custom, chunk);
        }
        case SwtiTypeFunction: {
            SwtiFunctionType* fn = (SwtiFunctionType*) type;
            return fixupTypeRefs((const SwtiType**) fn->parameterTypes, fn->parameterCount, chunk);
        }
        case SwtiTypeTuple: {
            SwtiTupleType* tuple = (SwtiTupleType*) type;
            return fixupTupleType(tuple, chunk);
        }
        case SwtiTypeAlias: {
            SwtiAliasType* alias = (SwtiAliasType*) type;
            return fixupTypeRef((const SwtiType**) &alias->targetType, chunk);
        }
        case SwtiTypeRefId: {
            SwtiTypeRefIdType * typeRefId = (SwtiTypeRefIdType *) type;
            return fixupTypeRef((const SwtiType**) &typeRefId->referencedType, chunk);
        }
        case SwtiTypeRecord: {
            SwtiRecordType* record = (SwtiRecordType*) type;
            return fixupRecordType(record, chunk);
        }
        case SwtiTypeArray: {
            SwtiArrayType* array = (SwtiArrayType*) type;
            return fixupTypeRef((const SwtiType**) &array->itemType, chunk);
        }
        case SwtiTypeList: {
            SwtiListType* list = (SwtiListType*) type;
            return fixupTypeRef((const SwtiType**) &list->itemType, chunk);
        }
        // All these do not require fixup
        case SwtiTypeBoolean:
        case SwtiTypeBlob:
        case SwtiTypeFixed:
        case SwtiTypeInt:
        case SwtiTypeAny:
        case SwtiTypeAnyMatchingTypes:
        case SwtiTypeUnmanaged:
        case SwtiTypeString:
        case SwtiTypeChar:
        case SwtiTypeResourceName:
            return 0;
    }

    CLOG_ERROR("type information: don't know how to fixup type %d", type->type);
    return -1;
}

int swtisDeserializeFixup(SwtiChunk* chunk)
{
    int error;
    for (size_t i = 0; i < chunk->typeCount; ++i) {
        const SwtiType* item = chunk->types[i];
        if ((error = fixupType((SwtiType*) item, chunk)) != 0) {
            CLOG_SOFT_ERROR("fixupType %d %s", error, item->name)
            return error;
        }
    }

    return 0;
}
