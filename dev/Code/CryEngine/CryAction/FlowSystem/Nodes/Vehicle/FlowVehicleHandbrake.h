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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Implements a flow node to enable/disable vehicle handbrake.

#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEHANDBRAKE_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEHANDBRAKE_H
#pragma once

#include "FlowVehicleBase.h"

class CFlowVehicleHandbrake
    : public CFlowVehicleBase
{
public:

    CFlowVehicleHandbrake(SActivationInfo* pActivationInfo);

    virtual ~CFlowVehicleHandbrake();

    // CFlowBaseNode

    virtual IFlowNodePtr Clone(SActivationInfo* pActivationInfo);

    virtual void GetConfiguration(SFlowNodeConfig& nodeConfig);

    virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo);

    // ~CFlowBaseNode

    // IVehicleEventListener

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);

    // ~IVehicleEventListener

    virtual void GetMemoryUsage(ICrySizer* pCrySizer) const;

protected:

    enum EInputs
    {
        eIn_Activate = 0,
        eIn_Deactivate,
        eIn_ResetTimer
    };
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEHANDBRAKE_H
