/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <flood/out_stream.h>
#include <swamp-typeinfo/chunk.h>
#include <swamp-typeinfo-serialize/serialize.h>
#include <swamp-typeinfo/typeinfo.h>
#include <tiny-libc/tiny_libc.h>
#include <clog/clog.h>
#include <swamp-typeinfo-serialize/version.h>

static int writeString(FldOutStream *stream, const char *outString)
{
    int error;
    uint8_t count = tc_strlen(outString);
    if ((error = fldOutStreamWriteUInt8(stream, count)) != 0)
    {
        return error;
    }

    if ((error = fldOutStreamWriteOctets(stream, (const uint8_t *)outString, count)) != 0)
    {
        return error;
    }

    return 0;
}

static int writeTypeRef(FldOutStream *stream, const SwtiType *type)
{
    uint8_t index = type->index;
    int error;
    if ((error = fldOutStreamWriteUInt16(stream, index)) != 0)
    {
        return error;
    }

    return 0;
}

static int writeTypeRefs(FldOutStream *stream, const SwtiType **types, size_t count)
{
    int error;

    if ((error = fldOutStreamWriteUInt8(stream, count)) != 0)
    {
        return error;
    }

    for (uint8_t i = 0; i < count; i++)
    {
        if ((error = writeTypeRef(stream, types[i])) != 0)
        {
            return error;
        }
    }

    return 0;
}

static int writeMemoryOffset(FldOutStream *stream, uint16_t offset)
{
    int error;
    if ((error = fldOutStreamWriteUInt16(stream, offset)) != 0)
    {
        return error;
    }

    return 0;
}

static int writeVariant(FldOutStream *stream, const SwtiCustomTypeVariant *variant)
{
    writeString(stream, variant->name);

    for (size_t i = 0; i < variant->paramCount; ++i)
    {
        writeTypeRef(stream, variant->fields[i].fieldType);
        writeMemoryOffset(stream, variant->fields[i].memoryOffsetInfo.memoryOffset);
    }

    return 0;
}

static int writeVariants(FldOutStream *stream, const SwtiCustomType *custom)
{
    int error;

    if ((error = fldOutStreamWriteUInt8(stream, custom->variantCount)) != 0)
    {
        return error;
    }

    for (uint8_t i = 0; i < custom->variantCount; i++)
    {
        if ((error = writeVariant(stream, (const SwtiCustomTypeVariant *)&custom->variantTypes[i])) != 0)
        {
            return error;
        }
    }

    return 0;
}

static int writeCustomType(FldOutStream *stream, const SwtiCustomType *custom)
{
    int error;

    if ((error = writeString(stream, custom->internal.name)) != 0)
    {
        return error;
    }

    if ((error = writeVariants(stream, custom)) != 0)
    {
        return error;
    }

    return 0;
}

static int writeRecordField(FldOutStream *stream, const SwtiRecordTypeField *field)
{
    int error;
    if ((error = writeString(stream, field->name)) != 0)
    {
        return error;
    }

    return writeTypeRef(stream, field->fieldType);
}

static int writeRecordFields(FldOutStream *stream, const SwtiRecordType *record)
{
    int error;

    for (uint8_t i = 0; i < record->fieldCount; i++)
    {
        if ((error = writeRecordField(stream, &record->fields[i])) != 0)
        {
            return error;
        }
    }

    return 0;
}

static int writeRecord(FldOutStream *stream, const SwtiRecordType *record)
{
    int error;
    if ((error = fldOutStreamWriteUInt8(stream, record->fieldCount)) != 0)
    {
        return error;
    }

    if ((error = writeRecordFields(stream, record)) != 0)
    {
        return error;
    }

    return 0;
}

static int writeArray(FldOutStream *stream, const SwtiArrayType *array)
{
    int error;
    if ((error = writeTypeRef(stream, array->itemType)) != 0)
    {
        return error;
    }

    return 0;
}

static int writeList(FldOutStream *stream, const SwtiListType *list)
{
    int error;
    if ((error = writeTypeRef(stream, list->itemType)) != 0)
    {
        return error;
    }

    return 0;
}

static int writeFunction(FldOutStream *stream, const SwtiFunctionType *fn)
{
    int error;
    if ((error = writeTypeRefs(stream, fn->parameterTypes, fn->parameterCount)) != 0)
    {
        return error;
    }

    return 0;
}

static int writeTuple(FldOutStream *stream, const SwtiTupleType *tuple)
{
    int error;
    if ((error = writeTypeRefs(stream, 0 /*todo*/, tuple->fieldCount)) != 0)
    {
        return error;
    }

    return 0;
}

static int writeAlias(FldOutStream *stream, const SwtiAliasType *alias)
{
    int error;
    if ((error = writeString(stream, alias->internal.name)) != 0)
    {
        return error;
    }

    if ((error = writeTypeRef(stream, alias->targetType)) != 0)
    {
        return error;
    }

    return 0;
}

static int writeUnmanaged(FldOutStream *stream, const SwtiUnmanagedType *unmanaged)
{
    int error;
    if ((error = writeString(stream, unmanaged->internal.name)) != 0)
    {
        return error;
    }

    if ((error = fldOutStreamWriteUInt16(stream, unmanaged->userTypeId)) != 0)
    {
        return error;
    }

    return 0;
}

static int writeType(FldOutStream *stream, const SwtiType *type)
{
    int error;
    if ((error = fldOutStreamWriteUInt8(stream, type->type)) != 0)
    {
        return error;
    }

    error = -99;
    switch (type->type)
    {
    case SwtiTypeCustom:
    {
        error = writeCustomType(stream, (const SwtiCustomType *)type);
        break;
    }
    case SwtiTypeFunction:
    {
        error = writeFunction(stream, (const SwtiFunctionType *)type);
        break;
    }
    case SwtiTypeAlias:
    {
        error = writeAlias(stream, (const SwtiAliasType *)type);
        break;
    }
    case SwtiTypeRecord:
    {
        error = writeRecord(stream, (const SwtiRecordType *)type);
        break;
    }
    case SwtiTypeArray:
    {
        error = writeArray(stream, (const SwtiArrayType *)type);
        break;
    }
    case SwtiTypeList:
    {
        error = writeList(stream, (const SwtiListType *)type);
        break;
    }
    case SwtiTypeUnmanaged:
    {
        error = writeUnmanaged(stream, (const SwtiUnmanagedType *)type);
        break;
    }
    case SwtiTypeTuple:
    {
        error = writeTuple(stream, (const SwtiTupleType *)type);
        break;
    }
    case SwtiTypeString:
    {
        error = 0;
        break;
    }
    case SwtiTypeInt:
    {
        error = 0;
        break;
    }
    case SwtiTypeFixed:
    {
        error = 0;
        break;
    }
    case SwtiTypeBoolean:
    {
        error = 0;
        break;
    }
    case SwtiTypeBlob:
    {
        error = 0;
        break;
    }
    case SwtiTypeResourceName:
    {
        error = 0;
        break;
    }
    case SwtiTypeChar:
    {
        error = 0;
        break;
    }
    case SwtiTypeAny:
    {
        error = 0;
        break;
    }
    case SwtiTypeAnyMatchingTypes:
    {
        error = 0;
        break;
    }
    case SwtiTypeCustomVariant:
    {
      error = 0;
      break;
    }
    case SwtiTypeRefId:
    {
      error = 0;
      break;
    }
    default:
    {
        CLOG_ERROR("Unknown type %d", type->type);
    }
    }

    return error;
}

int swtisSerializeToStream(FldOutStream *stream, const struct SwtiChunk *source)
{
    int error;

    int tell = stream->pos;

    if ((error = fldOutStreamWriteUInt8(stream, SWTI_SERIALIZE_VERSION_MAJOR)) != 0)
    {
        return error;
    }
    if ((error = fldOutStreamWriteUInt8(stream, SWTI_SERIALIZE_VERSION_MINOR)) != 0)
    {
        return error;
    }
    if ((error = fldOutStreamWriteUInt8(stream, SWTI_SERIALIZE_VERSION_PATCH)) != 0)
    {
        return error;
    }

    if ((error = fldOutStreamWriteUInt16(stream, source->typeCount)) != 0)
    {
        return error;
    }

    for (uint8_t i = 0; i < source->typeCount; i++)
    {
        const SwtiType *item = source->types[i];
        if (item->index != i)
        {
            return -2;
        }
        if ((error = writeType(stream, item)) != 0)
        {
            return error;
        }
    }

    int octetsWritten = stream->pos - tell;

    return octetsWritten;
}

int swtisSerialize(uint8_t *octets, size_t maxCount, const SwtiChunk *source)
{
    FldOutStream stream;

    fldOutStreamInit(&stream, octets, maxCount);

    return swtisSerializeToStream(&stream, source);
}
