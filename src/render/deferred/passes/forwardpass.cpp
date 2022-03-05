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

#include "render/deferred/passes/forwardpass.hpp"

namespace xengine {
    ForwardPass::ForwardPass(RenderDevice &device)
            : RenderPass(device), pipeline(device) {}

    ForwardPass::~ForwardPass() = default;

    void ForwardPass::render(GBuffer &gBuffer, Scene &scene) {
        auto &target = gBuffer.getPassTarget();

        target.setNumberOfColorAttachments(1);
        target.attachColor(0, *output.color);
        target.attachDepthStencil(*output.depth);

        pipeline.render(target, scene);

        target.detachColor(0);
        target.detachDepthStencil();
    }
}