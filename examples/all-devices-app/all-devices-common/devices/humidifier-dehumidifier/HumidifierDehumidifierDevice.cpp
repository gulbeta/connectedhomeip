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

#include <devices/Types.h>
#include <devices/humidifier-dehumidifier/HumidifierDehumidifierDevice.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>

namespace chip {
namespace app {

HumidifierDehumidifierDevice::HumidifierDehumidifierDevice(
    TimerDelegate & timerDelegate, BitFlags<Clusters::Humidistat::Feature> features,
    Clusters::HumidistatCluster::OptionalAttributeSet optionalAttributes,
    Clusters::HumidistatCluster::StartupConfiguration humidistatConfig,
    Clusters::RelativeHumidityMeasurementCluster::Config humidityConfig) :
    SingleEndpointDevice(Span<const DataModel::DeviceTypeEntry>(&Device::Type::kHumidifierDehumidifier, 1)),
    mTimerDelegate(timerDelegate), mFeatures(features), mOptionalAttributes(optionalAttributes),
    mHumidistatConfig(humidistatConfig), mHumidityConfig(humidityConfig)
{}

CHIP_ERROR HumidifierDehumidifierDevice::Register(EndpointId endpoint, CodeDrivenDataModelProvider & provider,
                                                  EndpointComposition composition)
{
    VerifyOrReturnError(mEndpointId == kInvalidEndpointId, CHIP_ERROR_INCORRECT_STATE);
    DeviceRegistrationTransaction transaction(*this, provider);

    ReturnErrorOnFailure(RegisterDescriptor(endpoint, provider, composition));

    mIdentifyCluster.Create(Clusters::IdentifyCluster::Config(endpoint, mTimerDelegate));
    ReturnErrorOnFailure(provider.AddCluster(mIdentifyCluster.Registration()));

    mHumidistatCluster.Create(endpoint, mFeatures, mOptionalAttributes, mHumidistatConfig);
    ReturnErrorOnFailure(provider.AddCluster(mHumidistatCluster.Registration()));

    mRelativeHumidityMeasurementCluster.Create(endpoint, mHumidityConfig);
    ReturnErrorOnFailure(provider.AddCluster(mRelativeHumidityMeasurementCluster.Registration()));

    ReturnErrorOnFailure(provider.AddEndpoint(mEndpointRegistration));
    transaction.Commit();
    return CHIP_NO_ERROR;
}

void HumidifierDehumidifierDevice::Unregister(CodeDrivenDataModelProvider & provider)
{
    UnregisterDescriptor(provider);
    if (mRelativeHumidityMeasurementCluster.IsConstructed())
    {
        LogErrorOnFailure(provider.RemoveCluster(&mRelativeHumidityMeasurementCluster.Cluster()));
        mRelativeHumidityMeasurementCluster.Destroy();
    }
    if (mHumidistatCluster.IsConstructed())
    {
        LogErrorOnFailure(provider.RemoveCluster(&mHumidistatCluster.Cluster()));
        mHumidistatCluster.Destroy();
    }
    if (mIdentifyCluster.IsConstructed())
    {
        LogErrorOnFailure(provider.RemoveCluster(&mIdentifyCluster.Cluster()));
        mIdentifyCluster.Destroy();
    }
}

} // namespace app
} // namespace chip
