#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

#ifdef TRANSLATED_FIXTURE
extern "C" void translated_block(uint64_t *state);
#else
extern "C" void execute_case();
#endif
extern "C" {
std::array<uint64_t, 32> fixture_input{};
std::array<uint64_t, 24> fixture_output{};
std::array<uint64_t, 32> fixture_xmm_input{};
std::array<uint64_t, 32> fixture_xmm_output{};
std::array<uint64_t, 16> fixture_x87_input{};
std::array<uint64_t, 16> fixture_x87_output{};
}

extern "C" __attribute__((noinline)) uint64_t fixture_call_target(
    uint64_t arg0, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
  uint64_t result;
  __asm__ volatile("pushfq\n\tleaq 24(%1), %0\n\tpopfq"
                   : "=a"(result)
                   : "D"(arg0)
                   : "memory");
  return result;
}

namespace {
enum RegisterIndex {
  RAX,
  RBX,
  RCX,
  RDX,
  RSI,
  RDI,
  RBP,
  RSP,
  R8,
  R9,
  R10,
  R11,
  R12,
  R13,
  R14,
  R15,
  RIP,
  EFLAGS,
  CS,
  SS,
  DS,
  ES,
  FS,
  GS,
  RegisterCount
};
constexpr size_t MemoryIndex = RegisterCount;
constexpr size_t MemoryWords = 8;

const std::array<std::string, RegisterCount> RegisterNames = {
    "rax", "rbx",    "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
    "r8",  "r9",     "r10", "r11", "r12", "r13", "r14", "r15",
    "rip", "eflags", "cs",  "ss",  "ds",  "es",  "fs",  "gs"};
const std::map<std::string, unsigned> FlagBits = {
    {"cf", 0}, {"pf", 2}, {"af", 4},  {"zf", 6}, {"sf", 7},
    {"tf", 8}, {"if", 9}, {"df", 10}, {"of", 11}};

struct Fixture {
  std::map<std::string, std::string> registers;
  std::map<std::string, std::string> flags;
  std::map<std::string, std::string> memory;
  std::map<std::string, std::string> vectors;
  std::map<std::string, std::string> x87;
};

std::string trim(std::string text) {
  while (!text.empty() &&
         std::isspace(static_cast<unsigned char>(text.front())))
    text.erase(text.begin());
  while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
    text.pop_back();
  return text;
}

Fixture parseToml(const char *path) {
  std::ifstream input(path);
  if (!input)
    throw std::runtime_error(std::string("cannot open ") + path);
  Fixture result;
  std::string section, line;
  while (std::getline(input, line)) {
    if (size_t comment = line.find('#'); comment != std::string::npos)
      line.erase(comment);
    line = trim(line);
    if (line.empty())
      continue;
    if (line.front() == '[' && line.back() == ']') {
      section = trim(line.substr(1, line.size() - 2));
      continue;
    }
    size_t equal = line.find('=');
    if (equal == std::string::npos)
      throw std::runtime_error("invalid TOML");
    std::string key = trim(line.substr(0, equal));
    std::string value = trim(line.substr(equal + 1));
    if (key.size() >= 2 && key.front() == '"')
      key = key.substr(1, key.size() - 2);
    if (value.size() >= 2 && value.front() == '"')
      value = value.substr(1, value.size() - 2);
    auto *target = section == "registers" ? &result.registers
                   : section == "flags"   ? &result.flags
                   : section == "memory"  ? &result.memory
                                          : nullptr;
    if (section == "vectors")
      target = &result.vectors;
    if (section == "x87")
      target = &result.x87;
    if (!target)
      throw std::runtime_error("unsupported TOML section: " + section);
    (*target)[key] = value;
  }
  return result;
}

size_t registerIndex(const std::string &name) {
  for (size_t i = 0; i < RegisterNames.size(); ++i)
    if (RegisterNames[i] == name)
      return i;
  throw std::runtime_error("unsupported register: " + name);
}

uint64_t number(const std::string &value) {
  return std::stoull(value, nullptr, 0);
}

size_t memoryIndex(const std::string &address) {
  constexpr const char *Prefix = "rdi+";
  if (address.rfind(Prefix, 0) != 0)
    throw std::runtime_error("unsupported memory address: " + address);
  uint64_t offset = std::stoull(address.substr(4), nullptr, 0);
  if (offset < 24 || (offset - 24) % 8 != 0 || (offset - 24) / 8 >= MemoryWords)
    throw std::runtime_error("memory address is outside fixture storage: " +
                             address);
  return MemoryIndex + (offset - 24) / 8;
}

std::array<uint64_t, 2> vectorNumber(std::string value) {
  if (value.rfind("0x", 0) == 0)
    value.erase(0, 2);
  if (value.size() > 32)
    throw std::runtime_error("vector value exceeds 128 bits");
  value.insert(0, 32 - value.size(), '0');
  return {std::stoull(value.substr(16), nullptr, 16),
          std::stoull(value.substr(0, 16), nullptr, 16)};
}

std::array<uint64_t, 2> x87Number(std::string value) {
  if (value.rfind("0x", 0) == 0)
    value.erase(0, 2);
  if (value.size() > 20)
    throw std::runtime_error("x87 value exceeds 80 bits");
  value.insert(0, 20 - value.size(), '0');
  return {std::stoull(value.substr(4), nullptr, 16),
          std::stoull(value.substr(0, 4), nullptr, 16)};
}

void requireComplete(const Fixture &fixture, const char *path) {
  for (const std::string &name : RegisterNames)
    if (!fixture.registers.count(name))
      throw std::runtime_error(std::string(path) + " missing register " + name);
  for (const auto &[name, bit] : FlagBits)
    if (!fixture.flags.count(name))
      throw std::runtime_error(std::string(path) + " missing flag " + name);
}

uint64_t composeFlags(const Fixture &fixture) {
  uint64_t flags = number(fixture.registers.at("eflags")) | 2;
  for (const auto &[name, bit] : FlagBits) {
    flags &= ~(uint64_t{1} << bit);
    flags |= (number(fixture.flags.at(name)) & 1) << bit;
  }
  return flags;
}

bool check(const std::string &kind, const std::string &name, uint64_t actual,
           const std::string &expected, uint64_t unchanged) {
  if (expected == "ignore" || (name == "eflags" && expected == "flags"))
    return true;
  uint64_t wanted = expected == "unchanged" ? unchanged : number(expected);
  if (actual == wanted)
    return true;
  std::cerr << OUTPUT_FILE << ": " << kind << ' ' << name << " expected 0x"
            << std::hex << wanted << ", got 0x" << actual << '\n';
  return false;
}

void executeFixture() {
#ifdef TRANSLATED_FIXTURE
  // The translated block ABI contains 16 GPR slots, RFLAGS, 16 XMM registers
  // in two slots each, and eight logical x87 stack values in two slots each.
  std::array<uint64_t, 65> state{};
  for (size_t index = RAX; index <= R15; ++index)
    state[index] = fixture_input[index];
  state[16] = fixture_input[EFLAGS];
  for (size_t index = 0; index < 16; ++index) {
    state[17 + index * 2] = fixture_xmm_input[index * 2];
    state[18 + index * 2] = fixture_xmm_input[index * 2 + 1];
  }
  for (size_t index = 0; index < 8; ++index) {
    state[49 + index * 2] = fixture_x87_input[index * 2];
    state[50 + index * 2] = fixture_x87_input[index * 2 + 1];
  }
  translated_block(state.data());

  for (size_t index = 0; index < RegisterCount; ++index)
    fixture_output[index] = fixture_input[index];
  for (size_t index = RAX; index <= R15; ++index)
    fixture_output[index] = state[index];
  fixture_output[EFLAGS] = state[16];
  for (size_t index = 0; index < 16; ++index) {
    fixture_xmm_output[index * 2] = state[17 + index * 2];
    fixture_xmm_output[index * 2 + 1] = state[18 + index * 2];
  }
  for (size_t index = 0; index < 8; ++index) {
    fixture_x87_output[index * 2] = state[49 + index * 2];
    fixture_x87_output[index * 2 + 1] = state[50 + index * 2];
  }
#else
  execute_case();
#endif
}
} // namespace

int main() {
  try {
    Fixture input = parseToml(INPUT_FILE);
    Fixture output = parseToml(OUTPUT_FILE);
    requireComplete(input, INPUT_FILE);
    requireComplete(output, OUTPUT_FILE);

    for (const auto &[name, value] : input.registers) {
      size_t index = registerIndex(name);
      if (value == "runtime" || value == "run_case")
        continue;
      fixture_input[index] =
          value == "memory_base"
              ? reinterpret_cast<uint64_t>(&fixture_input[MemoryIndex]) - 24
          : value == "call_target"
              ? reinterpret_cast<uint64_t>(&fixture_call_target)
              : number(value);
    }
    fixture_input[EFLAGS] = composeFlags(input);
    for (size_t index = 0; index < 16; ++index) {
      std::string name = "xmm" + std::to_string(index);
      auto value = input.vectors.find(name);
      if (value == input.vectors.end())
        continue;
      auto bits = vectorNumber(value->second);
      fixture_xmm_input[index * 2] = bits[0];
      fixture_xmm_input[index * 2 + 1] = bits[1];
    }
    for (size_t index = 0; index < 8; ++index) {
      std::string name = "st" + std::to_string(index);
      auto value = input.x87.find(name);
      if (value == input.x87.end())
        continue;
      auto bits = x87Number(value->second);
      fixture_x87_input[index * 2] = bits[0];
      fixture_x87_input[index * 2 + 1] = bits[1];
    }
    if (!input.memory.count("rdi+24"))
      throw std::runtime_error("missing rdi+24 input");
    for (const auto &[address, value] : input.memory)
      fixture_input[memoryIndex(address)] =
          value == "call_target"
              ? reinterpret_cast<uint64_t>(&fixture_call_target)
              : number(value);

    executeFixture();
    bool passed = true;
    for (const auto &[name, expected] : output.registers) {
      size_t index = registerIndex(name);
      std::string resolved =
          expected == "rip+2"    ? std::to_string(fixture_input[RIP] + 2)
          : expected == "rdi+24" ? std::to_string(fixture_input[RDI] + 24)
          : expected == "low32(rdi+24)"
              ? std::to_string(static_cast<uint32_t>(fixture_input[RDI] + 24))
              : expected;
      passed &= check("register", name, fixture_output[index], resolved,
                      fixture_input[index]);
    }
    for (const auto &[name, expected] : output.flags) {
      unsigned bit = FlagBits.at(name);
      uint64_t actual = (fixture_output[EFLAGS] >> bit) & 1;
      uint64_t before = (fixture_input[EFLAGS] >> bit) & 1;
      passed &= check("flag", name, actual, expected, before);
    }
    if (!output.memory.count("rdi+24"))
      throw std::runtime_error("missing rdi+24 output");
    for (const auto &[address, expected] : output.memory) {
      uint64_t actual = fixture_input[memoryIndex(address)];
      passed &= check("memory", address, actual, expected, actual);
    }
    for (const auto &[name, expected] : output.vectors) {
      if (name.rfind("xmm", 0) != 0)
        throw std::runtime_error("unsupported vector register: " + name);
      size_t index = std::stoul(name.substr(3));
      if (index >= 16)
        throw std::runtime_error("unsupported vector register: " + name);
      auto wanted =
          expected == "unchanged"
              ? std::array<uint64_t, 2>{fixture_xmm_input[index * 2],
                                        fixture_xmm_input[index * 2 + 1]}
              : vectorNumber(expected);
      passed &= check("vector-low", name, fixture_xmm_output[index * 2],
                      std::to_string(wanted[0]), wanted[0]);
      passed &= check("vector-high", name, fixture_xmm_output[index * 2 + 1],
                      std::to_string(wanted[1]), wanted[1]);
    }
    for (const auto &[name, expected] : output.x87) {
      if (name.rfind("st", 0) != 0)
        throw std::runtime_error("unsupported x87 register: " + name);
      size_t index = std::stoul(name.substr(2));
      if (index >= 8)
        throw std::runtime_error("unsupported x87 register: " + name);
      if (expected == "ignore")
        continue;
      auto wanted =
          expected == "unchanged"
              ? std::array<uint64_t, 2>{fixture_x87_input[index * 2],
                                        fixture_x87_input[index * 2 + 1]}
              : x87Number(expected);
      passed &= check("x87-low", name, fixture_x87_output[index * 2],
                      std::to_string(wanted[0]), wanted[0]);
      passed &= check("x87-high", name, fixture_x87_output[index * 2 + 1],
                      std::to_string(wanted[1]), wanted[1]);
    }
    return passed ? 0 : 1;
  } catch (const std::exception &error) {
    std::cerr << "fixture error: " << error.what() << '\n';
    return 2;
  }
}
