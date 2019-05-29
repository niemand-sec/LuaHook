// LuaInjector.cpp : Defines the exported functions for the DLL application.
//
// Standard imports
#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include "MinHook.h"
#include <tlhelp32.h>
#include <Psapi.h>

#if defined _M_X64
#pragma comment(lib, "libMinHook-x64-v141-mdd.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook-x86-v141-mdd.lib")
#endif




typedef void* lua_State;

// LUA STATE HOOK DEFINITIONS
uint64_t baseAddress = 0;
uintptr_t targetFunction = 0;
lua_State * lua_State_ptr = 0;
lua_State * new_lua_State_ptr = 0;

typedef int(__cdecl *gettop)(int);
typedef lua_State *(__cdecl *newThread)(lua_State *);
typedef void(__cdecl *pushinteger)(lua_State *, int);
typedef int(__cdecl *tointeger)(lua_State *, int);
typedef int(__cdecl *_luaStatus)(lua_State *);
typedef int(__cdecl *_luaL_loadstring)(lua_State *, const char *);
//typedef int(__cdecl *_lua_pcall)(lua_State *, int, int, int);


gettop origTargetFunction = NULL;


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
	SetConsoleTitle("[+] LUA hook by Niemand");
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	freopen("CONIN$", "r", stdin);
	std::cout << "Injecting..." << std::endl;
}


gettop newFunction = NULL;

DWORD _gettop(int state)
{
	std::cout << "[!] Hook called :O" << std::endl;
	if (lua_State_ptr == 0) 
	{
		lua_State_ptr = (lua_State *)state;
		std::cout << "[+] State Obtained: \t" << std::hex << lua_State_ptr << std::endl;
		if (MH_DisableHook((LPVOID)targetFunction) != MH_OK)
		{
			std::cout << "[+] Hook disabled" << std::endl;
		}
	}
	return (*(DWORD *)(state + 0x18) - *(DWORD *)(state + 0x10)) >> 3;
}



//Get all module related info, this will include the base DLL.
//and the size of the module
MODULEINFO GetModuleInfo(char *szModule)
{
	MODULEINFO modinfo = { 0 };
	HMODULE hModule = GetModuleHandle(szModule);
	if (hModule == 0)
		return modinfo;
	GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
	return modinfo;
}

//Pattern Scan https://guidedhacking.com/threads/c-signature-scan-pattern-scanning-tutorial-difficulty-3-10.3981
uintptr_t FindPattern(char *module, char *pattern, char *mask)
{
	//Get all module related information
	MODULEINFO mInfo = GetModuleInfo(module);
	
	//Assign our base and module size
	uintptr_t base = (uintptr_t)mInfo.lpBaseOfDll;

	uintptr_t size = (uintptr_t)mInfo.SizeOfImage;

	//Get length for our mask, this will allow us to loop through our array
	DWORD patternLength = (DWORD)strlen(mask);

	for (DWORD i = 0; i < size - patternLength; i++)
	{
		bool found = true;
		for (DWORD j = 0; j < patternLength; j++)
		{
			//if we have a ? in our mask then we have true by default,
			//or if the bytes match then we keep searching until finding it or not
			found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);
		}

		//found = true, our entire pattern was found
		if (found)
		{
			std::cout << "[+] Pattern found: " << base + i << std::endl;
			return base + i;
		}
	}
	std::cout << "[-] Pattern not found" << std::endl;
	return NULL;
}



void printValues()
{
	std::cout << "targetFunction Addr: \t" << std::hex << targetFunction << std::endl;
	std::cout << "newFunction Addr: \t" << std::hex << &_gettop << std::endl;
	/*
	std::cout << "fn lua_newthread_p: \t" << std::hex << val.lua_newthread_p << std::endl;
	std::cout << "fn lua_pushinteger_p: \t" << std::hex << val.lua_pushinteger_p << std::endl;
	std::cout << "fn lua_tointeger_p: \t" << std::hex << val.lua_tointeger_p << std::endl;
	std::cout << "fn lua_status_p: \t" << std::hex << val.lua_status_p << std::endl;
	std::cout << "fn luaL_loadstring_p: \t" << std::hex << val.luaL_loadstring_p << std::endl;
	std::cout << "fn lua_pcall_p: \t" << std::hex << val.lua_pcall_p << std::endl;
	*/
}



int detourLuaState()
{
	//_gettop(lua_State); i don't think you need this but if you do you can just add it!
	std::cout << "[+] Hooking targetFunction: " << (LPVOID)targetFunction << std::endl;

	
	// Create a hook for MessageBoxW, in disabled state.
	if (MH_CreateHook((LPVOID)targetFunction, &_gettop, reinterpret_cast<LPVOID*>(&origTargetFunction)) != MH_OK)
	{
		std::cout << "[-] Hooking failed" << std::endl;
		return 1;
	}
	std::cout << "[+] Hooked" << std::endl;
	if (MH_EnableHook((LPVOID)targetFunction) != MH_OK)
	{
		return 1;
	}
	std::cout << "[+] Hooked enabled" << std::endl;
	std::cout << "[+] origTargetFunction " << std::hex << origTargetFunction << std::endl;
	return 0;
	/*
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(LPVOID&)val.lua_gettop_p, (PBYTE)_gettop); // Detours the original lua_gettop_p with our _gettop
	DetourTransactionCommit();
	*/
}



int WINAPI main()
{
	ConsoleSetup();

	//newFunction = (gettop)_gettop;

	baseAddress = (uintptr_t)GetModuleHandle(NULL);
	char* test = new char[14];
	char* test2 = new char[14];
	strcpy(test, "\x48\x8B\x41\x18\x48\x2B\x41\x10\x48\xc1\xf8\x03\xc3");
	strcpy(test2, "xxxxxxxxxxxxx");
	targetFunction = FindPattern(NULL, test, test2);

	printValues();

	// Initialize MinHook.
	if (MH_Initialize() != MH_OK)
	{
		return 1;
	}

	detourLuaState();

	Sleep(10000);

	return 1;
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

