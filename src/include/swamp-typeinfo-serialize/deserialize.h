/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_TYPEINFO_SERIALIZE_DESERIALIZE_H
#define SWAMP_TYPEINFO_SERIALIZE_DESERIALIZE_H

#include <stdint.h>
#include <stdlib.h>

struct SwtiChunk;
struct FldInStream;

int swtisDeserialize(const uint8_t* octets, size_t count, struct SwtiChunk* target);
int swtisDeserializeFromStream(struct FldInStream* stream, struct SwtiChunk* target);

#endif
