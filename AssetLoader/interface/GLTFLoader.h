/*     Copyright 2019 Diligent Graphics LLC
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#pragma once

#include <vector>
#include <memory>

#include "../../../DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "../../../DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "../../../DiligentCore/Common/interface/RefCntAutoPtr.h"
#include "../../../DiligentCore/Common/interface/AdvancedMath.h"

namespace tinygltf
{

class Node;
class Model;

} // namespace tinygltf

namespace Diligent
{

namespace GLTF
{

struct Material
{
    enum ALPHA_MODE
    {
        ALPHAMODE_OPAQUE,
        ALPHAMODE_MASK,
        ALPHAMODE_BLEND
    };
    ALPHA_MODE AlphaMode = ALPHAMODE_OPAQUE;

    float  AlphaCutoff     = 1.0f;
    float  MetallicFactor  = 1.0f;
    float  RoughnessFactor = 1.0f;
    float4 BaseColorFactor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 EmissiveFactor  = float4(1.0f, 1.0f, 1.0f, 1.0f);

    RefCntAutoPtr<ITexture> pBaseColorTexture;
    RefCntAutoPtr<ITexture> pMetallicRoughnessTexture;
    RefCntAutoPtr<ITexture> pNormalTexture;
    RefCntAutoPtr<ITexture> pOcclusionTexture;
    RefCntAutoPtr<ITexture> pEmissiveTexture;

    struct TextureCoordinateSets
    {
        Uint8 BaseColor          = 0;
        Uint8 MetallicRoughness  = 0;
        Uint8 SpecularGlossiness = 0;
        Uint8 Normal             = 0;
        Uint8 Occlusion          = 0;
        Uint8 Emissive           = 0;
    };
    TextureCoordinateSets TexCoordSets;

    struct Extension
    {
        RefCntAutoPtr<ITexture> pSpecularGlossinessTexture;
        RefCntAutoPtr<ITexture> pDiffuseTexture;
        float4                  DiffuseFactor  = float4(1.0f, 1.0f, 1.0f, 1.0f);
        float3                  SpecularFactor = float3(1.0f, 1.0f, 1.0f);
    };
    Extension extension;

    enum class PbrWorkflow
    {
        MetallicRoughness,
        SpecularGlossiness
    };
    PbrWorkflow workflow = PbrWorkflow::MetallicRoughness;
};


struct Primitive
{
    Uint32    FirstIndex  = 0;
    Uint32    IndexCount  = 0;
    Uint32    VertexCount = 0;
    Material& material;
    bool      hasIndices;

    BoundBox BB;
    bool     IsValidBB = false;

    Primitive(Uint32    _FirstIndex,
              Uint32    _IndexCount,
              Uint32    _VertexCount,
              Material& _material) :
        FirstIndex{_FirstIndex},
        IndexCount{_IndexCount},
        VertexCount{_VertexCount},
        material{_material},
        hasIndices{_IndexCount > 0}
    {
    }

    void SetBoundingBox(const float3& min, const float3& max)
    {
        BB.Min    = min;
        BB.Max    = max;
        IsValidBB = true;
    }
};



struct Mesh
{
    std::vector<std::unique_ptr<Primitive>> Primitives;

    BoundBox BB;
    BoundBox AABB;
    bool     IsValidBB = false;

    struct TransformData
    {
        static constexpr Uint32 MaxNumJoints = 128u;

        float4x4 matrix;
        float4x4 jointMatrix[MaxNumJoints] = {};
        int      jointcount                = 0;
    };

    TransformData Transforms;

    Mesh(IRenderDevice* pDevice, const float4x4& matrix);
    void SetBoundingBox(const float3& min, const float3& max);
};


struct Node;
struct Skin
{
    std::string           Name;
    Node*                 pSkeletonRoot = nullptr;
    std::vector<float4x4> InverseBindMatrices;
    std::vector<Node*>    Joints;
};


struct Node
{
    std::string Name;
    Node*       Parent = nullptr;
    Uint32      Index;

    std::vector<std::unique_ptr<Node>> Children;

    float4x4              Matrix;
    std::unique_ptr<Mesh> _Mesh;
    Skin*                 _Skin     = nullptr;
    Int32                 SkinIndex = -1;
    float3                Translation;
    float3                Scale = float3(1.0f, 1.0f, 1.0f);
    Quaternion            Rotation;
    BoundBox              BVH;
    BoundBox              AABB;
    bool                  IsValidBVH = false;

    float4x4 LocalMatrix() const;
    float4x4 GetMatrix() const;
    void     Update();
};


struct AnimationChannel
{
    enum PATH_TYPE
    {
        TRANSLATION,
        ROTATION,
        SCALE
    };
    PATH_TYPE PathType;
    Node*     node         = nullptr;
    Uint32    SamplerIndex = static_cast<Uint32>(-1);
};


struct AnimationSampler
{
    enum INTERPOLATION_TYPE
    {
        LINEAR,
        STEP,
        CUBICSPLINE
    };
    INTERPOLATION_TYPE  Interpolation;
    std::vector<float>  Inputs;
    std::vector<float4> OutputsVec4;
};

struct Animation
{
    std::string                   Name;
    std::vector<AnimationSampler> Samplers;
    std::vector<AnimationChannel> Channels;

    float Start = std::numeric_limits<float>::max();
    float End   = std::numeric_limits<float>::min();
};


struct Model
{
    struct Vertex
    {
        float3 pos;
        float3 normal;
        float2 uv0;
        float2 uv1;
        float4 joint0;
        float4 weight0;
    };

    RefCntAutoPtr<IBuffer> pVertexBuffer;
    RefCntAutoPtr<IBuffer> pIndexBuffer;
    Uint32                 IndexCount = 0;

    float4x4 aabb;

    std::vector<std::unique_ptr<Node>> Nodes;
    std::vector<Node*>                 LinearNodes;

    std::vector<std::unique_ptr<Skin>> Skins;

    std::vector<RefCntAutoPtr<ITexture>> Textures;
    std::vector<RefCntAutoPtr<ISampler>> TextureSamplers;
    std::vector<Material>                Materials;
    std::vector<Animation>               Animations;
    std::vector<std::string>             Extensions;

    struct Dimensions
    {
        float3 min = float3{+FLT_MAX, +FLT_MAX, +FLT_MAX};
        float3 max = float3{-FLT_MAX, -FLT_MAX, -FLT_MAX};
    } dimensions;

    Model(IRenderDevice* pDevice, IDeviceContext* pContext, const std::string& filename);

    void UpdateAnimation(Uint32 index, float time);

private:
    void LoadFromFile(IRenderDevice* pDevice, IDeviceContext* pContext, const std::string& filename);
    void LoadNode(IRenderDevice*         pDevice,
                  Node*                  parent,
                  const tinygltf::Node&  gltf_node,
                  uint32_t               nodeIndex,
                  const tinygltf::Model& gltf_model,
                  std::vector<uint32_t>& indexBuffer,
                  std::vector<Vertex>&   vertexBuffer);

    void LoadSkins(const tinygltf::Model& gltf_model);

    void LoadTextures(IRenderDevice*         pDevice,
                      IDeviceContext*        pCtx,
                      const tinygltf::Model& gltf_model);

    void  LoadTextureSamplers(IRenderDevice* pDevice, const tinygltf::Model& gltf_model);
    void  LoadMaterials(const tinygltf::Model& gltf_model);
    void  LoadAnimations(const tinygltf::Model& gltf_model);
    void  CalculateBoundingBox(Node* node, const Node* parent);
    void  GetSceneDimensions();
    Node* FindNode(Node* parent, Uint32 index);
    Node* NodeFromIndex(uint32_t index);
};

} // namespace GLTF

} // namespace Diligent
