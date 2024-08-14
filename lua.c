/*
** 2024-08-09
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
**
** This file will implement the ability to use the Lua scripting language
** to define and call user defined functions.
**
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include <assert.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static lua_State *L = NULL;

/*
** Implementation of the lua() function that accepts lua code and one or more arguments holding column data.
*/
static void lua_func(sqlite3_context *context, int argc, sqlite3_value **argv) {
    if (!L) {
        L = luaL_newstate();
        luaL_openlibs(L);
    }

    if (argc < 1) {
        sqlite3_result_error(context, "Lua function requires at least one argument with lua code. All subsequent arguments will be passed to the lua code, accessible as 'args[1]', 'args[2]', ...", -1);
        return;
    }

    const char *lua_code = (const char *)sqlite3_value_text(argv[0]);
    lua_newtable(L);
    for (int i = 1; i < argc; i++) {
        switch (sqlite3_value_type(argv[i])) {
            case SQLITE_INTEGER:
                lua_pushinteger(L, sqlite3_value_int64(argv[i]));
            break;
            case SQLITE_FLOAT:
                lua_pushnumber(L, sqlite3_value_double(argv[i]));
            break;
            case SQLITE_TEXT:
                lua_pushstring(L, (const char *)sqlite3_value_text(argv[i]));
            break;
            case SQLITE_NULL:
                lua_pushnil(L);
            break;
            default:
                lua_pushnil(L);
        }
        lua_rawseti(L, -2, i);
    }
    lua_setglobal(L, "args");

    if (luaL_dostring(L, lua_code) != LUA_OK) {
        sqlite3_result_error(context, lua_tostring(L, -1), -1);
        return;
    }

    if (lua_isstring(L, -1)) {
        const char *result = lua_tostring(L, -1);
        sqlite3_result_text(context, result, -1, SQLITE_TRANSIENT);
    } else if (lua_isnumber(L, -1)) {
        double result = lua_tonumber(L, -1);
        sqlite3_result_double(context, result);
    } else if (lua_isinteger(L, -1)) {
        sqlite3_int64 result = lua_tointeger(L, -1);
        sqlite3_result_int64(context, result);
    } else if (lua_isnil(L, -1)) {
        sqlite3_result_null(context);
    } else {
        sqlite3_result_error(context, "Lua script did not return a string, number, or nil", -1);
    }

    lua_settop(L, 0); // Clean up the stack
}


#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_lua_init(
  sqlite3 *db,
  char **pzErrMsg,
  const sqlite3_api_routines *pApi
){
  int rc = SQLITE_OK;
  SQLITE_EXTENSION_INIT2(pApi);
  (void)pzErrMsg;  /* Unused parameter */
  rc = sqlite3_create_function(db, "lua", -1, SQLITE_UTF8 , 0, lua_func, 0, 0);
  /*if( rc==SQLITE_OK ){
    rc = sqlite3_create_function(db, "lua", -1, SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC, 0, lua_func, 0, 0);
  }*/
  return rc;
}
