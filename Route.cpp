#include "Route.hpp"

Route::Route() {
    dir_listing = false;
    max_body_size = std::numeric_limits<unsigned long long>::max();
}