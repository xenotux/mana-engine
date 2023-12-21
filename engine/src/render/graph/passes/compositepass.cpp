/**
 *  xEngine - C++ Game Engine Library
 *  Copyright (C) 2023  Julian Zampiccoli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "xng/render/graph/passes/compositepass.hpp"

#include "xng/render/graph/framegraphbuilder.hpp"

#include "xng/render/geometry/vertexstream.hpp"

#include "graph/compositepass_vs.hpp" // Generated by cmake
#include "graph/compositepass_fs.hpp" // Generated by cmake

namespace xng {
    void CompositePass::setup(FrameGraphBuilder &builder) {
        auto resolution = builder.getRenderResolution();

        if (!pipeline.assigned) {
            RenderPipelineDesc pdesc{};
            pdesc.shaders = {
                    {VERTEX,   compositepass_vs},
                    {FRAGMENT, compositepass_fs}
            };
            pdesc.bindings = {
                    BIND_TEXTURE_BUFFER,
                    BIND_TEXTURE_BUFFER
            };
            pdesc.primitive = TRIANGLES;
            pdesc.vertexLayout = mesh.vertexLayout;
            pdesc.enableBlending = true;
            pdesc.enableDepthTest = true;
            pdesc.depthTestWrite = true;
            //https://stackoverflow.com/a/16938711
            pdesc.colorBlendSourceMode = SRC_ALPHA;
            pdesc.colorBlendDestinationMode = ONE_MINUS_SRC_ALPHA;
            pdesc.alphaBlendSourceMode = ONE;
            pdesc.alphaBlendDestinationMode = ONE_MINUS_SRC_ALPHA;
            pipeline = builder.createRenderPipeline(pdesc);
        }

        builder.persist(pipeline);

        auto screenColor = builder.getSlot(SLOT_SCREEN_COLOR);
        auto screenDepth = builder.getSlot(SLOT_SCREEN_DEPTH);

        auto deferredColor = builder.getSlot(SLOT_DEFERRED_COLOR);
        auto deferredDepth = builder.getSlot(SLOT_DEFERRED_DEPTH);

        auto forwardColor = builder.getSlot(SLOT_FORWARD_COLOR);
        auto forwardDepth = builder.getSlot(SLOT_FORWARD_DEPTH);

        auto backgroundColor = builder.getSlot(SLOT_BACKGROUND_COLOR);

        if (!vertexBuffer.assigned) {
            VertexBufferDesc desc;
            desc.size = mesh.vertices.size() * mesh.vertexLayout.getSize();
            vertexBuffer = builder.createVertexBuffer(desc);

            builder.upload(vertexBuffer,
                           [this]() {
                               return FrameGraphUploadBuffer::createArray(VertexStream()
                                                                                  .addVertices(mesh.vertices)
                                                                                  .getVertexBuffer());
                           });
        }

        builder.persist(vertexBuffer);

        builder.blitColor(backgroundColor, screenColor, {}, {}, resolution, resolution, NEAREST, 0, 0);

        builder.beginPass({FrameGraphAttachment::texture(screenColor)},
                          FrameGraphAttachment::texture(screenDepth));

        builder.bindPipeline(pipeline);
        builder.bindVertexBuffers(vertexBuffer, {}, {}, mesh.vertexLayout, {});

        builder.bindShaderResources(std::vector<FrameGraphCommand::ShaderData>{
                FrameGraphCommand::ShaderData{deferredColor, {{FRAGMENT, ShaderResource::READ}}},
                FrameGraphCommand::ShaderData{deferredDepth, {{FRAGMENT, ShaderResource::READ}}}
        });
        builder.drawArray(DrawCall(0, mesh.vertices.size()));

        builder.bindShaderResources(std::vector<FrameGraphCommand::ShaderData>{
                FrameGraphCommand::ShaderData{forwardColor, {{FRAGMENT, ShaderResource::READ}}},
                FrameGraphCommand::ShaderData{forwardDepth, {{FRAGMENT, ShaderResource::READ}}}
        });
        builder.drawArray(DrawCall(0, mesh.vertices.size()));

        builder.finishPass();
    }

    std::type_index CompositePass::getTypeIndex() const {
        return typeid(CompositePass);
    }
}