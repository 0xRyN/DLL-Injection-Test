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

void* AllocatePageNearAddress(void* targetAddr)
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    const uint64_t PAGE_SIZE = sysInfo.dwPageSize;

    uint64_t startAddr = (uint64_t(targetAddr) & ~(PAGE_SIZE - 1)); //round down to nearest page boundary
    uint64_t minAddr = min(startAddr - 0x7FFFFF00, (uint64_t)sysInfo.lpMinimumApplicationAddress);
    uint64_t maxAddr = max(startAddr + 0x7FFFFF00, (uint64_t)sysInfo.lpMaximumApplicationAddress);

    uint64_t startPage = (startAddr - (startAddr % PAGE_SIZE));

    uint64_t pageOffset = 1;
    while (1)
    {
        uint64_t byteOffset = pageOffset * PAGE_SIZE;
        uint64_t highAddr = startPage + byteOffset;
        uint64_t lowAddr = (startPage > byteOffset) ? startPage - byteOffset : 0;

        bool needsExit = highAddr > maxAddr && lowAddr < minAddr;

        if (highAddr < maxAddr)
        {
            void* outAddr = VirtualAlloc((void*)highAddr, PAGE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (outAddr)
                return outAddr;
        }

        if (lowAddr > minAddr)
        {
            void* outAddr = VirtualAlloc((void*)lowAddr, PAGE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (outAddr != nullptr)
                return outAddr;
        }

        pageOffset++;

        if (needsExit)
        {
            break;
        }
    }

    return nullptr;
}

void WriteAbsoluteJump64(void* absJumpMemory, void* addrToJumpTo)
{
    uint8_t absJumpInstructions[] = { 0x49, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x41, 0xFF, 0xE2 };

    uint64_t addrToJumpTo64 = (uint64_t)addrToJumpTo;
    memcpy(&absJumpInstructions[2], &addrToJumpTo64, sizeof(addrToJumpTo64));
    memcpy(absJumpMemory, absJumpInstructions, sizeof(absJumpInstructions));
}


void Hook_Function(uint64_t rvaFunction, void* payloadFunction) {
    uint64_t baseAddress = GetBaseModuleAddress();
    if (baseAddress == 0) {
        MessageBoxA(NULL, "Failed to get base module address.", "Error!", MB_OK);
        return;
    }

    uint64_t originalFunction = baseAddress + rvaFunction;

    // Relay function, close to original function, jumps to payload
    void* relayFunction = AllocatePageNearAddress((void*)originalFunction);

    WriteAbsoluteJump64(relayFunction, payloadFunction);

    // Original function jumps to relay function
    uint8_t jumpInstruction[5] = { 0xE9, 0x0, 0x0, 0x0, 0x0 };

    uint64_t jumpOffset = (uint64_t)relayFunction - ((uint64_t) originalFunction + sizeof(jumpInstruction));

    // Copy the 32-bit offset into the jump instruction
    memcpy(&jumpInstruction[1], &jumpOffset, sizeof(jumpOffset));

    OutputDebugStringA("Hooking initiated...\n");

    // Change the memory protection to allow writing
    DWORD oldProtect;
    if (!VirtualProtect((LPVOID)originalFunction, sizeof(jumpInstruction), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        OutputDebugStringA("VirtualProtect failed.\n");
        return;
    }

    // Write the jump instruction -> relay to the original function
    memcpy((LPVOID)originalFunction, jumpInstruction, sizeof(jumpInstruction));

    // Restore the original protection
    VirtualProtect((LPVOID)originalFunction, sizeof(jumpInstruction), oldProtect, &oldProtect);

    OutputDebugStringA("Hook installed successfully.\n");
}


int substract(int a, int b) {
    return a - b;
}

void HookFunction_VMT(uint64_t vmtOffset, int vmtFunctionIndex, void* payloadFunc) {
    uint64_t baseAddress = GetBaseModuleAddress();
    if (baseAddress == 0) {
        MessageBoxA(NULL, "Failed to get base module address.", "Error!", MB_OK);
        return;
    }

    uint64_t vmtAddress = baseAddress + vmtOffset;

    char buf[100];
    sprintf_s(buf, sizeof(buf), "vmtAddress: %p\n", (void*)vmtAddress);
    OutputDebugStringA(buf);

    char buf2[100];
    sprintf_s(buf2, sizeof(buf2), "fnAddress: %p\n", payloadFunc);
    OutputDebugStringA(buf2);

    DWORD oldProtect;
    if (!VirtualProtect((LPVOID)vmtAddress, sizeof(vmtAddress), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        OutputDebugStringA("VirtualProtect failed.\n");
        return;
    }

    // Method 1: Copy pointer address (which is the address pointed by the pointer)
    // memcpy((void*) vmtAddress, &payloadFunc, sizeof(payloadFunc));

    // Method 2: Dereference and assign the pointer
    uint64_t* entry = (uint64_t*)vmtAddress;
    *entry = (uint64_t)payloadFunc;

    OutputDebugStringA("VMT Hook installed successfully.\n");
}

void work() {
    std::cout << "I am hacker! I steal your job hahaha >:)" << std::endl;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    uint64_t addFunctionRVA = 0x12280;
    uint64_t vmtOffset = 0x1bce0;

    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        HookFunction_VMT(vmtOffset, 0, work);
		
    }

    return TRUE;
}

