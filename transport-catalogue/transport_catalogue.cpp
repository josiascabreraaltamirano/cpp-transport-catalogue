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




        }//namespace detail     
        void TransportCatalogue::AddStop(std::string_view stop, geo::Coordinates coordinates) { 
            //add a Stop the its main container 
            stops_.push_back({std::string(stop), std::move(coordinates)}); 
            //access to the element that was just added 
            const Stop& last_elem = stops_.back(); 
            //add the bus to the container of searching 
            stopname_to_stop_[last_elem.name] = std::make_shared<Stop>(last_elem); 
        } 
 
        void TransportCatalogue::AddRoute(std::string_view route, const StopNames& stopnames, bool is_roundtrip) { 
            //create a temp container and reserve the right storage 
            std::vector<StopPtr> stop_ptrs; 
            stop_ptrs.reserve(stopnames.size()); 
            //fill stop_ptrs 
            for (const auto& stopname: stopnames) { 
                //if is a valid ptr is added 
                stop_ptrs.push_back(EnsureStopPtr(stopname));  
            } 
            //add a Route to its main container 
            routes_.push_back({std::string(route), std::move(stop_ptrs), is_roundtrip}); 
            //access to the element that was just added 
            const Route& last_elem = routes_.back(); 
            //add the route to the stops it passes through 
            for (const auto& stop : last_elem.stops) { 
                stopname_to_routes_[stop -> name].insert(std::make_shared<Route>(last_elem)); 
            } 
            //add the route to the container of searching 
            routename_to_route_[last_elem.name] = std::make_shared<Route>(last_elem); 
        } 
        //added in sprint 9  
        void TransportCatalogue::SetDistance(std::string_view from, std::string_view to, Distance distance) { 
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
 
        Distance TransportCatalogue::GetDistance(std::string_view from, std::string_view to) const { 
            auto origin = EnsureStopPtr(from); 
            auto destination = EnsureStopPtr(to); 
             
            auto end = stops_to_distance_.end(); 
            if (auto iter = stops_to_distance_.find({origin, destination}); iter != end) { 
                return iter -> second; 
            } 
 
            auto inverse_distance = stops_to_distance_.find({destination, origin}); 
            return inverse_distance != end ? inverse_distance -> second : 
            throw std::out_of_range(std::string("The required data does not exist in the database")); 
        } 

        StopStats TransportCatalogue::GetStopStats(std::string_view stop) const { 
            return !stopname_to_stop_.count(stop) ? StopStats{} : 
            !stopname_to_routes_.count(stop) ? StopStats{true, {}} :  
            StopStats{true, std::make_shared<const Routes>(stopname_to_routes_.at(stop))}; 
        } 
 
        RouteStats TransportCatalogue::GetRouteStats(std::string_view route) const { 
            const auto route_ptr = FindRoute(route); 
            //check wheter the elements exist 
            if (!route_ptr) {   
                return {}; 
            } 
            //if exists save a reference to the object 
            const auto& stops = route_ptr -> stops;
            bool is_roundtrip = route_ptr -> is_roundtrip; 
 
            int total_stops(stops.size()); 
            int unique_stops((std::unordered_set<StopPtr>{stops.begin(), stops.end()}).size()); 
            //save the length in meters 
            Distance length = 0; 
            //save the length based on the given coordinates 
            double geo_length = 0.0; 

            CalcLength(stops.begin(), stops.end(), length, geo_length);

            //if the route is not roundtrip, duplicate the length values
            if (!is_roundtrip) {
                CalcLength(stops.rbegin(), stops.rend(), length, geo_length);
                total_stops = total_stops * 2 -1;
            }
 
            double curvature(static_cast<double>(static_cast<double>(length)/static_cast<double>(geo_length))); 
 
            return RouteStats{total_stops, unique_stops, length, static_cast<double>(curvature)}; 
        } 

        std::vector<StopPtr> TransportCatalogue::GetActiveStops() const {
            std::vector<StopPtr> stops;
            stops.reserve(stopname_to_routes_.size());
            for (const auto& [stopname, routes] : stopname_to_routes_) {
                stops.push_back(FindStop(stopname));
            }
            return stops;
        }
		
        std::vector<RoutePtr> TransportCatalogue::GetActiveRoutes() const {
            std::vector<RoutePtr> routes;
            routes.reserve(routename_to_route_.size());
            for (const auto& [routename, routeptr] : routename_to_route_) {
                if (routeptr && !routeptr -> stops.empty()) {
                    routes.push_back(routeptr);
                }
            }
            return routes;
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
