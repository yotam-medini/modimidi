#pragma once
#include <functional>

enum class State { None, Play, Pause };

using OnStateChange_t = std::function<void(State)>;
