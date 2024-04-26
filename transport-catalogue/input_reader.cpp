//my libraries
#include "input_reader.h"
//std libraries
#include <algorithm>
#include <cassert>
#include <iterator>

namespace catalogue {
    namespace input {
        namespace detail {
            /**
            * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
            */
            geo::Coordinates ParseCoordinates(std::string_view string) {
                static const double nan = std::nan("");

                auto not_space = string.find_first_not_of(' ');
                auto comma = string.find(',');

                if (comma == string.npos) {
                    return {nan, nan};
                }

                auto not_space2 = string.find_first_not_of(' ', comma + 1);
                auto comma2 = string.find_first_of(',', not_space2 + 1);

                double lat = std::stod(std::string(string.substr(not_space, comma - not_space)));
                double lng = std::stod(std::string(comma2 == std::string_view::npos ? 
                                                   string.substr(not_space2) : 
                                                   string.substr(not_space2, comma2 - not_space2)));

                return {lat, lng};
            }

            /**
             * Удаляет пробелы в начале и конце строки
             */
            std::string_view Trim(std::string_view string) {
                const auto start = string.find_first_not_of(' ');
                if (start == string.npos) {
                    return {};
                }
                return string.substr(start, string.find_last_not_of(' ') + 1 - start);
            }

            /**
             * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
             */
            std::vector<std::string_view> Split(std::string_view string, char delim) {
                std::vector<std::string_view> result;

                size_t pos = 0;
                while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
                    auto delim_pos = string.find(delim, pos);
                    if (delim_pos == string.npos) {
                        delim_pos = string.size();
                    }
                    if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
                        result.push_back(substr);
                    }
                    pos = delim_pos + 1;
                }

                return result;
            }

            /**
             * Парсит маршрут.
             * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
             * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
             */
            std::vector<std::string_view> ParseRoute(std::string_view route) {
                if (route.find('>') != route.npos) {
                    return Split(route, '>');
                }
                auto stops = Split(route, '-');
                std::vector<std::string_view> results(stops.begin(), stops.end());
                results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

                return results;
            }

            CommandDescription ParseCommandDescription(std::string_view line) {
                auto colon_pos = line.find(':');
                if (colon_pos == line.npos) {
                    return {};
                }

                auto space_pos = line.find(' ');
                if (space_pos >= colon_pos) {
                    return {};
                }

                auto not_space = line.find_first_not_of(' ', space_pos);
                if (not_space >= colon_pos) {
                    return {};
                }

                return {std::string(line.substr(0, space_pos)),
                        std::string(line.substr(not_space, colon_pos - not_space)),
                        std::string(line.substr(colon_pos + 1))};
            }

            //added in sprint 9
            //Helper function to parse distance data
            std::vector<DistanceData> ParseDistanceData(std::string_view string , std::string_view from) {
                using namespace std::literals;

                //value to return
                std::vector<DistanceData> distances;
                //raw distance lines 
                const auto lines = Split(string, ',');
                //reserve memory
                distances.reserve(lines.size());
                
                //simplify npos use
                const auto np = std::string_view::npos;
                //key_word to find distance components
                std::string_view key_word = " to "sv;
                //parse each line
                for (auto line : lines) {
                    auto kw_pos = line.find(key_word);
                    auto no_space = kw_pos == np ? np : line.find_first_not_of(' ' , kw_pos + key_word.size());
                    if (kw_pos == np || no_space == np) {
                        continue;
                    }
                    distances.push_back({std::string(from), std::string(line.substr(no_space)), std::stoi(std::string{line.substr(0, kw_pos - 1)})});
                }
                return distances;
            }

            //added in sprint 9
            std::string_view ParseDistanceLine(std::string_view string) {
                //simplify npos use
                const auto np = std::string_view::npos;
                //position of first comma for coordinates
                auto comma1 = string.find_first_of(',');
                //position of comma after coordinates
                auto comma2 = comma1 == np ? np : string.find_first_of(',', comma1 + 1);
                if (comma1 == np || comma2 == np) {
                    return std::string_view{};
                }
                //in this pos starts the command
                auto begin = string.find_first_not_of(' ', comma2 + 1);
                return begin == np ? std::string_view{} : string.substr(begin);
            }
        } //namespace detail

        void InputReader::ParseLine(std::string_view line) {
            auto command_description = detail::ParseCommandDescription(line);
            if (command_description) {
                //if Stop
                if (command_description.command.size() == 4) {
                    stop_commands_.push_back(std::move(command_description));
                    //added in sprint 9
                    const auto& last_elem = stop_commands_.back();
                    //parse the description for getting a distance line 
                    auto distance_line = detail::ParseDistanceLine(last_elem.description);
                    //check if distance line contains any distance data
                    if (!distance_line.empty()) {
                        //create the object and move it to the queue
                        auto distance_data = detail::ParseDistanceData(distance_line , last_elem.id);
                        for (auto i = distance_data.rbegin(); i != distance_data.rend(); i++) {
                            distance_commands_.push_back(std::move(*i));
                        }
                    }
                    return;
                }
                //otherwise Bus
                bus_commands_.push_back(std::move(command_description));
            }
        }

        void InputReader::ApplyCommands([[maybe_unused]] database::TransportCatalogue& catalogue) const {
            for (const auto& c : stop_commands_) {
                catalogue.AddStop(c.id, detail::ParseCoordinates(c.description));
            }

            for (const auto& c : bus_commands_) {
                catalogue.AddRoute(c.id, detail::ParseRoute(c.description));
            }

            for (const auto& c : distance_commands_) {
                catalogue.AddDistance(c.from, c.to, c.distance);
            }     
        } 
    } //namespace input
} //namespace catalogue

