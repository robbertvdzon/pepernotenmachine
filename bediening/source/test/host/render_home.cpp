// Renders the home screen sprites (the real firmware drawing code) to PPM files for a visual check.
//   g++ -std=c++11 -Iinclude test/host/render_home.cpp src/home_sprites.cpp src/gfx.cpp -o /tmp/render_home && /tmp/render_home /tmp/out
// Writes band_<state>_<ms>.ppm and tiles_<name>.ppm (needs an existing output directory).

#include <cstdio>
#include <string>
#include <vector>

#include "gfx.h"
#include "home_config.h"
#include "home_sprites.h"

static void ppm(const std::string& path, const gfx::Surface& s) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", s.w, s.h);
    for (int i = 0; i < s.w * s.h; i++) {
        const uint16_t c = s.px[i];
        const unsigned char rgb[3] = {static_cast<unsigned char>(((c >> 11) & 31) * 255 / 31),
                                      static_cast<unsigned char>(((c >> 5) & 63) * 255 / 63),
                                      static_cast<unsigned char>((c & 31) * 255 / 31)};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

int main(int argc, char** argv) {
    const std::string out = argc > 1 ? argv[1] : ".";
    std::vector<uint16_t> bandMem(HOME_BAND_W * HOME_BAND_H), tileMem(HOME_TILES_W * HOME_TILES_H);
    const gfx::Surface band = {bandMem.data(), HOME_BAND_W, HOME_BAND_H}, tiles = {tileMem.data(), HOME_TILES_W, HOME_TILES_H};
    for (int st = 0; st <= static_cast<int>(BandState::Rearm); st++) {
        const uint32_t times[] = {0, 450, 900, 1300};
        for (uint32_t t : times) {
            sprites::drawBand(band, static_cast<BandState>(st), t, 500, 3);
            ppm(out + "/band_" + std::to_string(st) + "_" + std::to_string(t) + ".ppm", band);
        }
    }
    const TileState sets[][TILE_COUNT] = {
        {TileState::Active, TileState::Active, TileState::Active, TileState::Active, TileState::Active, TileState::Off, TileState::Active},
        {TileState::On, TileState::On, TileState::On, TileState::On, TileState::On, TileState::On, TileState::On},
        {TileState::Pending, TileState::Pending, TileState::Pending, TileState::Pending, TileState::Pending, TileState::Pending, TileState::Pending},
        {TileState::Problem, TileState::Problem, TileState::Problem, TileState::Problem, TileState::Problem, TileState::Problem, TileState::Problem},
        {TileState::Off, TileState::Off, TileState::Off, TileState::Off, TileState::Off, TileState::Off, TileState::Off}};
    for (int k = 0; k < 5; k++) {
        for (uint32_t t : {0u, 150u, 300u}) {
            sprites::drawTiles(tiles, sets[k], t);
            ppm(out + "/tiles_" + std::to_string(k) + "_" + std::to_string(t) + ".ppm", tiles);
        }
    }
    return 0;
}
