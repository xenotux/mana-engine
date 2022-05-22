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

#ifndef XENGINE_GBUFFERPASS_HPP
#define XENGINE_GBUFFERPASS_HPP

#include "render/graph/renderpass.hpp"
#include "render/graph/gbuffer.hpp"
#include "asset/scene.hpp"

namespace xengine {
    class XENGINE_EXPORT GBufferPass : public RenderPass {
    public:
        explicit GBufferPass(RenderDevice &device);

        void setup(FrameGraphBuilder &builder) override;

        void execute(RenderPassResources &resources, Renderer &ren, FrameGraphBlackboard &board) override;

    private:
        RenderDevice &device;

        GBuffer gBuffer;

        Scene scene;

        std::unique_ptr<TextureBuffer> defaultTexture;
        std::unique_ptr<ShaderProgram> shader;

        FrameGraphResource renderTarget;

        std::map<Uri, FrameGraphResource> sceneResources;
    };
}
#endif //XENGINE_GBUFFERPASS_HPP
