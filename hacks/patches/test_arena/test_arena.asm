; Preserve the mission's terrain and player spawn records without story actors or scripts.
hookStub mission_script_spawnGroup
hookStub mission_script_runStream
hookReplace rng_reseed, testArena_reseed
