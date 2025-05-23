/* NetHack 3.7	seffects.h	$NHDT-Date: 1693253118 2023/08/28 20:05:18 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.3 $ */
/* Copyright (c) Michael Allison, 2023                                */
/* NetHack may be freely redistributed.  See license for details. */

#if defined(SEFFECTS_ENUM) || defined(SEFFECTS_AUTOMAP)

#if defined(SEFFECTS_ENUM)
#define seffect(basename) se_##basename
#else
#if defined(SEFFECTS_AUTOMAP)
#define seffect(basename) \
    { se_##basename, #basename }
#endif
#endif
    seffect(air_crackles),
    seffect(alarm),
    seffect(angry_drone),
    seffect(angry_snakes),
    seffect(angry_voice),
    seffect(applause),
    seffect(avian_screak),
    seffect(bang_weapon_side),
    seffect(bars_clink),
    seffect(bars_clonk),
    seffect(bars_flapp),
    seffect(bars_whang),
    seffect(bars_whap),
    seffect(bees),
    seffect(blast),
    seffect(board_squeak),
    seffect(board_squeaks_loudly),
    seffect(boing),
    seffect(bolt_of_lightning),
    seffect(bone_rattle),
    seffect(boomerang_klonk),
    seffect(boulder_drop),
    seffect(bovine_bellow),
    seffect(bovine_moo),
    seffect(bubble_rising),
    seffect(bugle_playing_reveille),
    seffect(buzz),
    seffect(canine_bark),
    seffect(canine_growl),
    seffect(canine_howl),
    seffect(canine_whine),
    seffect(canine_yelp),
    seffect(canine_yip),
    seffect(canine_yowl),
    seffect(chain_shatters),
    seffect(chains_rattling_gears_turning),
    seffect(chant),
    seffect(chirp),
    seffect(clanging_sound),
    seffect(clank),
    seffect(clanking_pipe),
    seffect(clash),
    seffect(cockatrice_hiss),
    seffect(cough),
    seffect(courtly_conversation),
    seffect(cracking_sound),
    seffect(crackling),
    seffect(crackling_of_hellfire),
    seffect(crash),
    seffect(crash_door),
    seffect(crash_something_broke),
    seffect(crash_throne_destroyed),
    seffect(crash_through_floor),
    seffect(crashed_ceiling),
    seffect(crashing_boulder),
    seffect(crashing_rock),
    seffect(crashing_sound),
    seffect(croc_bellow),
    seffect(crumbling_sound),
    seffect(crunching_sound),
    seffect(crushing_sound),
    seffect(deafening_roar_atmospheric),
    seffect(destroy_web),
    seffect(distant_thunder),
    seffect(divine_music),
    seffect(door_crash_open),
    seffect(door_open),
    seffect(door_unlock_and_open),
    seffect(drain_noises),
    seffect(dry_throat_rattle),
    seffect(egg_cracking),
    seffect(egg_splatting),
    seffect(elephant_trumpet),
    seffect(equine_neigh),
    seffect(equine_whicker),
    seffect(equine_whinny),
    seffect(explosion),
    seffect(faint_chime),
    seffect(faint_sloshing),
    seffect(faint_splashing),
    seffect(feline_meow),
    seffect(feline_mew),
    seffect(feline_purr),
    seffect(feline_yelp),
    seffect(feline_yip),
    seffect(feline_yowl),
    seffect(furious_bubbling),
    seffect(gear_turn),
    seffect(gears_turning_chains_rattling),
    seffect(glass_crashing),
    seffect(glass_shattering),
    seffect(groan),
    seffect(groans_and_moans),
    seffect(growl),
    seffect(grunt),
    seffect(guards_footsteps),
    seffect(gurgle),
    seffect(gushing_sound),
    seffect(heart_beat),
    seffect(hiss),
    seffect(hollow_sound),
    seffect(horn_being_played),
    seffect(iron_ball_dragging_you),
    seffect(iron_ball_hits_you),
    seffect(item_tumble_downwards),
    seffect(jabberwock_burble),
    seffect(kaablamm_of_mine),
    seffect(kaboom),
    seffect(kaboom_boom_boom),
    seffect(kaboom_door_explodes),
    seffect(kadoom_boulder_falls_in),
    seffect(kerplunk_boulder_gone),
    seffect(kick_door_it_crashes_open),
    seffect(kick_door_it_shatters),
    seffect(klick),
    seffect(klunk),
    seffect(klunk_pipe),
    seffect(laughter),
    seffect(lid_slams_open_falls_shut),
    seffect(loud_click),
    seffect(loud_crash),
    seffect(loud_pop),
    seffect(loud_splash),
    seffect(low_buzzing),
    seffect(low_hum),
    seffect(maniacal_laughter),
    seffect(masticating_sound),
    seffect(mon_chugging_potion),
    seffect(monster_behind_boulder),
    seffect(mutter_imprecations),
    seffect(mutter_incantation),
    seffect(orc_grunt),
    seffect(paranoid_confirmation),
    seffect(potion_crash_and_break),
    seffect(ring_in_drain),
    seffect(ripping_sound),
    seffect(snarl),
    seffect(roar),
    seffect(rumbling),
    seffect(rumbling_of_earth),
    seffect(rushing_wind_noise),
    seffect(rustling_paper),
    seffect(sad_wailing),
    seffect(sceptor_pounding),
    seffect(scratching),
    seffect(scream),
    seffect(screech),
    seffect(sewer_song),
    seffect(sharp_crack),
    seffect(shriek),
    seffect(shrill_whistle),
    seffect(sinister_laughter),
    seffect(sizzling),
    seffect(slurping_sound),
    seffect(smashing_and_crushing),
    seffect(snake_rattle),
    seffect(snakes_hissing),
    seffect(soft_click),
    seffect(soft_crackling),
    seffect(someone_bowling),
    seffect(someone_searching),
    seffect(someone_summoning),
    seffect(someone_yells),
    seffect(splash),
    seffect(splat_egg),
    seffect(splat_from_engulf),
    seffect(squawk),
    seffect(squeak),
    seffect(squeak_A),
    seffect(squeak_B),
    seffect(squeak_B_flat),
    seffect(squeak_C),
    seffect(squeak_D),
    seffect(squeak_D_flat),
    seffect(squeak_E),
    seffect(squeak_E_flat),
    seffect(squeak_F),
    seffect(squeak_F_sharp),
    seffect(squeak_G),
    seffect(squeak_G_sharp),
    seffect(squeal),
    seffect(squelch),
    seffect(stone_breaking),
    seffect(stone_crumbling),
    seffect(swoosh),
    seffect(sword_blade_rings),
    seffect(thud),
    seffect(thump),
    seffect(thunderclap),
    seffect(tumbler_click),
    seffect(typing_noise),
    seffect(wail),
    seffect(wailing_of_the_banshee),
    seffect(wall_of_force),
    seffect(yelp),
    seffect(zap),
    seffect(zap_then_explosion),
#undef seffect
#endif   /* SEFFECTS_ENUM || SEFFECTS_AUTOMAP */

/* seffects.h */

