#pragma once

#include <string>
#include <exception>

#if defined(_WIN32)
struct _EXCEPTION_POINTERS;
#endif

namespace Systems {

/**
 * @class CrashHandler
 * @brief Handles unexpected application crashes, unhandled exceptions, and fatal signals,
 *        generating detailed crash logs in a readable format.
 */
class CrashHandler {
public:
    /**
     * @brief Initializes crash hooks (Windows SEH, std::terminate, POSIX signals).
     *        Call this at the very beginning of main().
     */
    static void init();

    /**
     * @brief Manually logs a caught C++ exception to the crash log file.
     * @param e Standard exception reference.
     * @param context Additional context where the exception occurred.
     */
    static void logException(const std::exception& e, const std::string& context = "");

    /**
     * @brief Manually logs a custom crash / fatal error message to the crash log file.
     * @param message Description of the fatal error.
     * @param context Additional context.
     */
    static void logCrash(const std::string& message, const std::string& context = "");

    /**
     * @brief Sets the path for the crash log file. Default is "crash_log.txt".
     */
    static void setLogFilePath(const std::string& path);

    /**
     * @brief Gets the current path of the crash log file.
     */
    static std::string getLogFilePath();

private:
    static void writeCrashReport(const std::string& title, const std::string& details);
    static std::string getTimestamp();
    static std::string getStackTrace();

#if defined(_WIN32)
    static long __stdcall windowsUnhandledExceptionFilter(struct ::_EXCEPTION_POINTERS* exceptionInfo);
#endif
    static void handleSignal(int signalCode);
    static void handleTerminate();
};

} // namespace Systems
