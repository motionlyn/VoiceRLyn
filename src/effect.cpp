#include <hardware/audio_effect.h>
#include <string.h>
#include <new>
#include "VoiceEngine.h"

const effect_descriptor_t voicerlyn_descriptor = {
    {0x11223344, 0x5566, 0x7788, 0x99aa, {0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00}}, // type UUID
    {0x11223344, 0x5566, 0x7788, 0x99aa, {0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00}}, // uuid
    EFFECT_CONTROL_API_VERSION,
    (EFFECT_FLAG_TYPE_PRE_PROC | EFFECT_FLAG_DEVICE_IND | EFFECT_FLAG_AUDIO_PRO),
    0, // cpu load
    0, // memory usage
    "VoiceRLyn Mic Changer",
    "MochiLynnn"
};

struct VoiceRLynContext {
    const struct effect_interface_s *itfe;
    effect_config_t config;
    VoiceEngine* engine;
};

int32_t VoiceRLyn_process(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer) {
    VoiceRLynContext *pContext = (VoiceRLynContext *)self;
    if (pContext == nullptr || pContext->engine == nullptr) return -EINVAL;

    float* in = (float*)inBuffer->raw;
    float* out = (float*)outBuffer->raw;
    int frames = inBuffer->frameCount;

    pContext->engine->process(in, out, frames);

    return 0;
}

extern "C" {
    __attribute__((visibility("default")))
    audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
        .tag = AUDIO_EFFECT_LIBRARY_TAG,
        .version = EFFECT_LIBRARY_API_VERSION,
        .name = "VoiceRLyn Native Library",
        .implementor = "MochiLynnn",
        .create_effect = [](const effect_uuid_t *uuid, int32_t sessionId, int32_t ioId, effect_handle_t *pHandle) -> int32_t {
            VoiceRLynContext *context = new VoiceRLynContext;
            context->engine = new VoiceEngine(48000.0f);
            *pHandle = (effect_handle_t)context;
            return 0;
        },
        .release_effect = [](effect_handle_t handle) -> int32_t {
            VoiceRLynContext *context = (VoiceRLynContext *)handle;
            delete context->engine;
            delete context;
            return 0;
        },
        .get_descriptor = [](const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor) -> int32_t {
            *pDescriptor = voicerlyn_descriptor;
            return 0;
        }
    };
}
