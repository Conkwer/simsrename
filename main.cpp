// simsrename - edit save slots in The Sims Bustin' Out (N-Gage) THESIMS.SAV
//
// Save layout (8192 bytes):
//   0x0000  16-byte file header (unchanged)
//   0x0010  4 slots x 2040 bytes, stride 0x7F8
//     slot+0x00B  3-byte little-endian money (0..16,777,215)
//     slot+0x00E  16-byte ASCII name, zero padded
//     slot+0x7F6  16-bit checksum (last LE word)
//   Checksum invariant: sum of all 1020 little-endian uint16 words
//   in the slot == 0 (mod 65536).
//
// Build: g++ -O2 -std=c++17 -o simsrename main.cpp
//        (or: cmake -B build && cmake --build build)

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kFileSize      = 8192;
constexpr std::size_t kSlotStart     = 0x10;
constexpr std::size_t kSlotSize      = 0x7F8;   // 2040
constexpr std::size_t kSlotCount     = 4;
constexpr std::size_t kMoneyOffset   = 0x0B;
constexpr std::size_t kMoneyBytes    = 3;
constexpr std::size_t kNameOffset    = 0x0E;
constexpr std::size_t kNameLength    = 16;
constexpr std::size_t kChecksumOffset = 0x7F6;
constexpr std::size_t kMaxName       = 6;
constexpr std::uint32_t kMaxMoney    = 999999;

uint16_t readLE16(const std::uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

void writeLE16(std::uint8_t* p, uint16_t v) {
    p[0] = static_cast<std::uint8_t>(v & 0xFF);
    p[1] = static_cast<std::uint8_t>(v >> 8);
}

uint32_t readLE24(const std::uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16);
}

void writeLE24(std::uint8_t* p, uint32_t v) {
    p[0] = static_cast<std::uint8_t>(v & 0xFF);
    p[1] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
    p[2] = static_cast<std::uint8_t>((v >> 16) & 0xFF);
}

uint16_t wordSum(const std::uint8_t* slot, std::size_t words) {
    uint32_t sum = 0;
    for (std::size_t i = 0; i < words; ++i)
        sum += readLE16(slot + i * 2);
    return static_cast<uint16_t>(sum & 0xFFFF);
}

void fixChecksum(std::uint8_t* slot) {
    const uint16_t rest = wordSum(slot, kSlotSize / 2 - 1);
    writeLE16(slot + kChecksumOffset,
              static_cast<uint16_t>((0u - rest) & 0xFFFF));
}

bool parseSlot(const std::string& s, bool& all, int& only) {
    if (s == "all") { all = true; only = -1; return true; }
    try {
        std::size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos != s.size() || v < 0 || v >= static_cast<int>(kSlotCount))
            return false;
        all = false;
        only = v;
        return true;
    } catch (...) {
        return false;
    }
}

void usage(const char* argv0) {
    std::fprintf(stderr,
        "Usage:\n"
        "  %s <THESIMS.SAV> <name> [slot|all]\n"
        "      Set player name (1-%zu printable ASCII chars).\n"
        "  %s <THESIMS.SAV> --money <slot|all> <amount>\n"
        "      Set money (0-%u).\n"
        "  slot: 0-3 or 'all'.\n",
        argv0, kMaxName, argv0, kMaxMoney);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        usage(argv[0]);
        return 2;
    }

    const std::string path = argv[1];
    const bool moneyMode = (std::string(argv[2]) == "--money");

    std::string name;
    uint32_t money = 0;
    bool allSlots = true;
    int onlySlot = -1;

    if (moneyMode) {
        if (argc != 5) {
            usage(argv[0]);
            return 2;
        }
        if (!parseSlot(argv[3], allSlots, onlySlot)) {
            std::fprintf(stderr, "error: slot must be 0-3 or 'all'\n");
            return 2;
        }
        const std::string as = argv[4];
        try {
            std::size_t pos = 0;
            unsigned long v = std::stoul(as, &pos);
            if (pos != as.size() || v > kMaxMoney) {
                std::fprintf(stderr, "error: amount must be 0-%u\n", kMaxMoney);
                return 2;
            }
            money = static_cast<uint32_t>(v);
        } catch (...) {
            std::fprintf(stderr, "error: amount must be 0-%u\n", kMaxMoney);
            return 2;
        }
    } else {
        if (argc > 4) {
            usage(argv[0]);
            return 2;
        }
        name = argv[2];
        if (name.empty() || name.size() > kMaxName) {
            std::fprintf(stderr, "error: name must be 1-%zu characters\n", kMaxName);
            return 2;
        }
        for (unsigned char c : name) {
            if (c < 0x20 || c > 0x7E) {
                std::fprintf(stderr,
                    "error: name must be printable ASCII (Latin)\n");
                return 2;
            }
        }
        if (argc == 4 && !parseSlot(argv[3], allSlots, onlySlot)) {
            std::fprintf(stderr, "error: slot must be 0-3 or 'all'\n");
            return 2;
        }
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "error: cannot open '%s'\n", path.c_str());
        return 1;
    }
    std::vector<std::uint8_t> data(
        (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    if (data.size() != kFileSize) {
        std::fprintf(stderr, "error: unexpected file size %zu (expected %zu)\n",
                     data.size(), kFileSize);
        return 1;
    }
    if (std::memcmp(data.data(), "ASIE", 4) != 0) {
        std::fprintf(stderr, "error: not a THESIMS.SAV (bad magic)\n");
        return 1;
    }

    // Back up once, before the first modification.
    {
        const std::string bak = path + ".bak";
        std::ifstream test(bak, std::ios::binary);
        if (!test) {
            std::ofstream out(bak, std::ios::binary);
            if (!out) {
                std::fprintf(stderr, "error: cannot create backup '%s'\n", bak.c_str());
                return 1;
            }
            out.write(reinterpret_cast<const char*>(data.data()),
                      static_cast<std::streamsize>(data.size()));
            if (!out) {
                std::fprintf(stderr, "error: failed writing backup\n");
                return 1;
            }
        }
    }

    for (std::size_t i = 0; i < kSlotCount; ++i) {
        if (!allSlots && static_cast<int>(i) != onlySlot)
            continue;

        std::uint8_t* slot = data.data() + kSlotStart + i * kSlotSize;
        const uint16_t before = wordSum(slot, kSlotSize / 2);
        if (before != 0)
            std::fprintf(stderr, "warning: slot %zu checksum was invalid (%04x)\n",
                         i, before);

        if (moneyMode) {
            writeLE24(slot + kMoneyOffset, money);
        } else {
            std::memset(slot + kNameOffset, 0, kNameLength);
            std::memcpy(slot + kNameOffset, name.data(), name.size());
        }
        fixChecksum(slot);

        char shown[kNameLength + 1];
        std::memcpy(shown, slot + kNameOffset, kNameLength);
        shown[kNameLength] = '\0';
        std::printf("slot %zu: name='%s' money=%u checksum=%04X\n",
                    i, shown, readLE24(slot + kMoneyOffset),
                    readLE16(slot + kChecksumOffset));
    }

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        std::fprintf(stderr, "error: cannot write '%s'\n", path.c_str());
        return 1;
    }
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    if (!out) {
        std::fprintf(stderr, "error: failed writing file\n");
        return 1;
    }

    std::printf("done: '%s' patched\n", path.c_str());
    return 0;
}
