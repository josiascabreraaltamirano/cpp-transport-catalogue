#pragma once

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"
namespace catalogue {
    namespace output {
        struct RequestInfo{
            std::string_view type;
            std::string_view name;
        };

        void ParseAndPrintStat(const database::TransportCatalogue& transport_catalogue, std::string_view request,
                            std::ostream& output);
    } //namespace output
} // namespace catalogue   