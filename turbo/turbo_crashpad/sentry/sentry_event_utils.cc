#include "sentry_event_utils.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "mpack.h"
#include "snapshot/exception_snapshot.h"
#include "snapshot/module_snapshot.h"

#ifdef _WIN32
#    include <Windows.h>
#endif

namespace turbo_crashpad {

namespace {
    constexpr long long kMicrosecondsPerMillisecond = 1000LL;
    constexpr long long kMicrosecondsPerSecond = 1000000LL;

    bool
    IsInDirectory(const std::filesystem::path &file_path,
        const std::filesystem::path &dir_path)
    {
        auto file_canonical = file_path.lexically_normal();
        auto dir_canonical = dir_path.lexically_normal();

        auto [file_it, dir_it] = std::mismatch(file_canonical.begin(),
            file_canonical.end(), dir_canonical.begin(), dir_canonical.end());

        return dir_it == dir_canonical.end();
    }

#ifdef _WIN32
    const std::string kSystemModuleNames[] = {
        "ntdll.dll",
        "kernel32.dll",
        "kernelbase.dll",
    };

    std::string
    GetSystemDirectory()
    {
        wchar_t path[MAX_PATH];
        UINT len = ::GetWindowsDirectoryW(path, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            return "";
        }

        int size = WideCharToMultiByte(
            CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
        std::string result(size - 1, '\0');
        WideCharToMultiByte(
            CP_UTF8, 0, path, -1, &result[0], size, nullptr, nullptr);

        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
#else
    const std::string kSystemModuleNames[] = {};

    std::string
    GetSystemDirectory()
    {
        return "";
    }
#endif

    void
    WriteSentryEvent(const base::FilePath &path, std::string_view part_key,
        std::string_view category_key,
        const std::unordered_map<std::string, std::string> &details)
    {
        std::vector<char> existing_data;
        std::ifstream input_file(path.value(), std::ios::binary);
        if (input_file.is_open()) {
            input_file.seekg(0, std::ios::end);
            size_t file_size = input_file.tellg();
            input_file.seekg(0, std::ios::beg);
            existing_data.resize(file_size);
            input_file.read(existing_data.data(), file_size);
            input_file.close();
        }

        // 2. 解析现有 msgpack 数据到 map
        std::unordered_map<std::string, mpack_node_t> existing_root_map;
        mpack_tree_t tree;
        bool has_existing_data = false;

        if (!existing_data.empty()) {
            mpack_tree_init_data(
                &tree, existing_data.data(), existing_data.size());
            mpack_tree_parse(&tree);

            if (mpack_tree_error(&tree) == mpack_ok) {
                mpack_node_t root = mpack_tree_root(&tree);
                if (mpack_node_type(root) == mpack_type_map) {
                    has_existing_data = true;
                    // 记录现有的 root-level keys（稍后需要复制）
                }
            }
        }
    }

} // namespace

std::string
GetProcessUptimeStr(const crashpad::ProcessSnapshot &process_snapshot)
{
    std::string uptime_str = "0ms";

    timeval start_time;
    process_snapshot.ProcessStartTime(&start_time);

    timeval crash_time;
    process_snapshot.SnapshotTime(&crash_time);

    long long total_microseconds
        = (crash_time.tv_sec - start_time.tv_sec) * kMicrosecondsPerSecond
        + (crash_time.tv_usec - start_time.tv_usec);

    if (total_microseconds <= 0) {
        uptime_str = "0us";
    } else if (total_microseconds < kMicrosecondsPerMillisecond) {
        uptime_str = std::to_string(total_microseconds) + "us";
    } else if (total_microseconds < kMicrosecondsPerSecond) {
        uptime_str
            = std::to_string(total_microseconds / kMicrosecondsPerMillisecond)
            + "ms";
    } else {
        uptime_str
            = std::to_string(total_microseconds / kMicrosecondsPerSecond) + "s";
    }

    return uptime_str;
}

ModuleInfo
GetExceptionModuleInfo(const crashpad::ProcessSnapshot &process_snapshot)
{
    ModuleInfo info = {};

    const auto *exception = process_snapshot.Exception();
    if (!exception) {
        return info;
    }

    const uint64_t crash_address = exception->ExceptionAddress();
    const auto &modules = process_snapshot.Modules();

    std::string main_dir;
    for (const auto *mod : modules) {
        if (mod->GetModuleType()
            == crashpad::ModuleSnapshot::kModuleTypeExecutable) {
            const std::string main_path = mod->Name();
            const size_t slash = main_path.find_last_of("\\/");
            if (slash != std::string::npos) {
                main_dir = main_path.substr(0, slash);
                std::transform(main_dir.begin(), main_dir.end(),
                    main_dir.begin(), ::tolower);
            }

            break;
        }
    }

    const std::string system_directory = GetSystemDirectory();
    for (const auto *module : modules) {
        if (!module) {
            continue;
        }

        const uint64_t module_base = module->Address();
        const uint64_t module_size = module->Size();
        if (crash_address >= module_base) {
            uint64_t offset = crash_address - module_base;
            if (offset < module_size) {
                info.full_path = module->Name();
                const size_t last_slash = info.full_path.find_last_of("\\/");
                if (last_slash != std::string::npos) {
                    info.name = info.full_path.substr(last_slash + 1);
                } else {
                    info.name = info.full_path;
                }

                std::string path_lower = info.full_path;
                std::transform(path_lower.begin(), path_lower.end(),
                    path_lower.begin(), ::tolower);

                std::string name_lower = info.name;
                std::transform(name_lower.begin(), name_lower.end(),
                    name_lower.begin(), ::tolower);

                const bool in_system_dir = !system_directory.empty()
                    && IsInDirectory(path_lower, system_directory);
                const bool is_system_module_name
                    = std::find(std::begin(kSystemModuleNames),
                          std::end(kSystemModuleNames), name_lower)
                    != std::end(kSystemModuleNames);

                const bool in_main_dir
                    = !main_dir.empty() && IsInDirectory(path_lower, main_dir);

                if (in_system_dir || is_system_module_name) {
                    info.category = "system";
                } else if (in_main_dir) {
                    info.category = "own";
                } else {
                    info.category = "third_party";
                }

                break;
            }
        }
    }

    return info;
}

void
EnrichSentryCrashEvent(const crashpad::ProcessSnapshot &process_snapshot,
    const std::vector<base::FilePath> &attachments)
{
    std::unordered_map<std::string, std::string> crash_details;

    const auto *exception_snapshot = process_snapshot.Exception();
    if (!!exception_snapshot) {
        const auto exception_code = process_snapshot.Exception()->Exception();
        crash_details["exception_code"] = std::to_string(exception_code);
        const auto exception_module_info
            = GetExceptionModuleInfo(process_snapshot);
        crash_details["exception_module_name"] = exception_module_info.name;
        crash_details["exception_module_category"]
            = exception_module_info.category;
    }

    const auto &system_details = GetSystemDetails();
    const auto &process_details = GetProcessDetails(process_snapshot);

    for (const auto &attachment : attachments) {
        const auto attachment_name = attachment.BaseName().value();
        if (attachment_name != L"__sentry-event") {
            continue;
        }

        WriteSentryEvent(
            attachment, "contexts", "Exception Details", crash_details);
        WriteSentryEvent(
            attachment, "contexts", "Process Details", process_details);
        WriteSentryEvent(
            attachment, "contexts", "System Details", system_details);
    }
}

} // namespace turbo_crashpad
