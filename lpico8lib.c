//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2020 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//


#include <cmath>
#include <cctype>
#include <cstring>
#include <algorithm>

#define lpico8lib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "llimits.h"
#include "lobject.h"

#define TAU 6.2831853071795864769252867665590057683936

static int pico8_max(lua_State *l) {
    lua_pushnumber(l, lua_Number::max(lua_tonumber(l, 1), lua_tonumber(l, 2)));
    return 1;
}

static int pico8_min(lua_State *l) {
    lua_pushnumber(l, lua_Number::min(lua_tonumber(l, 1), lua_tonumber(l, 2)));
    return 1;
}

static int pico8_mid(lua_State *l) {
    lua_Number x = lua_tonumber(l, 1);
    lua_Number y = lua_tonumber(l, 2);
    lua_Number z = lua_tonumber(l, 3);
    lua_pushnumber(l, x > y ? y > z ? y : lua_Number::min(x, z)
                            : x > z ? x : lua_Number::min(y, z));
    return 1;
}

static int pico8_ceil(lua_State *l) {
    lua_pushnumber(l, lua_Number::ceil(lua_tonumber(l, 1)));
    return 1;
}

static int pico8_flr(lua_State *l) {
    lua_pushnumber(l, lua_Number::floor(lua_tonumber(l, 1)));
    return 1;
}

//using modf to get fractional part of number here because bittboy build seems to have problems
//with numbers > 1 (treats them as ints?)
//would be better to just replace these with lookup tables using the fractional part
static int pico8_cos(lua_State *l) {
    lua_pushnumber(l, cast_num(std::cos(-TAU * (double)(lua_Number::modf(lua_tonumber(l, 1))))));
    return 1;
}

static int pico8_sin(lua_State *l) {
    lua_pushnumber(l, cast_num(std::sin(-TAU * (double)(lua_Number::modf(lua_tonumber(l, 1))))));
    return 1;
}

static int pico8_atan2(lua_State *l) {
    lua_Number x = lua_tonumber(l, 1);
    lua_Number y = lua_tonumber(l, 2);
    // This could simply be atan2(-y,x) but since PICO-8 decided that
    // atan2(0,0) = 0.75 we need to do the same in our version.
    double a = 0.75 + std::atan2((double)x, (double)y) / TAU;
    lua_pushnumber(l, cast_num(a >= 1 ? a - 1 : a));
    return 1;
}

static int pico8_sqrt(lua_State *l) {
    lua_Number x = lua_tonumber(l, 1);
    lua_pushnumber(l, cast_num(x.bits() >= 0 ? std::sqrt((double)x) : 0));
    return 1;
}

static int pico8_abs(lua_State *l) {
    lua_pushnumber(l, lua_Number::abs(lua_tonumber(l, 1)));
    return 1;
}

static int pico8_sgn(lua_State *l) {
    lua_pushnumber(l, cast_num(lua_tonumber(l, 1).bits() >= 0 ? 1.0 : -1.0));
    return 1;
}

static int pico8_band(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) & lua_tonumber(l, 2));
    return 1;
}

static int pico8_bor(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) | lua_tonumber(l, 2));
    return 1;
}

static int pico8_bxor(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) ^ lua_tonumber(l, 2));
    return 1;
}

static int pico8_bnot(lua_State *l) {
    lua_pushnumber(l, ~lua_tonumber(l, 1));
    return 1;
}

static int pico8_shl(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) << int(lua_tonumber(l, 2)));
    return 1;
}

static int pico8_lshr(lua_State *l) {
    lua_pushnumber(l, lua_Number::lshr(lua_tonumber(l, 1), int(lua_tonumber(l, 2))));
    return 1;
}

static int pico8_shr(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) >> int(lua_tonumber(l, 2)));
    return 1;
}

static int pico8_rotl(lua_State *l) {
    lua_pushnumber(l, lua_Number::rotl(lua_tonumber(l, 1), int(lua_tonumber(l, 2))));
    return 1;
}

static int pico8_rotr(lua_State *l) {
    lua_pushnumber(l, lua_Number::rotr(lua_tonumber(l, 1), int(lua_tonumber(l, 2))));
    return 1;
}

static int pico8_tostr(lua_State *l) {
    char buffer[20];
    char const *s = buffer;
        
    uint16_t flags = 0;
    if (lua_isboolean(l, 2)){
        flags = lua_toboolean(l, 2) ? 1 : 0;
    }
    else if (lua_isnumber(l, 2)){
        flags = lua_tointeger(l, 2);
    }

    switch (lua_type(l, 1))
    {
        case LUA_TNONE:
            // PICO-8 0.2.2 changelog: tostr() returns "" instead of nil
            buffer[0] = '\0';
            break;
        case LUA_TNUMBER: {
            lua_Number x = lua_tonumber(l, 1);
            if (flags) {
                uint32_t b = (uint32_t)x.bits();
                if ((flags & 0x3) == 0x3) {
                    sprintf(buffer, "0x%04x%04x", (b >> 16) & 0xffff, b & 0xffff);
                }
                else if ((flags & 0x2) == 0x2) {
                    sprintf(buffer, "%d", b);
                }
                else {
                    sprintf(buffer, "0x%04x.%04x", (b >> 16) & 0xffff, b & 0xffff);
                }
            } else {
                lua_number2str(buffer, x);
            }
            break;
        }
        case LUA_TSTRING:
            lua_pushvalue(l, 1);
            return 1;
        case LUA_TBOOLEAN: s = lua_toboolean(l, 1) ? "true" : "false"; break;
        case LUA_TTABLE:
            // PICO-8 0.1.12d changelog: “__tostring metatable method
            // observed by tostr() / print() / printh()”
            if (luaL_callmeta(l, 1, "__tostring")) {
                luaL_tolstring(l, 1, NULL);
                return 1;
            }
            [[fallthrough]];
        case LUA_TFUNCTION:
            // PICO-8 0.1.12d changelog: “tostr(x,true) can also be used to view
            // the hex value of functions and tables (uses Lua's tostring)”
            if (flags) {
                luaL_tolstring(l, 1, NULL);
                return 1;
            }
            [[fallthrough]];
        default:
            sprintf(buffer, "[%s]", luaL_typename(l, 1));
            break;
    }
    lua_pushstring(l, s);
    return 1;
}

static int pico8_tonum(lua_State *l) {
    int numargs = lua_gettop(l);
    if (numargs < 1) {
        return 0;
    }

    uint16_t flags = numargs > 1 ? lua_tointeger(l, 2) : 0;

    if (lua_isboolean(l, 1)){
        lua_pushnumber(l, lua_toboolean(l, 1));
        return 1;
    }
    if (lua_isstring(l, 1)) {
        const char *s = lua_tostring(l, 1);
        char hexBuffer[9];
        if (flags) {
            bool shifted = (flags & 0x2) == 0x2;
            bool hex = (flags & 0x1) == 0x1;
            if (hex){
                size_t i = 0;

                while (s[i] != '\0'){
                    //if non-hex character
                    if ( s[i] < 48 || 
                        (s[i] > 57 && s[i] < 65) ||
                        (s[i] > 70 && s[i] < 97) ||
                         s[i] > 102){
                        hexBuffer[i] = '0';
                    }
                    else {
                        hexBuffer[i] = s[i];
                    }
                    i++;
                }

                uint32_t bits = strtol(hexBuffer, NULL, 16);
                if (shifted) {
                    bits = bits >> 16;
                }
                lua_pushnumber(l, (lua_Number)bits);
                return 1; 
            }
            else if (shifted){
                uint32_t bits = strtol(s, NULL, 10);
                bits = bits >> 16;
                lua_pushnumber(l, (lua_Number)bits);
                return 1;
            }
            
        }

        lua_Number ret;
        // If parsing failed, PICO-8 returns nothing
        if (!luaO_str2d(s, strlen(s), &ret)) return 0;
        lua_pushnumber(l, ret);
        return 1;
    }
    if (lua_isnumber(l, 1)) {
        lua_pushnumber(l, lua_tonumber(l, 1));
        return 1;
    }
    

    return 0;
}

static int pico8_chr(lua_State *l) {
    int numargs = lua_gettop(l);
    //pico 8's lua seems to top out at allowing 248 arguments
    char s[249];
    s[numargs] = '\0';
    for(int i = 0; i < numargs; i++) {
        s[i] = (char)(uint8_t)lua_tonumber(l, i + 1);
    }
    lua_pushlstring(l, s, numargs);
    return 1;
}

static int pico8_ord(lua_State *l) {
    size_t len;
    int n = 0;
    int count = 1;
    char const *s = luaL_checklstring(l, 1, &len);
    if (!lua_isnone(l, 2)) {
        if (!lua_isnumber(l, 2))
            return 0;
        n = int(lua_tonumber(l, 2)) - 1;
    }
    if (!lua_isnone(l, 3)) {
        if (!lua_isnumber(l, 3))
            return 0;
        count = int(lua_tonumber(l, 3));
    }
    
    if (n < 0 || size_t(n + count) > len)
        return 0;

    //min stack is only 20. This could be a much longer string
    lua_checkstack(l, count);
    for(int i = 0; i < count; i++) {
        lua_pushnumber(l, uint8_t(s[n + i]));
    }

    return count;
}

static int pico8_split(lua_State *l) {
    int numArgs = lua_gettop(l);
    if (numArgs == 0){
        return 0;
    }

    if (lua_isnil(l, 1)) {
        return 0;
    }

    size_t count = 0, hlen;
    char const *haystack = luaL_checklstring(l, 1, &hlen);
    if (!haystack)
        return 0;
    lua_newtable(l);
    // Split either by chunk size or by needle position
    int size = 0;
    char needle = ',';
    if (lua_isnumber(l, 2)) {
        size = int(lua_tonumber(l, 2));
        if (size <= 0)
            size = 1;
    } else if (lua_isstring(l, 2)) {
        needle = *lua_tostring(l, 2);
    }
    auto convert = lua_isnone(l, 3) || lua_toboolean(l, 3);
    char const *end = haystack + hlen + (!size && needle);
    for (char const *parser = haystack; parser < end; ) {
        lua_Number num;
        char const *next = size ? parser + size
                         : needle ? strchr(parser, needle) : parser + 1;
        if (!next || next > end)
            next = haystack + hlen;
        char saved = *next; // temporarily put a null terminator here
        *(char *)next = '\0';
        if (convert && luaO_str2d(parser, next - parser, &num))
            lua_pushnumber(l, num);
        else
            lua_pushstring(l, parser);
        *(char *)next = saved;
        lua_rawseti(l, -2, int(++count));
        parser = next + (!size && needle);
    }
    return 1;
}

//discussion here: https://www.lexaloffle.com/bbs/?tid=47314
static int pico8_inext (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  int i;
  if (lua_isnil(L, 2)){
    i = 0;
    lua_pushinteger(L, i);
  }
  else {
    i = luaL_checkint(L, 2);
  }
  i++;  /* next value */
  lua_pushinteger(L, i);
  lua_rawgeti(L, 1, i);
  return (lua_isnil(L, -1)) ? 1 : 2;
}

static const luaL_Reg pico8lib[] = {
  {"max",   pico8_max},
  {"min",   pico8_min},
  {"mid",   pico8_mid},
  {"ceil",  pico8_ceil},
  {"flr",   pico8_flr},
  {"cos",   pico8_cos},
  {"sin",   pico8_sin},
  {"atan2", pico8_atan2},
  {"sqrt",  pico8_sqrt},
  {"abs",   pico8_abs},
  {"sgn",   pico8_sgn},
  {"band",  pico8_band},
  {"bor",   pico8_bor},
  {"bxor",  pico8_bxor},
  {"bnot",  pico8_bnot},
  {"shl",   pico8_shl},
  {"shr",   pico8_shr},
  {"lshr",  pico8_lshr},
  {"rotl",  pico8_rotl},
  {"rotr",  pico8_rotr},
  {"tostr", pico8_tostr},
  {"tonum", pico8_tonum},
  {"chr",   pico8_chr},
  {"ord",   pico8_ord},
  {"split", pico8_split},
  //added in PICO-8 0.2.5
  //discussion here: https://www.lexaloffle.com/bbs/?tid=47314
  {"inext", pico8_inext},
  {NULL, NULL}
};


/*
** Register PICO-8 functions in global table
*/
LUAMOD_API int luaopen_pico8 (lua_State *L) {
  lua_pushglobaltable(L);
  luaL_setfuncs(L, pico8lib, 0);
  return 1;
}

