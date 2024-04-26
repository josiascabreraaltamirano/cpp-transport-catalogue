//my libraries
#include "transport_catalogue.h"
//std libraries
#include <unordered_set>
//added in sprint 9
#include <stdexcept>
#include <cassert>
#include <optional>
#include <limits>

namespace catalogue {
    namespace database {
        namespace detail {
            //added in sprint 9
            template <typename T>
            std::optional<T> SafeAdd(T a, T b) {
                if (a > 0 && b > 0) {
                    if (a > std::numeric_limits<T>::max() - b) {
                        return std::nullopt; // Переполнение при сложении положительных чисел
                    }
                } else if (a < 0 && b < 0) {
                    if (a < std::numeric_limits<T>::min() - b) {
                        return std::nullopt; // Переполнение при сложении отрицательных чисел
                    }
                }
                // Здесь гарантируется, что переполнения не возникнет
                return a + b;
            }
        }//namespace detail    
        void TransportCatalogue::AddStop(std::string_view stop, geo::Coordinates coordinates) {
            //add a Stop the its main container
            stops_.push_back({std::string(stop), std::move(coordinates)});
            //access to the element that was just added
            const Stop& last_elem = stops_.back();
            //add the bus to the container of searching
            stopname_to_stop_[last_elem.name] = std::make_shared<Stop>(last_elem);
        }

        void TransportCatalogue::AddRoute(std::string_view route, const StopNames& stopnames) {
            //create a temp container and reserve the right storage
            std::vector<StopPtr> stop_ptrs;
            stop_ptrs.reserve(stopnames.size());
            //fill stop_ptrs
            for (const auto stopname: stopnames) {
                //if is a valid ptr is added
                stop_ptrs.push_back(EnsureStopPtr(stopname)); 
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
            routename_to_route_[last_elem.name] = std::make_shared<Route>(last_elem);
        }
        //added in sprint 9 
        void TransportCatalogue::AddDistance(std::string_view from, std::string_view to, Distance distance) {
            assert(distance > 0 && distance <= 1'000'000);
            stops_to_distance_[StopPair{EnsureStopPtr(from), EnsureStopPtr(to)}] = distance;
        }

        const StopPtr TransportCatalogue::FindStop(std::string_view stop) const {
            auto iter = stopname_to_stop_.find(stop);
            return iter == stopname_to_stop_.end() ? StopPtr{} : iter -> second;
        } 

        const RoutePtr TransportCatalogue::FindRoute(std::string_view route) const {
            auto iter = routename_to_route_.find(route);
            return iter == routename_to_route_.end() ? RoutePtr{} : iter -> second;
        }

        Distance TransportCatalogue::FindDistance(std::string_view from, std::string_view to) const {
            auto a = EnsureStopPtr(from);
            auto b = EnsureStopPtr(to);
            
            auto end = stops_to_distance_.end();
            if (auto iter = stops_to_distance_.find({a, b}); iter != end) {
                return iter -> second;
            }

            auto inverse_distance = stops_to_distance_.find({b, a});
            return inverse_distance != end ? inverse_distance -> second :
            throw std::out_of_range(std::string("The required data does not exist in the database"));
        }
        
        //modified in sprint 9
        RouteInfo TransportCatalogue::GetRouteInfo(std::string_view route) const {
            const auto route_data = FindRoute(route);
            //check wheter the elements exist
            if (!route_data) {  
                return RouteInfo{};
            }
            //if exists save a reference to the object
            const auto& stops = route_data -> stops;

            int total_stops = stops.size();
            int unique_stops((std::unordered_set<StopPtr>{stops.begin(), stops.end()}).size());
            //save the length in meters
            Distance length = 0;
            //save the length based on the given coordinates
            double geo_length = 0.0;

            for (auto i = (stops.begin() + 1); i != stops.end(); i++) {
                //prev iter
                auto prev_i = i + (-1);
                //check wether the ptrs are not nullptr
                assert(*prev_i && *i);
                //compute geo length of the route
                auto pre_add_geo = detail::SafeAdd(geo_length, geo::ComputeDistance((*prev_i) -> coordinates , (*i) -> coordinates));
                //check wether the addition is within the limits
                assert(pre_add_geo);
                //safely assign the result
                geo_length = pre_add_geo.value();
                //compute route length in mtrs.
                auto pre_add_route = detail::SafeAdd(length, FindDistance((*prev_i) -> name, (*i) -> name));
                //check wether the addition is within the limits
                assert(pre_add_route);
                //safely assign the result
                length = pre_add_route.value();
            }

            double curvature(static_cast<double>(length/geo_length));

            return RouteInfo{total_stops, unique_stops, length, curvature};
        }

        StopInfo TransportCatalogue::GetStopInfo(std::string_view stop) const {
            return !stopname_to_stop_.count(stop) ? StopInfo{{}, false} :
            (!stopname_to_routes_.count(stop) ? StopInfo{{}, true} : 
            StopInfo{std::make_shared<RouteNames>(stopname_to_routes_.at(stop)), true});
        }

        //private methods
        const StopPtr TransportCatalogue::EnsureStopPtr(std::string_view stopname) const {
            auto stop_ptr = FindStop(stopname);
            if (!stop_ptr) {
                //if are not valid throw an exception
                throw std::invalid_argument(std::string("Unable to work with the introduced parameters"));
            }
            return stop_ptr;

        }
    } //namespace database
} //namespace catalogue
