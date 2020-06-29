#include <BlackBone/Process/Process.h>
#include <BlackBone/Patterns/PatternSearch.h>
#include <BlackBone/Process/RPC/RemoteFunction.hpp>
#include <BlackBone/Syscalls/Syscall.h>
#include "inject_civilization.h"
#include <thread>

using namespace blackbone;

void showMessageBox(const char* title, const char* text);

namespace CivilizationVI {

std::vector<uint8_t> hook_code(
    {
        0x42, 0x8D, 0x04, 0x02, 0x49, 0x8B, 0xC9, 0x48,
        0x8D, 0x54, 0x24, 0x30, 0x89, 0x44, 0x24, 0x30,
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
    bool hack(std::vector<std::pair<uintptr_t, uint32_t>>& pointers, int mode, void* selected_address);
    ~HackState();

private:
    bool attachProcess();
    MemBlock getGoldSetter();
    ModuleDataPtr getDll(bool inject);
    bool writePayload();
    void syncDllConfig(bool push);
    bool resetProcessCode();
    bool update_hack_state(std::vector<std::pair<uintptr_t, uint32_t>>& pointers, int mode, void* selected_address);

    std::unique_ptr<Process> process;
    bool gold_adding_overwritten = false;
    DWORD pid = -1;

    struct DllConfig dll_config;
};

HackState::~HackState() {
    resetProcessCode();
}

bool HackState::attachProcess() {
    // Attach to SpeedRunners.exe (excluding self)
    auto pids = Process::EnumByName(L"CivilizationVI.exe");
    // This process maybe have the same name -> remove it from the list
    pids.erase(std::remove(pids.begin(), pids.end(), GetCurrentProcessId()), pids.end());

    if (pids.size() != 1) {
        throw HackStateException("Error", "Found " + std::to_string(pids.size()) + " CivilizationVI.exe processes, expected 1!");
    } else {
        // Assume: same process name and pid -> same process instance
        if (pid != pids.front() /*|| !process->valid()*/) {
            process.reset();       // replace with new Process object, but first delete old object
            process.reset(new Process());
            gold_adding_overwritten = false;
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

MemBlock HackState::getGoldSetter() {
    // Inject or search custom dll
    auto& modules = process->modules();

    auto gamecore_module = modules.GetModule(L"GameCore_Base_FinalRelease.dll");
    ptr_t hook_ptr = gamecore_module->baseAddress + 0x224EE4;

    return MemBlock(&process->memory(), hook_ptr, true);
}

ModuleDataPtr HackState::getDll(bool inject) {
    // Inject or search custom dll
    auto& modules = process->modules();

    auto inject_civilization = modules.GetModule(L"inject_civilization.dll");
    if (!inject_civilization && inject) {
        // const wchar_t* dll_dir = L"C:\\Users\\***REMOVED***\\source\\repos\\speedrunners\\out\\build\\x86-Debug\\speedrunners\\speedrunners_inject.dll";
        std::wstring dll_dir = Utils::GetExeDirectory() + L"\\inject_civilization.dll";
        auto result = modules.Inject(dll_dir);
        if (!result.success()) {
            throw HackStateException("Error", "Injection of inject_civilization.dll failed!");
        } else {
            inject_civilization = result.result();
        }
    }
    return inject_civilization;
}

bool HackState::writePayload() {
    ptr_t dll_callback;
    ptr_t target_dll_config;
    {
        auto& modules = process->modules();
        auto inject_civilization = getDll(true);

        auto result = modules.GetExport(inject_civilization, "dll_callback");
        if (!result.success()) {
            throw HackStateException("Error", "Export symbol dll_callback not found!");
        }
        dll_callback = result.result().procAddress;

        result = modules.GetExport(inject_civilization, "dll_config");
        if (!result.success()) {
            throw HackStateException("Error", "Export symbol dll_config not found!");
        }
        target_dll_config = result.result().procAddress;
    }

    // JIT Assembler
#define code_offset hook_code.size()
    auto asmPtr = AsmFactory::GetAssembler(process->core().isWow64());
    if (asmPtr) {
        auto& a = *asmPtr;
        a->setBaseAddress(getGoldSetter().ptr());

        //a->jmp(noskipConditional);
        a->mov(asmjit::x86::rax, dll_callback);
        a->call(asmjit::x86::rax);
        while (a->getCodeSize() < code_offset)
            a->nop();
        assert(a->getCodeSize() == code_offset);

        size_t code_size = a->getCodeSize();
        auto code = a->make();

        printf("Writing to 0x%llx\n", getGoldSetter().ptr());
        for (int i = 0; i < code_size; ++i) {
            printf("\\x%x", 0xFF & (uint32_t)((uint8_t*)code)[i]);
        }
        printf("\n");

        // TODO: Return statements here do not free "code"
        syncDllConfig(true);
        if (NTSTATUS status = getGoldSetter().Write(0, code_size, code); !NT_SUCCESS(status)) {
            throw HackStateException("Error", "Block writing 2 failed! Status: " + std::to_string(status) + "!");
            return false;
        }
        gold_adding_overwritten = true;

        a->getRuntime()->release(code);

        return true;
    } else {
        throw HackStateException("Error", "Assembler init failed!");
        return false;
    }
}

void HackState::syncDllConfig(bool push) {
    ptr_t target_dll_config;
    {
        auto& modules = process->modules();
        auto inject_civilization = getDll(true);

        auto result = modules.GetExport(inject_civilization, "dll_config");
        if (!result.success()) {
            throw HackStateException("Error", "Export symbol dll_config not found!");
        }
        target_dll_config = result.result().procAddress;
    }

    if (push) {
        if (NTSTATUS status = MemBlock(&process->memory(), target_dll_config, true).Write(0, sizeof(dll_config), &dll_config); !NT_SUCCESS(status)) {
            throw HackStateException("Error", "Block writing dll_config failed! Status: " + std::to_string(status) + "!");
        }
    } else {
        if (NTSTATUS status = MemBlock(&process->memory(), target_dll_config, true).Read(0, sizeof(dll_config), &dll_config); !NT_SUCCESS(status)) {
            throw HackStateException("Error", "Block read dll_config failed! Status: " + std::to_string(status) + "!");
        }
    }
}

bool HackState::resetProcessCode() {
    try {
        // Attach process and THEN check if *this* process instance was patched
        if (attachProcess()) {
            if (gold_adding_overwritten) {
                MemBlock& gold_setter = getGoldSetter();
                gold_setter.Write(0, hook_code.size(), hook_code.data());
                gold_adding_overwritten = false;
            }
            // Module unloading crashes process
            /*if (auto inject_civilization = getDll(false); inject_civilization) {
                auto& modules = process->modules();

                modules.Unload(inject_civilization);
            }*/

            process->Detach();
            return true;
        }
    }
    catch (const HackStateException& e) {
        showMessageBox(e.caption.c_str(), e.text.c_str());
        process->Detach();
        return false;
    }

    return false;
}

bool HackState::hack(std::vector<std::pair<uintptr_t, uint32_t>>& pointers, int mode, void* selected_address) {
    dll_config.overwrite_mode = OverwriteMode::None;
    dll_config.last_mode = OverwriteMode::None;
    dll_config.last_index = 0;
    dll_config.overwrite_pValue = NULL;
    dll_config.overwrite_value = 0;
    dll_config.keep_overwrite_mode = false;
    memset(dll_config.pValue, 0, sizeof(dll_config.pValue));
    memset(dll_config.value, 0, sizeof(dll_config.value));

    try {
        if (!attachProcess())
            throw HackStateException("Failed", "attachProcess()");
        if (!writePayload())
            throw HackStateException("Failed", "writePayload()");
        if (mode != -1 && !update_hack_state(pointers, mode, selected_address))
            throw HackStateException("Failed", "update_hack_state()");
    }
    catch (const HackStateException& e) {
        showMessageBox(e.caption.c_str(), e.text.c_str());
        if (process)
            process->Detach();
        return false;
    }
    // Always detach from process
    process->Detach();
    return true;
}

std::optional<HackState> hackstate;

bool HackState::update_hack_state(std::vector<std::pair<uintptr_t, uint32_t>>& pointers, int mode, void* selected_address) {
    syncDllConfig(false);

    dll_config.overwrite_mode = (OverwriteMode)mode;
    dll_config.overwrite_pValue = (uintptr_t)selected_address;
    dll_config.overwrite_value = 1000;
    dll_config.keep_overwrite_mode = true;

    syncDllConfig(true);

    pointers.clear();
    for (int i = 0; i < sizeof(dll_config.pValue) / sizeof(dll_config.pValue[0]); ++i) {
        std::pair<uintptr_t, uint32_t> data_pair = std::make_pair(dll_config.pValue[i], dll_config.value[i]);
        pointers.push_back(data_pair);
    }

    return true;
}

bool hack(std::vector<std::pair<uintptr_t, uint32_t>>& pointers, int mode, void* selected_address) {
    if (!hackstate && mode == -1)
        hackstate.emplace();

    if (hackstate) {
        if (hackstate->hack(pointers, mode, selected_address)) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

bool deinit_hack() {
    hackstate.reset();
    return true;
}

}
