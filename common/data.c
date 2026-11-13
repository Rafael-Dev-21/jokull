#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
#endif

#include "skfantasy.h"

TileType tile_types[TILE_COUNT] = {
	{ 0, 0, {}, {} },
	{ 1, 1 , {}, {} },
	{ 2, 2, {}, {} },
	{ 3, 3, {}, {} },
	{ 4, 4, {}, {} },
  { 3, 5, {}, {true, 1, TILE_MUD} },
  { 2, 6, {true, 0, TILE_SOIL}, {} },
};

ObjectType obj_types[OBJ_COUNT] = {
	{ 0, 0, false, false, {} },
	{ 5, 4, true, true, {} },
  { 6, 7, false, true, {} },
  { 1, 5, true, true, { true, 32, OBJ_WHEAT } },
  { 7, 5, true, true, {} },
};