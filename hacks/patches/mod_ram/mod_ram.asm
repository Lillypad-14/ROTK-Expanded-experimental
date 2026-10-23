.thumb

hookReplace memory_init, modRam_init
hookReplace memory_malloc, modRam_malloc
hookReplace memory_zalloc, modRam_zalloc
hookReplace memory_free, modRam_free
hookReplace memory_realloc, modRam_realloc
hookReplace actor_allocMain, modRam_allocMain
hookReplace actor_allocOverlay, modRam_allocOverlay
hookReplace sprite_free, modRam_freeActor
