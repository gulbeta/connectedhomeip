#include "humidistat-server.h"

#include <app/server-cluster/AttributeListBuilder.h>
#include <clusters/Humidistat/Metadata.h>

#include <app/CommandHandler.h>
#include <clusters/Humidistat/Commands.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

bool emberAfHumidistatClusterSetSettingsCallback(
    CommandHandler * commandObj,
    const ConcreteCommandPath & commandPath,
    const Humidistat::Commands::SetSettings::DecodableType & commandData)
{
    (void) commandObj;
    (void) commandPath;
    (void) commandData;
    return true;
}

namespace chip::app::Clusters {

using namespace Humidistat::Attributes;

DataModel::ActionReturnStatus HumidistatCluster::ReadAttribute(const DataModel::ReadAttributeRequest & request,
                                                               AttributeValueEncoder & encoder)
{
    // No attributes implemented yet.
    return Protocols::InteractionModel::Status::UnsupportedAttribute; // or kNotHandled (pick what your codebase uses)
}

DataModel::ActionReturnStatus HumidistatCluster::WriteAttribute(const DataModel::WriteAttributeRequest & request,
                                                                AttributeValueDecoder & decoder)
{
    // Writes not supported yet.
    return Protocols::InteractionModel::Status::UnsupportedAttribute;
}

CHIP_ERROR HumidistatCluster::Attributes(const ConcreteClusterPath & path,
                                         ReadOnlyBufferBuilder<DataModel::AttributeEntry> & builder)
{
    AttributeListBuilder listBuilder(builder);
    return listBuilder.Append(Span(Humidistat::Attributes::kMandatoryMetadata), {});
}

} // namespace chip::app::Clusters
