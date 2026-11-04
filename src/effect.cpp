#include <stdint.h>
#include <string.h>
#include <new>
#include <errno.h>
#include "VoiceEngine.h"

// --- MINIMAL AUDIO EFFECT ABI (Menggantikan <hardware/audio_effect.h>) ---
typedef struct effect_uuid_s {
    uint32_t timeLow;
    uint16_t timeMid;
    uint16_t timeHiAndVersion;
    uint16_t clockSeq;
    uint8_t node[6];
} effect_uuid_t;

#define EFFECT_CONTROL_API_VERSION 0x00010000
#define EFFECT_FLAG_TYPE_PRE_PROC  0x00000002
#define EFFECT_FLAG_DEVICE_IND     0x00000000
#define EFFECT_FLAG_AUDIO_PRO      0x00000000

typedef struct effect_descriptor_s {
    effect_uuid_t type;
    effect_uuid_t uuid;
    uint32_t apiVersion;
    uint32_t flags;
    uint16_t cpuLoad;
    uint16_t memoryUsage;
    char    name[64];
    char    implementor[64];
} effect_descriptor_t;

typedef struct audio_buffer_s {
    size_t frameCount;
    union {
        void* raw;
        int32_t* s32;
        int16_t* s16;
        uint8_t* u8;
    };
} audio_buffer_t;

typedef struct effect_interface_s *effect_handle_t;

struct effect_interface_s {
    int32_t (*process)(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer);
    int32_t (*command)(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize, void *pCmdData, uint32_t *replySize, void *pReplyData);
    int32_t (*get_descriptor)(effect_handle_t self, effect_descriptor_t *pDescriptor);
    void * (*process_reverse)(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer);
};

#define AUDIO_EFFECT_LIBRARY_TAG ((uint32_t)('A' | ('E' << 8) | ('L' << 16) | ('I' << 24)))
#define EFFECT_LIBRARY_API_VERSION 0x00030000

typedef struct audio_effect_library_s {
    uint32_t tag;
    uint32_t version;
    const char *name;
    const char *implementor;
    int32_t (*create_effect)(const effect_uuid_t *uuid, int32_t sessionId, int32_t ioId, effect_handle_t *pHandle);
    int32_t (*release_effect)(effect_handle_t handle);
    int32_t (*get_descriptor)(const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor);
} audio_effect_library_t;
// --- END MINIMAL ABI ---

const effect_descriptor_t voicerlyn_descriptor = {
    {0x11223344, 0x5566, 0x7788, 0x99aa, {0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00}}, // type UUID
    {0x11223344, 0x5566, 0x7788, 0x99aa, {0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00}}, // uuid
    EFFECT_CONTROL_API_VERSION,
    (EFFECT_FLAG_TYPE_PRE_PROC | EFFECT_FLAG_DEVICE_IND | EFFECT_FLAG_AUDIO_PRO),
    0, 0,
    "VoiceRLyn Mic Changer",
    "MochiLynnn"
};

struct VoiceRLynContext {
    const struct effect_interface_s *itfe;
    VoiceEngine* engine;
};

// Deklarasi fungsi wajib HAL
int32_t VoiceRLyn_process(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer);
int32_t VoiceRLyn_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize, void *pCmdData, uint32_t *replySize, void *pReplyData);
int32_t VoiceRLyn_getDescriptor(effect_handle_t self, effect_descriptor_t *pDescriptor);

const struct effect_interface_s gInterface = {
    VoiceRLyn_process,
    VoiceRLyn_command,
    VoiceRLyn_getDescriptor,
    nullptr // process_reverse tidak dipakai
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

int32_t VoiceRLyn_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize, void *pCmdData, uint32_t *replySize, void *pReplyData) {
    // Fungsi command ini dibiarkan kosong namun Wajib Ada agar audioserver tidak crash.
    if (replySize) { *replySize = 0; }
    return 0;
}

int32_t VoiceRLyn_getDescriptor(effect_handle_t self, effect_descriptor_t *pDescriptor) {
    *pDescriptor = voicerlyn_descriptor;
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
            context->itfe = &gInterface; // Mengikat interface agar audioserver bisa memanggilnya
            context->engine = new VoiceEngine(48000.0f);
            *pHandle = (effect_handle_t)context;
            return 0;
        },
        .release_effect = [](effect_handle_t handle) -> int32_t {
            VoiceRLynContext *context = (VoiceRLynContext *)handle;
            if (context) {
                delete context->engine;
                delete context;
            }
            return 0;
        },
        .get_descriptor = [](const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor) -> int32_t {
            *pDescriptor = voicerlyn_descriptor;
            return 0;
        }
    };
}
