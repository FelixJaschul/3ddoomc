#include "util/sound.h"
#include "state.h"
#include "util/input.h"

#include <soloud_c.h>

RELOAD_VISIBLE void free_wav(void *p, void*) {
    Wav_destroy((Wav*) p);
}

int sound_init() {
    state->sound = calloc(1, sizeof(*state->sound));
    sound_state_t *ss = state->sound;

    ss->soloud = Soloud_create();
    if (!ss->soloud) { return INT_MIN; }

    int res;
    if ((res = Soloud_init(ss->soloud))) { return res; }

    map_init(
        &ss->sounds,
        map_hash_str,
        NULL,
        NULL,
        map_dup_str,
        map_cmp_str,
        map_default_free,
        free_wav,
        NULL);

    LOG(
        "initialized sound engine: %s/%d/%d",
        Soloud_getBackendString(ss->soloud),
        Soloud_getBackendChannels(ss->soloud),
        Soloud_getBackendSamplerate(ss->soloud));

    Soloud_setGlobalVolume(ss->soloud, 1.0f);
    return 0;
}

void sound_destroy() {
    sound_state_t *ss = state->sound;
    map_destroy(&ss->sounds);
    Soloud_deinit(ss->soloud);
    Soloud_destroy(ss->soloud);
}

void sound_clear() {
    map_clear(&state->sound->sounds);
}

sound_id sound_play(resource_t name) {
    strtoupper(name.name);

    sound_state_t *ss = state->sound;

    Wav **pwav = map_find(Wav*, &ss->sounds, RESOURCE_STR(name)), *wav = NULL;

    if (pwav) {
        wav = *pwav;
    } else {
        // check if sound exists as resource
        char path[1024];
        if (!resource_find_file(
                name, SOUNDS_BASE_DIR, "wav", path, sizeof(path))) {
            WARN("could not find sound %s", name.name);
            return -1;
        }

        wav = Wav_create();
        LOG("loading new wav %s from %s", name.name, path);

        if (!wav) {
            WARN("could not create wav");
            return -1;
        }

        int res;
        if ((res = Wav_load(wav, path))) {
            WARN("bad wav load %s: %d", path, res);
            return -1;
        }

        map_insert(
            &ss->sounds,
            strdup(name.name),
            wav);
    }

    sound_id id = Soloud_play(ss->soloud, wav);
    Soloud_setVolume(ss->soloud, id, 1.0f);
    return id;
}

sound_id sound_play_loop(resource_t name) {
    strtoupper(name.name);
    sound_state_t *ss = state->sound;

    Wav **pwav = map_find(Wav*, &ss->sounds, RESOURCE_STR(name)), *wav = NULL;

    if (!pwav) {
        char path[1024];
        if (!resource_find_file(name, SOUNDS_BASE_DIR, "wav", path, sizeof(path))) {
            WARN("could not find sound %s", name.name);
            return -1;
        }

        wav = Wav_create();
        if (!wav) return -1;

        if (Wav_load(wav, path)) {
            Wav_destroy(wav);
            return -1;
        }

        map_insert(&ss->sounds, strdup(name.name), wav);
    } else {
        wav = *pwav;
    }

    Wav_setLooping(wav, 1);  // Set looping on
    sound_id id = Soloud_play(ss->soloud, wav);
    Soloud_setVolume(ss->soloud, id, 1.0f);
    return id;
}

void sound_stop(sound_id id) {
    sound_state_t *ss = state->sound;
    if (ss && ss->soloud && id >= 0) {
        Soloud_stop(ss->soloud, id);
    }
}

void sound_stop_all() {
    sound_state_t *ss = state->sound;
    if (ss && ss->soloud) {
        Soloud_stopAll(ss->soloud);
    }
}
