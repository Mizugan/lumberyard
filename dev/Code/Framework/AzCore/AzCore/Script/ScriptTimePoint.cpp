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

#if !defined(AZCORE_EXCLUDE_LUA)

#include <AzCore/Script/ScriptTimePoint.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/Component.h>

namespace AZ
{
    void ScriptTimePoint::Reflect(ReflectContext* reflection)
    {
        BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->Class<ScriptTimePoint>()->
                Method("ToString", &ScriptTimePoint::ToString)->
                    Attribute(Script::Attributes::Operator,Script::Attributes::OperatorType::ToString)->
                Method("GetSeconds", &ScriptTimePoint::GetSeconds)->
                Method("GetMilliseconds", &ScriptTimePoint::GetMilliseconds)
            ;
        }

        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<AZStd::chrono::system_clock::time_point>()
                ;

            serializeContext->Class<ScriptTimePoint>()
                ->Field("m_timePoint", &ScriptTimePoint::m_timePoint);
        }
    }
}

#endif // #if !defined(AZCORE_EXCLUDE_LUA)