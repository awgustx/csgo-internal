#pragma once


#include "../../dependencies/imgui/imgui.h"
#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../movement/movement.h"


#ifdef _DEBUG


// debugger menu


namespace n_debugger
{
	struct impl_t {
		void draw_server_hitbox_model( )
		{
			static auto get_player_by_index = []( int index ) {
				static void* pattern = reinterpret_cast< void* >( g_modules[ SERVER_DLL ].find_pattern( "85 C9 7E 32 A1 ? ? ? ?" ) );


				auto util_player_by_index = reinterpret_cast< uintptr_t( __fastcall* )( unsigned int ) >( pattern );


				return util_player_by_index( index );
			};


			static void* pattern =
				reinterpret_cast< void* >( g_modules[ SERVER_DLL ].find_pattern( "55 8B EC 81 EC ? ? ? ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE" ) );


			for ( int i = 1; i <= g_interfaces.m_global_vars_base->m_max_clients; i++ ) {
				uintptr_t player = get_player_by_index( i );
				if ( !player )
					return;


				static float duration = -1.0f;


				__asm
				{
					pushad
					movss xmm1, duration
					push 1
					mov ecx, player
					call pattern
					popad
				}
			}
		}


		void on_paint_traverse( )
		{
			if ( !GET_VARIABLE( g_variables.m_debugger_visual, bool ) )
				return;


			float offset = 0.f;


			constexpr auto render_debug = [ & ]( const char* indicator_name, const c_color& color, const bool active ) {
				ImAnimationHelper debug_animation = ImAnimationHelper( fnv1a::hash( indicator_name ), ImGui::GetIO( ).DeltaTime );
				debug_animation.Update( 2.f, active ? 2.f : -2.f );


				if ( debug_animation.AnimationData->second <= 0.f )
					return;


				const auto text_size = g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ]->CalcTextSizeA(
					g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ]->FontSize, FLT_MAX, 0.f, indicator_name );


				g_render.m_draw_data.emplace_back(
					e_draw_type::draw_type_text,
					std::make_any< text_draw_object_t >(
						g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ],
