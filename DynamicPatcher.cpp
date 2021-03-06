﻿// created by i-saint
// distributed under Creative Commons Attribution (CC BY) license.
// https://github.com/i-saint/DynamicPatcher

#include "DynamicPatcher.h"
#include "dpInternal.h"

#ifdef dpWithTDisasm
#pragma comment(lib,"disasm" dpLibArch ".lib")
#endif // dpWithTDisasm

static dpContext *g_dpDefaultContext = nullptr;
static __declspec(thread) dpContext *g_dpCurrentContext = nullptr;

dpAPI dpContext* dpCreateContext()
{
    return new dpContext();
}

dpAPI void dpDeleteContext(dpContext *ctx)
{
    delete ctx;
};

dpAPI dpContext* dpGetDefaultContext()
{
    return g_dpDefaultContext;
}

dpAPI void dpSetCurrentContext(dpContext *ctx)
{
    g_dpCurrentContext = ctx;
}

dpAPI dpContext* dpGetCurrentContext()
{
    if(!g_dpCurrentContext) { g_dpCurrentContext=g_dpDefaultContext; }
    return g_dpCurrentContext;
}


dpAPI bool dpInitialize(const dpConfig &conf)
{
    if(!g_dpDefaultContext) {
        DWORD opt = ::SymGetOptions();
        opt |= SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES;
        opt &= ~SYMOPT_UNDNAME;
        ::SymSetOptions(opt);
        ::SymInitialize(::GetCurrentProcess(), NULL, TRUE);

        dpConfig &g_dpConfig = dpGetConfig();
        g_dpConfig = conf;
        g_dpConfig.starttime = dpGetSystemTime();

        dpConfigFile cf;
        bool config_loaded = false;
        if((conf.sys_flags&dpE_SysLoadConfig)!=0 && (conf.configfile ? cf.load(conf.configfile) : cf.load())) {
            config_loaded = true;
            if(cf.log_flags!=-1) { g_dpConfig.log_flags=cf.log_flags; }
            if(cf.sys_flags!=-1) { g_dpConfig.sys_flags=cf.sys_flags; }
            if(cf.vc_ver!=-1)    { g_dpConfig.vc_ver=cf.vc_ver; }
        }
        g_dpDefaultContext = new dpContext();

        if(config_loaded) {
            if(!cf.loads.empty()) {
                dpEach(cf.loads, [](const std::string &path){
                    dpLoad(path.c_str());
                });
                dpLink();
            }
            dpEach(cf.source_paths,     [](const std::string &v){ dpAddSourcePath(v.c_str()); });
            dpEach(cf.module_paths,     [](const std::string &v){ dpAddModulePath(v.c_str()); });
            dpEach(cf.preload_paths,    [](const std::string &v){ dpAddPreloadPath(v.c_str()); });
            dpEach(cf.msbuild_commands, [](const std::string &v){ dpAddMSBuildCommand(v.c_str()); });
            dpEach(cf.build_commands,   [](const std::string &v){ dpAddBuildCommand(v.c_str()); });
            dpEach(cf.force_host_symbol_patterns, [](const std::string &v){ dpAddForceHostSymbolPattern(v.c_str()); });
            if(!cf.source_paths.empty() && (!cf.msbuild_commands.empty() || !cf.build_commands.empty())) {
                dpStartAutoBuild();
            }
            if(!cf.preload_paths.empty()) {
                dpStartPreload();
            }
        }
        if((g_dpConfig.sys_flags & dpE_SysOpenConsole)!=0) {
            ::AllocConsole();
        }

        return true;
    }
    return false;
}

dpAPI bool dpFinalize()
{
    if(g_dpDefaultContext) {
        delete g_dpDefaultContext;
        g_dpDefaultContext = nullptr;
        if((dpGetConfig().sys_flags & dpE_SysOpenConsole)!=0) {
            ::FreeConsole();
        }
        return true;
    }
    return false;
}

dpAPI size_t dpLoad(const char *path)    { return dpGetCurrentContext()->load(path); }
dpAPI bool   dpLoadObj(const char *path) { return dpGetCurrentContext()->getLoader()->loadObj(path)!=nullptr; }
dpAPI bool   dpLoadLib(const char *path) { return dpGetCurrentContext()->getLoader()->loadLib(path)!=nullptr; }
dpAPI bool   dpLoadDll(const char *path) { return dpGetCurrentContext()->getLoader()->loadDll(path)!=nullptr; }
dpAPI bool   dpUnload(const char *path)  { return dpGetCurrentContext()->getLoader()->unload(path); };
dpAPI bool   dpLink()                    { return dpGetCurrentContext()->getLoader()->link(); }
dpAPI size_t dpLoadMapFiles()            { return dpGetCurrentContext()->getLoader()->loadMapFiles(); };

dpAPI size_t dpPatchByFile(const char *filename, const char *filter_regex)
{
    return dpGetCurrentContext()->patchByFile(filename, filter_regex);
}

dpAPI size_t dpPatchByFile(const char *filename, const std::function<bool (const dpSymbolS&)> &condition)
{
    return dpGetCurrentContext()->patchByFile(filename, condition);
}

dpAPI bool dpPatchNameToName(const char *target_name, const char *hook_name)
{
    return dpGetCurrentContext()->patchNameToName(target_name, hook_name);
}

dpAPI bool dpPatchAddressToName(const char *target_name, void *hook_addr)
{
    return dpGetCurrentContext()->patchAddressToName(target_name, hook_addr);
}

dpAPI bool dpPatchAddressToAddress(void *target_adr, void *hook_addr)
{
    return dpGetCurrentContext()->patchAddressToAddress(target_adr, hook_addr);
}

dpAPI bool dpPatchByAddress(void *hook_addr)
{
    return dpGetCurrentContext()->patchByAddress(hook_addr);
}

dpAPI bool dpUnpatchByAddress(void *target_or_hook_addr)
{
    return dpGetCurrentContext()->unpatchByAddress(target_or_hook_addr);
}

dpAPI void dpUnpatchAll()
{
    return dpGetCurrentContext()->unpatchAll();
}

dpAPI void* dpGetUnpatched(void *target_or_hook_addr)
{
    return dpGetCurrentContext()->getUnpatched(target_or_hook_addr);
}

dpAPI void dpAddForceHostSymbolPattern(const char *pattern)
{
    return dpGetCurrentContext()->addForceHostSymbolPattern(pattern);
}


dpAPI void dpAddModulePath(const char *path)
{
    dpGetCurrentContext()->getBuilder()->addModulePath(path);
}

dpAPI void dpAddSourcePath(const char *path)
{
    dpGetCurrentContext()->getBuilder()->addSourcePath(path);
}

dpAPI void dpAddPreloadPath(const char *path)
{
    dpGetCurrentContext()->getBuilder()->addPreloadPath(path);
}

dpAPI void dpAddMSBuildCommand(const char *msbuild_option)
{
    dpGetCurrentContext()->getBuilder()->addMSBuildCommand(msbuild_option);
}
dpAPI void dpAddCLBuildCommand(const char *cl_option)
{
    dpGetCurrentContext()->getBuilder()->addCLBuildCommand(cl_option);
}
dpAPI void dpAddBuildCommand(const char *any_command)
{
    dpGetCurrentContext()->getBuilder()->addBuildCommand(any_command);
}

dpAPI bool dpStartAutoBuild()
{
    return dpGetCurrentContext()->getBuilder()->startAutoBuild();
}

dpAPI bool dpStopAutoBuild()
{
    return dpGetCurrentContext()->getBuilder()->stopAutoBuild();
}

dpAPI bool dpStartPreload()
{
    return dpGetCurrentContext()->getBuilder()->startPreload();
}

dpAPI bool dpStopPreload()
{
    return dpGetCurrentContext()->getBuilder()->stopPreload();
}

dpAPI void dpUpdate()
{
    dpGetCurrentContext()->getBuilder()->update();
}

dpAPI const char* dpGetVCVarsPath()
{
    return dpGetCurrentContext()->getBuilder()->getVCVarsPath();
}


#include <tlhelp32.h>

// F: [](DWORD thread_id)->void
template<class F>
inline void dpEnumerateThreads(DWORD pid, const F &f)
{
    HANDLE ss = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if(ss!=INVALID_HANDLE_VALUE) {
        THREADENTRY32 te;
        te.dwSize = sizeof(te);
        if(::Thread32First(ss, &te)) {
            do {
                if(te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID)+sizeof(te.th32OwnerProcessID) &&
                    te.th32OwnerProcessID==pid)
                {
                    f(te.th32ThreadID);
                }
                te.dwSize = sizeof(te);
            } while(::Thread32Next(ss, &te));
        }
        ::CloseHandle(ss);
    }
}

void dpExecExclusive(const std::function<void ()> &f)
{
    std::vector<HANDLE> threads;
    DWORD pid = ::GetCurrentProcessId();
    dpEnumerateThreads(pid, [&](DWORD tid){
        if(tid==::GetCurrentThreadId()) { return; }
        if(HANDLE thread=::OpenThread(THREAD_ALL_ACCESS, FALSE, tid)) {
            ::SuspendThread(thread);
            threads.push_back(thread);
        }
    });
    f();
    std::for_each(threads.begin(), threads.end(), [](HANDLE thread){
        ::ResumeThread(thread);
        ::CloseHandle(thread);
    });
}


bool g_dp_stop_periodic_update = false;
bool g_dp_periodic_update_running = false;

unsigned __stdcall dpPeriodicUpdate(void *)
{
    while(!g_dp_stop_periodic_update) {
        dpExecExclusive([&](){ dpUpdate(); });
        ::Sleep(1000);
    }
    g_dp_periodic_update_running = false;
    return 0;
}

dpAPI void dpBeginPeriodicUpdate()
{
    dpExecExclusive([&](){ dpInitialize(); });
    g_dp_stop_periodic_update = false;
    g_dp_periodic_update_running = true;
    // std::thread 使いたいが VS2010 対応を考慮して使わない方向で
    _beginthreadex(nullptr, 0, &dpPeriodicUpdate, nullptr, 0, nullptr);
}

dpAPI void dpEndPeriodicUpdate()
{
    if(g_dp_periodic_update_running) {
        g_dp_stop_periodic_update = true;
        while(!g_dp_periodic_update_running) {
            ::SwitchToThread();
        }
    }
}
