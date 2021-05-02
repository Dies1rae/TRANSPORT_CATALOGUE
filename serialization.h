#pragma once

#include "json.h"
#include "json_reader.h"
#include "map_render.h"
#include "transport_catalogue.h"
#include "transport_catalogue.pb.h"
#include "transport_router.h"

#include <iostream>
#include <fstream>
#include <map>

using namespace std::literals;

proto::Color ColorToProto(const svg::Color& color);

void serialize(std::istream& in);

void database_restore(const proto::TransportCatalogue& database, database::TransportCatalogue& catalogue);

svg::Color protoToColor(const proto::Color& color);

void settings_restore(const proto::TransportCatalogue& database, rndr::renderSettings& renderSettings);

void desirialize(std::istream& in);
