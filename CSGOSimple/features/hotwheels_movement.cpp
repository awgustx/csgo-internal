#include "hotwheels_movement.hpp"

#include "../helpers/prediction.hpp"
#include "../options.hpp"
#include "../valve_sdk/interfaces/IEngineTrace.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    struct EdgeBugState
    {
        bool will_edgebug = false;
        bool will_fail = false;
        bool strafing = false;
        bool ducking = false;
        int ticks_to_stop = 0;
        int last_tick = 0;
        short saved_mousedx = 0;
        float starting_yaw = 0.f;
        float yaw_delta = 0.f;
        float forward_move = 0.f;
        float side_move = 0.f;

        void reset()
        {
            *this = {};
        }
    };

    EdgeBugState g_edge_bug_state;

    struct PlayerSnapshot
    {
        Vector origin{};
        Vector velocity{};
        int flags = 0;
        float duck_amount = 0.f;

        void capture()
        {
            origin = g_LocalPlayer->m_vecOrigin();
            velocity = g_LocalPlayer->m_vecVelocity();
            flags = g_LocalPlayer->m_fFlags();
            duck_amount = g_LocalPlayer->m_flDuckAmount();
        }

        void restore() const
        {
            g_LocalPlayer->m_vecOrigin() = origin;
            g_LocalPlayer->m_vecVelocity() = velocity;
            g_LocalPlayer->m_fFlags() = flags;
            g_LocalPlayer->m_flDuckAmount() = duck_amount;
        }
    };

    bool IsInvalidMoveType(const int move_type)
    {
        return move_type == MOVETYPE_LADDER || move_type == MOVETYPE_NOCLIP || move_type == MOVETYPE_FLY || move_type == MOVETYPE_OBSERVER;
    }

    float NormalizeYaw(float yaw)
    {
        return std::remainderf(yaw, 360.f);
    }

    void Simulate(PredictionSystem& prediction, CUserCmd* cmd)
    {
        prediction.StartPrediction(cmd);
        prediction.EndPrediction();
    }

    void DetectEdgeBug(CUserCmd* cmd, const int original_flags, const Vector& original_velocity, const Vector& original_origin)
    {
        const auto current_velocity = g_LocalPlayer->m_vecVelocity();
        const auto current_flags = g_LocalPlayer->m_fFlags();

        if (original_velocity.z > 0.f || IsInvalidMoveType(g_LocalPlayer->m_nMoveType()))
        {
            g_edge_bug_state.will_edgebug = false;
            g_edge_bug_state.will_fail = true;
            return;
        }

        if (std::round(current_velocity.z) == 0.f || (original_flags & FL_ONGROUND))
        {
            g_edge_bug_state.will_edgebug = false;
            g_edge_bug_state.will_fail = true;
            return;
        }

        if (original_velocity.z < -6.f && current_velocity.z > original_velocity.z && current_velocity.z < -6.f &&
            !(current_flags & FL_ONGROUND) && original_origin.z > g_LocalPlayer->m_vecOrigin().z)
        {
            const auto gravity = g_CVar->FindVar("sv_gravity")->GetFloat();
            if (std::floor(original_velocity.z) < -7.f && std::floor(current_velocity.z) == -7.f &&
                current_velocity.Length2D() >= original_velocity.Length2D())
            {
                g_edge_bug_state.will_edgebug = true;
                g_edge_bug_state.will_fail = false;
                return;
            }

            const auto previous_velocity = current_velocity.z;
            const auto expected_vertical_velocity = std::roundf((-gravity) * g_GlobalVars->interval_per_tick + previous_velocity);
            g_edge_bug_state.will_edgebug = expected_vertical_velocity == std::roundf(g_LocalPlayer->m_vecVelocity().z);
            g_edge_bug_state.will_fail = !g_edge_bug_state.will_edgebug;
        }
    }
}

namespace HotwheelsMovement
{
    void BunnyHop(CUserCmd* cmd)
    {
        if (!(g_LocalPlayer->m_fFlags() & FL_ONGROUND))
            cmd->buttons &= ~IN_JUMP;
    }

    void JumpBug(CUserCmd* cmd, const int original_flags)
    {
        static bool ducked = false;

        if (!(cmd->buttons & IN_JUMP))
        {
            if ((g_LocalPlayer->m_fFlags() & FL_ONGROUND) && !(original_flags & FL_ONGROUND) && !ducked)
            {
                cmd->buttons |= IN_DUCK;
                ducked = true;
            }
            else
            {
                ducked = false;
            }

            if ((original_flags & FL_ONGROUND) && ducked)
                ducked = false;
        }
        else
        {
            if ((g_LocalPlayer->m_fFlags() & FL_ONGROUND) && !(original_flags & FL_ONGROUND))
                cmd->buttons |= IN_DUCK;

            if (g_LocalPlayer->m_fFlags() & FL_ONGROUND)
                cmd->buttons &= ~IN_JUMP;

            if (!(g_LocalPlayer->m_fFlags() & FL_ONGROUND) && (original_flags & FL_ONGROUND))
                cmd->buttons &= ~IN_DUCK;
        }
    }

    void EdgeBug(CUserCmd* cmd, const int original_flags, PredictionSystem& prediction)
    {
        if (!g_Options.edge_bug || !GetAsyncKeyState(g_Options.edge_bug_key) || !g_LocalPlayer->IsAlive())
        {
            g_edge_bug_state.reset();
            return;
        }

        constexpr int edge_bug_ticks = 32;
        constexpr float edge_bug_strafe_delta_max = 10.f;
        const auto original_forward_move = cmd->forwardmove;
        const auto original_side_move = cmd->sidemove;
        const auto original_viewangles = cmd->viewangles;
        static float last_tick_yaw = original_viewangles.yaw;
        const auto yaw_delta = std::clamp(original_viewangles.yaw - last_tick_yaw,
            -180.f / edge_bug_ticks, 180.f / edge_bug_ticks);
        last_tick_yaw = original_viewangles.yaw;
        const auto original_velocity = g_LocalPlayer->m_vecVelocity();
        const auto original_origin = g_LocalPlayer->m_vecOrigin();
        const PlayerSnapshot baseline = [&]
        {
            PlayerSnapshot snapshot;
            snapshot.capture();
            return snapshot;
        }();

        const auto loop_through_ticks = [&](const bool ducked, const bool strafe)
        {
            if (g_edge_bug_state.will_edgebug)
                return;

            g_edge_bug_state.starting_yaw = original_viewangles.yaw;

            for (int i = 0; i <= edge_bug_ticks; ++i)
            {
                baseline.restore();
                CUserCmd simulated_cmd = *cmd;
                simulated_cmd.buttons |= IN_BULLRUSH;

                if (ducked)
                {
                    simulated_cmd.buttons |= IN_DUCK;
                    g_LocalPlayer->m_fFlags() |= FL_DUCKING;
                }
                else
                {
                    simulated_cmd.buttons &= ~IN_DUCK;
                    g_LocalPlayer->m_fFlags() &= ~FL_DUCKING;
                }

                if (!strafe)
                {
                    g_edge_bug_state.strafing = false;
                    simulated_cmd.forwardmove = 0.f;
                    simulated_cmd.sidemove = 0.f;
                    simulated_cmd.buttons &= ~(IN_JUMP | IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
                }
                else
                {
                    g_edge_bug_state.strafing = true;
                    simulated_cmd.forwardmove = original_forward_move;
                    simulated_cmd.sidemove = original_side_move;
                    simulated_cmd.viewangles.yaw = NormalizeYaw(original_viewangles.yaw + (yaw_delta * i));
                }

                Simulate(prediction, &simulated_cmd);

                if ((original_flags & FL_ONGROUND) || std::round(original_velocity.z) >= 0.f ||
                    (g_LocalPlayer->m_fFlags() & FL_ONGROUND) || g_LocalPlayer->m_nMoveType() == MOVETYPE_LADDER)
                {
                    g_edge_bug_state.will_edgebug = false;
                    break;
                }

                if (!g_edge_bug_state.will_edgebug)
                    DetectEdgeBug(&simulated_cmd, original_flags, original_velocity, original_origin);

                if (g_edge_bug_state.will_edgebug)
                {
                    g_edge_bug_state.saved_mousedx = static_cast<short>(std::abs(simulated_cmd.mousedx));
                    g_edge_bug_state.ticks_to_stop = i + 1;
                    g_edge_bug_state.last_tick = g_GlobalVars->tickcount;
                    g_edge_bug_state.ducking = ducked;

                    if (strafe)
                    {
                        g_edge_bug_state.yaw_delta = yaw_delta;
                        g_edge_bug_state.forward_move = original_forward_move;
                        g_edge_bug_state.side_move = original_side_move;
                    }
                    else
                    {
                        g_edge_bug_state.forward_move = 0.f;
                        g_edge_bug_state.side_move = 0.f;
                    }
                    break;
                }

                if (g_edge_bug_state.will_fail)
                {
                    g_edge_bug_state.will_fail = false;
                    break;
                }
            }
        };

        loop_through_ticks(false, false);
        loop_through_ticks(true, false);

        if (!g_edge_bug_state.will_edgebug && std::abs(yaw_delta) < edge_bug_strafe_delta_max / 10000.f)
        {
            loop_through_ticks(false, true);
            loop_through_ticks(true, true);
        }

        baseline.restore();

        if (!g_edge_bug_state.will_edgebug)
            return;

        if (g_GlobalVars->tickcount < g_edge_bug_state.ticks_to_stop + g_edge_bug_state.last_tick + 1)
        {
            cmd->buttons &= ~(IN_JUMP | IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
            cmd->forwardmove = g_edge_bug_state.forward_move;
            cmd->sidemove = g_edge_bug_state.side_move;
            if (g_edge_bug_state.strafing)
            {
                cmd->viewangles.yaw = NormalizeYaw(g_edge_bug_state.starting_yaw +
                    (g_edge_bug_state.yaw_delta * (g_GlobalVars->tickcount - g_edge_bug_state.last_tick)));
            }

            if (g_edge_bug_state.ducking)
                cmd->buttons |= IN_DUCK;
            else
                cmd->buttons &= ~IN_DUCK;
        }
        else
        {
            g_edge_bug_state.reset();
        }
    }

    void Reset()
    {
        g_edge_bug_state.reset();
    }
}
