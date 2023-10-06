#include <unistd.h>

//

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

// #macro, steady(true/false), description
struct clockdef {
    std::string macro;
    bool steady;
    std::string description;
    friend std::istream& operator>>(std::istream& is, clockdef& cd) {
        std::string line;
        while(std::getline(is, line) && (line.empty() || line.front() == '#')) {
        }
        if(is) {
            std::istringstream iss(line);
            iss >> std::boolalpha;
            if(not std::getline((std::getline(iss, cd.macro, ',') >> cd.steady).ignore(2, ' '), cd.description)) {
                is.setstate(std::ios::failbit);
            }
        }
        return is;
    }
};

enum result { UNSUPP, CLOCK_GETTIME, CLOCK_NANOSLEEP };
std::ostream& operator<<(std::ostream& os, result r) {
    switch(r) {
    case UNSUPP:
        os << "UNSUPPORTED";
        break;
    case CLOCK_GETTIME:
        os << "Exists - Can clock_nanosleep(): NO";
        break;
    case CLOCK_NANOSLEEP:
        os << "Exists - Can clock_nanosleep(): YES";
        break;
    }
    return os;
}

struct pres {
    int exitstatus;
    std::string output;
};

std::optional<pres> command(std::string command) {
    command += " 2>&1";
    std::FILE* pip = ::popen(command.c_str(), "r");
    if(not pip) return std::nullopt;

    constexpr std::size_t chunksize = 1024;
    std::string buf(chunksize, '\0');
    for(std::size_t len; (len = std::fread(buf.data() + buf.size() - chunksize, 1, chunksize, pip)) > 0;) {
        buf.resize(buf.size() + len);
    }
    buf.resize(buf.size() - chunksize);
    return pres{::pclose(pip), std::move(buf)};
}

result test(const clockdef& cd, int width) {
    std::cout << "Clock under test: " << std::setw(width) << cd.macro;
    auto res = command("make clock_test_driver CLOCK_TO_TEST=" + cd.macro);
    if(not res || (*res).exitstatus) return UNSUPP;

    res = command("./clock_test_driver");
    std::remove("clock_test_driver");

    if(not res) return UNSUPP;
    if((*res).exitstatus) return CLOCK_GETTIME;

    return CLOCK_NANOSLEEP;
}

std::string mkname(std::string macro) {
    // All macros are assumed to start with "CLOCK_" for now.
    std::transform(macro.begin() + 6, macro.end(), macro.begin() + 6, [](unsigned char ch) { return std::tolower(ch); });
    return macro.substr(6) + "_clock";
}

int createit() {
    auto& seen_file = "seen_clockid_t";
    std::cout << std::boolalpha;
    std::ifstream cids(seen_file);
    if(not cids) {
        std::cerr << seen_file << ": " << std::strerror(errno) << '\n';
        return 1;
    }
    std::vector<clockdef> cds(std::istream_iterator<clockdef>(cids), std::istream_iterator<clockdef>{});
    if(cds.empty()) {
        std::cerr << seen_file << ": No valid clock definitions read\n";
        return 1;
    }
    int maxwidth = std::max_element(cds.begin(), cds.end(), [](const clockdef& lhs, const clockdef& rhs) {
        return lhs.macro.size() < rhs.macro.size();
    })->macro.size();

    std::filesystem::copy("base.hpp", "clocxx.hpp", std::filesystem::copy_options::overwrite_existing);

    auto& target = "clocxx.hpp";
    std::ofstream ofs(target, std::ios::app);
    if(not ofs) {
        std::cerr << target << ": " << std::strerror(errno) << '\n';
        return 1;
    }
    ofs << std::boolalpha;

    for(auto& cd : cds) {
        auto rv = test(cd, maxwidth);
        std::cout << ": " << rv << '\n';
        if(rv != UNSUPP) {
            auto clname = mkname(cd.macro);
            ofs << "/*\n * " << clname << " - " << cd.description << "\n */\n";
            ofs << "struct " << clname << " : public detail::clock_base<" << clname << ",true," << (rv == CLOCK_NANOSLEEP) << ','
                << cd.macro << "> {};\n\n";
        }
    }
    ofs << "} // namespace chrono\n} // namespace lyn\n#endif\n";
    return 0;
}

int main() {
    int rv = createit();
    if(rv == 0) {
        return system("clang-format -i clocxx.hpp");
    }
    return rv;
}
