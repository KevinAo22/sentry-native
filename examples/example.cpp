#ifdef _WIN32
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    define NOMINMAX
#    define _CRT_SECURE_NO_WARNINGS
#endif

#include "sentry.h"

#include <cstring>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>

#ifdef SENTRY_PLATFORM_WINDOWS
#    include <Windows.h>

#    include <DbgHelp.h>
#    pragma comment(lib, "dbghelp.lib")

#    define sleep_s(SECONDS) Sleep((SECONDS) * 1000)
#else
#    include <unistd.h>
#    define sleep_s(SECONDS) sleep(SECONDS)
#endif

// 检查命令行参数
static bool
has_arg(int argc, char **argv, const char *arg)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], arg) == 0) {
            return true;
        }
    }
    return false;
}

#ifdef SENTRY_PLATFORM_WINDOWS
// 打印调用栈
static void
print_stack_trace()
{
    const int max_frames = 32;
    void *stack[max_frames];

    // 捕获当前调用栈
    USHORT frames = ::CaptureStackBackTrace(0, max_frames, stack, NULL);

    std::cout << "\n=== Stack Trace (" << frames << " frames) ===" << std::endl;

    // 初始化符号处理
    HANDLE process = ::GetCurrentProcess();
    ::SymInitialize(process, NULL, TRUE);

    // 遍历栈帧
    for (USHORT i = 0; i < frames; i++) {
        // 获取符号信息
        DWORD64 address = (DWORD64)(stack[i]);

        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 displacement = 0;
        if (::SymFromAddr(process, address, &displacement, symbol)) {
            // 获取文件名和行号
            IMAGEHLP_LINE64 line;
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD line_displacement = 0;

            if (SymGetLineFromAddr64(
                    process, address, &line_displacement, &line)) {
                std::cout << "[" << i << "] " << symbol->Name << " at "
                          << line.FileName << ":" << line.LineNumber << " (0x"
                          << std::hex << address << std::dec << ")"
                          << std::endl;
            } else {
                std::cout << "[" << i << "] " << symbol->Name << " (0x"
                          << std::hex << address << std::dec << ")"
                          << std::endl;
            }
        } else {
            std::cout << "[" << i << "] 0x" << std::hex << address << std::dec
                      << std::endl;
        }
    }

    std::cout << "======================\n" << std::endl;

    ::SymCleanup(process);
}

static LONG WINAPI
veh_exception_handler(EXCEPTION_POINTERS *exception_pointers)
{
    if (exception_pointers->ExceptionRecord->ExceptionCode != 0xe06d7363) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    try {
        if (std::current_exception()) {
            throw;
        } else {
            std::cout << "veh handler called without an active exception"
                      << std::endl;
        }
    } catch (const std::bad_alloc &e) {
        std::cerr << "Uncaught bad_alloc exception: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Uncaught exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Uncaught non-standard exception" << std::endl;
    }
}

#endif

static void
handle_terminate()
{
    try {
        if (std::current_exception()) {
            throw;
        } else {
            std::cout << "terminate called without an active exception"
                      << std::endl;
        }
    } catch (const std::bad_alloc &e) {
        std::cerr << "Uncaught bad_alloc exception: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Uncaught exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Uncaught non-standard exception" << std::endl;
    }

    std::abort();
}

// before_send 回调
static sentry_value_t
before_send_callback(sentry_value_t event, void *hint, void *user_data)
{
    (void)hint;
    (void)user_data;

    std::cout << "before_send callback triggered" << std::endl;

    sentry_value_set_by_key(
        event, "adapted_by", sentry_value_new_string("before_send_cpp"));

    return event;
}

// on_crash 回调
static sentry_value_t
on_crash_callback(
    const sentry_ucontext_t *uctx, sentry_value_t event, void *user_data)
{
    (void)uctx;
    (void)user_data;

    std::cout << "Crash detected!" << std::endl;

    const auto exception = std::current_exception();
    if (exception) {
        try {
            std::rethrow_exception(exception);
        } catch (const std::bad_alloc &e) {
            std::cerr << "Caught bad_alloc exception: " << e.what()
                      << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Caught exception: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Caught non-standard exception" << std::endl;
        }
    }

#ifdef _WIN32
    if (!!uctx) {
        const EXCEPTION_RECORD *exception_record
            = (const EXCEPTION_RECORD *)uctx->exception_ptrs.ExceptionRecord;

        if (exception_record) {
            std::cout << "Exception Code: 0x" << std::hex
                      << exception_record->ExceptionCode << std::dec
                      << std::endl;

            std::cout << "Exception Flags: 0x" << std::hex
                      << exception_record->ExceptionFlags << std::dec;
            if (exception_record->ExceptionFlags & EXCEPTION_NONCONTINUABLE) {
                std::cout << " (NONCONTINUABLE)";
            }
            std::cout << std::endl;
        }
    }

    print_stack_trace();
#endif

    sentry_value_set_by_key(
        event, "on_crash", sentry_value_new_string("handled_cpp"));

    return event;
}

static void
print_envelope(sentry_envelope_t *envelope, void *unused_state)
{
    (void)unused_state;

    size_t size_out = 0;
    char *s = sentry_envelope_serialize(envelope, &size_out);

    std::cout << "\n=== Sentry Envelope ===" << std::endl;
    std::cout << std::string(s, size_out) << std::endl;
    std::cout << "=====================\n" << std::endl;

    sentry_free(s);
    sentry_envelope_free(envelope);
}

// 各种崩溃触发函数
namespace crash_triggers {

// 空指针解引用崩溃
void
null_pointer_crash()
{
    std::cout << "Triggering null pointer crash..." << std::endl;
    int *ptr = nullptr;
    *ptr = 42; // 崩溃
}

// 数组越界访问
void
array_overflow_crash()
{
    std::cout << "Triggering array overflow crash..." << std::endl;
    int arr[10];
    arr[1000000] = 42; // 崩溃
}

// 除零错误
void
divide_by_zero_crash()
{
    std::cout << "Triggering divide by zero crash..." << std::endl;
    volatile int x = 0;
    volatile int y = 42 / x; // 可能崩溃（取决于平台）
    (void)y;
}

// 堆栈溢出
void
stack_overflow_crash()
{
    std::cout << "Triggering stack overflow crash..." << std::endl;
    volatile char buffer[1024 * 1024]; // 1MB
    (void)buffer;
    stack_overflow_crash(); // 递归调用
}

// OOM - bad_alloc 异常
void
oom_bad_alloc_crash()
{
    std::cout << "Triggering OOM bad_alloc crash..." << std::endl;
    // 尝试分配一个巨大的内存块
    size_t enormous_size = SIZE_MAX - 1;
    char *ptr = new char[enormous_size];
    (void)ptr;
    // throw std::bad_alloc();
}

// C++ 异常崩溃
void
runtime_error_crash()
{
    std::cout << "Triggering C++ exception crash..." << std::endl;
    throw std::runtime_error("Intentional crash exception");
}

// 访问无效内存
void
invalid_memory_crash()
{
    std::cout << "Triggering invalid memory access crash..." << std::endl;
#ifdef SENTRY_PLATFORM_AIX
    void *invalid_mem = (void *)0xFFFFFFFFFFFFFF9B;
#else
    void *invalid_mem = (void *)1;
#endif
    memset(static_cast<char *>(invalid_mem), 1, 100);
}

#ifdef SENTRY_PLATFORM_WINDOWS

// Windows 特定的 fastfail 崩溃
void
fastfail_crash()
{
    std::cout << "Triggering Windows fastfail crash..." << std::endl;
    __fastfail(77);
}

void
raise_exception_crash()
{
    ::RaiseException(
        STATUS_FATAL_APP_EXIT, EXCEPTION_NONCONTINUABLE, 0, nullptr);
}

#endif

} // namespace crash_triggers

int
main(int argc, char **argv)
{
    // 初始化 Sentry
    sentry_options_t *options = sentry_options_new();

    // 设置数据库路径
    sentry_options_set_database_path(options, ".sentry-native-cpp");

    sentry_options_set_auto_session_tracking(options, false);
    sentry_options_set_symbolize_stacktraces(options, true);

    sentry_options_set_environment(options, "development");
    if (!has_arg(argc, argv, "release-env")) {
        sentry_options_set_release(options, "test-example-release");
    }

    // 启用调试日志
    if (has_arg(argc, argv, "log")) {
        sentry_options_set_debug(options, 1);
    }

    if (has_arg(argc, argv, "enable-logs")) {
        sentry_options_set_enable_logs(options, true);
    }

    // 设置回调函数
    if (has_arg(argc, argv, "before-send")) {
        sentry_options_set_before_send(options, before_send_callback, nullptr);
    }

    if (has_arg(argc, argv, "on-crash")) {
        sentry_options_set_on_crash(options, on_crash_callback, nullptr);
    }

    if (has_arg(argc, argv, "stdout")) {
        sentry_options_set_transport(
            options, sentry_transport_new(print_envelope));
    }

    if (has_arg(argc, argv, "terminate-handler")) {
        std::set_terminate(handle_terminate);
    }

#ifdef _WIN32
    if (has_arg(argc, argv, "veh-handler")) {
        ::AddVectoredExceptionHandler(1, veh_exception_handler);
    }
#endif

    // 初始化 Sentry
    if (sentry_init(options) != 0) {
        std::cerr << "Failed to initialize Sentry" << std::endl;
        return 1;
    }

    std::cout << "Sentry initialized successfully" << std::endl;

    // 设置用户信息
    sentry_value_t user = sentry_value_new_object();
    sentry_value_set_by_key(
        user, "id", sentry_value_new_string("test_user_123"));
    sentry_value_set_by_key(
        user, "username", sentry_value_new_string("test_user"));
    sentry_value_set_by_key(
        user, "email", sentry_value_new_string("test_user@example.com"));
    sentry_set_user(user);

    // 设置标签
    sentry_set_tag("language", "cpp");
    sentry_set_tag("example", "true");

    // 设置上下文
    sentry_value_t context = sentry_value_new_object();
    sentry_value_set_by_key(
        context, "type", sentry_value_new_string("runtime"));
    sentry_value_set_by_key(
        context, "name", sentry_value_new_string("cpp-runtime"));
    sentry_value_set_by_key(context, "version", sentry_value_new_string("17"));
    sentry_set_context("runtime", context);

    // 添加面包屑
    sentry_value_t breadcrumb
        = sentry_value_new_breadcrumb("default", "Application started");
    sentry_value_set_by_key(
        breadcrumb, "category", sentry_value_new_string("app.lifecycle"));
    sentry_add_breadcrumb(breadcrumb);

    // 捕获消息事件
    if (has_arg(argc, argv, "capture-event")) {
        std::cout << "Capturing test event..." << std::endl;

        sentry_value_t event = sentry_value_new_message_event(
            SENTRY_LEVEL_INFO, "logger", "Hello from C++!");

        sentry_value_set_by_key(
            event, "message_origin", sentry_value_new_string("cpp"));

        sentry_capture_event(event);

        std::cout << "Event captured" << std::endl;
    }

    // sleep_s(5);

    // 触发各种崩溃
    if (has_arg(argc, argv, "crash-null-pointer")) {
        crash_triggers::null_pointer_crash();
    }

    if (has_arg(argc, argv, "crash-array")) {
        crash_triggers::array_overflow_crash();
    }

    if (has_arg(argc, argv, "crash-divide-zero")) {
        crash_triggers::divide_by_zero_crash();
    }

    if (has_arg(argc, argv, "crash-stack-overflow")) {
        crash_triggers::stack_overflow_crash();
    }

    if (has_arg(argc, argv, "crash-oom")) {
        crash_triggers::oom_bad_alloc_crash();
    }

    if (has_arg(argc, argv, "crash-runtime-error")) {
        crash_triggers::runtime_error_crash();
    }

    if (has_arg(argc, argv, "crash-invalid-1`memory")) {
        crash_triggers::invalid_memory_crash();
    }

#ifdef SENTRY_PLATFORM_WINDOWS
    if (has_arg(argc, argv, "crash-fastfail")) {
        crash_triggers::fastfail_crash();
    }

    if (has_arg(argc, argv, "crash-raise-exception")) {
        crash_triggers::raise_exception_crash();
    }
#endif

    // 延迟以确保事件发送
    sleep_s(10);

    // 关闭 Sentry
    // std::cout << "Shutting down Sentry..." << std::endl;
    // sentry_close();

    std::cout << "Program completed successfully" << std::endl;
    return 0;
}
