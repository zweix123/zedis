#include "util.h"

// Scoped enum
enum class Color { Red, Green, Blue };

// Unscoped enum
enum Orientation { Horizontal, Vertical };

// Another scoped enum
enum class ExecStatus { Idle, Started, Running };

int main() {
    std::cout << Color::Blue << "\n";
    std::cout << Vertical << "\n";
    std::cout << ExecStatus::Running << "\n";
    return 0;
}