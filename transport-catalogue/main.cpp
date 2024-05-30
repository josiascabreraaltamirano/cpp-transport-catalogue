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
    assert(requests.base_requests && requests.render_settings && requests.stat_requests);

    catalogue::database::TransportCatalogue database;
    svg::MapRenderer renderer(*requests.render_settings);
    catalogue::output::RequestHandler handler(database, renderer);

    json::input::ApplyBaseRequests(database, requests.base_requests);
    catalogue::output::PrintStats(handler, requests.stat_requests, std::cout);
    //std::cout << "success"sv << '\n';
    /*
     * Примерная структура программы:
     *
     * Считать JSON из stdin
     * Построить на его основе JSON базу данных транспортного справочника
     * Выполнить запросы к справочнику, находящиеся в массива "stat_requests", построив JSON-массив
     * с ответами Вывести в stdout ответы в виде JSON
     */
    return 0;
}

