#pragma once

#include <vector>
#include "Stage.hpp"

struct ProfileData
{
  int fixedPartsVersion = 1;
  std::vector<float> tags;
  std::vector<Stage> stages;
};