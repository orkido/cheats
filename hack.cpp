#include <BlackBone/Process/Process.h>
#include <BlackBone/Patterns/PatternSearch.h>
#include <BlackBone/Process/RPC/RemoteFunction.hpp>
#include <BlackBone/Syscalls/Syscall.h>
#include <iostream>

#include <QMainWindow>

using namespace blackbone;

int hack() {
    // List all process PIDs matching name
    auto pids = Process::EnumByName(L"SpeedRunners.exe");

    // Attach to a process
    Process SpeedRunner;
    if (pids.empty() || !NT_SUCCESS(SpeedRunner.Attach(pids.front())))
    {
         (NULL, L"Fail: Didn't find SpeedRunners.exe", L"Fail", 0);
        return false;
    }

    // Pattern scanning
    if (Process process; NT_SUCCESS(process.Attach(GetCurrentProcessId())))
    {
        PatternSearch ps{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };

        std::vector<ptr_t> results;
        ps.SearchRemoteWhole(process, false, 0, results);
    }

    // Process memory manipulation
    {
        auto& memory = SpeedRunner.memory();
        auto mainMod = SpeedRunner.modules().GetMainModule();

        //
        // Read memory
        //
        // Method 3
        auto [status, dosHeader] = memory.Read<IMAGE_DOS_HEADER>(mainMod->baseAddress);

        // Change memory protection
        if (NT_SUCCESS(memory.Protect(mainMod->baseAddress, sizeof(dosHeader), PAGE_READWRITE)))
        {
            //
            // Write memory
            //

            // Method 1
            memory.Write(mainMod->baseAddress, dosHeader);
        }

        // Allocate memory
        auto [block, status2] = memory.Allocate(0x1000, PAGE_EXECUTE_READWRITE);
        if (NT_SUCCESS(status2)) {
            // Write into memory block
            block->Write(0x10, 12.0);

            // Read from memory block
            [[maybe_unused]] auto dval = block->Read<double>(0x10, 0.0);
        }

        // Enumerate regions
        auto regions = memory.EnumRegions();
    }

    // JIT Assembler
    if (auto asmPtr = AsmFactory::GetAssembler()) {
        auto& a = *asmPtr;

        a.GenPrologue();
        a->add(a->zcx, a->zdx);
        a->mov(a->zax, a->zcx);
        a.GenEpilogue();

        auto func = reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t, uintptr_t)>(a->make());
        [[maybe_unused]] uintptr_t r = func(10, 5);
    }

    return 0;
}
