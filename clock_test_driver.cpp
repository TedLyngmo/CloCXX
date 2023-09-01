#include <ctime>

int main() {
    using namespace std;
    ::timespec req{};
    return ::clock_nanosleep(CLOCK_TO_TEST, 0, &req, nullptr) * 2;
}
