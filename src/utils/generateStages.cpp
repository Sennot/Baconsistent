#include "generateStages.hpp"
#include "fixedTraining.hpp"

matjson::Value generateStages(std::vector<float> sps)
{
    return matjson::Serialize<std::vector<Stage>>::toJson(bacon::makeFixedStages(std::move(sps)));
}
