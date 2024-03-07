#include "transport_catalogue.h"
#include <unordered_set>

namespace catalogue {
    namespace database {	
        void TransportCatalogue::AddStop(std::string_view stop, geo::Coordinates coordinates) {
            //add a Stop the its main container
            stops_.push_back({std::string(stop), std::move(coordinates)});
            //access to the element that was just added
            const Stop& last_elem = stops_.back();
            //add the bus to the container of searching
            stopname_to_stop_[last_elem.name] = &(last_elem);
        }

        void TransportCatalogue::AddRoute(std::string_view route, const std::vector<std::string_view>& stopnames) {
            //create a temp container and reserve the right storage
            std::vector<const Stop*> stop_ptrs;
            stop_ptrs.reserve(stopnames.size());
            //fill stop_ptrs
            for (const auto stopname: stopnames) {
                stop_ptrs.push_back(FindStop(stopname));
            }
            //add a Route to its main container
            routes_.push_back({std::string(route), std::move(stop_ptrs)});
            //access to the element that was just added
            const Route& last_elem = routes_.back();
            //add the route to the stops it passes through
            for (const auto& stop : last_elem.stops) {
                stopname_to_routes_[stop -> name].insert(last_elem.name);
            }
            //add the route to the container of searching
            routename_to_route_[last_elem.name] = &(last_elem);
        }

        const Stop* TransportCatalogue::FindStop(std::string_view stop) const {
            auto result = stopname_to_stop_.find(stop);
            return result == stopname_to_stop_.end() ? nullptr : (result -> second);
        } 

        const Route* TransportCatalogue::FindRoute(std::string_view route) const {
            auto result = routename_to_route_.find(route);
            return result == routename_to_route_.end() ? nullptr : result -> second;
        }

        RouteInfo TransportCatalogue::GetRouteInfo(std::string_view route) const {
            //check wheter the elements exist
            const auto route_info = FindRoute(route);
            if (!route_info) {
                return RouteInfo{};
            }
            const auto& stops = route_info -> stops;
            //compute the unique stops
            int unique_stops((std::unordered_set<const Stop*>{stops.begin(), stops.end()}).size());
            //save the route length result
            double route_length(0.0);
            //compute distance between the stops and add it to the total length
            for (size_t i = 1; i < stops.size(); i++) {
                    route_length += geo::ComputeDistance(stops.at(i - 1) -> coordinates , stops.at(i) -> coordinates);
            }
            return RouteInfo{int(stops.size()), unique_stops, route_length};
        }

        StopInfo TransportCatalogue::GetStopInfo(std::string_view stop) const {
            return !stopname_to_stop_.count(stop) ? StopInfo{nullptr, false} :
            (!stopname_to_routes_.count(stop) ? StopInfo{nullptr, true} : 
            StopInfo{&stopname_to_routes_.at(stop), true});
        }
    } //namespace database
} //namespace catalogue
