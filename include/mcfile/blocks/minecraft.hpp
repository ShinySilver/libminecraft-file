#pragma once

namespace mcfile::blocks::minecraft {

enum : BlockId {
    acacia_button = 1,
    acacia_door,
    acacia_fence,
    acacia_fence_gate,
    acacia_leaves,
    acacia_log,
    acacia_planks,
    acacia_pressure_plate,
    acacia_sapling,
    acacia_sign,
    acacia_slab,
    acacia_stairs,
    acacia_trapdoor,
    acacia_wall_sign,
    acacia_wood,
    activator_rail,
    air,
    allium,
    andesite,
    andesite_slab,
    andesite_stairs,
    andesite_wall,
    anvil,
    attached_melon_stem,
    attached_pumpkin_stem,
    azure_bluet,
    bamboo,
    bamboo_sapling,
    barrel,
    barrier,
    beacon,
    bedrock,
    beetroots,
    bell,
    birch_button,
    birch_door,
    birch_fence,
    birch_fence_gate,
    birch_leaves,
    birch_log,
    birch_planks,
    birch_pressure_plate,
    birch_sapling,
    birch_sign,
    birch_slab,
    birch_stairs,
    birch_trapdoor,
    birch_wall_sign,
    birch_wood,
    black_banner,
    black_bed,
    black_carpet,
    black_concrete,
    black_concrete_powder,
    black_glazed_terracotta,
    black_shulker_box,
    black_stained_glass,
    black_stained_glass_pane,
    black_terracotta,
    black_wall_banner,
    black_wool,
    blast_furnace,
    coal_block,
    diamond_block,
    emerald_block,
    gold_block,
    iron_block,
    quartz_block,
    redstone_block,
    blue_banner,
    blue_bed,
    blue_carpet,
    blue_concrete,
    blue_concrete_powder,
    blue_glazed_terracotta,
    blue_ice,
    blue_orchid,
    blue_shulker_box,
    blue_stained_glass,
    blue_stained_glass_pane,
    blue_terracotta,
    blue_wall_banner,
    blue_wool,
    bone_block,
    bookshelf,
    brain_coral,
    brain_coral_block,
    brain_coral_fan,
    brain_coral_wall_fan,
    brewing_stand,
    brick_slab,
    brick_stairs,
    brick_wall,
    bricks,
    brown_banner,
    brown_bed,
    brown_carpet,
    brown_concrete,
    brown_concrete_powder,
    brown_glazed_terracotta,
    brown_mushroom,
    brown_mushroom_block,
    brown_shulker_box,
    brown_stained_glass,
    brown_stained_glass_pane,
    brown_terracotta,
    brown_wall_banner,
    brown_wool,
    bubble_column,
    bubble_coral,
    bubble_coral_block,
    bubble_coral_fan,
    bubble_coral_wall_fan,
    cactus,
    cake,
    campfire,
    carrots,
    cartography_table,
    carved_pumpkin,
    cauldron,
    cave_air,
    chain_command_block,
    chest,
    chipped_anvil,
    chiseled_quartz_block,
    chiseled_red_sandstone,
    chiseled_sandstone,
    chiseled_stone_bricks,
    chorus_flower,
    chorus_plant,
    clay,
    coal_ore,
    coarse_dirt,
    cobblestone,
    cobblestone_slab,
    cobblestone_stairs,
    cobblestone_wall,
    cobweb,
    cocoa,
    command_block,
    composter,
    conduit,
    cornflower,
    cracked_stone_bricks,
    crafting_table,
    creeper_head,
    creeper_wall_head,
    cut_red_sandstone,
    cut_red_sandstone_slab,
    cut_sandstone,
    cut_sandstone_slab,
    cyan_banner,
    cyan_bed,
    cyan_carpet,
    cyan_concrete,
    cyan_concrete_powder,
    cyan_glazed_terracotta,
    cyan_shulker_box,
    cyan_stained_glass,
    cyan_stained_glass_pane,
    cyan_terracotta,
    cyan_wall_banner,
    cyan_wool,
    damaged_anvil,
    dandelion,
    dark_oak_button,
    dark_oak_door,
    dark_oak_fence,
    dark_oak_fence_gate,
    dark_oak_leaves,
    dark_oak_log,
    dark_oak_planks,
    dark_oak_pressure_plate,
    dark_oak_sapling,
    dark_oak_sign,
    dark_oak_slab,
    dark_oak_stairs,
    dark_oak_trapdoor,
    dark_oak_wall_sign,
    dark_oak_wood,
    dark_prismarine,
    dark_prismarine_slab,
    dark_prismarine_stairs,
    daylight_detector,
    dead_brain_coral,
    dead_brain_coral_block,
    dead_brain_coral_fan,
    dead_brain_coral_wall_fan,
    dead_bubble_coral,
    dead_bubble_coral_block,
    dead_bubble_coral_fan,
    dead_bubble_coral_wall_fan,
    dead_bush,
    dead_fire_coral,
    dead_fire_coral_block,
    dead_fire_coral_fan,
    dead_fire_coral_wall_fan,
    dead_horn_coral,
    dead_horn_coral_block,
    dead_horn_coral_fan,
    dead_horn_coral_wall_fan,
    dead_tube_coral,
    dead_tube_coral_block,
    dead_tube_coral_fan,
    dead_tube_coral_wall_fan,
    detector_rail,
    diamond_ore,
    diorite,
    diorite_slab,
    diorite_stairs,
    diorite_wall,
    dirt,
    dispenser,
    dragon_egg,
    dragon_head,
    dragon_wall_head,
    dried_kelp_block,
    dropper,
    emerald_ore,
    enchanting_table,
    end_gateway,
    end_portal,
    end_portal_frame,
    end_rod,
    end_stone,
    end_stone_brick_slab,
    end_stone_brick_stairs,
    end_stone_brick_wall,
    end_stone_bricks,
    ender_chest,
    farmland,
    fern,
    fire,
    fire_coral,
    fire_coral_block,
    fire_coral_fan,
    fire_coral_wall_fan,
    fletching_table,
    flower_pot,
    frosted_ice,
    furnace,
    glass,
    glass_pane,
    glowstone,
    gold_ore,
    granite,
    granite_slab,
    granite_stairs,
    granite_wall,
    grass,
    grass_block,
    // grass_path, // renamed to dirt_path in 1.17
    gravel,
    gray_banner,
    gray_bed,
    gray_carpet,
    gray_concrete,
    gray_concrete_powder,
    gray_glazed_terracotta,
    gray_shulker_box,
    gray_stained_glass,
    gray_stained_glass_pane,
    gray_terracotta,
    gray_wall_banner,
    gray_wool,
    green_banner,
    green_bed,
    green_carpet,
    green_concrete,
    green_concrete_powder,
    green_glazed_terracotta,
    green_shulker_box,
    green_stained_glass,
    green_stained_glass_pane,
    green_terracotta,
    green_wall_banner,
    green_wool,
    grindstone,
    hay_block,
    heavy_weighted_pressure_plate,
    hopper,
    horn_coral,
    horn_coral_block,
    horn_coral_fan,
    horn_coral_wall_fan,
    ice,
    infested_chiseled_stone_bricks,
    infested_cobblestone,
    infested_cracked_stone_bricks,
    infested_mossy_stone_bricks,
    infested_stone,
    infested_stone_bricks,
    iron_bars,
    iron_door,
    iron_ore,
    iron_trapdoor,
    jack_o_lantern,
    jigsaw,
    jukebox,
    jungle_button,
    jungle_door,
    jungle_fence,
    jungle_fence_gate,
    jungle_leaves,
    jungle_log,
    jungle_planks,
    jungle_pressure_plate,
    jungle_sapling,
    jungle_sign,
    jungle_slab,
    jungle_stairs,
    jungle_trapdoor,
    jungle_wall_sign,
    jungle_wood,
    kelp,
    kelp_plant,
    ladder,
    lantern,
    lapis_block,
    lapis_ore,
    large_fern,
    lava,
    lectern,
    lever,
    light_blue_banner,
    light_blue_bed,
    light_blue_carpet,
    light_blue_concrete,
    light_blue_concrete_powder,
    light_blue_glazed_terracotta,
    light_blue_shulker_box,
    light_blue_stained_glass,
    light_blue_stained_glass_pane,
    light_blue_terracotta,
    light_blue_wall_banner,
    light_blue_wool,
    light_gray_banner,
    light_gray_bed,
    light_gray_carpet,
    light_gray_concrete,
    light_gray_concrete_powder,
    light_gray_glazed_terracotta,
    light_gray_shulker_box,
    light_gray_stained_glass,
    light_gray_stained_glass_pane,
    light_gray_terracotta,
    light_gray_wall_banner,
    light_gray_wool,
    light_weighted_pressure_plate,
    lilac,
    lily_of_the_valley,
    lily_pad,
    lime_banner,
    lime_bed,
    lime_carpet,
    lime_concrete,
    lime_concrete_powder,
    lime_glazed_terracotta,
    lime_shulker_box,
    lime_stained_glass,
    lime_stained_glass_pane,
    lime_terracotta,
    lime_wall_banner,
    lime_wool,
    loom,
    magenta_banner,
    magenta_bed,
    magenta_carpet,
    magenta_concrete,
    magenta_concrete_powder,
    magenta_glazed_terracotta,
    magenta_shulker_box,
    magenta_stained_glass,
    magenta_stained_glass_pane,
    magenta_terracotta,
    magenta_wall_banner,
    magenta_wool,
    magma_block,
    melon,
    melon_stem,
    mossy_cobblestone,
    mossy_cobblestone_slab,
    mossy_cobblestone_stairs,
    mossy_cobblestone_wall,
    mossy_stone_brick_slab,
    mossy_stone_brick_stairs,
    mossy_stone_brick_wall,
    mossy_stone_bricks,
    moving_piston,
    mushroom_stem,
    mycelium,
    nether_brick_fence,
    nether_brick_slab,
    nether_brick_stairs,
    nether_brick_wall,
    nether_bricks,
    nether_portal,
    nether_quartz_ore,
    nether_wart,
    nether_wart_block,
    netherrack,
    note_block,
    oak_button,
    oak_door,
    oak_fence,
    oak_fence_gate,
    oak_leaves,
    oak_log,
    oak_planks,
    oak_pressure_plate,
    oak_sapling,
    oak_sign,
    oak_slab,
    oak_stairs,
    oak_trapdoor,
    oak_wall_sign,
    oak_wood,
    observer,
    obsidian,
    orange_banner,
    orange_bed,
    orange_carpet,
    orange_concrete,
    orange_concrete_powder,
    orange_glazed_terracotta,
    orange_shulker_box,
    orange_stained_glass,
    orange_stained_glass_pane,
    orange_terracotta,
    orange_tulip,
    orange_wall_banner,
    orange_wool,
    oxeye_daisy,
    packed_ice,
    peony,
    petrified_oak_slab,
    pink_banner,
    pink_bed,
    pink_carpet,
    pink_concrete,
    pink_concrete_powder,
    pink_glazed_terracotta,
    pink_shulker_box,
    pink_stained_glass,
    pink_stained_glass_pane,
    pink_terracotta,
    pink_tulip,
    pink_wall_banner,
    pink_wool,
    piston,
    piston_head,
    player_head,
    player_wall_head,
    podzol,
    polished_andesite,
    polished_andesite_slab,
    polished_andesite_stairs,
    polished_diorite,
    polished_diorite_slab,
    polished_diorite_stairs,
    polished_granite,
    polished_granite_slab,
    polished_granite_stairs,
    poppy,
    potatoes,
    potted_acacia_sapling,
    potted_allium,
    potted_azure_bluet,
    potted_bamboo,
    potted_birch_sapling,
    potted_blue_orchid,
    potted_brown_mushroom,
    potted_cactus,
    potted_cornflower,
    potted_dandelion,
    potted_dark_oak_sapling,
    potted_dead_bush,
    potted_fern,
    potted_jungle_sapling,
    potted_lily_of_the_valley,
    potted_oak_sapling,
    potted_orange_tulip,
    potted_oxeye_daisy,
    potted_pink_tulip,
    potted_poppy,
    potted_red_mushroom,
    potted_red_tulip,
    potted_spruce_sapling,
    potted_white_tulip,
    potted_wither_rose,
    powered_rail,
    prismarine,
    prismarine_brick_slab,
    prismarine_brick_stairs,
    prismarine_bricks,
    prismarine_slab,
    prismarine_stairs,
    prismarine_wall,
    pumpkin,
    pumpkin_stem,
    purple_banner,
    purple_bed,
    purple_carpet,
    purple_concrete,
    purple_concrete_powder,
    purple_glazed_terracotta,
    purple_shulker_box,
    purple_stained_glass,
    purple_stained_glass_pane,
    purple_terracotta,
    purple_wall_banner,
    purple_wool,
    purpur_block,
    purpur_pillar,
    purpur_slab,
    purpur_stairs,
    quartz_pillar,
    quartz_slab,
    quartz_stairs,
    rail,
    red_banner,
    red_bed,
    red_carpet,
    red_concrete,
    red_concrete_powder,
    red_glazed_terracotta,
    red_mushroom,
    red_mushroom_block,
    red_nether_brick_slab,
    red_nether_brick_stairs,
    red_nether_brick_wall,
    red_nether_bricks,
    red_sand,
    red_sandstone,
    red_sandstone_slab,
    red_sandstone_stairs,
    red_sandstone_wall,
    red_shulker_box,
    red_stained_glass,
    red_stained_glass_pane,
    red_terracotta,
    red_tulip,
    red_wall_banner,
    red_wool,
    comparator,
    redstone_wire,
    redstone_lamp,
    redstone_ore,
    repeater,
    redstone_torch,
    redstone_wall_torch,
    repeating_command_block,
    rose_bush,
    sand,
    sandstone,
    sandstone_slab,
    sandstone_stairs,
    sandstone_wall,
    scaffolding,
    sea_lantern,
    sea_pickle,
    seagrass,
    shulker_box,
    skeleton_skull,
    skeleton_wall_skull,
    slime_block,
    smithing_table,
    smoker,
    smooth_quartz,
    smooth_quartz_slab,
    smooth_quartz_stairs,
    smooth_red_sandstone,
    smooth_red_sandstone_slab,
    smooth_red_sandstone_stairs,
    smooth_sandstone,
    smooth_sandstone_slab,
    smooth_sandstone_stairs,
    smooth_stone,
    smooth_stone_slab,
    snow,
    snow_block,
    soul_sand,
    spawner,
    sponge,
    spruce_button,
    spruce_door,
    spruce_fence,
    spruce_fence_gate,
    spruce_leaves,
    spruce_log,
    spruce_planks,
    spruce_pressure_plate,
    spruce_sapling,
    spruce_sign,
    spruce_slab,
    spruce_stairs,
    spruce_trapdoor,
    spruce_wall_sign,
    spruce_wood,
    sticky_piston,
    stone,
    stone_brick_slab,
    stone_brick_stairs,
    stone_brick_wall,
    stone_bricks,
    stone_button,
    stone_pressure_plate,
    stone_slab,
    stone_stairs,
    stonecutter,
    stripped_acacia_log,
    stripped_acacia_wood,
    stripped_birch_log,
    stripped_birch_wood,
    stripped_dark_oak_log,
    stripped_dark_oak_wood,
    stripped_jungle_log,
    stripped_jungle_wood,
    stripped_oak_log,
    stripped_oak_wood,
    stripped_spruce_log,
    stripped_spruce_wood,
    structure_block,
    structure_void,
    sugar_cane,
    sunflower,
    sweet_berry_bush,
    tall_grass,
    tall_seagrass,
    terracotta,
    tnt,
    torch,
    trapped_chest,
    tripwire,
    tripwire_hook,
    tube_coral,
    tube_coral_block,
    tube_coral_fan,
    tube_coral_wall_fan,
    turtle_egg,
    vine,
    void_air,
    wall_torch,
    water,
    wet_sponge,
    wheat,
    white_banner,
    white_bed,
    white_carpet,
    white_concrete,
    white_concrete_powder,
    white_glazed_terracotta,
    white_shulker_box,
    white_stained_glass,
    white_stained_glass_pane,
    white_terracotta,
    white_tulip,
    white_wall_banner,
    white_wool,
    wither_rose,
    wither_skeleton_skull,
    wither_skeleton_wall_skull,
    yellow_banner,
    yellow_bed,
    yellow_carpet,
    yellow_concrete,
    yellow_concrete_powder,
    yellow_glazed_terracotta,
    yellow_shulker_box,
    yellow_stained_glass,
    yellow_stained_glass_pane,
    yellow_terracotta,
    yellow_wall_banner,
    yellow_wool,
    zombie_head,
    zombie_wall_head,

    // 1.15

    bee_nest,
    beehive,
    honey_block,
    honeycomb_block,

    // 1.16

    crimson_nylium,
    warped_nylium,
    crimson_planks,
    warped_planks,
    nether_gold_ore,
    crimson_stem,
    warped_stem,
    stripped_crimson_stem,
    stripped_warped_stem,
    crimson_hyphae,
    warped_hyphae,
    crimson_slab,
    warped_slab,
    cracked_nether_bricks,
    chiseled_nether_bricks,
    crimson_stairs,
    warped_stairs,
    netherite_block,
    soul_soil,
    basalt,
    polished_basalt,
    ancient_debris,
    crying_obsidian,
    blackstone,
    blackstone_slab,
    blackstone_stairs,
    gilded_blackstone,
    polished_blackstone,
    polished_blackstone_slab,
    polished_blackstone_stairs,
    chiseled_polished_blackstone,
    polished_blackstone_bricks,
    polished_blackstone_brick_slab,
    polished_blackstone_brick_stairs,
    cracked_polished_blackstone_bricks,
    crimson_fungus,
    warped_fungus,
    crimson_roots,
    warped_roots,
    nether_sprouts,
    weeping_vines,
    twisting_vines,
    crimson_fence,
    warped_fence,
    soul_torch,
    chain,
    blackstone_wall,
    polished_blackstone_wall,
    polished_blackstone_brick_wall,
    soul_lantern,
    soul_campfire,
    shroomlight,
    lodestone,
    respawn_anchor,
    crimson_pressure_plate,
    warped_pressure_plate,
    crimson_trapdoor,
    warped_trapdoor,
    crimson_fence_gate,
    warped_fence_gate,
    crimson_button,
    warped_button,
    crimson_door,
    warped_door,
    target,

    // bugfix for 1.16

    twisting_vines_plant,
    weeping_vines_plant,
    warped_wart_block,
    quartz_bricks,
    stripped_crimson_hyphae,
    stripped_warped_hyphae,
    crimson_sign,
    warped_sign,
    polished_blackstone_pressure_plate,
    polished_blackstone_button,
    crimson_wall_sign,
    warped_wall_sign,
    soul_fire,
    soul_wall_torch,

    // 1.17
    deepslate,
    cobbled_deepslate,
    polished_deepslate,
    calcite,
    tuff,
    dripstone_block,
    deepslate_coal_ore,
    deepslate_iron_ore,
    copper_ore,
    deepslate_copper_ore,
    deepslate_gold_ore,
    deepslate_redstone_ore,
    deepslate_emerald_ore,
    deepslate_lapis_ore,
    deepslate_diamond_ore,
    raw_iron_block,
    raw_copper_block,
    raw_gold_block,
    amethyst_block,
    budding_amethyst,
    copper_block,
    exposed_copper,
    weathered_copper,
    oxidized_copper,
    cut_copper,
    exposed_cut_copper,
    weathered_cut_copper,
    oxidized_cut_copper,
    waxed_copper_block,
    waxed_exposed_copper,
    waxed_weathered_copper,
    waxed_oxidized_copper,
    waxed_cut_copper,
    waxed_exposed_cut_copper,
    waxed_weathered_cut_copper,
    waxed_oxidized_cut_copper,
    azalea_leaves,
    tinted_glass,
    azalea,
    flowering_azalea,
    spore_blossom,
    moss_carpet,
    moss_block,
    hanging_roots,
    infested_deepslate,
    deepslate_bricks,
    cracked_deepslate_bricks,
    deepslate_tiles,
    cracked_deepslate_tiles,
    chiseled_deepslate,
    smooth_basalt,
    potted_azalea_bush,
    potted_flowering_azalea_bush,
    powder_snow,

    water_cauldron,
    lava_cauldron,
    powder_snow_cauldron,

    dirt_path,
    rooted_dirt,
    flowering_azalea_leaves,

    big_dripleaf,
    big_dripleaf_stem,
    small_dripleaf,

    candle,
    white_candle,
    orange_candle,
    magenta_candle,
    light_blue_candle,
    yellow_candle,
    lime_candle,
    pink_candle,
    gray_candle,
    light_gray_candle,
    cyan_candle,
    purple_candle,
    blue_candle,
    brown_candle,
    green_candle,
    red_candle,
    black_candle,

    candle_cake,
    white_candle_cake,
    orange_candle_cake,
    magenta_candle_cake,
    light_blue_candle_cake,
    yellow_candle_cake,
    lime_candle_cake,
    pink_candle_cake,
    gray_candle_cake,
    light_gray_candle_cake,
    cyan_candle_cake,
    purple_candle_cake,
    blue_candle_cake,
    brown_candle_cake,
    green_candle_cake,
    red_candle_cake,
    black_candle_cake,

    cut_copper_stairs,
    exposed_cut_copper_stairs,
    weathered_cut_copper_stairs,
    oxidized_cut_copper_stairs,

    waxed_cut_copper_stairs,
    waxed_exposed_cut_copper_stairs,
    waxed_weathered_cut_copper_stairs,
    waxed_oxidized_cut_copper_stairs,

    cobbled_deepslate_stairs,
    polished_deepslate_stairs,
    deepslate_brick_stairs,
    deepslate_tile_stairs,

    cut_copper_slab,
    exposed_cut_copper_slab,
    weathered_cut_copper_slab,
    oxidized_cut_copper_slab,

    waxed_cut_copper_slab,
    waxed_exposed_cut_copper_slab,
    waxed_weathered_cut_copper_slab,
    waxed_oxidized_cut_copper_slab,

    cobbled_deepslate_slab,
    polished_deepslate_slab,
    deepslate_brick_slab,
    deepslate_tile_slab,

    cobbled_deepslate_wall,
    polished_deepslate_wall,
    deepslate_brick_wall,
    deepslate_tile_wall,

    lightning_rod,

    small_amethyst_bud,
    medium_amethyst_bud,
    large_amethyst_bud,
    amethyst_cluster,

    pointed_dripstone,
    light,
    cave_vines,
    cave_vines_plant,
    glow_lichen,

    potted_crimson_fungus,
    potted_warped_fungus,
    potted_crimson_roots,
    potted_warped_roots,
    sculk_sensor,

    // 1.19
    reinforced_deepslate,
    sculk,
    sculk_catalyst,
    sculk_shrieker,
    sculk_vein,
    mangrove_planks,
    mangrove_roots,
    muddy_mangrove_roots,
    mangrove_log,
    stripped_mangrove_log,
    stripped_mangrove_wood,
    mangrove_wood,
    mangrove_slab,
    mud_brick_slab,
    packed_mud,
    mud_bricks,
    mangrove_fence,
    mangrove_stairs,
    mangrove_fence_gate,
    mud_brick_wall,
    mangrove_sign,
    mangrove_wall_sign,
    ochre_froglight,
    verdant_froglight,
    pearlescent_froglight,
    mangrove_leaves,
    mangrove_propagule,
    mangrove_button,
    mangrove_pressure_plate,
    mangrove_door,
    mangrove_trapdoor,
    frogspawn,
    potted_mangrove_propagule,
    mud,
    mud_brick_stairs,

    // 1.19.3 (rc1)
    acacia_hanging_sign,
    acacia_wall_hanging_sign,
    bamboo_block,
    bamboo_button,
    bamboo_door,
    bamboo_fence,
    bamboo_fence_gate,
    bamboo_hanging_sign,
    bamboo_mosaic,
    bamboo_mosaic_slab,
    bamboo_mosaic_stairs,
    bamboo_planks,
    bamboo_pressure_plate,
    bamboo_sign,
    bamboo_slab,
    bamboo_stairs,
    bamboo_trapdoor,
    bamboo_wall_hanging_sign,
    bamboo_wall_sign,
    birch_hanging_sign,
    birch_wall_hanging_sign,
    chiseled_bookshelf,
    crimson_hanging_sign,
    crimson_wall_hanging_sign,
    dark_oak_hanging_sign,
    dark_oak_wall_hanging_sign,
    jungle_hanging_sign,
    jungle_wall_hanging_sign,
    mangrove_hanging_sign,
    mangrove_wall_hanging_sign,
    oak_hanging_sign,
    oak_wall_hanging_sign,
    piglin_head,
    piglin_wall_head,
    spruce_hanging_sign,
    spruce_wall_hanging_sign,
    stripped_bamboo_block,
    warped_hanging_sign,
    warped_wall_hanging_sign,

    // 23w07a
    cherry_button,
    cherry_door,
    cherry_fence,
    cherry_fence_gate,
    cherry_hanging_sign,
    cherry_leaves,
    cherry_log,
    cherry_planks,
    cherry_pressure_plate,
    cherry_sapling,
    cherry_sign,
    cherry_slab,
    cherry_stairs,
    cherry_trapdoor,
    cherry_wall_hanging_sign,
    cherry_wall_sign,
    cherry_wood,
    decorated_pot,
    pink_petals,
    potted_cherry_sapling,
    potted_torchflower,
    stripped_cherry_log,
    stripped_cherry_wood,
    suspicious_sand,
    torchflower,
    torchflower_crop,

    // 1.20
    calibrated_sculk_sensor,
    pitcher_crop,
    pitcher_plant,
    sniffer_egg,
    suspicious_gravel,

    // 1.20.3
    tuff_stairs,
    tuff_slab,
    tuff_wall,
    chiseled_tuff,
    polished_tuff,
    polished_tuff_stairs,
    polished_tuff_slab,
    polished_tuff_wall,
    tuff_bricks,
    tuff_brick_stairs,
    tuff_brick_slab,
    tuff_brick_wall,
    chiseled_tuff_bricks,
    trial_spawner,

    chiseled_copper,
    exposed_chiseled_copper,
    weathered_chiseled_copper,
    oxidized_chiseled_copper,
    waxed_chiseled_copper,
    waxed_exposed_chiseled_copper,
    waxed_weathered_chiseled_copper,
    waxed_oxidized_chiseled_copper,

    copper_grate,
    exposed_copper_grate,
    weathered_copper_grate,
    oxidized_copper_grate,
    waxed_copper_grate,
    waxed_exposed_copper_grate,
    waxed_weathered_copper_grate,
    waxed_oxidized_copper_grate,

    copper_door,
    exposed_copper_door,
    weathered_copper_door,
    oxidized_copper_door,
    waxed_copper_door,
    waxed_exposed_copper_door,
    waxed_weathered_copper_door,
    waxed_oxidized_copper_door,

    copper_trapdoor,
    exposed_copper_trapdoor,
    weathered_copper_trapdoor,
    oxidized_copper_trapdoor,
    waxed_copper_trapdoor,
    waxed_exposed_copper_trapdoor,
    waxed_weathered_copper_trapdoor,
    waxed_oxidized_copper_trapdoor,

    copper_bulb,
    exposed_copper_bulb,
    weathered_copper_bulb,
    oxidized_copper_bulb,
    waxed_copper_bulb,
    waxed_exposed_copper_bulb,
    waxed_weathered_copper_bulb,
    waxed_oxidized_copper_bulb,

    minecraft_max_block_id,
};

}
