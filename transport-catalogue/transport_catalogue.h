#pragma once
//my libraries
#include "geo.h"
//std libraries
#include <deque>
#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>
#include <utility>
#include <functional>
#include <set>

//added in sprint 9
#include <memory>

namespace catalogue {
	namespace database {

		using Distance = int;

	
		struct Stop {
			std::string name;
			geo::Coordinates coordinates;

			bool operator==(const Stop& other) const {
				return (name == other.name || coordinates == other.coordinates);
			}
		};

		using StopPtr = std::shared_ptr<Stop>;
		using StopNames = std::vector<std::string_view>;

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
		};

		using RoutePtr = std::shared_ptr<Route>;
		using RouteNames = std::set<std::string_view>;

		struct RouteInfo {
			int total_stops = 0;
			int unique_stops = 0;
			Distance length = 0;
			//added in sprint 9
			double curvature = 0.0;
		};

		struct StopInfo {
			std::shared_ptr<RouteNames> routes;
			bool is_stop;
		};


		
		class TransportCatalogue {
		public:
			void AddStop(std::string_view stop, geo::Coordinates coordinates);
			void AddRoute(std::string_view route, const StopNames& stopnames);
			//added in sprint 9 - Add distance between two stops
			void AddDistance(std::string_view from, std::string_view to, Distance distance);
			const StopPtr FindStop(std::string_view stop) const; 
			const RoutePtr FindRoute(std::string_view route) const;
			//added in sprint 9
			Distance FindDistance(std::string_view from, std::string_view to) const;
			RouteInfo GetRouteInfo(std::string_view route) const;
			StopInfo GetStopInfo(std::string_view stop) const;
		private:
			//added in sprint 9 - Avoid working with nullptr 
			const StopPtr EnsureStopPtr(std::string_view stopname) const;

			std::deque<Stop> stops_;
			std::deque<Route> routes_;
			std::unordered_map<std::string_view, StopPtr> stopname_to_stop_;
			std::unordered_map<std::string_view, RoutePtr> routename_to_route_;
			std::unordered_map<std::string_view, RouteNames> stopname_to_routes_;
			//added in sprint 9
			std::unordered_map<StopPair, Distance, StopPairHasher> stops_to_distance_;
		};
	} //namespace database
} //namespace catalogue
