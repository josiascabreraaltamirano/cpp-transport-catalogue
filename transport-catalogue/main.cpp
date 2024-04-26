//my libraries
#include "input_reader.h"
#include "stat_reader.h"
//std libraries
#include <iostream>
#include <string>


using namespace std;

int main() {
    using namespace catalogue;
    database::TransportCatalogue catalogue;

    int base_request_count;
    cin >> base_request_count >> ws;

    {
        input::InputReader reader;
        for (int i = 0; i < base_request_count; ++i) {
            string line;
            getline(cin, line);
            reader.ParseLine(line);
        }
        reader.ApplyCommands(catalogue);
    }

    int stat_request_count;
    cin >> stat_request_count >> ws;
    for (int i = 0; i < stat_request_count; ++i) {
        string line;
        getline(cin, line);
        output::ParseAndPrintStat(catalogue, line, cout);
    }
}
