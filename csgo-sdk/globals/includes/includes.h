#pragma once
#include <Windows.h>


#undef max
#undef min


/* macros */
#include "../../globals/macros/macros.h"


/* hiding functions from the imports table */
#include "../../dependencies/lazy_importer/lazy_importer.h"


/* media player */
#include "../media_player/media_player.h"


/* configuration system */
#include "../config/config.h"
#include "../config/variables.h"


#include "../math/math.h"


/* string hashing */
#include "../../dependencies/fnv1a/fnv1a.h"


/* menu shart */
#include "../../hacks/menu/menu.h"


/* global utilities e.g. CreateThread(); */
#include "../../utilities/utilities.h"


/* console logging */
#include "../../utilities/console/console.h"


/* module handling */
#include "../../utilities/modules/modules.h"


/* game interfaces */
#include "../interfaces/interfaces.h"


/* input system */
#include "../../utilities/input/input.h"


/* convar stuff */
#include ".././convars/convars.h"


/* netvar manager */
#include "../netvars/netvars.h"


/* renderer */
#include "../render/render.h"


/* hooks */
#include "../../hooks/hooks.h"


/* hooking dependencies */
#include "../../dependencies/minhook/include/MinHook.h"


/* ImGui */
#include "../../dependencies/imgui/imgui.h"
#include "../../dependencies/imgui/imgui_freetype.h"
#include "../../dependencies/imgui/imgui_impl_dx9.h"
#include "../../dependencies/imgui/imgui_impl_win32.h"
#include "../../dependencies/imgui/imgui_internal.h"


/* formatting strings */
#include <fmt/format.h>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace std
{
	template< typename... args_t >
	auto make_format_args( args_t&&... args )
	{
		return std::forward_as_tuple( std::forward< args_t >( args )... );
	}

	template< typename... args_t >
	std::string vformat( const std::string_view format_string, std::tuple< args_t&&... >&& args )
	{
		return std::apply(
			[&]( auto&&... values ) { return fmt::format( fmt::runtime( format_string ), std::forward< decltype( values ) >( values )... ); },
			std::move( args ) );
	}

	template< typename... args_t >
	std::string format( const std::string_view format_string, args_t&&... args )
	{
		return fmt::format( fmt::runtime( format_string ), std::forward< args_t >( args )... );
	}
} // namespace std


/* thread */
