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
#include "SimulatingHumidifierDehumidifierDevice.h"
#include <app/server/Server.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>

#include <algorithm>

using namespace chip::app::Clusters;
using namespace chip::app::Clusters::Humidistat;

namespace chip {
namespace app {

namespace {

constexpr System::Clock::Seconds16 kSimulationTickIntervalSec = System::Clock::Seconds16(5);

// Test Event Trigger IDs for Humidistat (cluster 0x0205).
constexpr uint64_t kHumidistatLowHumidityTrigger  = 0x0205000000000000ULL;
constexpr uint64_t kHumidistatHighHumidityTrigger = 0x0205000000000001ULL;

SimulatingHumidifierDehumidifierDevice * sActiveHumidistatTriggerDevice = nullptr;

constexpr uint16_t kLowHumidityForTestTrigger  = 3000; // 30.00 % RH
constexpr uint16_t kHighHumidityForTestTrigger = 7000; // 70.00 % RH

// Step size per tick: 200 units == 2.00 % RH.
constexpr uint16_t kHumidityStepPerTick = 200;

// Starting simulated humidity: 3000 == 30.00 % RH.
constexpr uint16_t kInitialSimulatedHumidity = 3000;

const BitFlags<Feature> kDefaultFeatures{ Feature::kHumidifier, Feature::kDehumidifier, Feature::kSensor,
                                          Feature::kAuto,        Feature::kContinuous,   Feature::kColdMist };

// Enable TargetSetpoint so SyncTargetSetpointToUserSetpoint() keeps it in sync
// with UserSetpoint whenever the user changes the setpoint.
const HumidistatCluster::OptionalAttributeSet kDefaultOptionalAttributes =
    HumidistatCluster::OptionalAttributeSet{}.Set<Attributes::TargetSetpoint::Id>();

HumidistatCluster::StartupConfiguration DefaultHumidistatConfig()
{
    HumidistatCluster::StartupConfiguration config;
    config.mode           = ModeEnum::kHumidifier;
    config.systemState    = SystemStateEnum::kHumidifying;
    config.userSetpoint   = 50;
    config.targetSetpoint = 50;
    config.minSetpoint    = 30;
    config.maxSetpoint    = 60;
    config.mistType.Set(MistTypeBitmap::kMistCold);
    return config;
}

RelativeHumidityMeasurementCluster::Config DefaultHumidityConfig()
{
    RelativeHumidityMeasurementCluster::Config config;
    config.minMeasuredValue = 0;
    config.maxMeasuredValue = 10000;
    return config;
}

const char * ModeEnumName(ModeEnum m)
{
    switch (m)
    {
    case ModeEnum::kOff:
        return "kOff";
    case ModeEnum::kHumidifier:
        return "kHumidifier";
    case ModeEnum::kDehumidifier:
        return "kDehumidifier";
    case ModeEnum::kAuto:
        return "kAuto";
    case ModeEnum::kFanOnly:
        return "kFanOnly";
    default:
        return "unknown";
    }
}

const char * SystemStateEnumName(SystemStateEnum s)
{
    switch (s)
    {
    case SystemStateEnum::kOff:
        return "kOff";
    case SystemStateEnum::kIdle:
        return "kIdle";
    case SystemStateEnum::kHumidifying:
        return "kHumidifying";
    case SystemStateEnum::kDehumidifying:
        return "kDehumidifying";
    case SystemStateEnum::kFan:
        return "kFan";
    default:
        return "unknown";
    }
}

} // namespace

SimulatingHumidifierDehumidifierDevice::SimulatingHumidifierDehumidifierDevice() :
    HumidifierDehumidifierDevice(mTimerDelegate, kDefaultFeatures, kDefaultOptionalAttributes, DefaultHumidistatConfig(),
                                 DefaultHumidityConfig()),
    mSimulatedHumidity(kInitialSimulatedHumidity)
{}

SimulatingHumidifierDehumidifierDevice::~SimulatingHumidifierDehumidifierDevice()
{
    mTimerDelegate.CancelTimer(this);
}

CHIP_ERROR SimulatingHumidifierDehumidifierDevice::Register(EndpointId endpoint, CodeDrivenDataModelProvider & provider,
                                                            EndpointComposition composition)
{
    VerifyOrReturnError(sActiveHumidistatTriggerDevice == nullptr || sActiveHumidistatTriggerDevice == this,
                        CHIP_ERROR_INCORRECT_STATE);

    ReturnErrorOnFailure(HumidifierDehumidifierDevice::Register(endpoint, provider, composition));

    // Wire up the delegate so attribute changes call back into this class.
    HumidistatCluster().SetDelegate(this);

    // Seed the measured humidity with the initial simulated value.
    DataModel::Nullable<uint16_t> initialHumidity;
    initialHumidity.SetNonNull(mSimulatedHumidity);
    LogErrorOnFailure(RelativeHumidityMeasurementCluster().SetMeasuredValue(initialHumidity));

    // Kick off the periodic simulation loop.
    ReturnErrorOnFailure(mTimerDelegate.StartTimer(this, kSimulationTickIntervalSec));

    if (auto * triggerDelegate = Server::GetInstance().GetTestEventTriggerDelegate(); triggerDelegate != nullptr)
    {
        ReturnErrorOnFailure(triggerDelegate->AddHandler(&mTestEventTriggerHandler));
        sActiveHumidistatTriggerDevice = this;
    }

    return CHIP_NO_ERROR;
}

void SimulatingHumidifierDehumidifierDevice::Unregister(CodeDrivenDataModelProvider & provider)
{
    mTimerDelegate.CancelTimer(this);
    if (auto * triggerDelegate = Server::GetInstance().GetTestEventTriggerDelegate(); triggerDelegate != nullptr)
    {
        triggerDelegate->RemoveHandler(&mTestEventTriggerHandler);
    }
    if (sActiveHumidistatTriggerDevice == this)
    {
        sActiveHumidistatTriggerDevice = nullptr;
    }
    if (mHumidistatCluster.IsConstructed())
    {
        mHumidistatCluster.Cluster().SetDelegate(nullptr);
    }
    HumidifierDehumidifierDevice::Unregister(provider);
}

CHIP_ERROR SimulatingHumidifierDehumidifierDevice::HandleHumidistatTestEventTriggerInternal(uint64_t eventTrigger)
{
    if (!mHumidistatCluster.IsConstructed() || !mRelativeHumidityMeasurementCluster.IsConstructed() || mEndpointId == kInvalidEndpointId)
    {
        return CHIP_ERROR_INCORRECT_STATE;
    }

    if (eventTrigger == kHumidistatLowHumidityTrigger)
    {
        TriggerLowHumidityEvent();
        return CHIP_NO_ERROR;
    }
    if (eventTrigger == kHumidistatHighHumidityTrigger)
    {
        TriggerHighHumidityEvent();
        return CHIP_NO_ERROR;
    }

    return CHIP_ERROR_INVALID_ARGUMENT;
}

void SimulatingHumidifierDehumidifierDevice::TriggerLowHumidityEvent()
{
    ChipLogProgress(AppServer, "SimulatingHumidifierDehumidifier: test trigger LowHumidity (0x%016llX)",
                    static_cast<unsigned long long>(kHumidistatLowHumidityTrigger));
    mSimulatedHumidity = kLowHumidityForTestTrigger;
    RunSimulationStep();
}

void SimulatingHumidifierDehumidifierDevice::TriggerHighHumidityEvent()
{
    ChipLogProgress(AppServer, "SimulatingHumidifierDehumidifier: test trigger HighHumidity (0x%016llX)",
                    static_cast<unsigned long long>(kHumidistatHighHumidityTrigger));
    mSimulatedHumidity = kHighHumidityForTestTrigger;
    RunSimulationStep();
}

// ---------------------------------------------------------------------------
// Simulation loop
// ---------------------------------------------------------------------------

void SimulatingHumidifierDehumidifierDevice::TimerFired()
{
    RunSimulationStep();
    LogErrorOnFailure(mTimerDelegate.StartTimer(this, kSimulationTickIntervalSec));
}

void SimulatingHumidifierDehumidifierDevice::RunSimulationStep()
{
    if (!mHumidistatCluster.IsConstructed())
    {
        return;
    }

    auto & cluster = mHumidistatCluster.Cluster();

    const ModeEnum mode = cluster.GetMode();

    // targetSetpoint is chip::Percent (0–100); convert to 0.01 % units.
    const uint16_t targetRaw = static_cast<uint16_t>(cluster.GetTargetSetpoint()) * 100;

    // When Continuous is active the device keeps running even at setpoint.
    const bool continuous = cluster.GetContinuous();

    SystemStateEnum newState = cluster.GetSystemState();

    switch (mode)
    {
    // kOff: if humidity has drifted from the setpoint, automatically wake up in
    // the appropriate single-direction mode.  The OnModeChanged callback will
    // immediately call RunSimulationStep() again, which advances humidity and sets
    // the system state in the newly active mode.
    case ModeEnum::kOff:
        if (mSimulatedHumidity < targetRaw)
        {
            LogErrorOnFailure(cluster.SetMode(ModeEnum::kHumidifier));
            newState = SystemStateEnum::kHumidifying;
        }
        else if (mSimulatedHumidity > targetRaw)
        {
            LogErrorOnFailure(cluster.SetMode(ModeEnum::kDehumidifier));
            newState = SystemStateEnum::kDehumidifying;
        }
        else
        {
            newState = SystemStateEnum::kOff;
        }
        break;

    // kHumidifier: one-directional humidify only.  When the setpoint is reached the
    // mode transitions to Off so the device does not run indefinitely.
    // With Continuous active the cap is raised to 100 % so the device keeps running
    // past the setpoint (and therefore never auto-offs while Continuous is on).
    case ModeEnum::kHumidifier:
    {
        if (mSimulatedHumidity < targetRaw || (continuous && newState == SystemStateEnum::kHumidifying))
        {
            const uint32_t cap = continuous ? 10000u : static_cast<uint32_t>(targetRaw);
            mSimulatedHumidity =
                static_cast<uint16_t>(std::min<uint32_t>(static_cast<uint32_t>(mSimulatedHumidity) + kHumidityStepPerTick,
                                                         cap));
            newState = SystemStateEnum::kHumidifying;
        }
        else
        {
            // Setpoint reached — turn the device off.
            LogErrorOnFailure(cluster.SetMode(ModeEnum::kOff));
            newState = SystemStateEnum::kIdle;
        }
        break;
    }

    // kDehumidifier: one-directional dehumidify only.  Same auto-off behaviour.
    case ModeEnum::kDehumidifier:
    {
        if (mSimulatedHumidity > targetRaw || (continuous && newState == SystemStateEnum::kDehumidifying))
        {
            const int32_t floor = continuous ? 0 : static_cast<int32_t>(targetRaw);
            const int32_t next  = static_cast<int32_t>(mSimulatedHumidity) - kHumidityStepPerTick;
            mSimulatedHumidity  = static_cast<uint16_t>(std::max<int32_t>(next, floor));
            newState            = SystemStateEnum::kDehumidifying;
        }
        else
        {
            // Setpoint reached — turn the device off.
            LogErrorOnFailure(cluster.SetMode(ModeEnum::kOff));
            newState = SystemStateEnum::kIdle;
        }
        break;
    }

    // kAuto: bidirectional — the device stays in Auto and transitions between
    // kHumidifying and kDehumidifying without ever leaving the mode.
    // It goes kIdle when exactly at the setpoint.
    case ModeEnum::kAuto:
    {
        const uint32_t humidifyCap    = continuous ? 10000u : static_cast<uint32_t>(targetRaw);
        const int32_t dehumidifyFloor = continuous ? 0 : static_cast<int32_t>(targetRaw);

        if (mSimulatedHumidity < targetRaw || (continuous && newState == SystemStateEnum::kHumidifying))
        {
            mSimulatedHumidity =
                static_cast<uint16_t>(std::min<uint32_t>(static_cast<uint32_t>(mSimulatedHumidity) + kHumidityStepPerTick,
                                                         humidifyCap));
            newState = SystemStateEnum::kHumidifying;
        }
        else if (mSimulatedHumidity > targetRaw || (continuous && newState == SystemStateEnum::kDehumidifying))
        {
            const int32_t next = static_cast<int32_t>(mSimulatedHumidity) - kHumidityStepPerTick;
            mSimulatedHumidity = static_cast<uint16_t>(std::max<int32_t>(next, dehumidifyFloor));
            newState           = SystemStateEnum::kDehumidifying;
        }
        else
        {
            newState = SystemStateEnum::kIdle;
        }
        break;
    }

    case ModeEnum::kFanOnly:
        newState = SystemStateEnum::kFan;
        break;

    default:
        break;
    }

    if (newState != cluster.GetSystemState())
    {
        LogErrorOnFailure(cluster.SetSystemState(newState));
    }

    DataModel::Nullable<uint16_t> measuredValue;
    measuredValue.SetNonNull(mSimulatedHumidity);
    LogErrorOnFailure(RelativeHumidityMeasurementCluster().SetMeasuredValue(measuredValue));

    ChipLogProgress(AppServer, "SimulatingHumidifierDehumidifier: mode=%u(%s) state=%u(%s) humidity=%u (target=%u%s)",
                    static_cast<unsigned>(mode), ModeEnumName(mode), static_cast<unsigned>(newState),
                    SystemStateEnumName(newState), mSimulatedHumidity, targetRaw, continuous ? ", continuous" : "");
}

// ---------------------------------------------------------------------------
// HumidistatDelegate callbacks
// ---------------------------------------------------------------------------

void SimulatingHumidifierDehumidifierDevice::OnModeChanged(ModeEnum newMode)
{
    ChipLogProgress(AppServer, "SimulatingHumidifierDehumidifier: mode changed to %u(%s) — re-evaluating",
                    static_cast<unsigned>(newMode), ModeEnumName(newMode));
    RunSimulationStep();
}

void SimulatingHumidifierDehumidifierDevice::OnSystemStateChanged(SystemStateEnum newSystemState)
{
    ChipLogDetail(AppServer, "SimulatingHumidifierDehumidifier: system state changed to %u(%s)",
                  static_cast<unsigned>(newSystemState), SystemStateEnumName(newSystemState));
}

void SimulatingHumidifierDehumidifierDevice::OnUserSetpointChanged(chip::Percent newUserSetpoint)
{
    ChipLogProgress(AppServer, "SimulatingHumidifierDehumidifier: user setpoint changed to %u%%", newUserSetpoint);
}

void SimulatingHumidifierDehumidifierDevice::OnTargetSetpointChanged(chip::Percent newTargetSetpoint)
{
    ChipLogProgress(AppServer, "SimulatingHumidifierDehumidifier: target setpoint changed to %u%% — re-evaluating",
                    newTargetSetpoint);
    RunSimulationStep();
}

void SimulatingHumidifierDehumidifierDevice::OnMistTypeChanged(chip::BitMask<MistTypeBitmap> newMistType)
{
    ChipLogProgress(AppServer, "SimulatingHumidifierDehumidifier: mist type changed to 0x%02x",
                    static_cast<unsigned>(newMistType.Raw()));
}

void SimulatingHumidifierDehumidifierDevice::OnContinuousChanged(bool newContinuous)
{
    ChipLogProgress(AppServer, "SimulatingHumidifierDehumidifier: continuous mode %s — re-evaluating",
                    newContinuous ? "enabled" : "disabled");
    RunSimulationStep();
}

void SimulatingHumidifierDehumidifierDevice::OnSleepChanged(bool newSleep)
{
    ChipLogProgress(AppServer, "SimulatingHumidifierDehumidifier: sleep mode %s", newSleep ? "enabled" : "disabled");
}

void SimulatingHumidifierDehumidifierDevice::OnOptimalChanged(bool newOptimal)
{
    ChipLogProgress(AppServer, "SimulatingHumidifierDehumidifier: optimal mode %s", newOptimal ? "enabled" : "disabled");
}

bool HandleHumidistatTestEventTriggerForApp(uint64_t eventTrigger)
{
    VerifyOrReturnValue(sActiveHumidistatTriggerDevice != nullptr, false);
    return sActiveHumidistatTriggerDevice->HandleHumidistatTestEventTriggerInternal(eventTrigger) == CHIP_NO_ERROR;
}

} // namespace app
} // namespace chip

bool HandleHumidistatTestEventTrigger(uint64_t eventTrigger)
{
    return chip::app::HandleHumidistatTestEventTriggerForApp(eventTrigger);
}
