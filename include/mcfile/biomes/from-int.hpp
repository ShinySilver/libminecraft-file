#pragma once

namespace mcfile {
namespace biomes {

static inline BiomeId FromInt(int v) {
    static std::map<int, BiomeId> const mapping = {
        {0, minecraft::ocean},
        {1, minecraft::plains},
        {2, minecraft::desert},
        {3, minecraft::windswept_hills},
        {4, minecraft::forest},
        {5, minecraft::taiga},
        {6, minecraft::swamp},
        {7, minecraft::river},
        {8, minecraft::nether_wastes},
        {9, minecraft::the_end},
        {10, minecraft::frozen_ocean},
        {11, minecraft::frozen_river},
        {12, minecraft::snowy_plains},
        {13, minecraft::snowy_mountains},
        {14, minecraft::mushroom_fields},
        {15, minecraft::mushroom_field_shore},
        {16, minecraft::beach},
        {17, minecraft::desert_hills},
        {18, minecraft::wooded_hills},
        {19, minecraft::taiga_hills},
        {20, minecraft::mountain_edge},
        {21, minecraft::jungle},
        {22, minecraft::jungle_hills},
        {23, minecraft::sparse_jungle},
        {24, minecraft::deep_ocean},
        {25, minecraft::stony_shore},
        {26, minecraft::snowy_beach},
        {27, minecraft::birch_forest},
        {28, minecraft::birch_forest_hills},
        {29, minecraft::dark_forest},
        {30, minecraft::snowy_taiga},
        {31, minecraft::snowy_taiga_hills},
        {32, minecraft::old_growth_pine_taiga},
        {33, minecraft::giant_tree_taiga_hills},
        {34, minecraft::windswept_forest},
        {35, minecraft::savanna},
        {36, minecraft::savanna_plateau},
        {37, minecraft::badlands},
        {38, minecraft::wooded_badlands},
        {39, minecraft::badlands_plateau},
        {40, minecraft::small_end_islands},
        {41, minecraft::end_midlands},
        {42, minecraft::end_highlands},
        {43, minecraft::end_barrens},
        {44, minecraft::warm_ocean},
        {45, minecraft::lukewarm_ocean},
        {46, minecraft::cold_ocean},
        {47, minecraft::deep_warm_ocean},
        {48, minecraft::deep_lukewarm_ocean},
        {49, minecraft::deep_cold_ocean},
        {50, minecraft::deep_frozen_ocean},
        {127, minecraft::the_void},
        {129, minecraft::sunflower_plains},
        {130, minecraft::desert_lakes},
        {131, minecraft::windswept_gravelly_hills},
        {132, minecraft::flower_forest},
        {133, minecraft::taiga_mountains},
        {134, minecraft::swamp_hills},
        {140, minecraft::ice_spikes},
        {149, minecraft::modified_jungle},
        {151, minecraft::modified_jungle_edge},
        {155, minecraft::old_growth_birch_forest},
        {156, minecraft::tall_birch_hills},
        {157, minecraft::dark_forest_hills},
        {158, minecraft::snowy_taiga_mountains},
        {160, minecraft::old_growth_spruce_taiga},
        {161, minecraft::giant_spruce_taiga_hills},
        {162, minecraft::modified_gravelly_mountains},
        {163, minecraft::windswept_savanna},
        {164, minecraft::shattered_savanna_plateau},
        {165, minecraft::eroded_badlands},
        {166, minecraft::modified_wooded_badlands_plateau},
        {167, minecraft::modified_badlands_plateau},
        {168, minecraft::bamboo_jungle},
        {169, minecraft::bamboo_jungle_hills},

        // 20w06a
        {170, minecraft::soul_sand_valley},
        {171, minecraft::crimson_forest},
        {172, minecraft::warped_forest},

        // 20w15a
        {173, minecraft::basalt_deltas},

        // 1.18
        {174, minecraft::dripstone_caves},
        {175, minecraft::lush_caves},
    };
    auto mappingIt = mapping.find(v);
    if (mappingIt == mapping.end()) {
        return unknown;
    }
    return mappingIt->second;
}

} // namespace biomes
} // namespace mcfile
