#include "../include/Route.hpp"

Route::Route() {
    dir_listing = false;
    max_body_size = 1024 * 1024;
    upload_dir = "www/html/uploads/";
}