//my libraries
#include "stat_reader.h"
//std libraries
#include <iostream>
#include <iomanip>

namespace catalogue {    
    namespace output {

        using namespace ::std::literals;

        namespace detail {
            //Parse the request and return the type (Bus or Stop)
            RequestInfo ParseRequest(std::string_view string) {
                //simplify npos use
                const auto np = std::string_view::npos;
                //no space first pos
                auto start = string.find_first_not_of(' ');
                std::string_view type;
                if (start == np || 
                (string.substr(start, (type = "Bus"sv).size() + 1) == "Bus "sv ? 0 : 
                string.substr(start, (type = "Stop"sv).size() + 1) == "Stop "sv ? 0 : 1)) {
                    return RequestInfo{};
                }
                
                //linea donde se contiene el nombre de la ruta
                const auto raw_route_name = string.substr(start + type.size());
                //asigna la posicion, tomando como referencia la linea del nombre de la ruta
                auto new_start = raw_route_name.find_first_not_of(' '); 
                auto new_end = new_start == np ? np : raw_route_name.find_last_not_of(' ');
                return new_start != np && new_end != np ? 
                RequestInfo{type, raw_route_name.substr(new_start, new_end + 1 - new_start)} : 
                RequestInfo{type, std::string_view{}};
            }
        } //namespace detail

        std::ostream& operator<<(std::ostream& output, const std::set<std::string_view> obj) {
            for (const auto& elem : obj) {
                output << ' ' << elem;
            }
            return output;
        }
        
        void ParseAndPrintStat(const database::TransportCatalogue& transport_catalogue, std::string_view request,
                            std::ostream& output) {
            auto request_info = detail::ParseRequest(request);
            //checks if the request was valid
            if (request_info.type.empty()) {
                return;
            }
            //shows the type and name
            output << request_info.type << ' ' << request_info.name << ": "sv;
            //Bus
            if (request_info.type.size() == 3) {
                auto result = transport_catalogue.GetRouteInfo(request_info.name);
                result.total_stops == 0 ? (output << "not found"sv) :
                (output << result.total_stops << " stops on route, "sv << result.unique_stops <<
                std::setprecision(6) << " unique stops, "sv << result.length << " route length, "sv 
                << result.curvature << " curvature"sv);
                output << '\n';
                //finish    
                return;
            }
            //Stop
            const auto result = transport_catalogue.GetStopInfo(request_info.name);
            !result.is_stop ? (output << "not found"sv) : (!result.routes ? 
            (output << "no buses"sv) : (output << "buses"sv << *result.routes));
            output << '\n';
        }
    } //namespace output
} //namespace catalogue
