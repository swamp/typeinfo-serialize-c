/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_TYPEINFO_SERIALIZE_DESERIALIZE_INTERNAL_H
#define SWAMP_TYPEINFO_SERIALIZE_DESERIALIZE_INTERNAL_H

#include <stdint.h>
#include <stdlib.h>

struct SwtiChunk;

// Only for tests, do not use
//int swtiDeserializeRaw(const uint8_t* octets, size_t count, struct SwtiChunk* target);
int swtisDeserializeFixup(struct SwtiChunk* chunk);

#endif
