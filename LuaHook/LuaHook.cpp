// LuaInjector.cpp : Defines the exported functions for the DLL application.
//
// Standard imports
#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <iomanip>
#include "detours.h"
#include <stdio.h>
#include <stdlib.h>


typedef void* lua_State;

// LUA STATE HOOK DEFINITIONS
lua_State * lua_State_ptr = 0;
lua_State * new_lua_State_ptr = 0;
typedef int(__cdecl *gettop)(int);
typedef lua_State *(__cdecl *newThread)(lua_State *);
typedef void(__cdecl *pushinteger)(lua_State *, int);
typedef int(__cdecl *tointeger)(lua_State *, int);
typedef int(__cdecl *_luaStatus)(lua_State *);
typedef int(__cdecl *_luaL_loadstring)(lua_State *, const char *);
//typedef int(__cdecl *_lua_pcall)(lua_State *, int, int, int);


typedef int(*_luaL_loadfilex)(lua_State *L, const char *filename, const char *mode);
typedef int(*_lua_pcall)(lua_State *L, int nargs, int nresults, int errfunc);



typedef int luaL_loadbuffer_(lua_State *L, char *buff, size_t size, char *name);
typedef int lua_pcall_(lua_State *L, int nargs, int nresults, int errfunc);
typedef const char *lua_tostring_(lua_State *L, int32_t idx);
typedef uint32_t lua_isstring_(lua_State *L, int32_t idx);
typedef lua_State *lua_newthread_(lua_State *L);
typedef void logPointer(std::string name, uint64_t pointer);



typedef LONG(NTAPI *NtSuspendProcess)(IN HANDLE ProcessHandle);
typedef LONG(NTAPI *NtResumeProcess)(IN HANDLE ProcessHandle);


void ConsoleSetup()
{
	AllocConsole();
	SetConsoleTitle("Vermintide LUA hook by Niemand");
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	freopen("CONIN$", "r", stdin);
	std::cout << "Injecting..." << std::endl;
}

struct gameOffsets {
	uintptr_t getTop = 0xe160;
	uintptr_t newThread = 0xC9A0;
	uintptr_t pushinteger = 0xCF70;
	uintptr_t tointeger = 0xD6D0;
	uintptr_t luaStatus = 0xE370;
	uintptr_t loadstring = 0x40880;
	uintptr_t pCall = 0xBB20;
	uintptr_t loadfilex = 0x40980;
}offsets;

struct values {
	uintptr_t lua51Module;
	uintptr_t getTop;
	uintptr_t newThread_f;
	gettop lua_gettop_p;
	newThread lua_newthread_p;
	pushinteger lua_pushinteger_p;
	uintptr_t lua_pushinteger_f;
	tointeger lua_tointeger_p;
	uintptr_t lua_tointeger_f;
	uintptr_t lua_status_f;
	_luaStatus lua_status_p;
	uintptr_t luaL_loadstring_f;
	_luaL_loadstring luaL_loadstring_p;
	uintptr_t lua_pcall_f;
	_lua_pcall lua_pcall_p;
	uintptr_t luaL_loadfilex_f;
	_luaL_loadfilex luaL_loadfilex_p;
}val;


DWORD _gettop(int state)
{
	//std::cout << "Hook called :O" << std::endl;
	if (lua_State_ptr == 0) {
		lua_State_ptr = (lua_State *)state;
		std::cout << "[+] State Obtained: \t" << std::hex << lua_State_ptr << std::endl;
	}
	return (*(DWORD *)(state + 24) - *(DWORD *)(state + 16)) >> 3;
}


void detourLuaState()
{
	//_gettop(lua_State); i don't think you need this but if you do you can just add it!
	std::cout << "[+] Calling LuaState Detour" << std::endl;
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(LPVOID&)val.lua_gettop_p, (PBYTE)_gettop); // Detours the original lua_gettop_p with our _gettop
	DetourTransactionCommit();
}


void retrieveValues()
{
	val.lua51Module = (uintptr_t)GetModuleHandle("lua51.dll");
	val.getTop = (uintptr_t)(val.lua51Module + offsets.getTop);
	val.lua_gettop_p = (gettop)val.getTop; // this is the actual lua_gettop function in memory that we place the detour at

	val.newThread_f = (uintptr_t)(val.lua51Module + offsets.newThread);
	val.lua_newthread_p = (newThread)val.newThread_f;

	val.lua_pushinteger_f = (uintptr_t)(val.lua51Module + offsets.pushinteger);
	val.lua_pushinteger_p = (pushinteger)val.lua_pushinteger_f;

	val.lua_tointeger_f = (uintptr_t)(val.lua51Module + offsets.tointeger);
	val.lua_tointeger_p = (tointeger)val.lua_tointeger_f;

	val.lua_status_f = (uintptr_t)(val.lua51Module + offsets.luaStatus);
	val.lua_status_p = (_luaStatus)val.lua_status_f;

	//40880
	val.luaL_loadstring_f = (uintptr_t)(val.lua51Module + offsets.loadstring);
	val.luaL_loadstring_p = (_luaL_loadstring)val.luaL_loadstring_f;

	val.luaL_loadfilex_f = (uintptr_t)(val.lua51Module + offsets.loadfilex);
	val.luaL_loadfilex_p = (_luaL_loadfilex)val.luaL_loadfilex_f;

	//BB20
	val.lua_pcall_f = (uintptr_t)(val.lua51Module + offsets.pCall);
	val.lua_pcall_p = (_lua_pcall)val.lua_pcall_f;


}

void printValues()
{
	std::cout << "getTop Addr: \t" << std::hex << val.getTop << std::endl;
	std::cout << "fn lua_get_top_p: \t" << std::hex << val.lua_gettop_p << std::endl;
	std::cout << "fn lua_newthread_p: \t" << std::hex << val.lua_newthread_p << std::endl;
	std::cout << "fn lua_pushinteger_p: \t" << std::hex << val.lua_pushinteger_p << std::endl;
	std::cout << "fn lua_tointeger_p: \t" << std::hex << val.lua_tointeger_p << std::endl;
	std::cout << "fn lua_status_p: \t" << std::hex << val.lua_status_p << std::endl;
	std::cout << "fn luaL_loadstring_p: \t" << std::hex << val.luaL_loadstring_p << std::endl;
	std::cout << "fn lua_pcall_p: \t" << std::hex << val.lua_pcall_p << std::endl;

}


int refKey = 0;
lua_State * CreateThread()
{
	std::cout << "[+] Calling newThread" << std::endl;
	lua_State * thread = (val.lua_newthread_p)(lua_State_ptr);
	std::cout << "[+] New LuaState: " << thread << std::endl;

	return thread;
};

int WINAPI main()
{
	ConsoleSetup();

	retrieveValues();
	printValues();

	detourLuaState();

	//Forcing Sleep so lua state is created.
	Sleep(6000);
	//Creating New Lua Thread based on hooked State
	new_lua_State_ptr = CreateThread();
	//std::cout << "[+] New LuaStatus: " << lua_status(new_lua_State_ptr) << std::endl;
	//std::cout << "[+] newgettop: " << lua_gettop(new_lua_State_ptr) << std::endl;
	std::cout << "[+] LuaStatus: " << val.lua_status_p(lua_State_ptr) << std::endl;
	std::cout << "[+] New LuaStatus: " << val.lua_status_p(new_lua_State_ptr) << std::endl;
	std::cout << "++" << std::endl;


	while (true)
	{
		if (GetAsyncKeyState(VK_F9) & 1)
		{
			val.luaL_loadfilex_p(lua_State_ptr, "main.lua", NULL) || val.lua_pcall_p(lua_State_ptr, 0, -1, 0);
			std::cout << "[!] Main.lua Executed" << std::endl;
			//std::cout << "[+] LuaStatus: " << val.lua_status_p(lua_State_ptr) << std::endl;
			//std::cout << "[+] New LuaStatus: " << val.lua_status_p(new_lua_State_ptr) << std::endl;
		}
		Sleep(2000);
	}

}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)main, NULL, NULL, NULL);
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

