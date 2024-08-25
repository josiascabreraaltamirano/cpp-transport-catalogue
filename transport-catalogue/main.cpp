#include <cassert>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string_view>

#include "json.h"
#include "json_reader.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"


using namespace std::literals;

int main() {
    //std::ifstream json_input("input.json"s);
    //std::ofstream json_output("output.json"s);
    auto requests = json::input::ParseInput(std::cin);
    
    catalogue::database::TransportCatalogue database;

    json::input::ApplyBaseRequests(database, requests.base_requests);

    catalogue::router::TransportRouter router(database, requests.router_settings);
    svg::MapRenderer renderer(requests.render_settings);
    catalogue::request_handler::RequestHandler handler(database, router, renderer);
    
    json::output::PrintStats(handler, requests.stat_requests, std::cout);

    return 0;
}