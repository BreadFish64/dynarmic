/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <cstddef>
#include <string>

#if defined(__linux__) && defined(DYNARMIC_EMIT_PERF_MAP)

#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>

#include <fmt/format.h>

namespace Dynarmic::BackendX64 {

static std::FILE* file = nullptr;

static void OpenFile() {
    const pid_t pid = getpid();
    const std::string filename = fmt::format("/tmp/perf-{:d}.map", pid);

    file = std::fopen(filename.c_str(), "w");
    if (!file) {
        return;
    }

    std::setvbuf(file, nullptr, _IONBF, 0);
}

void PerfMapRegister(const void* entrypoint, size_t size, const std::string& friendly_name) {
    if (!file) {
        if (!std::getenv("PERF_BUILDID_DIR")) {
            return;
        }

        OpenFile();

        if (!file) {
            return;
        }
    }

    const std::string line = fmt::format("{:016x} {:016x} {:s}\n", reinterpret_cast<u64>(entrypoint), size, friendly_name);
    std::fwrite(line.data(), sizeof *line.data(), line.size(), file);
}

void PerfMapClear() {
    if (!file) {
        return;
    }

    std::fclose(file);
    file = nullptr;
    OpenFile();
}

} // namespace Dynarmic::BackendX64

#else

namespace Dynarmic::BackendX64 {

void PerfMapRegister(const void*, size_t, const std::string&) {}

void PerfMapClear() {}

} // namespace Dynarmic::BackendX64

#endif
