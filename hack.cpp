#include <BlackBone/Process/Process.h>
#include <BlackBone/Patterns/PatternSearch.h>
#include <BlackBone/Process/RPC/RemoteFunction.hpp>
#include <BlackBone/Syscalls/Syscall.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <processthreadsapi.h>

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
        0xD9, 0x9E, 0x3C, 0x02, 0x00, 0x00
    }
);

class HackState
{
public:
    bool hack(float boost_factor);
    ~HackState();

private:
    bool attachProcess();
    bool searchPattern();
    bool allocMemory();
    bool writePayload();
    bool resetProcessState();


    std::optional<MemBlock> payload_mem = std::nullopt;
    std::unique_ptr<Process> process;
    std::optional<MemBlock> boost_setter = std::nullopt;
    bool boost_setter_overwritten = false;
    DWORD pid = -1;

    float boost_factor;
};

HackState::~HackState() {
    resetProcessState();
    payload_mem->Free();
    payload_mem.reset();
    boost_setter.reset();
}

bool HackState::attachProcess() {
    // Attach to SpeedRunners.exe (excluding self)
    auto pids = Process::EnumByName(L"SpeedRunners.exe");
    // This process maybe have the same name -> remove it from the list
    pids.erase(std::remove(pids.begin(), pids.end(), GetCurrentProcessId()), pids.end());

    if (pids.size() != 1) {
        showMessageBox("Error", ("Found " + std::to_string(pids.size()) + " SpeedRunners.exe processes, expected 1!").c_str());
        return false;
    } else {
        // Assume: same process name and pid -> same process instance
        if (pid != pids.front() /*|| !process->valid()*/) {
            payload_mem.reset();   // set to empty
            boost_setter.reset();  // set to empty
            process.reset(new Process());       // replace with new Process object
            boost_setter_overwritten = false;
        } else {
            // Only attach again
        }

        pid = pids.front();
        if (NT_SUCCESS(process->Attach(pid))) {
            return true;
        } else {
            showMessageBox("Process attach", "Error: SpeedRunners.exe process not found!");
            return false;
        }
    }
}

bool HackState::searchPattern() {
    if (boost_setter)
        return true;

    PatternSearch ps(hook_code);

    std::vector<ptr_t> results;
    ps.SearchRemoteWhole(*process, false, 0, results);
    if (results.size() != 1) {
        showMessageBox("Error", ("Pattern matching failed: " + std::to_string(results.size()) + " results found, expected 1!").c_str());
        return false;
    }
    boost_setter = MemBlock(&process->memory(), results.front(), false);
    return true;
}

bool HackState::allocMemory() {
    if (payload_mem)
        return true;

    auto& memory = process->memory();

    // Allocate memory
    auto [alloc_status, _mem_payload] = memory.Allocate(0x1000, PAGE_EXECUTE_READWRITE);

    if (NT_SUCCESS(alloc_status)) {
        payload_mem = std::move(_mem_payload);
        return true;
    } else {
        std::cout << "Memory allocation failed: Status: " << alloc_status << "!" << std::endl;
        return false;
    }
}

bool HackState::writePayload() {
    //process->modules().Inject(L"inject.dll");
    // JIT Assembler
    auto asmPtr = AsmFactory::GetAssembler(process->core().isWow64());
    auto asmPtr2 = AsmFactory::GetAssembler(process->core().isWow64());
    if (asmPtr && asmPtr2) {
        auto& a = *asmPtr;
        auto& a2 = *asmPtr2;
        a->setBaseAddress(boost_setter->ptr());
        a2->setBaseAddress(payload_mem->ptr());

        asmjit::Label skipConditional(*a2.assembler());
        asmjit::Label noskipConditional(*a2.assembler());
        asmjit::Label continueCode(*a.assembler());

        //a->jmp(noskipConditional);
        a->jmp(payload_mem->ptr() + 0x8);
        while (a->getCodeSize() < 9)
            a->nop();
        //a->jmp(skipConditional);
        a->jmp(payload_mem->ptr() + 0x20);
        while (a->getCodeSize() < 18)
            a->nop();
        a->bind(continueCode);

        // Conditional code begins at offset 0x08, unconditional (always executed) code at offset 0x20
        asmjit::Label intermediateFactor(*a2.assembler());
        a2->bind(intermediateFactor);
        while (a2->getCodeSize() < 0x8)
            a2->nop();

        // Branch 1:
        a2->bind(noskipConditional);
        a2->push(asmjit::x86::ecx);
        a2->mov(asmjit::x86::ecx, 0);
        while (a2->getCodeSize() < 0x20)
            a2->nop();

        // Branch 2:
        a2->bind(skipConditional);
        asmjit::Label noReduce(*a2.assembler());
        a2->fld(asmjit::x86::dword_ptr(asmjit::x86::ebp, 8));      // new value
        a2->fcomp(asmjit::x86::dword_ptr(asmjit::x86::esi, 0x23C)); // old value
        a2->fstsw(asmjit::x86::ax);
        a2->sahf();
        a2->jl(noReduce);
        // here: new value < old value
        a2->fld(asmjit::x86::dword_ptr(asmjit::x86::ebp, 8));      // ST(0) = new value
        a2->fsub(asmjit::x86::dword_ptr(asmjit::x86::esi, 0x23C)); // ST(0) = new value - old value
        a2->fmul(asmjit::x86::dword_ptr(intermediateFactor));      // ST(0) = (new value - old value) * factor
        a2->fadd(asmjit::x86::dword_ptr(asmjit::x86::esi, 0x23C)); // ST(0) = old value + (new value - old value) * factor = old value - (old value - new value) * factor
        a2->fstp(asmjit::x86::dword_ptr(asmjit::x86::ebp, 8));     // Overwrite new value with ST(0)

        a2->bind(noReduce);
        // Execute overwriten code
        asmjit::Label skipConditinalRepeat(*a2.assembler());
        a2->test(asmjit::x86::ecx, asmjit::x86::ecx);
        a2->jnz(skipConditinalRepeat);
        // here: conditional branch was taken, reverse push(ecx) and do additional store operation
        a2->pop(asmjit::x86::ecx);
        a2->fld(asmjit::x86::dword_ptr(asmjit::x86::ebp, 8));
        a2->fstp(asmjit::x86::dword_ptr(asmjit::x86::eax, 0x23C));

        a2->bind(skipConditinalRepeat);
        a2->fld(asmjit::x86::dword_ptr(asmjit::x86::ebp, 8));
        a2->fstp(asmjit::x86::dword_ptr(asmjit::x86::esi, 0x23C));
        //a2->jmp(continueCode);
        a2->jmp(boost_setter->ptr() + 18);

        size_t code_size = a->getCodeSize();
        auto code = a->make();
        size_t code2_size = a2->getCodeSize();
        void* code2 = a2->make();


        // TODO: Return statements here do not free "code" and "code2"
        if (NTSTATUS status = payload_mem->Write(0, code2_size, code2); !NT_SUCCESS(status)) {
            std::cout << "Block writing 1 failed! Status: " << status << std::endl;
            return false;
        }
        if (NTSTATUS status = payload_mem->Write(0, static_cast<float>(boost_factor)); !NT_SUCCESS(status)) {
            std::cout << "Block writing 2 failed! Status: " << status << std::endl;
            return false;
        }
        if (NTSTATUS status = boost_setter->Write(0, code_size, code); !NT_SUCCESS(status)) {
            std::cout << "Block writing 3 failed! Status: " << status << std::endl;
            return false;
        }
        boost_setter_overwritten = true;

        a->getRuntime()->release(code);
        a2->getRuntime()->release(code2);
        
        return true;
    } else {
        std::cout << "Assembler init failed!" << std::endl;
        return false;
    }
}

bool HackState::resetProcessState() {
    // Attach process and THEN check if *this* process instance was patched
    if (attachProcess() && boost_setter_overwritten) {
        boost_setter->Write(0, hook_code);
        process->Detach();
        return true;
    }
    return false;
}

bool HackState::hack(float boost_factor) {
    struct MyException : public std::exception {
        const char* what() const throw () {
            return "C++ Exception";
        }
    };

    this->boost_factor = boost_factor;
    try {
        if (!attachProcess())
            throw MyException();
        if (!searchPattern())
            throw MyException();
        if (!allocMemory())
            throw MyException();
        if (!writePayload())
            throw MyException();
    } catch (const MyException&) {
        return false;
    }
    // Always detach from process
    process->Detach();

    return true;
}

bool hack(float boost_factor) {
    static HackState hackstate;

    if (hackstate.hack(boost_factor)) {
        showMessageBox("Ok", "Succeed!");
        return true;
    } else {
        return false;
    }
}
