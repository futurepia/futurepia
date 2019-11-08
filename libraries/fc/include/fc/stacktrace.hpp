#pragma once

#include <iostream>

namespace fc {

void print_stacktrace(std::ostream& out, unsigned int max_frames = 63, void* caller_overwrite_hack = nullptr);
void print_stacktrace_on_segfault();

}
