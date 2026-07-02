/*
 *
 *    Copyright (c) 2026 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
#pragma once

#include <app/clusters/humidistat-server/HumidistatCluster.h>
#include <data-model-providers/codedriven/CodeDrivenDataModelProvider.h>
#include <devices/humidifier-dehumidifier/HumidifierDehumidifierDevice.h>
#include <platform/DefaultTimerDelegate.h>

namespace chip {
namespace app {

/**
 * @brief A Humidifier/Dehumidifier device that simulates humidity control.
 *
 * Every kSimulationTickIntervalSec seconds the device steps its simulated
 * relative humidity toward the Humidistat target setpoint, updating
 * SystemState and the RelativeHumidityMeasurement cluster accordingly.
 * Reacts immediately to mode, setpoint, and continuous-mode changes via
 * HumidistatDelegate callbacks.
 */
class SimulatingHumidifierDehumidifierDevice : public HumidifierDehumidifierDevice,
                                               public TimerContext,
                                               public Clusters::HumidistatDelegate
{
public:
    SimulatingHumidifierDehumidifierDevice();
    ~SimulatingHumidifierDehumidifierDevice() override;

    CHIP_ERROR Register(EndpointId endpoint, CodeDrivenDataModelProvider & provider,
                        EndpointComposition composition = {}) override;
    void Unregister(CodeDrivenDataModelProvider & provider) override;

    // TimerContext
    void TimerFired() override;

    // HumidistatDelegate
    void OnModeChanged(Clusters::Humidistat::ModeEnum newMode) override;
    void OnSystemStateChanged(Clusters::Humidistat::SystemStateEnum newSystemState) override;
    void OnUserSetpointChanged(chip::Percent newUserSetpoint) override;
    void OnTargetSetpointChanged(chip::Percent newTargetSetpoint) override;
    void OnMistTypeChanged(chip::BitMask<Clusters::Humidistat::MistTypeBitmap> newMistType) override;
    void OnContinuousChanged(bool newContinuous) override;
    void OnSleepChanged(bool newSleep) override;
    void OnOptimalChanged(bool newOptimal) override;

private:
    void RunSimulationStep();

    DefaultTimerDelegate mTimerDelegate;

    // Simulated humidity in 0.01 % units (0–10000), i.e. 3000 == 30.00 % RH.
    uint16_t mSimulatedHumidity;
};

} // namespace app
} // namespace chip
