// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <cstdint>
#include <Windows.h>
#include <Psapi.h>
#include <stdlib.h>
#include <iostream>
#include <string>

using namespace std;

uint64_t GetBaseModuleAddress() {
    HANDLE process = GetCurrentProcess();
    HMODULE processModules[1024];
    DWORD numBytesWrittenInModuleArray = 0;
    if (!EnumProcessModules(process, processModules, sizeof(processModules), &numBytesWrittenInModuleArray)) {
        OutputDebugStringA("EnumProcessModules failed.\n");
        return 0;
    }

    DWORD numRemoteModules = numBytesWrittenInModuleArray / sizeof(HMODULE);
    CHAR processName[MAX_PATH];
    if (!GetModuleFileNameExA(process, NULL, processName, MAX_PATH)) { // Use ANSI version
        OutputDebugStringA("GetModuleFileNameExA failed.\n");
        return 0;
    }
    _strlwr_s(processName, MAX_PATH);

    HMODULE module = nullptr; // An HMODULE is the DLL's base address 

    for (DWORD i = 0; i < numRemoteModules; ++i)
    {
        CHAR moduleName[MAX_PATH];
        if (!GetModuleFileNameExA(process, processModules[i], moduleName, MAX_PATH)) { // Use ANSI version
            continue;
        }

        CHAR absoluteModuleName[MAX_PATH];
        if (!_fullpath(absoluteModuleName, moduleName, MAX_PATH)) {
            continue;
        }
        _strlwr_s(absoluteModuleName, MAX_PATH);

        if (strcmp(processName, absoluteModuleName) == 0)
        {
            module = processModules[i];
            break;
        }
    }

    if (module == nullptr) {
		MessageBoxA(NULL, "Base module not found.", "Error!", MB_OK); 
        OutputDebugStringA("Base module not found.\n");
    }

    // Output base module address
	OutputDebugStringA("Base module address: ");
	OutputDebugStringA(to_string((uint64_t)module).c_str());
    return (uint64_t)module;
}

void* AllocateMemoryNearAddress(void* where) {
    return NULL;
}

void Hook_Function(uint64_t rvaFunction, void* payloadFunction) {
    uint8_t jumpInstruction[5] = { 0xE9, 0x0, 0x0, 0x0, 0x0 };

    uint64_t baseAddress = GetBaseModuleAddress();
    if (baseAddress == 0) {
        MessageBoxA(NULL, "Failed to get base module address.", "Error!", MB_OK);
        return;
    }

    uint64_t originalFunction = baseAddress + rvaFunction;

    char buffer[256];
    sprintf_s(buffer, sizeof(buffer), "Original function address: 0x%llX\n", originalFunction);
    OutputDebugStringA(buffer);

    // Calculate the relative offset correctly using 64-bit arithmetic
    int64_t distance = static_cast<int64_t>((uintptr_t)payloadFunction - (originalFunction + sizeof(jumpInstruction)));
    if (distance < INT32_MIN || distance > INT32_MAX) {
        MessageBoxA(NULL, "Jump offset out of 32-bit range.", "Error!", MB_OK);
        OutputDebugStringA("Jump offset out of 32-bit range.\n");
		// Print both addresses and distance
		sprintf_s(buffer, sizeof(buffer), "Original function address: 0x%llX\n", originalFunction);
		OutputDebugStringA(buffer);
		sprintf_s(buffer, sizeof(buffer), "Payload function address: 0x%llX\n", (uintptr_t)payloadFunction);
		OutputDebugStringA(buffer);
		sprintf_s(buffer, sizeof(buffer), "Distance: %lld\n", distance);
		OutputDebugStringA(buffer);
        return;
    }

    int32_t jumpOffset = static_cast<int32_t>(distance);

    // Copy the 32-bit offset into the jump instruction
    memcpy(&jumpInstruction[1], &jumpOffset, sizeof(jumpOffset));

    OutputDebugStringA("Hooking initiated...\n");

    // Change the memory protection to allow writing
    DWORD oldProtect;
    if (!VirtualProtect((LPVOID)originalFunction, sizeof(jumpInstruction), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        OutputDebugStringA("VirtualProtect failed.\n");
        return;
    }

    // Write the jump instruction to the original function
    memcpy((LPVOID)originalFunction, jumpInstruction, sizeof(jumpInstruction));

    // Flush instruction cache to ensure CPU sees the changes
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)originalFunction, sizeof(jumpInstruction));

    // Restore the original protection
    VirtualProtect((LPVOID)originalFunction, sizeof(jumpInstruction), oldProtect, &oldProtect);

    OutputDebugStringA("Hook installed successfully.\n");
}


int substract(int a, int b) {
    return a - b;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    uint64_t addFunctionRVA = 0x12280;

    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        Hook_Function(addFunctionRVA, substract);
		
    }

    return TRUE;
}

