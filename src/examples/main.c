/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/out_stream.h>
#include <stdio.h>
#include <swamp-typeinfo/chunk.h>
#include <swamp-typeinfo/typeinfo.h>
#include <swamp-typeinfo/add.h>
#include <swamp-typeinfo-serialize/deserialize.h>
#include <swamp-typeinfo-serialize/deserialize_internal.h>
#include <swamp-typeinfo-serialize/serialize.h>

clog_config g_clog;

static void tyran_log_implementation(enum clog_type type, const char *string)
{
    (void)type;
    fprintf(stderr, "%s\n", string);
}

void test()
{
    SwtiStringType s;
    swtiInitString(&s);

    SwtiIntType integer;
    swtiInitInt(&integer);

    SwtiRecordTypeField a;
    a.fieldType = (const SwtiType *)&s;
    a.name = "firstField";

    SwtiRecordTypeField b;
    b.fieldType = (const SwtiType *)&integer;
    b.name = "second";

    const SwtiRecordTypeField fields[] = {a, b};

    SwtiRecordType r;
    swtiInitRecordWithFields(&r, fields, sizeof(fields) / sizeof(fields[0]));

    
    SwtiCustomTypeVariant v1;
    v1.fields = 0;
    v1.paramCount = 0;
    v1.name = "Nothing";


    SwtiIntType* ip = &integer;
    SwtiCustomTypeVariantField variantFields[1];
    variantFields[0].fieldType = (const SwtiType*)ip;

    SwtiCustomTypeVariant v2;
    v2.fields = variantFields;
    v2.paramCount = 1;
    v2.name = "Just";

    const SwtiCustomTypeVariant variants[] = {v2, v1};

    const SwtiType *generics[] = {(const SwtiType *)&integer};
    SwtiCustomType c;
    swtiInitCustomWithGenerics(&c, "Result", generics, 1, variants, sizeof(variants) / sizeof(variants[0]));

    SwtiAliasType maybe;

    swtiInitAlias(&maybe, "Maybe", (const SwtiType *)&c);

    SwtiAliasType alias;

    swtiInitAlias(&alias, "CoolRecord", (const SwtiType *)&r);

    const SwtiType *params[] = {(const SwtiType *)&maybe, (const SwtiType *)&s, (const SwtiType *)&integer};

    SwtiFunctionType fn;
    swtiInitFunction(&fn, params, sizeof(params) / sizeof(params[0]));

    char debugOut[2048];

    swtiDebugString((const SwtiType *)&fn, 0, debugOut, 2048);
    const char *hack1 = (const char *)debugOut;

    fprintf(stderr, "Constructed:\n%s\n", hack1);

    SwtiChunk chunk;

    const uint8_t octets[] = {0, 1, 4, 0x09,
                              SwtiTypeInt,
                              SwtiTypeList,
                              0x00,
                              SwtiTypeArray,
                              0x04,
                              SwtiTypeAlias,
                              0x4,
                              'C',
                              'o',
                              'o',
                              'l',
                              4,
                              SwtiTypeRecord,
                              2,
                              1,
                              'a',
                              0,
                              1,
                              'b',
                              1,
                              SwtiTypeString,
                              SwtiTypeFunction,
                              2,
                              1,
                              2,
                              SwtiTypeCustom,
                              0x6,
                              'R',
                              'e',
                              's',
                              'u',
                              'l',
                              't',
                              2,
                              5,
                              'M',
                              'a',
                              'y',
                              'b',
                              'e',
                              1,
                              2,
                              3,
                              'N',
                              'o',
                              't',
                              0,
                              SwtiTypeTuple,
                              2,
                              1, 0};

    int error = swtisDeserialize(octets, sizeof(octets), &chunk);
    if (error < 0)
    {
        CLOG_ERROR("problem with deserialization raw typeinformation %d", error);
    }

    swtiChunkDebugOutput(&chunk, 0, "after deserialization");

    SwtiChunk copyChunk;
    int newIndex;
    int worked = swtiChunkInitOnlyOneType(&copyChunk, chunk.types[8], &newIndex);
    if (worked < 0)
    {
        CLOG_ERROR("could not make a copy");
    }

    CLOG_INFO("newIndex %d", newIndex);
    swtiChunkDebugOutput(&copyChunk, 0, "after copy");
}

void file()
{
    FILE *f = fopen("output.swamp-typeinfo", "rb");
    if (f == 0)
    {
        CLOG_ERROR("can not find the file");
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *octets = malloc(fsize);
    fread(octets, 1, fsize, f);
    fclose(f);

    SwtiChunk chunk;
    int error = swtisDeserialize(octets, fsize, &chunk);
    if (error != 0)
    {
        CLOG_ERROR("problem with deserialization raw typeinformation");
    }
    printf("before:-----------------\n");
    swtiChunkDebugOutput(&chunk, 0, "before");

    uint8_t *outOctets = malloc(16 * 1024);

    int octetsWritten = swtisSerialize(outOctets, 16 * 1024, &chunk);
    if (octetsWritten < 0)
    {
        CLOG_ERROR("problem with serialization typeinformation");
    }

    SwtiChunk deserializedChunk;
    error = swtisDeserialize(outOctets, octetsWritten, &deserializedChunk);
    if (error != 0)
    {
        CLOG_ERROR("problem with deserialization raw typeinformation");
    }

    printf("WE GOT THIS BACK:-----------------\n");

    swtiChunkDebugOutput(&deserializedChunk, 0, "we got this back");

    SwtiChunk copyChunk;
    copyChunk.typeCount = 0;
    copyChunk.maxCount = 32;
    copyChunk.types = tc_malloc_type_count(const SwtiType *, chunk.maxCount);

    error = swtiChunkAddType(&copyChunk, deserializedChunk.types[17]);
    if (error != 0)
    {
        CLOG_ERROR("problem with consume");
    }
    error = swtiChunkAddType(&copyChunk, deserializedChunk.types[0]);
    if (error != 0)
    {
        CLOG_ERROR("problem with consume");
    }

    printf("Copied:-----------------\n");

    swtiChunkDebugOutput(&copyChunk, 0, "copied");
}

int main()
{
    g_clog.log = tyran_log_implementation;

    test();

    return 0;
}
