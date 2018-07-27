/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <cstddef>
#include <string>

namespace Dynarmic::BackendX64 {

void PerfMapRegister(const void* entrypoint, size_t size, const std::string& friendly_name);
void PerfMapClear();

} // namespace Dynarmic::BackendX64
