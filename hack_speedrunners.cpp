#include <BlackBone/Process/Process.h>
#include <BlackBone/Patterns/PatternSearch.h>
#include <BlackBone/Process/RPC/RemoteFunction.hpp>
#include <BlackBone/Syscalls/Syscall.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <processthreadsapi.h>
#include "inject_speedrunners.h"

using namespace blackbone;
void showMessageBox(const char* title, const char* text);


// D9 45 08 - fld dword ptr[ebp + 08]
// D9 98 3C020000 - fstp dword ptr[eax + 0000023C]
// D9 45 08 - fld dword ptr[ebp + 08]
// D9 9E 3C020000 - fstp dword ptr[esi + 0000023C]
std::vector<uint8_t> hook_code(
    {
        0xD9, 0x45, 0x08,
        0xD9, 0x98, 0x3C, 0x02, 0x00, 0x00,
        0xD9, 0x45, 0x08,
        0xD9, 0x9E, 0x3C, 0x02, 0x00, 0x00,
    }
);
#define PATTERN_WILDCARD 0x0
std::vector<uint8_t> hook_code_replaced(
    {
        PATTERN_WILDCARD, PATTERN_WILDCARD, PATTERN_WILDCARD, PATTERN_WILDCARD, PATTERN_WILDCARD,
        0x0f, 0x0b, // ud
        0xfc,       // cld
        0xaa,       // stosb

        PATTERN_WILDCARD, PATTERN_WILDCARD, PATTERN_WILDCARD, PATTERN_WILDCARD, PATTERN_WILDCARD,
        0x0f, 0xa2, // cpuid
        0x0f, 0x0b, // ud
    }
);


// D9 45 08 - fld dword ptr[ebp + 08]
// D9 98 3C020000 - fstp dword ptr[eax + 0000023C]
// 90 - nop
// D9 45 08 - fld dword ptr[ebp + 08]
// D9 9E 3C020000 - fstp dword ptr[esi + 0000023C]
std::vector<uint8_t> hook_code_insert(
    {
        0xD9, 0x45, 0x08,
        0xD9, 0x98, 0x3C, 0x02, 0x00, 0x00,
        0x90,
        0xD9, 0x45, 0x08,
        0xD9, 0x9E, 0x3C, 0x02, 0x00, 0x00,
    }
);

struct HackStateException : public std::exception {
    std::string caption;
    std::string text;

    HackStateException(const char* caption, const char* text) {
        this->caption = caption;
        this->text = text;
    }

    HackStateException(std::string caption, std::string text) {
        this->caption = caption;
        this->text = text;
    }
    /*const char* what() const throw () {
        return "C++ Exception";
    }*/
};

class HackState
{
public:
    bool hack(float boost_factor);
    ~HackState();

private:
    bool attachProcess();
    MemBlock& getBoostSetter();
    MemBlock& getAllocMemory();
    ModuleDataPtr getDll(bool inject);
    bool writePayload();
    bool resetProcessCode();


    std::optional<MemBlock> _payload_mem = std::nullopt;
    std::unique_ptr<Process> process;
    std::optional<MemBlock> _boost_setter = std::nullopt;
    bool boost_setter_overwritten = false;
    DWORD pid = -1;

    struct DllConfig dll_config;
};

HackState::~HackState() {
    resetProcessCode();
}

bool HackState::attachProcess() {
    // Attach to SpeedRunners.exe (excluding self)
    auto pids = Process::EnumByName(L"SpeedRunners.exe");
    // This process maybe have the same name -> remove it from the list
    pids.erase(std::remove(pids.begin(), pids.end(), GetCurrentProcessId()), pids.end());

    if (pids.size() != 1) {
        throw HackStateException("Error", "Found " + std::to_string(pids.size()) + " SpeedRunners.exe processes, expected 1!");
    } else {
        // Assume: same process name and pid -> same process instance
        if (pid != pids.front() /*|| !process->valid()*/) {
            _payload_mem.reset();  // set to empty, do not free old location, because process instance changed
            _boost_setter.reset(); // set to empty
            process.reset();       // replace with new Process object, but first delete old object
            process.reset(new Process());
            boost_setter_overwritten = false;
        } else {
            // Only attach again
        }

        pid = pids.front();
        if (NT_SUCCESS(process->Attach(pid))) {
            return true;
        } else {
            throw HackStateException("Process attach", "Error: Could not attach to SpeedRunners.exe process!");
        }
    }
}

MemBlock& HackState::getBoostSetter() {
    if (!_boost_setter) {
        {
            PatternSearch ps(hook_code);

            std::vector<ptr_t> results;
            ps.SearchRemoteWhole(*process, false, 0, results);
            if (results.size() == 1) {
                _boost_setter = MemBlock(&process->memory(), results.front(), false);
                boost_setter_overwritten = false;
                return _boost_setter.value();
            }
            else if (results.size() > 1) {
                throw HackStateException("Error", "Original pattern matching failed: " + std::to_string(results.size()) + " results found, expected 1!");
            } // ignore results.size() == 0
        }

        
        std::vector<ptr_t> results;
        PatternSearch ps(hook_code_replaced);
        ps.SearchRemoteWhole(*process, true, PATTERN_WILDCARD, results);
        if (results.size() == 1) {
            _boost_setter = MemBlock(&process->memory(), results.front(), false);
            boost_setter_overwritten = true;
            return _boost_setter.value();
        } else if (results.size() > 1) {
            throw HackStateException("Error", "Patched pattern matching failed: " + std::to_string(results.size()) + " results found, expected 1!");
        } else {
            throw HackStateException("Error", "Patched and original pattern matching failed: " + std::to_string(0) + " results found, expected 1!");
        }
    } else {
        return _boost_setter.value();
    }
}

MemBlock& HackState::getAllocMemory() {
    if (!_payload_mem) {
        PatternSearch ps(hook_code_insert);

        ptr_t result_addr_mask = ~0xFFF;
        std::vector<ptr_t> results;
        ps.SearchRemoteWhole(*process, false, 0, results);
        if (results.size() == 1) {
            _payload_mem = MemBlock(&process->memory(), result_addr_mask & results.front(), false);
            return _payload_mem.value();
        } else if (results.size() > 1) {
            throw HackStateException("Error", "Inserted code pattern matching failed: " + std::to_string(results.size()) + " results found, expected 1!");
        }

        auto& memory = process->memory();

        // Allocate memory
        auto [alloc_status, _mem_payload] = memory.Allocate(0x1000, PAGE_EXECUTE_READWRITE, 0, false);

        if (NT_SUCCESS(alloc_status)) {
            _payload_mem = std::move(_mem_payload);
            return _payload_mem.value();
        } else {
            throw HackStateException("Error", "Memory allocation failed: Status: " + std::to_string(alloc_status) + "!");
        }
    } else {
        return _payload_mem.value();
    }
}

ModuleDataPtr HackState::getDll(bool inject) {
    // Inject or search custom dll
    auto& modules = process->modules();

    auto speedrunners_inject = modules.GetModule(L"speedrunners_inject.dll");
    if (!speedrunners_inject && inject) {
        std::wstring dll_dir = Utils::GetExeDirectory() + L"\\speedrunners_inject.dll";
        auto result = modules.Inject(dll_dir);
        if (!result.success()) {
            throw HackStateException("Error", "Injection of speedrunners_inject.dll failed!");
        } else {
            speedrunners_inject = result.result();
        }
    }
    return speedrunners_inject;
}

bool HackState::writePayload() {
    ptr_t dll_callback;
    {
        auto& modules = process->modules();
        auto speedrunners_inject = getDll(true);

        auto result = modules.GetExport(speedrunners_inject, "_dll_callback@12");
        if (!result.success()) {
            throw HackStateException("Error", "Export symbol _dll_callback@12 not found!");
        }
        dll_callback = result.result().procAddress;
    }

    // JIT Assembler
#define code_offset (sizeof(dll_config) + 0x10)
#define code_offset2 (code_offset + 0x10)
    auto asmPtr = AsmFactory::GetAssembler(process->core().isWow64());
    auto asmPtr2 = AsmFactory::GetAssembler(process->core().isWow64());
    if (asmPtr && asmPtr2) {
        auto& a = *asmPtr;
        auto& a2 = *asmPtr2;
        a->setBaseAddress(getBoostSetter().ptr());
        a2->setBaseAddress(getAllocMemory().ptr());

        asmjit::Label skipConditional(*a2.assembler());
        asmjit::Label noskipConditional(*a2.assembler());
        asmjit::Label continueCode(*a.assembler());

        //a->jmp(noskipConditional);
        a->jmp(getAllocMemory().ptr() + code_offset);
        assert(a->getCodeSize() == 5);
        // signature instructions to find this location
        a->ud2();
        a->cld();
        a->stosb();
        assert(a->getCodeSize() == 9);

        //a->jmp(skipConditional);
        a->jmp(getAllocMemory().ptr() + code_offset2);
        assert(a->getCodeSize() == 14);
        // signature instructions to find this location
        a->cpuid();
        a->ud2();
        assert(a->getCodeSize() == 18);
        a->bind(continueCode);

        // Conditional code begins at offset 0x08, unconditional (always executed) code at offset 0x20
        asmjit::Label intermediateFactor(*a2.assembler());
        a2->bind(intermediateFactor);
        while (a2->getCodeSize() < code_offset)
            a2->nop();

        // Branch 1:
        a2->bind(noskipConditional);
        a2->push(asmjit::x86::ecx);
        a2->mov(asmjit::x86::ecx, 0);
        while (a2->getCodeSize() < code_offset2)
            a2->nop();

        // Branch 2:
        a2->bind(skipConditional);


        // Store registers
        a2->sub(asmjit::x86::esp, 108);
        a2->fsave(asmjit::x86::ptr(asmjit::x86::esp, 0, 108));
        a2->pusha();

        // Call DLL function
        a2->lea(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::ebp, 8));
        a2->push(asmjit::x86::eax);
        a2->lea(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::esi));
        a2->push(asmjit::x86::eax);
        a2->lea(asmjit::x86::eax, asmjit::x86::dword_ptr(intermediateFactor));
        a2->push(asmjit::x86::eax);
        a2->call(dll_callback);

        // Restore registers
        a2->popa();
        a2->frstor(asmjit::x86::ptr(asmjit::x86::esp, 0, 108));
        a2->add(asmjit::x86::esp, 108);


        // Execute overwritten code
        asmjit::Label skipConditinalRepeat(*a2.assembler());
        a2->test(asmjit::x86::ecx, asmjit::x86::ecx);
        a2->jnz(skipConditinalRepeat);
        // here: conditional branch was taken, reverse push(ecx) and do additional store operation
        a2->pop(asmjit::x86::ecx);
        a2->fld(asmjit::x86::dword_ptr(asmjit::x86::ebp, 8));
        a2->fstp(asmjit::x86::dword_ptr(asmjit::x86::eax, 0x23C));
        a2->nop();  // Make sure this code does not match the search pattern

        a2->bind(skipConditinalRepeat);
        a2->fld(asmjit::x86::dword_ptr(asmjit::x86::ebp, 8));
        a2->fstp(asmjit::x86::dword_ptr(asmjit::x86::esi, 0x23C));
        //a2->jmp(continueCode);
        a2->jmp(getBoostSetter().ptr() + 18);

        size_t code_size = a->getCodeSize();
        auto code = a->make();
        size_t code2_size = a2->getCodeSize();
        void* code2 = a2->make();


        // TODO: Return statements here do not free "code" and "code2"
        if (NTSTATUS status = getAllocMemory().Write(0, code2_size, code2); !NT_SUCCESS(status)) {
            throw HackStateException("Error", "Block writing 1 failed! Status: " + std::to_string(status) + "!");
            return false;
        }
        if (NTSTATUS status = getAllocMemory().Write(0, dll_config); !NT_SUCCESS(status)) {
            throw HackStateException("Error", "Block writing 2 failed! Status: " + std::to_string(status) + "!");
            return false;
        }
        if (NTSTATUS status = getBoostSetter().Write(0, code_size, code); !NT_SUCCESS(status)) {
            throw HackStateException("Error", "Block writing 3 failed! Status: " + std::to_string(status) + "!");
            return false;
        }
        boost_setter_overwritten = true;

        a->getRuntime()->release(code);
        a2->getRuntime()->release(code2);
        
        return true;
    } else {
        throw HackStateException("Error", "Assembler init failed!");
        return false;
    }
}

bool HackState::resetProcessCode() {
    try {
        // Attach process and THEN check if *this* process instance was patched
        if (attachProcess()) {
            if (boost_setter_overwritten) {
                MemBlock& boost_setter = getBoostSetter();
                boost_setter.Write(0, hook_code.size(), hook_code.data());
                boost_setter_overwritten = false;
            }
            _boost_setter.reset();

            if (_payload_mem) {
                _payload_mem.value().Free();
                _payload_mem.reset();
            }
            // Module unloading crashes process
            /*if (auto speedrunners_inject = getDll(false); speedrunners_inject) {
                auto& modules = process->modules();

                modules.Unload(speedrunners_inject);
            }*/

            process->Detach();
            return true;
        }
    } catch (const HackStateException& e) {
        showMessageBox(e.caption.c_str(), e.text.c_str());
        process->Detach();
        return false;
    }

    return false;
}

bool HackState::hack(float boost_factor) {
    StrCpyW(dll_config.username, L"***REMOVED***");
    dll_config.boost_factor = boost_factor;

    try {
        if (!attachProcess())
            throw HackStateException("", "");
        if (!writePayload())
            throw HackStateException("", "");
    } catch (const HackStateException& e) {
        showMessageBox(e.caption.c_str(), e.text.c_str());
        process->Detach();
        return false;
    }
    // Always detach from process
    process->Detach();
    return true;
}


std::optional<HackState> hackstate;

bool hack(float boost_factor) {
    
    if (!hackstate)
        hackstate.emplace();
    if (hackstate.value().hack(boost_factor)) {
        showMessageBox("Ok", "Succeed!");
        return true;
    } else {
        return false;
    }
}

bool deinit_hack() {
    hackstate.reset();
    return true;
}
