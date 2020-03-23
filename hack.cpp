#include <BlackBone/Process/Process.h>
#include <BlackBone/Patterns/PatternSearch.h>
#include <BlackBone/Process/RPC/RemoteFunction.hpp>
#include <BlackBone/Syscalls/Syscall.h>
#include <iostream>
#include <algorithm>
#include <processthreadsapi.h>

using namespace blackbone;

void showMessageBox(const char* title, const char* text);

bool hack() {
    Process SpeedRunner;

    // Attach to SpeedRunners.exe (excluding self)
    {
        // List all process PIDs matching name
        auto pids = Process::EnumByName(L"SpeedRunners.exe");
        // This process maybe have the same name -> remove it from the list
        pids.erase(std::remove(pids.begin(), pids.end(), GetCurrentProcessId()), pids.end());

        if (pids.empty()) {
            showMessageBox("Process not found", "Error: SpeedRunners.exe process not found!");
            return false;
        } else if (pids.size() > 1) {
            showMessageBox("Warning", "Warning: More than one SpeedRunners.exe process found!");
        }

        if (!NT_SUCCESS(SpeedRunner.Attach(pids.front()))) {
            showMessageBox("Process attach", "Error: SpeedRunners.exe process not found!");
        }
    }

    // Search for function pattern
    ptr_t boost_setter;
    {
        // D9 45 08 - fld dword ptr[ebp + 08]
        // D9 98 3C020000 - fstp dword ptr[eax + 0000023C]
        // D9 45 08 - fld dword ptr[ebp + 08]
        // D9 9E 3C020000 - fstp dword ptr[esi + 0000023C]

        PatternSearch ps{
            0xD9, 0x45, 0x08,
            0xD9, 0x98, 0x3C, 0x02, 0x00, 0x00,
            0xD9, 0x45, 0x08,
            0xD9, 0x9E, 0x3C, 0x02, 0x00, 0x00
        };

        std::vector<ptr_t> results;
        ps.SearchRemoteWhole(SpeedRunner, false, 0, results);
        if (results.size() != 1) {
            std::cout << "Pattern matching failed: " << results.size() << " results found, expected 1!" << std::endl;
            return false;
        }
        boost_setter = results.front();
    }

    auto& memory = SpeedRunner.memory();

    // Allocate memory
    auto [alloc_status, block] = memory.Allocate(0x1000, PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS(alloc_status)) {
        std::cout << "Memory allocation failed: " << alloc_status << "!" << std::endl;
        return false;
    }

    // JIT Assembler
    auto asmPtr = AsmFactory::GetAssembler(true);
    auto asmPtr2 = AsmFactory::GetAssembler(true);
    if (asmPtr && asmPtr2) {
        auto& a = *asmPtr;
        auto& a2 = *asmPtr2;
        a->setBaseAddress(boost_setter);
        a2->setBaseAddress(block->ptr());
        
        asmjit::Label skipConditional(*a.assembler());
        asmjit::Label noskipConditional(*a.assembler());
        asmjit::Label continueCode(*a2.assembler());

        //a->jmp(noskipConditional);
        a->jmp(block->ptr() + 0x8);
        while (a->getCodeSize() < 9)
            a->nop();
        //a->jmp(skipConditional);
        a->jmp(block->ptr() + 0x20);
        while (a->getCodeSize() < 18)
            a->nop();
        a->bind(continueCode);

        // Conditional code begins at offset 0x08, unconditional (always executed) code at offset 0x20
        asmjit::Label intermediateFactor(*a2.assembler());
        a2->bind(intermediateFactor);
        while (a2->getCodeSize() < 0x8)
            a2->nop();

        asmjit::Error err = a2->getError();
        // Branch 1:
        a2->bind(noskipConditional);
        a2->push(asmjit::x86::ecx);
        a2->mov(asmjit::x86::ecx, 0);
        while (a2->getCodeSize() < 0x20)
            a2->nop();
        asmjit::Error er1r = a2->getError();
        // Branch 2:
        a2->bind(skipConditional);
        asmjit::Label noReduce(*a2.assembler());
        a2->fld(asmjit::x86::dword_ptr(asmjit::x86::ebp, 8));      // new value
        a2->fcom(asmjit::x86::dword_ptr(asmjit::x86::esi, 0x23C)); // old value
        a2->fstsw(asmjit::x86::ax);
        a2->sahf();
        a2->jge(noReduce);
        // here: new value < old value
        a2->fsub(asmjit::x86::dword_ptr(asmjit::x86::esi, 0x23C)); // ST(0) = new value - old value
        a2->fmul(asmjit::x86::dword_ptr(intermediateFactor));      // ST(0) = (new value - old value) * factor
        a2->fadd(asmjit::x86::dword_ptr(asmjit::x86::esi, 0x23C)); // ST(0) = old value + (new value - old value) * factor = old value - (old value - new value) * factor
        a2->fstp(asmjit::x86::dword_ptr(asmjit::x86::ebp, 8));     // Overwrite new value with ST(0)
        asmjit::Error er2r = a2->getError();
        a2->bind(noReduce);
        // Execute overwriten code
        asmjit::Label skipConditinalRepeat(*a2.assembler());
        a2->test(asmjit::x86::ecx, asmjit::x86::ecx);
        a2->jnz(skipConditinalRepeat);
        // here: conditional branch was taken, reverse push(ecx) and do additional store operation
        a2->pop(asmjit::x86::ecx);
        a2->fld(asmjit::x86::dword_ptr(asmjit::x86::ebp, 8));
        a2->fcom(asmjit::x86::dword_ptr(asmjit::x86::eax, 0x23C));

        a2->bind(skipConditinalRepeat);
        a2->fld(asmjit::x86::dword_ptr(asmjit::x86::ebp, 8));
        a2->fcom(asmjit::x86::dword_ptr(asmjit::x86::esi, 0x23C));
        //a2->jmp(continueCode);
        a2->jmp(boost_setter + 18);

        while (a2->getCodeSize() % 4 != 0)
            a2->nop();

        size_t func_size = a->getCodeSize();
        auto func = a->make();
        size_t func2_size = a2->getCodeSize();
        void* func2 = a2->make();
        uint32_t* func2_ptr = static_cast<uint32_t*>(func2);
        
        for (uint64_t i = 0; i < func2_size / sizeof(uint32_t); ++i) {
            uint32_t val = func2_ptr[i];
            if (NTSTATUS status = block->Write(i * 4, val ); !NT_SUCCESS(status))
                std::cout << "Block writing 1 failed! Status: " << status << std::endl;
        }
        if (NTSTATUS status = block->Write(0, 0.1f); !NT_SUCCESS(status))
            std::cout << "Block writing 2 failed! Status: " << status << std::endl;
        // memory.Write(boost_setter, func_size, func);
    } else {
        std::cout << "Assembler init failed!" << std::endl;
        return false;
    }
    SpeedRunner.Detach();

    return true;
}
