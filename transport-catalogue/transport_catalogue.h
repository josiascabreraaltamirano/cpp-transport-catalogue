#pragma once 
//my libraries 
#include "geo.h"
#include "domain.h" 
//std libraries 
#include <deque> 
#include <vector> 
#include <unordered_map> 
#include <string> 
#include <string_view> 
#include <utility> 
#include <functional> 
#include <set> 
#include <optional>
#include <cassert>
#include <memory> 
 
namespace catalogue { 
	namespace database {
		using namespace domain;
		 
		class TransportCatalogue { 
		public: 
			void AddStop(std::string_view stop, geo::Coordinates coordinates); 
			void AddRoute(std::string_view route, const StopNames& stopnames, bool is_roundtrip); 
			void SetDistance(std::string_view from, std::string_view to, Distance distance); 
			const StopPtr FindStop(std::string_view stop) const;  
			const RoutePtr FindRoute(std::string_view route) const; 
			Distance GetDistance(std::string_view from, std::string_view to) const;
			StopStats GetStopStats(std::string_view stop) const;
			RouteStats GetRouteStats(std::string_view route) const; 
			//stops with at least one bus passing by
			std::vector<StopPtr> GetActiveStops() const;
			//buses passing by at least one stop
			std::vector<RoutePtr> GetActiveRoutes() const;
			
		private:
			template <typename T> 
            static std::optional<T> SafeAdd(T a, T b) { 
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
			template <typename Iter>
			void CalcLength(Iter begin, Iter end, Distance& length, double& geo_length) const {
				for (auto it = begin + 1; it != end; it++) {
					//prev iter 
					auto prev_it = it + (-1); 
					//check wether the ptrs are not nullptr 
					assert(*prev_it && *it); 
					//compute geo length of the route 
					auto pre_add_geo = SafeAdd(static_cast<double>(geo_length), static_cast<double>(geo::ComputeDistance((*prev_it) -> coordinates , (*it) -> coordinates))); 
					//check wether the addition is within the limits 
					assert(pre_add_geo); 
					//safely assign the result 
					geo_length = static_cast<double>(pre_add_geo.value()); 
					//compute route length in mtrs. 0
					auto pre_add_route = SafeAdd(length, GetDistance((*prev_it) -> name, (*it) -> name)); 
					//check wether the addition is within the limits 
					assert(pre_add_route); 
					//safely assign the result 
					length = pre_add_route.value();
				}
			}
			//Avoid working with nullptr  
			const StopPtr EnsureStopPtr(std::string_view stopname) const; 

			std::deque<Stop> stops_; 
			std::deque<Route> routes_; 
			std::unordered_map<std::string_view, StopPtr> stopname_to_stop_; 
			std::unordered_map<std::string_view, RoutePtr> routename_to_route_; 
			std::unordered_map<std::string_view, Routes> stopname_to_routes_; 
			std::unordered_map<StopPair, Distance, StopPairHasher> stops_to_distance_; 
		}; 
	} //namespace database 
} //namespace catalogue 
