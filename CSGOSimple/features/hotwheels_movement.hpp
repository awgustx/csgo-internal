#pragma once

#include "../valve_sdk/csgostructs.hpp"

class PredictionSystem;

namespace HotwheelsMovement
{
    void BunnyHop(CUserCmd* cmd);
    void JumpBug(CUserCmd* cmd, int original_flags);
    void EdgeBug(CUserCmd* cmd, int original_flags, PredictionSystem& prediction);
    void Reset();
}
