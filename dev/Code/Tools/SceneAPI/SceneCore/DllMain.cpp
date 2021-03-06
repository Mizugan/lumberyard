/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#if !defined(AZ_MONOLITHIC_BUILD)

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Components/Utilities/EntityConstructor.h>

#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkeletonGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IAnimationGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IBlendShapeRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ICommentRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMeshAdvancedRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IOriginRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IPhysicsRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ILodRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ISkeletonProxyRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBlendShapeData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMaterialData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>

#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/Export/MtlMaterialExporter.h>
#include <SceneAPI/SceneCore/Import/ManifestImportRequestHandler.h>
#include <SceneAPI/SceneCore/Utilities/PatternMatcher.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

static AZ::Entity* g_behaviors = nullptr;
static AZ::SceneAPI::Import::ManifestImportRequestHandler* g_manifestImporter = nullptr;
static AZStd::vector<AZ::ComponentDescriptor*> g_componentDescriptors;

extern "C" AZ_DLL_EXPORT void InitializeDynamicModule(void* env)
{
    AZ::Environment::Attach(static_cast<AZ::EnvironmentInstance>(env));

    // Explicitly creating this component early as this currently needs to be available to the 
    //      RC before Gems are loaded in order to know the file extension.
    if (!g_manifestImporter)
    {
        g_manifestImporter = aznew AZ::SceneAPI::Import::ManifestImportRequestHandler();
        g_manifestImporter->Activate();
    }
}

bool IMeshGroupConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
{
    if (classElement.GetVersion() == 1)
    {
        // There have been 2 version of IMeshGroup, one that directly inherited from IGroup and one that 
        //      inherited as IMeshGroup : ISceneNodeGroup (was IMeshBaseGroup) : IGroup. To fix this, check
        //      if {1D20FA11-B184-429E-8C86-745852234845} (ISceneNodeGroup) is present and if not add it.

        AZ::SerializeContext::DataElementNode& baseClass = classElement.GetSubElement(0);
        if (baseClass.GetId() != AZ::SceneAPI::DataTypes::ISceneNodeGroup::TYPEINFO_Uuid())
        {
            if (!baseClass.Convert<AZ::SceneAPI::DataTypes::ISceneNodeGroup>(context))
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Failed to upgrade IMeshGroup from version 1.");
                return false;
            }
        }
    }
    return true;
}

extern "C" AZ_DLL_EXPORT void Reflect(AZ::SerializeContext* context)
{
    if (!context)
    {
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
    }

    // Check if this library hasn't already been reflected. This can happen as the ResourceCompilerScene needs
    //      to explicitly load and reflect the SceneAPI libraries to discover the available extension, while
    //      Gems with system components need to do the same in the Project Configurator.
    if (context && (context->IsRemovingReflection() || !context->FindClassData(AZ::SceneAPI::DataTypes::IGroup::TYPEINFO_Uuid())))
    {
        context->Class<AZ::SceneAPI::DataTypes::IManifestObject>();
        // Register components
        AZ::SceneAPI::SceneCore::BehaviorComponent::Reflect(context);
        AZ::SceneAPI::SceneCore::LoadingComponent::Reflect(context);
        AZ::SceneAPI::SceneCore::ExportingComponent::Reflect(context);
        // Register group interfaces
        context->Class<AZ::SceneAPI::DataTypes::IGroup, AZ::SceneAPI::DataTypes::IManifestObject>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::ISceneNodeGroup, AZ::SceneAPI::DataTypes::IGroup>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::IMeshGroup, AZ::SceneAPI::DataTypes::ISceneNodeGroup>()->Version(2, &IMeshGroupConverter);
        context->Class<AZ::SceneAPI::DataTypes::ISkeletonGroup, AZ::SceneAPI::DataTypes::IGroup>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::ISkinGroup, AZ::SceneAPI::DataTypes::ISceneNodeGroup>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::IAnimationGroup, AZ::SceneAPI::DataTypes::IGroup>()->Version(1);

        // Register rule interfaces
        context->Class<AZ::SceneAPI::DataTypes::IRule, AZ::SceneAPI::DataTypes::IManifestObject>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::IBlendShapeRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::ICommentRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::IMaterialRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::IMeshAdvancedRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::IOriginRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::IPhysicsRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::ILodRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::ISkeletonProxyRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
        // Register graph data interfaces
        context->Class<AZ::SceneAPI::DataTypes::IAnimationData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::IBlendShapeData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::IBoneData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::IMaterialData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::IMeshData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::IMeshVertexColorData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::IMeshVertexUVData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::ISkinWeightData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
        context->Class<AZ::SceneAPI::DataTypes::ITransform, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);

        // Register base manifest types
        context->Class<AZ::SceneAPI::DataTypes::ISceneNodeSelectionList>()->Version(1);

        // Register containers
        AZ::SceneAPI::Containers::RuleContainer::Reflect(context);
        AZ::SceneAPI::Containers::SceneManifest::Reflect(context);

       // Register utilities
        AZ::SceneAPI::SceneCore::PatternMatcher::Reflect(context);
    }

    if (g_componentDescriptors.empty())
    {
        g_componentDescriptors.push_back(AZ::SceneAPI::Export::MaterialExporterComponent::CreateDescriptor());
        for (AZ::ComponentDescriptor* descriptor : g_componentDescriptors)
        {
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Handler::RegisterComponentDescriptor, descriptor);
        }
    }
}

extern "C" AZ_DLL_EXPORT void Activate()
{
    if (g_behaviors)
    {
        return;
    }
    g_behaviors = AZ::SceneAPI::SceneCore::EntityConstructor::BuildEntityRaw("Scene Behaviors", 
        AZ::SceneAPI::SceneCore::BehaviorComponent::TYPEINFO_Uuid());
}

extern "C" AZ_DLL_EXPORT void Deactivate()
{
    if (g_behaviors)
    {
        g_behaviors->Deactivate();
        delete g_behaviors;
        g_behaviors = nullptr;
    }
}

extern "C" AZ_DLL_EXPORT void UninitializeDynamicModule()
{
    if (!g_componentDescriptors.empty())
    {
        for (AZ::ComponentDescriptor* descriptor : g_componentDescriptors)
        {
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Handler::UnregisterComponentDescriptor, descriptor);
        }
        g_componentDescriptors.clear();
        g_componentDescriptors.shrink_to_fit();
    }

    if (g_manifestImporter)
    {
        g_manifestImporter->Deactivate();
        delete g_manifestImporter;
        g_manifestImporter = nullptr;
    }

    AZ::Environment::Detach();
}

#endif // !defined(AZ_MONOLITHIC_BUILD)
