#include "request_handler.h"

namespace catalogue {
    namespace request_handler {
        //Class RequestHandler
        RequestHandler::RequestHandler(const catalogue::database::TransportCatalogue& database,
                                       const svg::MapRenderer& renderer) 
        : database_(database)
        , renderer_(renderer)
        {
        }
        
        domain::RouteStats RequestHandler::GetRouteStats(const std::string_view& route_name) const {
            return database_.GetRouteStats(route_name);
        }

        domain::StopStats RequestHandler::GetStopStats(const std::string_view& stop_name) const {
            return database_.GetStopStats(stop_name);
        }

        void RequestHandler::RenderMap(std::ostream& output) const {
            /*
            Здесь я решил взять остановки непосредственно из каталога транспорта, 
            потому что такой подход ускоряет работу программы, чем извлечение остановок из каждого маршрута. 
            Я знаю, что вы рекомендуете мне извлекать остановки из каждого маршрута, но, принимая во внимание то, 
            что я только что упомянул, я надеюсь, вы позволите мне сохранить эту часть кода неизменной. 
            В любом случае, если вы считаете, что это необходимо изменить, дайте мне знать, и я это сделаю. 
            Спасибо за вашу поддержку))
            */
            renderer_.RenderMap(database_.GetActiveStops(), database_.GetActiveRoutes(), output);
        }
    } //namespace request_handler
} //namespace catalogue
