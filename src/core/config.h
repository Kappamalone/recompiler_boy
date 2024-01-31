#include "common.h"

enum class CPUTypes {
  INTERPRETER,
  CACHED_INTERPRETER,
};

class Config {
public:
  const char* rom_path;
  const char* bootrom_path;
  CPUTypes cpu_type;
};
