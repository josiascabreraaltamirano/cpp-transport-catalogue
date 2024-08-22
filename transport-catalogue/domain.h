#pragma once
//my libraries
#include "geo.h"
//std libraries
#include <string>
#include <memory>
#include <vector>
#include <set>
#include <cassert>
#include <deque>
#include <utility>
#include <unordered_map>

namespace catalogue {
    namespace domain {

		using Distance = int; 

		struct BaseRequests {
            struct StopData;
            struct BusData;
            struct DistanceData;

            std::deque<StopData> stops;
            std::deque<BusData> buses;
            std::deque<DistanceData> distances;

            void Add(StopData stop) {
                stops.push_back(std::move(stop));
            }

            void Add(BusData bus) {
                buses.push_back(std::move(bus));
            }
            
            void Add(DistanceData distance) {
                distances.push_back(std::move(distance));
            }

            struct StopData {
                std::string name;
                double latitude;
                double longitude;

                StopData& SetName(std::string stopname) {
                    name = std::move(stopname);
                    return *this;
                }

                StopData& SetLatitude(double lat) {
                    latitude = lat;
                    return *this;
                }

                StopData& SetLongitude(double lon) {
                    longitude = lon;
                    return *this;
                }
            };

            struct BusData {
                std::string name;
                std::vector<std::string> stops;
                bool is_roundtrip;

                BusData& SetName(std::string busname) {
                    name = std::move(busname);
                    return *this;
                }

                BusData& SetStops(std::vector<std::string> bus_stops) {
                    stops = std::move(bus_stops);
                    return *this;
                }

                BusData& SetIsRoundtrip(bool bus_is_roundtrip) {
                    is_roundtrip = bus_is_roundtrip;
                    return *this;
                }
            };
            
            struct DistanceData {
                std::string from;
                std::unordered_map<std::string, domain::Distance> road_distances;

                DistanceData& SetFrom(std::string origin) {
                    from = std::move(origin);
                    return *this;
                }

                DistanceData& SetRoadDistances(std::unordered_map<std::string, domain::Distance> destination_to_distance) {
                    road_distances = std::move(destination_to_distance);
                    return *this;
                }
            };
        };

        struct StatRequests {
            
			template<typename Request>
            void Add(Request request) {
                requests.push_back(std::make_shared<Request>(std::move(request)));
            }

            struct StatRequest {
                int id;
                std::string type;

                virtual ~StatRequest() = default;
            };

            template <typename Owner>
            class InlineObjectBuilder : public StatRequest {
            public:    
                Owner& SetId(int num) {
                    id = num;
                    return AsOwner();
                }

                Owner& SetType(std::string type_name) {
                    type = std::move(type_name);
                    return AsOwner();
                }
            private:
                Owner& AsOwner() {
                    return static_cast<Owner&>(*this);
                }
            };

            struct Transport : public InlineObjectBuilder<Transport> {
                std::string name;

                Transport& SetName(std::string transport_item) {
                    name = std::move(transport_item);
                    return *this;
                }
            };

            struct Router : public InlineObjectBuilder<Router> {
                std::string from;
                std::string to;

                Router& SetFrom(std::string origin) {
                    from = std::move(origin);
                    return *this;
                }
                
                Router& SetTo(std::string destination) {
                    to = std::move(destination);
                    return *this;
                }
            };
        
			std::deque<std::shared_ptr<StatRequest>> requests;
		};

        struct RouterSettings {
            int bus_wait_time;
            double bus_velocity;

            RouterSettings& SetBusWaitTime(int minutes) {
                bus_wait_time = minutes;
                return *this;
            }

            RouterSettings& SetBusVelocity(double kmph) {
                bus_velocity = kmph;
                return *this;
            }
        };
	 
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
