#pragma once

#include "../valve_sdk/csgostructs.hpp"

class PredictionSystem;

namespace DelusionalMovement
{
    void OnCreateMovePre(CUserCmd* cmd);
    void OnCreateMovePost(CUserCmd* cmd, int original_flags, PredictionSystem& prediction);
    void Reset();
}
