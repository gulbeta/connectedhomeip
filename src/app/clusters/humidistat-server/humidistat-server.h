/*
 *    Copyright (c) 2025 Project CHIP Authors
 *    All rights reserved.
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

#include <app/server-cluster/DefaultServerCluster.h>
#include <app/data-model-provider/MetadataTypes.h>

namespace chip::app::Clusters {

class HumidistatCluster : public ServerClusterInterface
{
public:
    static constexpr ClusterId kClusterId = 0xFFF100; // TODO: replace with real assigned ID.

    HumidistatCluster() = default;

    // Optional helper to expose ID (not overriding anything).
    static constexpr ClusterId Id() { return kClusterId; }

    // Implement required virtuals (signatures must match base).
    DataModel::ActionReturnStatus ReadAttribute(const DataModel::ReadAttributeRequest & request,
                                                AttributeValueEncoder & encoder) override;

    DataModel::ActionReturnStatus WriteAttribute(const DataModel::WriteAttributeRequest & request,
                                                 AttributeValueDecoder & decoder) override;

    // If the interface declares an Attributes(...) enumerator, keep this; otherwise drop override.
    CHIP_ERROR Attributes(const ConcreteClusterPath & path,
                          ReadOnlyBufferBuilder<DataModel::AttributeEntry> & builder) override;
};

} // namespace chip::app::Clusters