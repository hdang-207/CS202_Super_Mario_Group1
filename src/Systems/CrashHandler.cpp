#include "Systems/CrashHandler.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <csignal>
#include <cstdlib>
#include <mutex>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <dbghelp.h>
#endif

namespace Systems {

namespace {
    std::string s_logFilePath = "crash_log.txt";
    std::mutex s_logMutex;
    bool s_hasCrashed = false;

#if defined(_WIN32)
    const char* getExceptionDescription(DWORD code) {
        switch (code) {
            case EXCEPTION_ACCESS_VIOLATION:         return "EXCEPTION_ACCESS_VIOLATION (Segmentation Fault / Invalid Memory Access)";
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
            case EXCEPTION_BREAKPOINT:               return "EXCEPTION_BREAKPOINT";
            case EXCEPTION_DATATYPE_MISALIGNMENT:    return "EXCEPTION_DATATYPE_MISALIGNMENT";
            case EXCEPTION_FLT_DENORMAL_OPERAND:     return "EXCEPTION_FLT_DENORMAL_OPERAND";
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
            case EXCEPTION_FLT_INEXACT_RESULT:       return "EXCEPTION_FLT_INEXACT_RESULT";
            case EXCEPTION_FLT_INVALID_OPERATION:    return "EXCEPTION_FLT_INVALID_OPERATION";
            case EXCEPTION_FLT_OVERFLOW:             return "EXCEPTION_FLT_OVERFLOW";
            case EXCEPTION_FLT_STACK_CHECK:          return "EXCEPTION_FLT_STACK_CHECK";
            case EXCEPTION_FLT_UNDERFLOW:            return "EXCEPTION_FLT_UNDERFLOW";
            case EXCEPTION_ILLEGAL_INSTRUCTION:      return "EXCEPTION_ILLEGAL_INSTRUCTION";
            case EXCEPTION_IN_PAGE_ERROR:            return "EXCEPTION_IN_PAGE_ERROR";
            case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "EXCEPTION_INT_DIVIDE_BY_ZERO";
            case EXCEPTION_INT_OVERFLOW:             return "EXCEPTION_INT_OVERFLOW";
            case EXCEPTION_INVALID_DISPOSITION:      return "EXCEPTION_INVALID_DISPOSITION";
            case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
            case EXCEPTION_PRIV_INSTRUCTION:         return "EXCEPTION_PRIV_INSTRUCTION";
            case EXCEPTION_SINGLE_STEP:              return "EXCEPTION_SINGLE_STEP";
            case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW";
            default:                                 return "UNKNOWN_WINDOWS_EXCEPTION";
        }
    }
#endif
}

void CrashHandler::init() {
#if defined(_WIN32)
    SetUnhandledExceptionFilter(windowsUnhandledExceptionFilter);
#endif

    std::set_terminate(handleTerminate);

    std::signal(SIGSEGV, handleSignal);
    std::signal(SIGABRT, handleSignal);
    std::signal(SIGFPE,  handleSignal);
    std::signal(SIGILL,  handleSignal);
    std::signal(SIGTERM, handleSignal);
}

void CrashHandler::setLogFilePath(const std::string& path) {
    std::lock_guard<std::mutex> lock(s_logMutex);
    s_logFilePath = path;
}

std::string CrashHandler::getLogFilePath() {
    std::lock_guard<std::mutex> lock(s_logMutex);
    return s_logFilePath;
}

std::string CrashHandler::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    
    std::tm timeInfo;
#if defined(_WIN32)
    localtime_s(&timeInfo, &in_time_t);
#else
    localtime_r(&in_time_t, &timeInfo);
#endif

    ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string CrashHandler::getStackTrace() {
    std::stringstream ss;

#if defined(_WIN32)
    void* stack[64];
    HANDLE process = GetCurrentProcess();
    
    // Load DbgHelp dynamically if possible
    HMODULE hDbgHelp = LoadLibraryA("dbghelp.dll");
    typedef BOOL (WINAPI *SymInitializeFunc)(HANDLE, PCSTR, BOOL);
    typedef BOOL (WINAPI *SymFromAddrFunc)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
    typedef BOOL (WINAPI *SymGetLineFromAddr64Func)(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
    typedef BOOL (WINAPI *SymCleanupFunc)(HANDLE);

    SymInitializeFunc pSymInitialize = nullptr;
    SymFromAddrFunc pSymFromAddr = nullptr;
    SymGetLineFromAddr64Func pSymGetLine = nullptr;
    SymCleanupFunc pSymCleanup = nullptr;

    if (hDbgHelp) {
        pSymInitialize = (SymInitializeFunc)GetProcAddress(hDbgHelp, "SymInitialize");
        pSymFromAddr = (SymFromAddrFunc)GetProcAddress(hDbgHelp, "SymFromAddr");
        pSymGetLine = (SymGetLineFromAddr64Func)GetProcAddress(hDbgHelp, "SymGetLineFromAddr64");
        pSymCleanup = (SymCleanupFunc)GetProcAddress(hDbgHelp, "SymCleanup");
    }

    if (pSymInitialize) {
        pSymInitialize(process, NULL, TRUE);
    }

    WORD frames = CaptureStackBackTrace(0, 64, stack, NULL);

    char buffer[sizeof(SYMBOL_INFO) + 256 * sizeof(char)];
    PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (WORD i = 0; i < frames; ++i) {
        DWORD64 address = (DWORD64)(stack[i]);
        DWORD displacement = 0;
        DWORD64 displacement64 = 0;

        ss << "  [" << std::setw(2) << std::setfill('0') << i << "] 0x" 
           << std::hex << std::setw(16) << std::setfill('0') << address << std::dec;

        if (pSymFromAddr && pSymFromAddr(process, address, &displacement64, symbol)) {
            ss << " : " << symbol->Name;

            IMAGEHLP_LINE64 line;
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            if (pSymGetLine && pSymGetLine(process, address, &displacement, &line)) {
                ss << " (" << line.FileName << ":" << line.LineNumber << ")";
            }
        }
        ss << "\n";
    }

    if (pSymCleanup) {
        pSymCleanup(process);
    }
    if (hDbgHelp) {
        FreeLibrary(hDbgHelp);
    }
#else
    ss << "  [Stack trace capture is platform-specific and currently active on Windows]\n";
#endif

    return ss.str();
}

void CrashHandler::writeCrashReport(const std::string& title, const std::string& details) {
    std::lock_guard<std::mutex> lock(s_logMutex);

    std::string path = s_logFilePath;
    std::ofstream file(path, std::ios::out | std::ios::app);

    std::stringstream report;
    report << "================================================================================\n";
    report << "                          SUPER MARIO BROS - CRASH REPORT                       \n";
    report << "================================================================================\n";
    report << "Timestamp: " << getTimestamp() << "\n";
    report << "Crash Type: " << title << "\n\n";
    report << "[Details]\n";
    report << details << "\n\n";
    report << "[Callstack / Trace]\n";
    report << getStackTrace();
    report << "================================================================================\n\n";

    std::string reportStr = report.str();

    if (file.is_open()) {
        file << reportStr;
        file.flush();
        file.close();
    }

    std::cerr << "\n" << reportStr << std::endl;

#if defined(_WIN32)
    std::string messageBoxMsg = "The game encountered an unexpected error and needs to close.\n\n"
                                "A crash log has been saved to:\n" + path + "\n\n" +
                                "Error: " + title;
    MessageBoxA(NULL, messageBoxMsg.c_str(), "Super Mario Bros - Game Crash", MB_OK | MB_ICONERROR);
#endif
}

void CrashHandler::logException(const std::exception& e, const std::string& context) {
    std::stringstream ss;
    if (!context.empty()) {
        ss << "Context: " << context << "\n";
    }
    ss << "std::exception::what(): " << e.what();
    writeCrashReport("C++ Standard Exception Caught", ss.str());
}

void CrashHandler::logCrash(const std::string& message, const std::string& context) {
    std::stringstream ss;
    if (!context.empty()) {
        ss << "Context: " << context << "\n";
    }
    ss << "Message: " << message;
    writeCrashReport("Manual Fatal Crash", ss.str());
}

#if defined(_WIN32)
long __stdcall CrashHandler::windowsUnhandledExceptionFilter(struct ::_EXCEPTION_POINTERS* exceptionInfo) {
    if (s_hasCrashed) {
        return EXCEPTION_EXECUTE_HANDLER;
    }
    s_hasCrashed = true;

    DWORD code = exceptionInfo ? exceptionInfo->ExceptionRecord->ExceptionCode : 0;
    void* address = exceptionInfo ? exceptionInfo->ExceptionRecord->ExceptionAddress : nullptr;

    std::stringstream ss;
    ss << "Exception Code: 0x" << std::hex << code << " (" << getExceptionDescription(code) << ")\n";
    ss << "Fault Address: 0x" << std::hex << (uintptr_t)address << std::dec << "\n";

    writeCrashReport("Unhandled Windows SEH Exception", ss.str());

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void CrashHandler::handleSignal(int signalCode) {
    if (s_hasCrashed) {
        std::exit(signalCode);
    }
    s_hasCrashed = true;

    std::string sigName = "UNKNOWN_SIGNAL";
    switch (signalCode) {
        case SIGSEGV: sigName = "SIGSEGV (Segmentation Fault / Invalid Memory Access)"; break;
        case SIGABRT: sigName = "SIGABRT (Abort Signal / std::abort invoked)"; break;
        case SIGFPE:  sigName = "SIGFPE (Floating Point Exception / Division by Zero)"; break;
        case SIGILL:  sigName = "SIGILL (Illegal Instruction)"; break;
        case SIGTERM: sigName = "SIGTERM (Termination Request)"; break;
    }

    std::stringstream ss;
    ss << "Signal Code: " << signalCode << " (" << sigName << ")";

    writeCrashReport("Fatal Signal Caught", ss.str());
    std::exit(signalCode);
}

void CrashHandler::handleTerminate() {
    if (s_hasCrashed) {
        std::abort();
    }
    s_hasCrashed = true;

    std::string details = "std::terminate was invoked (e.g. unhandled exception thrown in noexcept function or destructor).";
    
    // Try to get active exception if any
    try {
        auto exPtr = std::current_exception();
        if (exPtr) {
            std::rethrow_exception(exPtr);
        }
    } catch (const std::exception& e) {
        details += "\nUnderlying exception: ";
        details += e.what();
    } catch (...) {
        details += "\nUnderlying exception: <Unknown non-std exception>";
    }

    writeCrashReport("std::terminate Called", details);
    std::abort();
}

} // namespace Systems
