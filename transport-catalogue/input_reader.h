#pragma once
//my libraries
#include "geo.h"
#include "transport_catalogue.h"
//std libraries
#include <string>
#include <string_view>
#include <vector>

namespace catalogue {
    namespace input {
        struct CommandDescription {
            // Определяет, задана ли команда (поле command непустое)
            explicit operator bool() const {
                return !command.empty();
            }

            bool operator!() const {
                return !operator bool();
            }

            std::string command;      // Название команды
            std::string id;           // id маршрута или остановки
            std::string description;  // Параметры команды
        };

        struct DistanceData {
            std::string from;
            std::string to;
            int distance;
        };
        

        class InputReader {
        public:
            /**
             * Парсит строку в структуру CommandDescription и сохраняет результат в its container
             */
            void ParseLine(std::string_view line);

            /**
             * Наполняет данными транспортный справочник, используя команды из commands_
             */
            void ApplyCommands(database::TransportCatalogue& catalogue) const;

        private:
            std::vector<CommandDescription> stop_commands_;
            std::vector<CommandDescription> bus_commands_;
            std::vector<DistanceData> distance_commands_;
        };
    } //namespace input
} //namespace catalogue 
