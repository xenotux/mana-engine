/**
 *  xEngine - C++ game engine library
 *  Copyright (C) 2021  Julian Zampiccoli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef XENGINE_SKYBOXPASS_HPP
#define XENGINE_SKYBOXPASS_HPP

#include "render/deferred/renderpass.hpp"

namespace xengine {
    class XENGINE_EXPORT SkyboxPass : public RenderPass {
    public:
        explicit SkyboxPass(RenderDevice &device);

        ~SkyboxPass() override = default;

        void render(GBuffer &gBuffer, Scene &scene) override;

        void resize(Vec2i size, int samples) override;

    private:
        std::unique_ptr<ShaderProgram> shader;
        std::unique_ptr<MeshBuffer> meshBuffer;
        std::unique_ptr<TextureBuffer> defaultTexture;

        TextureBuffer *texture = nullptr;
        ColorRGBA color;

        ShaderSource vert;
        ShaderSource frag;
    };
}

#endif //XENGINE_SKYBOXPASS_HPP
