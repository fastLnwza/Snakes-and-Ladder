#pragma once

#include "../../rendering/texture_loader.h"
#include "../../core/window.h"
#include <filesystem>

// Forward declarations
namespace game { struct RenderState; }
namespace game { namespace menu { struct MenuState; } }

namespace game
{
    namespace menu
    {
        struct MenuTextures
        {
            Texture panel{};
            bool loaded = false;
        };

        bool load_menu_textures(const std::filesystem::path& assets_dir);
        void destroy_menu_textures();
        void render_menu(const core::Window& window, const void* render_state_ptr, 
                        const MenuState& menu_state);
    }
}






