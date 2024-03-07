#pragma once
#include <deque>
#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>
#include <utility>
#include <functional>
#include <set>

#include "geo.h"


#include <iostream>
namespace catalogue {
	namespace database {	
		struct Stop {
			std::string name;
			geo::Coordinates coordinates;

			bool operator==(const Stop& other) const {
				return (name == other.name || coordinates == other.coordinates);
			}
		};

		struct Route {
			std::string name;
			std::vector<const Stop*> stops;
		};

		struct RouteInfo {
			int total_stops = 0;
			int unique_stops = 0;
			double route_length = 0.0;
		};

		struct StopInfo {
			const std::set<std::string_view>* routes;
			bool is_stop;
		};

		class TransportCatalogue {
		public:
			void AddStop(const std::string_view stop, geo::Coordinates coordinates);
			void AddRoute(const std::string_view route, const std::vector<std::string_view>& stopnames);
			const Stop* FindStop(std::string_view stop) const; 
			const Route* FindRoute(std::string_view route) const;
			RouteInfo GetRouteInfo(std::string_view route) const;
			StopInfo GetStopInfo(std::string_view stop) const;
		private:
			std::deque<Stop> stops_;
			std::deque<Route> routes_;
			std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
			std::unordered_map<std::string_view, const Route*> routename_to_route_;
			std::unordered_map<std::string_view, std::set<std::string_view>> stopname_to_routes_;
		};
	} //namespace database
} //namespace catalogue