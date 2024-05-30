#pragma once
//my libraries
#include "geo.h"
//std libraries
#include <string>
#include <memory>
#include <vector>
#include <set>
#include <cassert>

namespace catalogue {
    namespace domain {
		
		using Distance = int; 
 
	 
		struct Stop { 
			std::string name; 
			geo::Coordinates coordinates; 
 
			bool operator==(const Stop& other) const { 
				return (name == other.name || coordinates == other.coordinates); 
			} 
		}; 
 
		using StopPtr = std::shared_ptr<Stop>; 
		using StopNames = std::vector<std::string>; 
 
		//added in sprint 9 
		struct StopPair { 
			StopPtr from; 
			StopPtr to; 
			 
			bool operator==(const StopPair& other) const { 
				return (from == other.from && to == other.to);  
			} 
		}; 
 
		//added in sprint 9 
		struct StopPairHasher { 
			size_t operator()(const StopPair& stops) const { 
				return (hasher_(stops.from.get()) * N) + (hasher_(stops.to.get()) * N * N); 
			} 
			 
		private: 
			std::hash<const void *> hasher_; 
			//const num to be used for a higher accuracy to the hash production 
			const int N = 37;	 
		}; 
 
		struct Route { 
			std::string name; 
			std::vector<StopPtr> stops;
			bool is_roundtrip; 
		}; 

		using RoutePtr = std::shared_ptr<Route>; 

		inline bool operator==(const RoutePtr& l, const RoutePtr& r) {
			assert(l && r);
			return l -> name == r -> name;
		}

		struct RoutePtrComp {
			bool operator()(const RoutePtr& l, const RoutePtr& r) const {
				assert(l && r);
				return l -> name < r -> name;
			}
		};

		
		using Routes = std::set<RoutePtr, RoutePtrComp>;
		
		struct StopStats {
			StopStats() = default;

			StopStats(bool is_stop, std::shared_ptr<const Routes> routes) 
			: is_stop(is_stop)
			, routes(routes)
			{
			}

			bool is_stop = false;
			std::shared_ptr<const Routes> routes = nullptr;
		};
		
		struct RouteStats {
			RouteStats() = default;
			RouteStats(int total_stops, int unique_stops, Distance length, double curvature) 
			: is_route(true)
			, total_stops(total_stops)
			, unique_stops(unique_stops)
			, length(length)
			, curvature(curvature)
			{
			}

			//especial member
			bool is_route = false;

			int total_stops = 0; 
			int unique_stops = 0; 
			Distance length = 0; 
			double curvature = 0.0; 
		}; 
 
    } //namespace domain
} //namespace catalogue
