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

#include "xng/render/graph/passes/deferredlightingpass.hpp"

#include "xng/render/graph/framegraphbuilder.hpp"

#include "xng/render/graph/passes/constructionpass.hpp"
#include "xng/render/graph/framegraphsettings.hpp"

#include "xng/render/geometry/vertexstream.hpp"

#include "graph/deferredlightingpass_vs.hpp" // Generated by cmake
#include "graph/deferredlightingpass_fs.hpp" // Generated by cmake

namespace xng {
#pragma pack(push, 1)
    struct PointLightData {
        std::array<float, 4> position;
        std::array<float, 4> color;
        std::array<float, 4> farPlane;
    };

    struct DirectionalLightData {
        std::array<float, 4> direction;
        std::array<float, 4> color;
        std::array<float, 4> farPlane;
    };

    struct SpotLightData {
        std::array<float, 4> position;
        std::array<float, 4> direction_quadratic;
        std::array<float, 4> color;
        std::array<float, 4> farPlane;
        std::array<float, 4> cutOff_outerCutOff_constant_linear;
    };

    struct ShaderStorageData {
        std::array<float, 4> viewPosition{};
        std::array<int, 4> enableShadows{};
    };
#pragma pack(pop)

    static std::pair<std::vector<PointLightData>, std::vector<PointLightData>> getPointLights(const Scene &scene) {
        std::vector<PointLightData> lights;
        std::vector<PointLightData> shadowLights;
        for (auto &node: scene.rootNode.findAll({typeid(PointLightProperty)})) {
            auto l = node.getProperty<PointLightProperty>().light;
            auto t = node.getProperty<TransformProperty>().transform;
            auto v = l.color.divide();
            auto tmp = PointLightData{
                    .position =  Vec4f(t.getPosition().x,
                                       t.getPosition().y,
                                       t.getPosition().z,
                                       0).getMemory(),
                    .color = Vec4f(v.x * l.power, v.y * l.power, v.z * l.power, 1).getMemory(),
                    .farPlane = Vec4f(l.shadowFarPlane, 0, 0, 0).getMemory()
            };
            if (l.castShadows)
                shadowLights.emplace_back(tmp);
            else
                lights.emplace_back(tmp);
        }
        return {lights, shadowLights};
    }

    static std::pair<std::vector<DirectionalLightData>, std::vector<DirectionalLightData>>
    getDirLights(const Scene &scene) {
        std::vector<DirectionalLightData> lights;
        std::vector<DirectionalLightData> shadowLights;
        for (auto &node: scene.rootNode.findAll({typeid(DirectionalLightProperty)})) {
            auto l = node.getProperty<DirectionalLightProperty>().light;
            auto v = l.color.divide();
            auto tmp = DirectionalLightData{
                    .direction =  Vec4f(l.direction.x,
                                        l.direction.y,
                                        l.direction.z,
                                        0).getMemory(),
                    .color = Vec4f(v.x * l.power, v.y * l.power, v.z * l.power, 1).getMemory(),
                    .farPlane = Vec4f(l.shadowFarPlane, 0, 0, 0).getMemory()
            };
            if (l.castShadows)
                shadowLights.emplace_back(tmp);
            else
                lights.emplace_back(tmp);
        }
        return {lights, shadowLights};
    }

    static float getCutOff(float angleDegrees){
        return std::cos(degreesToRadians(angleDegrees));
    }

    static std::pair<std::vector<SpotLightData>, std::vector<SpotLightData>> getSpotLights(const Scene &scene) {
        std::vector<SpotLightData> lights;
        std::vector<SpotLightData> shadowLights;
        for (auto &node: scene.rootNode.findAll({typeid(SpotLightProperty)})) {
            auto l = node.getProperty<SpotLightProperty>().light;
            auto t = node.getProperty<TransformProperty>().transform;
            auto v = l.color.divide();
            auto tmp = SpotLightData{
                    .position =  Vec4f(t.getPosition().x,
                                       t.getPosition().y,
                                       t.getPosition().z,
                                       0).getMemory(),
                    .direction_quadratic =  Vec4f(l.direction.x,
                                                  l.direction.y,
                                                  l.direction.z,
                                                  l.quadratic).getMemory(),
                    .color = Vec4f(v.x * l.power, v.y * l.power, v.z * l.power, 1).getMemory(),
                    .farPlane = Vec4f(l.shadowFarPlane, 0, 0, 0).getMemory(),
                    .cutOff_outerCutOff_constant_linear = Vec4f(getCutOff(l.cutOff),
                                                                getCutOff(l.outerCutOff),
                                                                l.constant,
                                                                l.linear).getMemory()
            };
            if (l.castShadows)
                shadowLights.emplace_back(tmp);
            else
                lights.emplace_back(tmp);
        }
        return {lights, shadowLights};
    }

    DeferredLightingPass::DeferredLightingPass() = default;

    void DeferredLightingPass::setup(FrameGraphBuilder &builder) {
        scene = builder.getScene();

        if (!vertexBufferRes.assigned) {
            VertexBufferDesc desc;
            desc.size = mesh.vertices.size() * mesh.vertexLayout.getSize();
            vertexBufferRes = builder.createVertexBuffer(desc);

            VertexArrayObjectDesc oDesc;
            oDesc.vertexLayout = mesh.vertexLayout;
            vertexArrayObjectRes = builder.createVertexArrayObject(oDesc);

            builder.write(vertexBufferRes);
        }

        builder.persist(vertexBufferRes);
        builder.persist(vertexArrayObjectRes);
        builder.read(vertexBufferRes);
        builder.read(vertexArrayObjectRes);

        if (!pipelineRes.assigned) {
            pipelineRes = builder.createPipeline(RenderPipelineDesc{
                    .shaders = {
                            {VERTEX,   deferredlightingpass_vs},
                            {FRAGMENT, deferredlightingpass_fs}
                    },
                    .bindings = {BIND_SHADER_STORAGE_BUFFER,
                                 BIND_TEXTURE_BUFFER,
                                 BIND_TEXTURE_BUFFER,
                                 BIND_TEXTURE_BUFFER,
                                 BIND_TEXTURE_BUFFER,
                                 BIND_TEXTURE_BUFFER,
                                 BIND_TEXTURE_BUFFER,
                                 BIND_TEXTURE_ARRAY_BUFFER,
                                 BIND_SHADER_STORAGE_BUFFER,
                                 BIND_SHADER_STORAGE_BUFFER,
                                 BIND_SHADER_STORAGE_BUFFER,
                                 BIND_SHADER_STORAGE_BUFFER,
                                 BIND_SHADER_STORAGE_BUFFER,
                                 BIND_SHADER_STORAGE_BUFFER,
                    },
                    .primitive = TRIANGLES,
                    .vertexLayout = mesh.vertexLayout,
                    .enableDepthTest = true,
                    .depthTestWrite = true,
            });
        }

        builder.persist(pipelineRes);
        builder.read(pipelineRes);

        renderSize = builder.getBackBufferDescription().size
                     * builder.getSettings().get<float>(FrameGraphSettings::SETTING_RENDER_SCALE);

        targetRes = builder.createRenderTarget(RenderTargetDesc{
                .size = renderSize,
                .numberOfColorAttachments = 1,
                .hasDepthStencilAttachment = true
        });
        builder.read(targetRes);

        colorTextureRes = builder.getSlot(SLOT_DEFERRED_COLOR);
        builder.write(colorTextureRes);

        depthTextureRes = builder.getSlot(SLOT_DEFERRED_DEPTH);
        builder.write(depthTextureRes);

        passRes = builder.createRenderPass(RenderPassDesc{
                .numberOfColorAttachments = 1,
                .hasDepthStencilAttachment = true});

        builder.read(passRes);

        auto pointLightNodes = scene.rootNode.findAll({typeid(PointLightProperty)});

        size_t pointLights = 0;
        size_t shadowPointLights = 0;

        for (auto l: pointLightNodes) {
            if (l.getProperty<PointLightProperty>().light.castShadows)
                shadowPointLights++;
            else
                pointLights++;
        }

        auto dirLightNodes = scene.rootNode.findAll({typeid(DirectionalLightProperty)});

        size_t dirLights = 0;
        size_t shadowDirLights = 0;

        for (auto l: dirLightNodes) {
            if (l.getProperty<DirectionalLightProperty>().light.castShadows)
                shadowDirLights++;
            else
                dirLights++;
        }

        auto spotLightNodes = scene.rootNode.findAll({typeid(SpotLightProperty)});

        size_t spotLights = 0;
        size_t shadowSpotLights = 0;

        for (auto l: spotLightNodes) {
            if (l.getProperty<SpotLightProperty>().light.castShadows)
                shadowSpotLights++;
            else
                spotLights++;
        }

        shaderDataBufferRes = builder.createShaderStorageBuffer(
                ShaderStorageBufferDesc{.size =  sizeof(ShaderStorageData)});
        builder.read(shaderDataBufferRes);
        builder.write(shaderDataBufferRes);

        pointLightBufferRes = builder.createShaderStorageBuffer(ShaderStorageBufferDesc{
                .size = sizeof(PointLightData) * pointLights
        });
        builder.read(pointLightBufferRes);
        builder.write(pointLightBufferRes);

        shadowPointLightBufferRes = builder.createShaderStorageBuffer(ShaderStorageBufferDesc{
                .size = sizeof(PointLightData) * shadowPointLights
        });
        builder.read(shadowPointLightBufferRes);
        builder.write(shadowPointLightBufferRes);

        dirLightBufferRes = builder.createShaderStorageBuffer(ShaderStorageBufferDesc{
                .size = sizeof(DirectionalLightData) * dirLights
        });
        builder.read(dirLightBufferRes);
        builder.write(dirLightBufferRes);

        shadowDirLightBufferRes = builder.createShaderStorageBuffer(ShaderStorageBufferDesc{
                .size = sizeof(DirectionalLightData) * shadowDirLights
        });
        builder.read(shadowDirLightBufferRes);
        builder.write(shadowDirLightBufferRes);

        spotLightBufferRes = builder.createShaderStorageBuffer(ShaderStorageBufferDesc{
                .size = sizeof(SpotLightData) * spotLights
        });
        builder.read(spotLightBufferRes);
        builder.write(spotLightBufferRes);

        shadowSpotLightBufferRes = builder.createShaderStorageBuffer(ShaderStorageBufferDesc{
                .size = sizeof(SpotLightData) * shadowSpotLights
        });
        builder.read(shadowSpotLightBufferRes);
        builder.write(shadowSpotLightBufferRes);

        gBufferPosition = builder.getSlot(SLOT_GBUFFER_POSITION);
        builder.read(gBufferPosition);

        gBufferNormal = builder.getSlot(SLOT_GBUFFER_NORMAL);
        builder.read(gBufferNormal);

        gBufferTangent = builder.getSlot(SLOT_GBUFFER_TANGENT);
        builder.read(gBufferTangent);

        gBufferRoughnessMetallicAO = builder.getSlot(SLOT_GBUFFER_ROUGHNESS_METALLIC_AO);
        builder.read(gBufferRoughnessMetallicAO);

        gBufferAlbedo = builder.getSlot(SLOT_GBUFFER_ALBEDO);
        builder.read(gBufferAlbedo);

        gBufferModelObject = builder.getSlot(SLOT_GBUFFER_OBJECT_SHADOWS);
        builder.read(gBufferModelObject);

        gBufferDepth = builder.getSlot(SLOT_GBUFFER_DEPTH);
        builder.read(gBufferDepth);

        cameraTransform = builder.getScene().rootNode.find<CameraProperty>()
                .getProperty<TransformProperty>().transform;

        commandBuffer = builder.createCommandBuffer();
        builder.write(commandBuffer);

        this->scene = builder.getScene();

        if (builder.checkSlot(SLOT_SHADOW_MAP_POINT)) {
            pointLightShadowMapRes = builder.getSlot(FrameGraphSlot::SLOT_SHADOW_MAP_POINT);
            builder.read(pointLightShadowMapRes);
        } else {
            pointLightShadowMapRes = {};
        }

        pointLightShadowMapDefaultRes = builder.createTextureArrayBuffer({});
        builder.read(pointLightShadowMapDefaultRes);
    }

    void DeferredLightingPass::execute(FrameGraphPassResources &resources,
                                       const std::vector<std::reference_wrapper<CommandQueue>> &renderQueues,
                                       const std::vector<std::reference_wrapper<CommandQueue>> &computeQueues,
                                       const std::vector<std::reference_wrapper<CommandQueue>> &transferQueues) {
        auto &target = resources.get<RenderTarget>(targetRes);

        auto &pipeline = resources.get<RenderPipeline>(pipelineRes);
        auto &pass = resources.get<RenderPass>(passRes);

        auto &vertexBuffer = resources.get<VertexBuffer>(vertexBufferRes);
        auto &vertexArrayObject = resources.get<VertexArrayObject>(vertexArrayObjectRes);

        auto &uniformBuffer = resources.get<ShaderStorageBuffer>(shaderDataBufferRes);

        auto &pointLightBuffer = resources.get<ShaderStorageBuffer>(pointLightBufferRes);
        auto &shadowPointLightBuffer = resources.get<ShaderStorageBuffer>(shadowPointLightBufferRes);

        auto &dirLightBuffer = resources.get<ShaderStorageBuffer>(dirLightBufferRes);
        auto &shadowDirLightBuffer = resources.get<ShaderStorageBuffer>(shadowDirLightBufferRes);

        auto &spotLightBuffer = resources.get<ShaderStorageBuffer>(spotLightBufferRes);
        auto &shadowSpotLightBuffer = resources.get<ShaderStorageBuffer>(shadowSpotLightBufferRes);

        auto &colorTex = resources.get<TextureBuffer>(colorTextureRes);
        auto &depthTex = resources.get<TextureBuffer>(depthTextureRes);

        auto &cBuffer = resources.get<CommandBuffer>(commandBuffer);

        auto &pointLightShadowMap = pointLightShadowMapRes.assigned ? resources.get<TextureArrayBuffer>(
                pointLightShadowMapRes) : resources.get<TextureArrayBuffer>(pointLightShadowMapDefaultRes);

        auto pointLights = getPointLights(scene);
        auto dirLights = getDirLights(scene);
        auto spotLights = getSpotLights(scene);

        pointLightBuffer.upload(reinterpret_cast<const uint8_t *>(pointLights.first.data()),
                                pointLights.first.size() * sizeof(PointLightData));
        shadowPointLightBuffer.upload(reinterpret_cast<const uint8_t *>(pointLights.second.data()),
                                      pointLights.second.size() * sizeof(PointLightData));

        dirLightBuffer.upload(reinterpret_cast<const uint8_t *>(dirLights.first.data()),
                              dirLights.first.size() * sizeof(DirectionalLightData));
        shadowDirLightBuffer.upload(reinterpret_cast<const uint8_t *>(dirLights.second.data()),
                                      dirLights.second.size() * sizeof(DirectionalLightData));

        spotLightBuffer.upload(reinterpret_cast<const uint8_t *>(spotLights.first.data()),
                               spotLights.first.size() * sizeof(SpotLightData));
        shadowSpotLightBuffer.upload(reinterpret_cast<const uint8_t *>(spotLights.second.data()),
                                      spotLights.second.size() * sizeof(SpotLightData));

        if (!quadAllocated) {
            quadAllocated = true;
            auto verts = VertexStream().addVertices(mesh.vertices).getVertexBuffer();
            vertexBuffer.upload(0,
                                verts.data(),
                                verts.size());
            vertexArrayObject.setBuffers(vertexBuffer);
        }

        ShaderStorageData buf;
        buf.viewPosition = Vec4f(cameraTransform.getPosition().x,
                                 cameraTransform.getPosition().y,
                                 cameraTransform.getPosition().z,
                                 0).getMemory();
        buf.enableShadows.at(0) = pointLightShadowMapRes.assigned;
        uniformBuffer.upload(reinterpret_cast<const uint8_t *>(&buf),
                             sizeof(ShaderStorageData));

        auto &gBufPos = resources.get<TextureBuffer>(gBufferPosition);
        auto &gBufNorm = resources.get<TextureBuffer>(gBufferNormal);
        auto &gBufTan = resources.get<TextureBuffer>(gBufferTangent);
        auto &gBufRoughnessMetallicAO = resources.get<TextureBuffer>(gBufferRoughnessMetallicAO);
        auto &gBufAlbedo = resources.get<TextureBuffer>(gBufferAlbedo);
        auto &gBufModelObject = resources.get<TextureBuffer>(gBufferModelObject);
        auto &gBufDepth = resources.get<TextureBuffer>(gBufferDepth);

        target.setAttachments({RenderTargetAttachment::texture(colorTex)}, RenderTargetAttachment::texture(depthTex));

        std::vector<Command> commands;

        commands.emplace_back(pass.begin(target));
        commands.emplace_back(pass.setViewport({}, target.getDescription().size));

        commands.emplace_back(pipeline.bind());
        commands.emplace_back(vertexArrayObject.bind());
        commands.emplace_back(RenderPipeline::bindShaderResources({
                                                                          {uniformBuffer,           {{FRAGMENT, ShaderResource::READ}}},
                                                                          {gBufPos,                 {{FRAGMENT, ShaderResource::READ}}},
                                                                          {gBufNorm,                {{FRAGMENT, ShaderResource::READ}}},
                                                                          {gBufRoughnessMetallicAO, {{FRAGMENT, ShaderResource::READ}}},
                                                                          {gBufAlbedo,              {{FRAGMENT, ShaderResource::READ}}},
                                                                          {gBufModelObject,         {{FRAGMENT, ShaderResource::READ}}},
                                                                          {gBufDepth,               {{FRAGMENT, ShaderResource::READ}}},
                                                                          {pointLightShadowMap,     {{FRAGMENT, ShaderResource::READ}}},
                                                                          {pointLightBuffer,        {{FRAGMENT, ShaderResource::READ}}},
                                                                          {shadowPointLightBuffer,  {{FRAGMENT, ShaderResource::READ}}},
                                                                          {dirLightBuffer,  {{FRAGMENT, ShaderResource::READ}}},
                                                                          {shadowDirLightBuffer,  {{FRAGMENT, ShaderResource::READ}}},
                                                                          {spotLightBuffer,  {{FRAGMENT, ShaderResource::READ}}},
                                                                          {shadowSpotLightBuffer,  {{FRAGMENT, ShaderResource::READ}}},
                                                                  }));
        commands.emplace_back(pass.drawArray(DrawCall(0, mesh.vertices.size())));
        commands.emplace_back(pass.end());

        cBuffer.begin();
        cBuffer.add(commands);
        cBuffer.end();

        renderQueues.at(0).get().submit(cBuffer);

        target.clearAttachments();
    }

    std::type_index DeferredLightingPass::getTypeIndex() const {
        return typeid(DeferredLightingPass);
    }
}