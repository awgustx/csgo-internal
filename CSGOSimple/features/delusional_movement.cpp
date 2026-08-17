#include "delusional_movement.hpp"

#include "../helpers/prediction.hpp"
#include "../options.hpp"

#include <algorithm>

namespace
{
    bool Enabled()
    {
        return g_Options.delusional_movement;
    }

    bool KeyDown(const int key)
    {
        return key != 0 && (GetAsyncKeyState(key) & 0x8000);
    }

    bool InvalidMoveType(const int move_type)
    {
        return move_type == MOVETYPE_LADDER || move_type == MOVETYPE_NOCLIP || move_type == MOVETYPE_FLY ||
               move_type == MOVETYPE_OBSERVER;
    }

    void ClearDirectionButtons(CUserCmd* cmd)
    {
        cmd->buttons &= ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
    }
}

namespace DelusionalMovement
{
    void DelayHop(CUserCmd* cmd)
    {
        if (!Enabled() || !g_Options.delusional_delay_hop || !KeyDown(g_Options.delusional_delay_hop_key) ||
            !g_LocalPlayer || !g_LocalPlayer->IsAlive())
            return;

        static int last_jump_tick = 0;
        if (InvalidMoveType(g_LocalPlayer->m_nMoveType()))
            return;

        const int delay_ticks = std::max(0, g_Options.delusional_delay_hop_ticks);
        if (g_LocalPlayer->m_fFlags() & FL_ONGROUND)
        {
            if (cmd->tick_count - last_jump_tick >= delay_ticks)
            {
                cmd->buttons |= IN_JUMP;
                last_jump_tick = cmd->tick_count;
            }
        }
        else
        {
            last_jump_tick = cmd->tick_count;
        }
    }

    void NullStrafing(CUserCmd* cmd)
    {
        if (!Enabled() || !g_Options.delusional_null_strafing || !g_LocalPlayer || !g_LocalPlayer->IsAlive() ||
            (g_LocalPlayer->m_fFlags() & FL_ONGROUND))
            return;

        if (cmd->mousedx > 0 && (cmd->buttons & IN_MOVERIGHT) && (cmd->buttons & IN_MOVELEFT))
            cmd->sidemove = -450.f;
        else if (cmd->mousedx < 0 && (cmd->buttons & IN_MOVELEFT) && (cmd->buttons & IN_MOVERIGHT))
            cmd->sidemove = 450.f;
    }

    void CrouchBug(CUserCmd* cmd, const int original_flags)
    {
        if (!Enabled() || !g_Options.delusional_crouch_bug || !KeyDown(g_Options.delusional_crouch_bug_key) ||
            !g_LocalPlayer || !g_LocalPlayer->IsAlive())
            return;

        if (!(original_flags & FL_ONGROUND) && (g_LocalPlayer->m_fFlags() & FL_ONGROUND))
            cmd->buttons |= IN_DUCK;
    }

    void LadderGlide(CUserCmd* cmd)
    {
        if (!Enabled() || !g_Options.delusional_ladder_glide || !KeyDown(g_Options.delusional_ladder_glide_key) ||
            !g_LocalPlayer || !g_LocalPlayer->IsAlive())
            return;

        if (g_LocalPlayer->m_nMoveType() == MOVETYPE_LADDER)
        {
            cmd->upmove = 450.f;
            cmd->buttons |= IN_JUMP;
        }
    }

    void OnCreateMovePre(CUserCmd* cmd)
    {
        if (!cmd || !Enabled())
            return;

        DelayHop(cmd);
        NullStrafing(cmd);
    }

    void OnCreateMovePost(CUserCmd* cmd, const int original_flags, PredictionSystem& prediction)
    {
        if (!cmd || !Enabled())
            return;

        (void)prediction;
        CrouchBug(cmd, original_flags);
        LadderGlide(cmd);
    }

    void Reset()
    {
    }
}
