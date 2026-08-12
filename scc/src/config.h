#pragma once

enum class SccLocation {
  RED,
  BLUE,
  SCORING,
};

struct Config {
  SccLocation location;
};

const char *GetSCCLocationString(SccLocation location);

Config LoadConfig();
