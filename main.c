#define DEBUG 1
#define _CRT_SECURE_NO_WARNINGS 1
#include "core.h"
#include <cassert>
#include <cstdio>

/* Platform Windows */
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include <windows.h>

#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32")

/* Renderer DX11 */
#ifndef D311_NO_HELPERS
    #define D311_NO_HELPERS
#endif

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#pragma comment(linker, "/subsystem:windows")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler")

#include <xaudio2.h>
#pragma comment(lib, "ole32")

/* --------------------------------------------------
   Window management
   -------------------------------------------------- */

typedef enum {
    _KEY_INVALID,

	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,

	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,

	KEY_SHIFT_LEFT,
	KEY_SHIFT_RIGHT,
	KEY_CONTROL_LEFT,
	KEY_CONTROL_RIGHT,
	KEY_ALT_LEFT,
	KEY_ALT_RIGHT,
	KEY_SUPER_LEFT,
	KEY_SUPER_RIGHT,

	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_BACKSPACE,
	KEY_DELETE,
	KEY_ENTER,
	KEY_ESCAPE,
	KEY_SPACE,
	KEY_TAB,

    KEY_ANY,

    _KEY_COUNT,
    _KEY_MAX = 0xFF
} key_codes;

typedef struct {
    u32_t width;
    u32_t height;
    char const* title;
    DWORD style;

    HWND window_handle;
    ATOM window_class_atom;

    b8_t should_close;

    u32_t target_fps;
    u64_t timer_previous;
    f64_t timer_frequency_ms;
    f64_t delta_time;

    /* TODO: We do not want to occupy this much memory on platforms with very
     * little of it. Especially if we dont have a keyboard anyway...*/
    b8_t key_down[512];
    b8_t key_down_prev[512];
    b8_t any_key_down;
    b8_t any_key_down_prev;
    u16_t key_code_to_scan_code[_KEY_COUNT];
    key_codes scan_code_to_key_code[512];
} window_ctx;

window_ctx* window_create(u32_t i_width, u32_t i_height, char const* i_title);
void window_destroy(window_ctx* io_window);
void window_poll_events(window_ctx* io_window);
b8_t window_key_down(window_ctx* i_window, key_codes i_key);
b8_t window_key_up(window_ctx* i_window, key_codes i_key);
b8_t window_key_pressed(window_ctx* i_window, key_codes i_key);
b8_t window_key_released(window_ctx* i_window, key_codes i_key);
LRESULT CALLBACK window_proc(HWND hWindow, UINT uMessage, WPARAM wParam, LPARAM lParam);

/* --------------------------------------------------
   DirectX11 graphics backend
   -------------------------------------------------- */

#define GRAPHICS_IMAGES_MAX 8
#define GRAPHICS_STRUCTURED_BUFFERS_MAX 8
#define GRAPHICS_CONSTANT_BUFFERS_MAX 8
#define GRAPHICS_VERTEX_ATTRIBUTES_MAX 16
#define GRAPHICS_VERTEX_BUFFERS_MAX 16

typedef struct {
    ID3D11Device* device;
    ID3D11DeviceContext* device_context;
    IDXGISwapChain* swap_chain;
    DXGI_SWAP_CHAIN_DESC swap_chain_desc;
    ID3D11Texture2D* render_target;
    ID3D11RenderTargetView* render_target_view;

    #if DEBUG
    ID3D11InfoQueue* debug_info_queue;
    #endif
} graphics_d3d11_ctx;

typedef struct {
    void* data;
    sz_t size;
} graphics_buffer_desc;

typedef struct {
    graphics_d3d11_ctx context;
    sz_t size;
    ID3D11Buffer* buffer;
} graphics_buffer;

typedef struct {
    void* data;
    sz_t size;
    sz_t count;
} graphics_structured_buffer_desc;

typedef struct {
    graphics_d3d11_ctx context;
    sz_t size;
    ID3D11Buffer* buffer;
    ID3D11ShaderResourceView* shader_resource_view;
} graphics_structured_buffer;

typedef enum {
    GRAPHICS_IMAGE_TYPE_R8G8B8A8_UINT = 0,
    GRAPHICS_IMAGE_TYPE_R8G8B8_UINT,
    GRAPHICS_IMAGE_TYPE_R8G8_UINT,
    GRAPHICS_IMAGE_TYPE_R8_UINT,
    _GRAPHICS_IMAGE_TYPE_COUNT,
    _GRAPHICS_IMAGE_TYPE_MAX = 0xFF,
} graphics_image_type;

typedef enum {
    GRAPHICS_FILTER_TYPE_POINT = 0,
    GRAPHICS_FILTER_TYPE_LINEAR,
    _GRAPHICS_FILTER_TYPE_COUNT,
    _GRAPHICS_FILTER_TYPE_MAX = 0xFF
} graphics_filter_type;

typedef enum {
    GRAPHICS_WARP_TYPE_REPEAT = 0,
    GRAPHICS_WRAP_TYPE_CLAMP,
    GRAPHICS_WRAP_TYPE_REPEAT_MIRROR,
    _GRAPHICS_WRAP_COUNT,
    _GRAPHICS_WRAP_MAX = 0xFF
} graphics_warp_type;

typedef struct {
    void* bytes;
    u32_t width;
    u32_t height;
    graphics_image_type type;
    graphics_filter_type filter;
    graphics_warp_type wrap_u;
    graphics_warp_type wrap_v;
} graphics_image_desc;

typedef struct {
    DXGI_FORMAT format;
    ID3D11Texture2D* texture_2d;
    ID3D11ShaderResourceView* shader_resource_view;
    ID3D11SamplerState* sampler_state; /* TODO: Should this really be part of the image instead of shader? */
} graphics_image;

typedef struct {
    u32_t vertex_constant_buffer_sizes[GRAPHICS_CONSTANT_BUFFERS_MAX];
    char const* vertex_shader_model;
    char const* vertex_entry_point;
    char const* vertex_source;

    u32_t fragment_constant_buffer_sizes[GRAPHICS_CONSTANT_BUFFERS_MAX];
    char const* fragment_shader_model;
    char const* fragment_entry_point;
    char const* fragment_source;
} graphics_shader_desc;

typedef struct {
    ID3D11Buffer* vertex_constant_buffers[GRAPHICS_CONSTANT_BUFFERS_MAX];
    ID3D11VertexShader* vertex_shader;
    ID3DBlob* vertex_shader_blob;

    ID3D11Buffer* fragment_constant_buffers[GRAPHICS_CONSTANT_BUFFERS_MAX];
    ID3D11PixelShader* fragment_shader;
    ID3DBlob* fragment_shader_blob;
} graphics_shader;

typedef enum  {
    GRAPHICS_VERTEX_ATTRIBUTE_INVALID = 0,
    GRAPHICS_VERTEX_ATTRIBUTE_FLOAT,
    GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2,
    GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3,
    GRAPHICS_VERTEX_ATTRIBUTE_FLOAT4,
    GRAPHICS_VERTEX_ATTRIBUTE_NUM,
    GRAPHICS_VERTEX_ATTRIBUTE_MAX = 0xFF
} graphics_vertex_attribute;

typedef enum {
    GRAPHICS_VERTEX_TOPOLOGY_TRIANGLES = 0,
    GRAPHICS_VERTEX_TOPOLOGY_TRIANGLE_STRIP,
    GRAPHICS_VERTEX_TOPOLOGY_LINES,
    GRAPHICS_VERTEX_TOPOLOGY_LINE_STRIP,
    GRAPHICS_VERTEX_TOPOLOGY_POINTS,
    GRAPHICS_VERTEX_TOPOLOGY_NUM,
    GRAPHICS_VERTEX_TOPOLOGY_MAX = 0xFF,
} graphics_vertex_topology;

typedef enum {
    GRAPHICS_VERTEX_STEP_FUNC_PER_VERTEX = 0,
    GRAPHICS_VERTEX_STEP_FUNC_PER_INSTANCE,
    GRAPHICS_VERTEX_STEP_FUNC_NUM,
    GRAPHICS_VERTEX_STEP_FUNC_MAX = 0xFF
} graphics_vertex_step_func;

typedef struct {
    char const* semantic_name;
    graphics_vertex_attribute format;
    u32_t offset;
    graphics_vertex_step_func step_func;
    u32_t step_rate;
    u32_t semantic_index;
    u32_t input_slot;
} graphics_vertex_attribute_desc;

typedef struct {
    graphics_shader shader;
    graphics_vertex_topology topology;
    graphics_vertex_attribute_desc attributes[GRAPHICS_VERTEX_ATTRIBUTES_MAX];
    u32_t stride;
} graphics_pipeline_desc;

typedef struct {
    graphics_d3d11_ctx context;
    graphics_shader shader;
    D3D_PRIMITIVE_TOPOLOGY topology;
    u32_t stride;
    ID3D11InputLayout* input_layout;
} graphics_pipeline;

typedef struct {
    graphics_pipeline pipeline;
    graphics_image images[GRAPHICS_IMAGES_MAX];
    graphics_structured_buffer structured_buffers[GRAPHICS_STRUCTURED_BUFFERS_MAX];
    graphics_buffer vertex_buffers[GRAPHICS_VERTEX_BUFFERS_MAX];
} graphics_bindings;

typedef enum {
    GRAPHICS_SHADER_STAGE_VERTEX = 0,
    GRAPHICS_SHADER_STAGE_FRAGMENT,
    _GRAPHICS_SHADER_STAGE_COUNT,
    _GRAPHICS_SHADER_STAGE_MAX = 0xFF,
} graphics_shader_stage;

graphics_d3d11_ctx graphics_d3d11_init(window_ctx* i_window_ctx);
void graphics_d3d11_destroy(graphics_d3d11_ctx* io_d3d11_ctx);
void graphics_print_messages(graphics_d3d11_ctx* i_d3d11_ctx);

graphics_buffer graphics_buffer_create(graphics_d3d11_ctx* i_d3d11_ctx, graphics_buffer_desc i_desc);
void graphics_buffer_destroy(graphics_buffer* io_buffer);
void graphics_buffer_update(graphics_buffer* i_buffer, void* i_data, size_t i_size);

graphics_structured_buffer graphics_structured_buffer_create(graphics_d3d11_ctx* i_d3d11_ctx, graphics_structured_buffer_desc i_desc);
void graphics_structured_buffer_destory(graphics_structured_buffer* io_buffer);
void graphics_structured_buffer_update(graphics_structured_buffer* i_buffer, void* i_data, size_t i_size);

graphics_image graphics_image_create(graphics_d3d11_ctx* i_d3d11_ctx, graphics_image_desc i_desc);
void graphics_image_destroy(graphics_image* io_image);

graphics_shader graphics_shader_create(graphics_d3d11_ctx* i_d3d11_ctx, graphics_shader_desc i_desc);
void graphics_shader_destroy(graphics_shader* io_shader);
ID3DBlob* graphics_shader_compile(char const* i_source, char const* i_shader_model, char const* i_entry_point);

graphics_pipeline graphics_pipeline_create(graphics_d3d11_ctx* i_d3d11_ctx, graphics_pipeline_desc i_desc);
void graphics_pipeline_destroy(graphics_pipeline* io_pipline);

graphics_bindings graphics_bindings_create(graphics_d3d11_ctx* i_d3d11_ctx, graphics_bindings i_desc);
void graphics_bindings_set(graphics_bindings* i_bindings);

void graphics_pass_begin(graphics_d3d11_ctx* i_d3d11_ctx, u32_t i_width, u32_t i_height);
void graphics_pass_end(graphics_d3d11_ctx* i_d3d11_ctx);
void graphics_constant_buffer_update(graphics_pipeline* i_pipeline, graphics_shader_stage i_stage, sz_t i_index, void* i_data);

fmat44_t graphics_calc_image_projection(window_ctx* i_window, f32_t i_image_width, f32_t i_image_height);

/* --------------------------------------------------
   XAudio2 backend 
   -------------------------------------------------- */

#define AUDIO_SOUNDS_MAX 32 
#define AUDIO_CONCURRENT_SOUNDS_MAX 16
#define AUDIO_VOLUME_DEFAULT 0.25

typedef enum {
    AUDIO_FLAG_NONE     = 0,
    AUDIO_FLAG_PLAY     = 1<<0,
    AUDIO_FLAG_STOP     = 1<<1,
    AUDIO_FLAG_FADE_IN  = 1<<2,
    AUDIO_FLAG_FADE_OUT = 1<<3,
    AUDIO_FLAG_LOOP     = 1<<4,

    _AUDIO_FLAG_COUNT = 5,
    _AUDIO_FLAG_MAX = 0xFF
} audio_flags;

typedef u8_t audio_sound_id;

/* Inheritance is C-style code! Whats next? Pigs fly?! Stupid Microsoft... 
 * TODO: (tbf I can probably rewrite this without...) */
struct audio_xaudio2_voice : IXAudio2VoiceCallback {
    IXAudio2SourceVoice* voice;
    f32_t volume;
    f32_t fade_in_duration;
    f32_t fade_out_duration;
    f32_t fade_timer;
    audio_sound_id sound_id;
    u8_t flags;
    b8_t is_playing;

    audio_xaudio2_voice() = default;
    virtual ~audio_xaudio2_voice() = default;

    void OnStreamEnd() noexcept                                         { voice->Stop(); is_playing = FALSE; }
    void OnBufferStart(void* io_buffer_cxt) noexcept                    { (void)io_buffer_cxt; is_playing = TRUE; }
    void OnBufferEnd(void* io_buffer_cxt) noexcept                      { (void)io_buffer_cxt; }
    void OnVoiceProcessingPassStart(UINT32 i_samples_required) noexcept { (void)i_samples_required; }
    void OnVoiceProcessingPassEnd() noexcept                            {}
    void OnLoopEnd(void* io_buffer_cxt) noexcept                        { (void)io_buffer_cxt; }
    void OnVoiceError(void* io_buffer_cxt, HRESULT i_error) noexcept    { (void)io_buffer_cxt; (void)i_error; }
};

typedef struct {
    audio_sound_id sound_id;
    u8_t flags;

    u8_t* data;
    u32_t size;

    f32_t volume;
    f32_t fade_in_duration;
    f32_t fade_out_duration;
} audio_sound;

typedef struct {
    f32_t volume;
    IXAudio2* xaudio2;
    IXAudio2MasteringVoice* master_voice;
    audio_xaudio2_voice* voices[AUDIO_CONCURRENT_SOUNDS_MAX];

    audio_sound sounds[AUDIO_SOUNDS_MAX];
    u32_t sounds_count;
    audio_sound_id sound_id_next;
} audio_xaudio2_ctx;

audio_xaudio2_ctx* audio_xaudio2_init();
void audio_xaudio2_destory(audio_xaudio2_ctx* io_xaudio2_ctx);
void audio_push_sounds(audio_xaudio2_ctx* i_xaudio2_ctx, f32_t i_delta_time);
audio_sound_id audio_play_sound(audio_xaudio2_ctx* i_xaudio2_ctx, void* i_data, u32_t i_size, f32_t i_volume, u8_t i_flags);

/* --------------------------------------------------
   Game 
   -------------------------------------------------- */

#define printf(...) {char buf[512]; sprintf(buf, __VA_ARGS__); OutputDebugStringA(buf);}
#define xstringify(...) stringify(__VA_ARGS__)
#define stringify(...) #__VA_ARGS__

#define RENDER_WIDTH 1600
#define RENDER_HEIGHT 900

typedef struct {
    f32_t x, y, z;
    f32_t u, v;
    f32_t r, g, b, a;
} vertex_data;

typedef struct {
    fmat44 model_view_projection;
    fvec2 render_size;
} camera_data;

typedef enum {
    BACKGROUND_TYPE_CREDITS = 0,
    BACKGROUND_TYPE_MAIN_MENU,
    BACKGROUND_TYPE_LEVEL,
    BACKGROUND_TYPE_END,
    _BACKGROUND_TYPE_COUNT,
    _BACKGROUND_TYPE_MAX = 0xFF
} background_type;
background_type g_background_type;

typedef struct {
    fvec2 position;
    fvec2 debug;
    f32 radius;
    u32 growth_state;
    f32 growth_factor; /* 0-1, range splits into thirds. */
    f32 time;
    u32 level_edit_object;
    f32 screen_fade;
    f32 scale;
    u32 maggots;
    u32 deaths;
    b8 is_dead;
    b8 enable_input;
} player_data;
player_data g_player;

typedef enum
{
    ENTITY_TYPE_INVALID = 0,
    ENTITY_TYPE_TUMOR,
    ENTITY_TYPE_T_CELL,
    ENTITY_TYPE_WALL,
    ENTITY_TYPE_PORTAL,
    ENTITY_TYPE_MAGGOT,
    ENTITY_TYPE_PARTICLES,
    ENTITY_TYPE_BOX_TEXT,
    ENTITY_TYPE_MOTHER,
    _ENTITY_TYPE_COUNT,
    _ENTITY_TYPE_MAX = 0xFF,
} entity_type;

typedef struct {
    entity_type type;
    fvec2 position;
    fvec3 growth_sizes1;
    fvec3 growth_sizes2;
    f32 sprite_index[2];
    f32 timer;
    b8 should_grow;
    b8 is_talking;
} entity_data;

#define ENTITIES_COUNT_MAX 128 
entity_data g_entities[ENTITIES_COUNT_MAX];
sz g_entities_count;

/* Defines so we can share these with the shader as a nice trick. */
#define SDF_PRIMITIVES_COUNT_MAX        128
#define SDF_PRIMITIVE_INVALID           0
#define SDF_PRIMITIVE_CIRCLE            1
#define SDF_PRIMITIVE_SPIKED_CIRCLE     2
#define SDF_PRIMITIVE_BOX               3
#define SDF_PRIMITIVE_BOX_TEXTURED      4
#define SDF_PRIMITIVE_BOX_TEXT          5
#define SDF_PRIMITIVE_BOX_FLOWER        6
#define SDF_PRIMITIVE_PORTAL            7
#define SDF_PRIMITIVE_MAGGOT            8
#define SDF_PRIMITIVE_PARTICLES         9

typedef struct {
    /* Packing of this struct is important as it will be send to the GPU. */
    u32 type;
    u32 entity;
    fvec3 growth_sizes1; /* Or x: size.x, y: pad, z: scale */
    fvec3 growth_sizes2; /* Or x: size.y, y: pad, z: pad */
    fvec2 position;
    f32 sprite_index[2];
} sdf_primitive;

sdf_primitive g_level_primitives[SDF_PRIMITIVES_COUNT_MAX];
sdf_primitive g_overlay_primitives[SDF_PRIMITIVES_COUNT_MAX];

#define LEVEL_WIDTH 1600
#define LEVEL_HEIGHT 900

#define SDF_RESULT_DISTANCE_INVALID 999999
#define SDF_RESULT_OBJECT_INVALID 0xFFFFFF

typedef struct {
    f32 distance;
    u32 closest_object;
    f32 overlapped_distance;
    u32 overlapped_object;
} sdf_result;

f32 sdf_circle(fvec2 i_position, f32 i_radius)
{
    return fvec2_len(i_position) - i_radius;
}

f32 sdf_box(fvec2 i_position, fvec2 i_size)
{
    fvec2 d = fvec2_sub(fvec2_abs(i_position), i_size);
    fvec2 c = { math_max(d.x, 0.0f), math_max(d.y, 0.0f) };
    return fvec2_len(c) + math_min(math_max(d.x, d.y), 0.0f);
}

f32 lerp_growth_factors(fvec3 i_sizes, f32 i_factor)
{
    f32 f = i_factor;
    if (f >= 0.0f/3.0f && f < 1.0f/3.0f)            return f32_lerp(i_sizes.x, i_sizes.y, (f - 0.0f/3.0f) * 3.0f);
    else if (f >= 1.0f/3.0f && f < 2.0f/3.0f)       return f32_lerp(i_sizes.y, i_sizes.z, (f - 1.0f/3.0f) * 3.0f);
    else/* if (f >= 2.0f/3.0f && f < 3.0f/3.0f) */  return f32_lerp(i_sizes.z, i_sizes.x, (f - 2.0f/3.0f) * 3.0f);
}

sdf_result sdf_get_distance(fvec2 i_position, f32 growth_factor, f32 i_time)
{
    sdf_result result;
    result.distance = SDF_RESULT_DISTANCE_INVALID;
    result.closest_object = SDF_RESULT_OBJECT_INVALID;
    result.overlapped_distance = SDF_RESULT_DISTANCE_INVALID;
    result.overlapped_object = SDF_RESULT_OBJECT_INVALID;

    (void)i_time;

    for (u32 i = 0; i < SDF_PRIMITIVES_COUNT_MAX; ++i)
    {
        float prev_distance = result.distance;
        float prev_overlapped_distance = result.overlapped_distance;

        /* Invalid means end of the list has been reached. */
        sdf_primitive* primitive = &g_level_primitives[i];
        if (primitive->type == SDF_PRIMITIVE_INVALID)
        {
            break;
        }

        f32 growth_size1 = lerp_growth_factors(primitive->growth_sizes1, growth_factor);
        f32 growth_size2 = lerp_growth_factors(primitive->growth_sizes2, growth_factor);

        /* Disable collision for 0 sized objects. */
        if (growth_size1 <= 0.0f) {
          continue;
        }

        /* Calculate the distance to this sdf shape. */
        switch (primitive->type)
        {
            case SDF_PRIMITIVE_CIRCLE: 
            {
                if (growth_size1 != 0.0f)
                {
                    /* Use a smooth min to help with collision resolution. Correct for 0 sized objects. */
                    f32 distance = sdf_circle(fvec2_sub(primitive->position, i_position), growth_size1);
                    result.distance = f32_min_smooth(result.distance, distance, 10.0f);
                }
            } break;
            case SDF_PRIMITIVE_SPIKED_CIRCLE: 
            {
                if (growth_size1 != 0.0f)
                {
                    /* Use a smooth min to help with collision resolution. Correct for 0 sized objects. 
                     * Adjust size to account for effects. */
                    f32 size = growth_size1;
                    f32 distance = sdf_circle(fvec2_sub(primitive->position, i_position), size);
                    result.distance = f32_min_smooth(result.distance, distance, 10.0f);
                }
            } break;
            case SDF_PRIMITIVE_BOX: 
            {
                /* Adjust size to account for effects. */
                fvec2 size = fvec2{ growth_size1 * 0.85f, growth_size2 * 0.85f };
                f32 distance = sdf_box(fvec2_sub(primitive->position, i_position), size);
                result.distance = math_min(result.distance, distance);
            } break;
            case SDF_PRIMITIVE_PORTAL:
            {
                f32 distance = sdf_circle(fvec2_sub(primitive->position, i_position), growth_size1);
                result.overlapped_distance = math_min(result.overlapped_distance, distance);
            } break;
            case SDF_PRIMITIVE_MAGGOT:
            {
                f32 distance = sdf_circle(fvec2_sub(primitive->position, i_position), growth_size1);
                result.overlapped_distance = math_min(result.overlapped_distance, distance);
            } break;
        }

        /* Track the closest object for hit detection. */
        if (prev_distance > result.distance) {
            result.closest_object = i;
        }
        if (prev_overlapped_distance > result.overlapped_distance) {
            result.overlapped_object = i;
        }
    }

    result.distance -= g_player.radius;
    result.overlapped_distance -= g_player.radius;
    return result;
}

fvec2 sdf_get_surface_normal(fvec2 i_position, f32 growth_factor, f32 i_time)
{
    f32 epsilon = 10.0f; /* Rather large sample distance to get around the non continuous sdf interior. */
    f32 x1 = sdf_get_distance({ i_position.x + epsilon, i_position.y }, growth_factor, i_time).distance; 
    f32 x2 = sdf_get_distance({ i_position.x - epsilon, i_position.y }, growth_factor, i_time).distance;
    f32 y1 = sdf_get_distance({ i_position.x, i_position.y + epsilon }, growth_factor, i_time).distance;
    f32 y2 = sdf_get_distance({ i_position.x, i_position.y - epsilon }, growth_factor, i_time).distance;

    f32 x_gradient = x1 - x2;
    f32 y_gradient = y1 - y2;
    return fvec2_norm({ x_gradient, y_gradient }); 
}

/* --------------------------------------------------
   Resources 
   -------------------------------------------------- */

/* Who needs a resource loading system if you can just hard code it into the binary. */
#include "assets/noise_texture.h"
#include "assets/credits_texture.h"
#include "assets/main_menu_texture.h"
#include "assets/background_texture.h"
#include "assets/end_texture.h"
#include "assets/spritesheet_texture.h"
#include "assets/music.h"
#include "assets/sound_heartbeat.h"
#include "assets/sound_maggot.h"
#include "assets/sound_death.h"
#include "levels.h"

int WINAPI WinMain(
    HINSTANCE hInstance, /* handle of the application */
    HINSTANCE hPrevInstance, /* NULL */
    LPSTR pCmdLine, /* unicode string pointer to command line arguments*/
    int nCmdShow /* flag indicating minimized, maximized, normal, etc. */
)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)pCmdLine;
    (void)nCmdShow;

    vertex_data vertices_square[] = {
        { /* position */ 0.0f,          0.0f,           0.0f, /* texcoord */ 0.0f, 0.0f, /* color */ 1.0f, 0.0f, 1.0f, 1.0f },
        { /* position */ RENDER_WIDTH,  0.0f,           0.0f, /* texcoord */ 1.0f, 0.0f, /* color */ 1.0f, 1.0f, 0.0f, 1.0f },
        { /* position */ 0.0f,          RENDER_HEIGHT,  0.0f, /* texcoord */ 0.0f, 1.0f, /* color */ 0.0f, 1.0f, 1.0f, 1.0f },
        { /* position */ RENDER_WIDTH,  RENDER_HEIGHT,  0.0f, /* texcoord */ 1.0f, 1.0f, /* color */ 1.0f, 1.0f, 1.0f, 1.0f }
    };

    u32_t image_checker[] = {
        0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF,
        0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF,
        0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF,
        0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF,
        0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF,
        0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF,
        0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF,
        0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF, 0x000000FF, 0xFFFFFFFF
    };

    char const* vertex_shader_src = xstringify(
        cbuffer c_camera
        {
            float4x4 c_camera_model_view_projection;
            float2 c_camera_render_size;
        };

        struct VOut
        {
            float4 position : SV_POSITION;
            float2 uv : UV;
            float4 color : COLOR;
        };

        VOut main(float4 position : POSITION, float2 uv : UV, float4 color : COLOR)
        {
            VOut output;
            output.position = mul(position, c_camera_model_view_projection);
            output.uv = uv * c_camera_render_size;
            output.color = color;
            return output;
        }
    );

    char const* fragment_shader_src = xstringify(
        /* TODO: This is the worst graphics programming you will ever see. Enjoy! */
        cbuffer c_player : register(b0) {
            float2 c_player_position;
            float2 c_debug;
            float c_player_radius;
            uint c_player_growth_state;
            float c_player_growth_factor;
            float c_player_time;
            uint c_level_edit_object;
            float c_player_screen_fade;
            float c_player_scale;
            uint  c_player_maggots;
            uint c_player_deaths;
        };

        Texture2D noise_texture : register(t0);
        SamplerState noise_sampler;

        Texture2D background_texture : register(t1);
        SamplerState background_sampler;

        Texture2D spritesheet_texture : register(t2);
        SamplerState spritesheet_sampler;

        struct sdf_primitive 
        {
            uint type;
            uint entity;
            float3 growth_sizes1; /* Or x: time, y: pad, z:pad */
            float3 growth_sizes2;
            float2 position;
            float2 sprite_index;
        };
        StructuredBuffer<sdf_primitive> sdf_level_primitives : register(t3);
        StructuredBuffer<sdf_primitive> sdf_overlay_primitives : register(t4);

        struct color_distance
        {
            float4 color;
            float distance;
        };

        float lerp3(float i_start, float i_middle, float i_end, float i_percentage)
        {
            float pick = round(i_percentage);
            float lerp1 = (1.0 - pick) * lerp(i_start, i_middle, i_percentage * 2.0);
            float lerp2 = pick * lerp(i_middle, i_end, (i_percentage - 0.5) * 2.0);
            return lerp1 + lerp2;
        }

        float atan(float y, float x)
        {
            if(x == 0 && y == 0) x = 1;
            return atan2(y, x);
        }

        float mod(float x, float y)
        {
            return x - y * floor(x / y);
        }

        float smin(float a, float b, float k)
        {
            float h = max(k-abs(a-b),0.0f);
            return min(a, b) - h*h*0.25f/k;
        }

        /* The following are progressively smoother noise functions.
         * TODO: Should really be a single backed noise map with multiple channels. 
         * But as this project progressed I kept finding the need to try slightly different noise functions and could not be bothered to bake them... */
        float random(float2 i_position)
        {
            return frac(sin(dot(i_position, float2(12.9898, 4.1414))) * 43758.5453);
        }

        float noise(float2 i_position, float i_offset)
        {
            float2 i = floor(i_position + float2(i_offset, 0.0));
            float2 f = frac(i_position + float2(i_offset, 0.0));
            f = f * f * (3.0 - 2.0 * f);
            float a = random(i + float2(0.0, 0.0));
            float b = random(i + float2(1.0, 0.0));
            float c = random(i + float2(0.0, 1.0));
            float d = random(i + float2(1.0, 1.0));
            return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
        }

        float sample_noise(float2 i_position, float i_offset)
        {
            float4 noise_sample = noise_texture.Sample(noise_sampler, i_position / float2(1600.0, 900.0));
            float t = 6.283185 * frac(i_offset / 2.0);
            float k = 1.57079625;
            float4 sc = float4(0.0, k, 2.0 * k, 3.0 * k) + t;
            float4 sn = (sin(sc) + 1.0) * 0.4;
            return dot(sn, noise_sample);
        }

        float2 rotate(float2 i_position, float i_rotation){
            float angle = i_rotation * 3.1415925 * 2.0 * -1.0;
            return float2(cos(angle) * i_position.x + sin(angle) * i_position.y, 
                          cos(angle) * i_position.y - sin(angle) * i_position.x);
        }

        /* Fundamental sdf shapes
         * Credit to iq (https://iquilezles.org/articles/distfunctions2d/) for the original signed distance functions. */

        float sdf_circle(float2 i_uv, float2 i_position, float i_radius)
        {
            return length(i_position - i_uv) - i_radius;
        }

        float sdf_box(float2 i_uv, float2 i_position, float2 i_size)
        {
            float2 distance = abs(i_position - i_uv) - i_size;
            return length(max(distance, 0.0)) + min(max(distance.x, distance.y), 0.0);
        }

        float sdf_segment(float2 i_uv, float2 i_position, float2 i_start, float2 i_end)
        {
            float2 position_start = (i_position - i_uv) - i_start;
            float2 end_start = i_end - i_start;
            float height = clamp( dot(position_start, end_start) / dot(end_start, end_start), 0.0, 1.0 );
            return length(position_start - end_start * height);
        }

        float sdf_triangle_isosceles(float2 i_uv, float2 i_position, float2 i_half_width_height)
        {
            i_position.x = abs(i_position.x);
            float2 a = i_position - i_half_width_height * clamp(dot(i_position, i_half_width_height) / dot(i_half_width_height, i_half_width_height), 0.0, 1.0);
            float2 b = i_position - i_half_width_height * float2(clamp(i_position.x / i_half_width_height.x, 0.0, 1.0 ), 1.0);
            float s = -sign(i_half_width_height.y);
            float2 d = min(float2(dot(a, a), s * (i_position.x * i_half_width_height.y - i_position.y * i_half_width_height.x)), float2(dot(b, b), s * (i_position.y - i_half_width_height.y)));
            return -sqrt(d.x) * sign(d.y);
        }
        
        float sdf_spiral(float2 i_uv, float2 i_position, float i_width, float i_rotations)
        {
            /* Body */
            i_position -= i_uv;
            float tau = 6.283185307;
            float r = length(i_position);
            float a = atan(i_position.y, i_position.x);
            float n = floor(0.5 / i_width + (log2(r / i_width) * i_rotations-a) / tau);
            float ra = i_width * exp2((a + tau * (min(n + 0.0, 0.0) - 0.5)) / i_rotations);
            float rb = i_width * exp2((a + tau * (min(n + 1.0, 0.0) - 0.5)) / i_rotations);
            float d = min(abs(r - ra), abs(r - rb));

            /* Tip */
            return min(d, length(i_position + float2(i_width, 0.0))) - 2.0;
        }

        /* Assembled sdf shapes. 
         * TODO: Don't you love random hard coded values... */

        float sdf_player(float2 i_uv, float2 i_position, float i_scale, float i_time)
        {
            float2 scale = float2(abs(sin(i_time * 3.0)), abs(cos(i_time * 3.0)));
            scale *= 0.25;
            scale = (1.0 + scale) / 2.0;
            scale *= float2(i_scale, i_scale);

            i_uv += float2(800.0, 450.0) * (i_position / float2(800.0, 450.0)) * scale;
            i_uv /= 1.0 + scale;

            float radius = 27.0 * i_scale;
            float circle = sdf_circle(i_uv, i_position, radius);
            circle = min(circle, sdf_circle(i_uv, i_position - float2(25.0, 10.0), radius * 0.3));
            circle = min(circle, sdf_circle(i_uv, i_position - float2(10.0, 20.0), radius * 0.5));
            circle = min(circle, sdf_circle(i_uv, i_position - float2(-15.0, 0.0), radius * 0.6));
            circle = min(circle, sdf_circle(i_uv, i_position - float2(-15.0, 20.0), radius * 0.3));

            float arm1 = sdf_segment(i_uv, i_position, float2(15.0, 10.0), i_scale * float2(15.0, -20.0 * (1.0 + scale.x * 0.4)));
            float arm2 = sdf_segment(i_uv, i_position, float2(-15.0, 10.0), i_scale * float2(-15.0, -20.0 * (1.0 + scale.y * 0.4)));
            arm1 -= 5.0 * i_scale;
            arm2 -= 5.0 * i_scale;
            return min(circle, min(arm1, arm2));
        }

        float sdf_wobbly_circle(float2 i_uv, float2 i_position, float i_radius)
        {
            return sdf_circle(i_uv, i_position, i_radius) - 25.0 * sample_noise(i_uv, c_player_time);
        }

        float sdf_spiked_circle(float2 i_uv, float2 i_position, float i_size)
        {
            float2 position = i_position - i_uv;
            float b = 6.283185 / 24.0;
            float a = atan(position.y, position.x);
            float i = floor(a / b);

            float c1 = b*(i+0.0); 
            float c2 = b*(i+1.0); 
            float2 p1 = mul(transpose(float2x2(cos(c1), -sin(c1), sin(c1), cos(c1))), position);
            float2 p2 = mul(transpose(float2x2(cos(c2), -sin(c2), sin(c2), cos(c2))), position);

            float o = mod(round(a / 6.283185 * 16.0), 2.0);
            float t = (1.0 + sin(c_player_time)) / 2.0;
            float r = o + t - 2.0 * o * t;

            float m = 4.712388;
            p1 = mul(transpose(float2x2(cos(m), sin(m), -sin(m), cos(m))), (p1 - float2(i_size * 0.9, 0.0)));
            p2 = mul(transpose(float2x2(cos(m), sin(m), -sin(m), cos(m))), (p2 - float2(i_size * 0.9, 0.0)));
            p1.y -= r * 0.15 * i_size;
            p2.y -= r * 0.15 * i_size;

            float2 spike_size = float2(0.02, 0.15 - r * 0.15) * i_size; 
            float spikes = min(sdf_triangle_isosceles(float2(0.0, 0.0), p1, spike_size), 
                               sdf_triangle_isosceles(float2(0.0, 0.0), p2, spike_size));

            return min(spikes, sdf_circle(i_uv, i_position, 0.75f * i_size));
        }

        float sdf_wobbly_box(float2 i_uv, float2 i_position, float2 i_size)
        {
            float distance = sdf_box(i_uv, i_position, i_size) - 25.0 * sample_noise(i_uv, 0.0);
            if (distance <= 0.0)
            {
                distance *= -step(sample_noise(i_uv, c_player_time * 0.1), 0.7);
            }
            return distance;
        }

        color_distance sdf_box_textured(float2 i_uv, float2 i_position, float2 i_size, float2 sprite_index, float2 sprite_size, float2 spritesheet_size)
        {
            float2 uv_size = float2(1600.0, 900.0);

            float box_distance = sdf_box(i_uv, i_position, i_size);
            float2 top_left = i_position - i_size;
            float2 box_uv = (i_uv - top_left) / i_size * 0.5;

            float2 scale = sprite_size / spritesheet_size;
            box_uv *= scale;
            box_uv += scale * sprite_index;

            float4 sprite = spritesheet_texture.Sample(spritesheet_sampler, box_uv);
            color_distance result;
            result.color = sprite;
            result.distance = 1.0 - step(box_distance, 0.0);
            return result;
        }
        
        float sdf_portal(float2 i_uv, float2 i_position, float i_size, float i_time)
        {
            float2 position = i_position - i_uv;
            position = rotate(position, 0.1 * c_player_time);
            float b = 6.283185 / 9.0;
            float a = atan(position.y, position.x);
            float i = floor(a / b);

            float c1 = b*(i+0.0); 
            float c2 = b*(i+1.0); 
            float2 p1 = mul(transpose(float2x2(cos(c1), -sin(c1), sin(c1), cos(c1))), position);
            float2 p2 = mul(transpose(float2x2(cos(c2), -sin(c2), sin(c2), cos(c2))), position);
            p1 -= float2(i_size, 0.0);
            p2 -= float2(i_size, 0.0);

            float portal = min(sdf_circle(float2(0.0, 0.0), p1, i_size * 0.1), 
                               sdf_circle(float2(0.0, 0.0), p2, i_size * 0.1));
            portal -= 0.6 * i_size * sample_noise(i_uv, c_player_time);
            portal = min(portal, sdf_circle(i_uv, i_position, i_size * 0.5));

            float spiral = sdf_spiral(float2(0.0, 0.0), position, 2.0 * i_size, 6.0);
            spiral -= 0.1 * i_size * sample_noise(i_uv, c_player_time);

            return max(-spiral, portal);
        }

        float sdf_maggot(float2 i_uv, float2 i_position, float i_size, float i_time)
        {
            float2 scale = float2(abs(sin(i_time * 3.0)), abs(cos(i_time * 3.0)));
            scale *= 0.25;
            scale = (1.0 + scale) / 2.0;

            i_uv += float2(800.0, 450.0) * (i_position / float2(800.0, 450.0)) * scale;
            i_uv /= 1.0 + scale;

            i_position -= i_uv;
            i_position -= i_size * float2(0.0, -0.1);
            i_position = rotate(i_position, 0.25 * i_time);

            /* body */
            float b1 = length(i_position - 2.0 * i_size * float2(0.05, -0.25)) - i_size * 0.95;
            float b2 = length(i_position - 2.0 * i_size * float2(0.3, -0.05)) - i_size * 0.75;
            float b3 = length(i_position - 2.0 * i_size * float2(0.35, 0.25)) - i_size * 0.5;
            float b4 = length(i_position - 2.0 * i_size * float2(0.25, 0.5)) - i_size * 0.3;
            float b5 = length(i_position - 2.0 * i_size * float2(0.05, 0.59)) - i_size * 0.25;
            float b6 = length(i_position - 2.0 * i_size * float2(-0.1, 0.57)) - i_size * 0.2;

            /* eyes */
            float e7 = length(i_position - 2.0 * i_size * float2(-0.4, -0.3)) - i_size * 0.25;
            float e8 = length(i_position - 2.0 * i_size * float2(0.25, -0.3)) - i_size * 0.25;

            float d1 = min(b1, b2);
            float d2 = min(d1, b3);
            float d3 = min(d2, b4);
            float d4 = min(d3, b5);
            float d5 = min(d4, b6);

            float d6 = max(d5, -e7);
            float d7 = max(d6, -e8);
            return d7;
        }

        float sdf_particles(float2 i_uv, float2 i_position, float i_factor)
        {
            float distance = SDF_RESULT_DISTANCE_INVALID;
            for (uint i = 0; i < 32; ++i)
            {
                float2 seed = float2(i_position.x, i_position.y + float(i));
                float2 position = float2(noise(seed, 1.0) - 0.5, noise(seed, 2.0) - 0.5);

                float2 velocity = normalize(position) * lerp(100.0, 500.0, noise(seed, 3.0));
                position += velocity * i_factor;

                float radius = lerp(8.0, 14.0, sample_noise(seed, 3.0));
                radius *= 1.0 - i_factor;
                distance = min(distance, sdf_circle(i_uv, i_position + position, radius));
            }
            return distance;
        }

        float lerp_growth_factors(float3 i_sizes, float i_factor)
        {
            /* TODO: Feels like this can be done better... Too lazy to do math. */
            float f = i_factor;
            if (f >= 0.0f/3.0f && f < 1.0f/3.0f)            return lerp(i_sizes.x, i_sizes.y, (f - 0.0f/3.0f) * 3.0f);
            else if (f >= 1.0f/3.0f && f < 2.0f/3.0f)       return lerp(i_sizes.y, i_sizes.z, (f - 1.0f/3.0f) * 3.0f);
            else/* if (f >= 2.0f/3.0f && f < 3.0f/3.0f) */  return lerp(i_sizes.z, i_sizes.x, (f - 2.0f/3.0f) * 3.0f);
        }

        color_distance get_distance(StructuredBuffer<sdf_primitive> i_primitives, float2 i_uv, float i_growth_factor, float i_time)
        {
            float4 cur_color = float4(0.0, 0.0, 0.0, 0.0);
            float cur_distance = SDF_RESULT_DISTANCE_INVALID;

            for (uint i = 0; i < SDF_PRIMITIVES_COUNT_MAX; ++i)
            {
                sdf_primitive primitive = i_primitives[i];
                if (primitive.type == SDF_PRIMITIVE_INVALID)
                {
                    break;
                }

                float prev_distance = cur_distance;
                float distance = SDF_RESULT_DISTANCE_INVALID;
                float4 color = float4(0.0, 0.0, 0.0, 0.0);

                float growth_size1 = lerp_growth_factors(primitive.growth_sizes1, i_growth_factor);
                float growth_size2 = lerp_growth_factors(primitive.growth_sizes2, i_growth_factor);

                switch (primitive.type)
                {
                    case SDF_PRIMITIVE_CIRCLE: 
                    {
                        color = float4(1.0, 0.05, 0.0, 0.0);
                        distance = sdf_wobbly_circle(i_uv, primitive.position, growth_size1);
                    } break;
                    case SDF_PRIMITIVE_SPIKED_CIRCLE:
                    {
                        color = float4(1.0, 0.05, 0.0, 0.0);
                        distance = sdf_spiked_circle(i_uv, primitive.position, growth_size1);
                    } break;
                    case SDF_PRIMITIVE_BOX:
                    {
                        color = float4(1.0, 0.05, 0.0, 0.0);
                        distance = sdf_wobbly_box(i_uv, primitive.position, float2(growth_size1, growth_size2));
                    } break;
                    case SDF_PRIMITIVE_BOX_TEXTURED:
                    {
                        color_distance result = sdf_box_textured(
                            i_uv, 
                            primitive.position, 
                            float2(growth_size1, growth_size2), 
                            primitive.sprite_index, 
                            float2(384.0, 128.0), 
                            float2(TEXTURE_SPRITESHEET_WIDTH, TEXTURE_SPRITESHEET_HEIGHT)
                        );
                        color = result.color;
                        distance = result.distance; 
                    } break;
                    case SDF_PRIMITIVE_BOX_TEXT:
                    {
                        float background = sdf_box(
                            i_uv, 
                            primitive.position, 
                            float2(primitive.growth_sizes1.x - 10.0, primitive.growth_sizes2.x - 10.0) * primitive.growth_sizes1.z 
                        );
                        background -= 25.0 * sample_noise(i_uv, 0.0) * primitive.growth_sizes1.z;

                        color_distance text = sdf_box_textured(
                            i_uv, 
                            primitive.position, 
                            float2(primitive.growth_sizes1.x, primitive.growth_sizes2.x) * primitive.growth_sizes1.z, 
                            primitive.sprite_index, 
                            float2(512.0, 162.0), 
                            float2(TEXTURE_SPRITESHEET_WIDTH, TEXTURE_SPRITESHEET_HEIGHT)
                        );
                        color = lerp(float4(1.0, 1.0, 1.0, 1.0), 0.0 * text.color, clamp(1.0 - text.distance, 0.0, 1.0) * text.color.x);
                        color.x = 1.0;
                        distance = background; 
                    } break;
                    case SDF_PRIMITIVE_BOX_FLOWER:
                    {
                        /* TODO: this has no purpose being a separate sdf type. 
                         * Just pass sprite size as part of the data instead. */
                        color_distance result = sdf_box_textured(
                            i_uv, 
                            primitive.position, 
                            float2(growth_size1, growth_size2), 
                            primitive.sprite_index, 
                            float2(128.0, 128.0), 
                            float2(TEXTURE_SPRITESHEET_WIDTH, TEXTURE_SPRITESHEET_HEIGHT)
                        );
                        color = result.color;
                        distance = result.distance; 
                    } break;
                    case SDF_PRIMITIVE_PORTAL:
                    {
                        color = float4(1.0, 0.0, 0.0, 0.0);
                        distance = sdf_portal(i_uv, primitive.position, growth_size1, i_time);
                    } break;
                    case SDF_PRIMITIVE_MAGGOT:
                    {
                        color = float4(1.0, 0.0, 0.0, 0.0);
                        distance = sdf_maggot(i_uv, primitive.position, growth_size1, i_time);
                    } break;
                    case SDF_PRIMITIVE_PARTICLES:
                    {
                        color = float4(1.0, 0.0, 0.0, 0.0);
                        distance = sdf_particles(i_uv, primitive.position, primitive.growth_sizes1.z);
                    } break;
                }

                cur_distance = min(cur_distance, distance);
                if (cur_distance < prev_distance)
                {
                    cur_color = lerp(cur_color, color, clamp(1.0 - cur_distance, 0.0, 1.0) * color.x);
                }

                /* Debug */
                /*
                if (distance <= 0.0)
                {
                    if (i == c_level_edit_object)
                    {
                        cur_color = float4(1.0, 0.0, 0.0, 1.0);
                    }
                }
                */
            }

            color_distance result;
            result.color = cur_color;
            result.distance = cur_distance;
            return result;
        }
    )
    xstringify(
        float dust_particles(float2 i_uv, float i_factor)
        {
            float fov = 150.0;
            float resolution = 1.0;
            uint layers = 4;

            float color = 0.0;
            for (uint i = 1; i < layers; ++i)
            {
                float2 transformed_uv = i_uv / float2(900.0, 900.0) - 0.5;
                transformed_uv *= tan(radians(i * 4.0 + fov) / 2.0);
                transformed_uv += float2(-i_factor / 8.0 + i / 12.0, sin(i - i_factor / 4.0 + transformed_uv.x) * 0.3);

                float strength = 0.01 * sample_noise(ceil(transformed_uv * resolution + i * 9.0) + i, i_factor);
                float distance = length(frac(transformed_uv) - 0.5 / resolution);

                if (distance < strength) 
                {
                    color += 1.0 / i - distance * i * 6.0;;
                }
            }

            return color;
        }

        color_distance player_hud(float2 i_uv)
        {
            color_distance result;
            result.distance = SDF_RESULT_DISTANCE_INVALID;
            result.color = float4(1.0, 1.0, 1.0, 1.0);

            float2 position = float2 (20.0, 20.0);

            result.distance = min(result.distance, sdf_wobbly_circle(i_uv, position + float2(0.0, -50.0), 100.0));
            result.distance = min(result.distance, sdf_wobbly_circle(i_uv, position + float2(100.0, -50.0), 50.0));

            color_distance deaths_icon = sdf_box_textured(
                i_uv, 
                position + float2(16.0, 0.0), 
                float2(16.0, 16.0), 
                float2(19.0, 1.0), 
                float2(64.0, 64.0), 
                float2(TEXTURE_SPRITESHEET_WIDTH, TEXTURE_SPRITESHEET_HEIGHT)
            );
            color_distance deaths_10 = sdf_box_textured(
                i_uv, 
                position + float2(16.0 + 32.0, 0.0), 
                float2(16.0, 16.0), 
                float2(18.0, floor(c_player_deaths / 10.0)), 
                float2(64.0, 64.0), 
                float2(TEXTURE_SPRITESHEET_WIDTH, TEXTURE_SPRITESHEET_HEIGHT)
            );
            color_distance deaths_01 = sdf_box_textured(
                i_uv, 
                position + float2(16.0 + 48.0, 0.0), 
                float2(16.0, 16.0), 
                float2(18.0, mod(c_player_deaths, 10.0)), 
                float2(64.0, 64.0), 
                float2(TEXTURE_SPRITESHEET_WIDTH, TEXTURE_SPRITESHEET_HEIGHT)
            );
            deaths_icon.color = float4(deaths_icon.color.x, float3(1.0, 1.0, 1.0) - deaths_icon.color.yzw);
            deaths_01.color = float4(deaths_01.color.x, float3(1.0, 1.0, 1.0) - deaths_01.color.yzw);
            deaths_10.color = float4(deaths_10.color.x, float3(1.0, 1.0, 1.0) - deaths_10.color.yzw);

            result.distance = min(result.distance, deaths_icon.distance);
            result.distance = min(result.distance, deaths_01.distance);
            result.distance = min(result.distance, deaths_10.distance);
            result.color = lerp(result.color, deaths_icon.color, step(deaths_icon.distance, 0.0) * deaths_icon.color.x);
            result.color = lerp(result.color, deaths_01.color, step(deaths_01.distance, 0.0) * deaths_01.color.x);
            result.color = lerp(result.color, deaths_10.color, step(deaths_10.distance, 0.0) * deaths_10.color.x);

            color_distance maggots_icon = sdf_box_textured(
                i_uv, 
                position + float2(0.0, 28.0), 
                float2(16.0, 16.0), 
                float2(19.0, 0.0), 
                float2(64.0, 64.0), 
                float2(TEXTURE_SPRITESHEET_WIDTH, TEXTURE_SPRITESHEET_HEIGHT)
            );
            color_distance maggots_10 = sdf_box_textured(
                i_uv, 
                position + float2(32.0, 28.0), 
                float2(16.0, 16.0), 
                float2(18.0, floor(c_player_maggots / 10.0)), 
                float2(64.0, 64.0), 
                float2(TEXTURE_SPRITESHEET_WIDTH, TEXTURE_SPRITESHEET_HEIGHT)
            );
            color_distance maggots_01 = sdf_box_textured(
                i_uv, 
                position + float2(48.0, 28.0), 
                float2(16.0, 16.0), 
                float2(18.0, mod(c_player_maggots, 10.0)), 
                float2(64.0, 64.0), 
                float2(TEXTURE_SPRITESHEET_WIDTH, TEXTURE_SPRITESHEET_HEIGHT)
            );
            maggots_icon.color = float4(maggots_icon.color.x, float3(1.0, 1.0, 1.0) - maggots_icon.color.yzw);
            maggots_01.color = float4(maggots_01.color.x, float3(1.0, 1.0, 1.0) - maggots_01.color.yzw);
            maggots_10.color = float4(maggots_10.color.x, float3(1.0, 1.0, 1.0) - maggots_10.color.yzw);

            result.distance = min(result.distance, maggots_icon.distance);
            result.distance = min(result.distance, maggots_01.distance);
            result.distance = min(result.distance, maggots_10.distance);
            result.color = lerp(result.color, maggots_icon.color, step(maggots_icon.distance, 0.0) * maggots_icon.color.x);
            result.color = lerp(result.color, maggots_01.color, step(maggots_01.distance, 0.0) * maggots_01.color.x);
            result.color = lerp(result.color, maggots_10.color, step(maggots_10.distance, 0.0) * maggots_10.color.x);
            return result;
        }

        float3 intro_screen(float2 i_uv, float i_progress)
        {
            float3 color = float3(0.0, 0.0, 0.0);
            color_distance credits = sdf_box_textured(
                i_uv, 
                float2(800.0, 400.0), 
                float2(192.0, 256.0), 
                float2(0, 2.6), 
                float2(384.0, 512.0), 
                float2(TEXTURE_SPRITESHEET_WIDTH, TEXTURE_SPRITESHEET_HEIGHT)
            );
            color = lerp(color, credits.color.yzw, step(credits.distance, 0.0) * credits.color.x * i_progress);
            return color;
        }

        struct VOut
        {
            float4 position : SV_POSITION;
            float2 uv : UV;
            float4 color : COLOR;
        };

        float4 main(VOut vertex_input) : SV_TARGET
        {
            float4 background = background_texture.Sample(background_sampler, vertex_input.uv / float2(1600.0, 900.0));
            float3 color = background.wyz;

            float2 uv = vertex_input.uv / float2(1600.0, 900.0);
            uv *=  1.0 - uv.yx;
            float vig = uv.x*uv.y * 25.0;
            vig = pow(vig, 0.3);
            color *= vig;

            color_distance level_sdf = get_distance(sdf_level_primitives, vertex_input.uv, c_player_growth_factor, c_player_time);
            color = lerp(color, level_sdf.color.yzw, clamp(1.0 - level_sdf.distance, 0.0, 1.0));

            color_distance overlay_sdf = get_distance(sdf_overlay_primitives, vertex_input.uv, c_player_growth_factor, c_player_time);
            color = lerp(color, overlay_sdf.color.yzw, clamp(1.0 - overlay_sdf.distance, 0.0, 1.0) * overlay_sdf.color.x);
            
            /* Debug */
            /*
            float2 surface_normal = c_debug; //get_surface_normal(c_player_position);
            float distance_along_vec = dot(vertex_input.uv - c_player_position, surface_normal);
            float distance_to_vec2 = length(vertex_input.uv - c_player_position) * length(vertex_input.uv - c_player_position) - distance_along_vec * distance_along_vec;
            float surface_line = sign(distance_along_vec) * step(distance_to_vec2, 1.0);
            color = lerp(color, float3(0.0, 1.0, 1.0), surface_line);
            */

            float player = step(sdf_player(vertex_input.uv, c_player_position, c_player_scale, c_player_time), 0.0);
            color_distance face = sdf_box_textured(
                vertex_input.uv, 
                c_player_position, 
                float2(c_player_scale * (192.0/3.0), c_player_scale * (64.0/3.0)) * (1.0 + c_player_growth_state * 0.1), 
                float2(4.0, c_player_growth_state), 
                float2(192.0, 64.0), 
                float2(TEXTURE_SPRITESHEET_WIDTH, TEXTURE_SPRITESHEET_HEIGHT)
            );
            color = lerp(color, float3(0.0, 0.0, 0.0), player);
            color = lerp(color, face.color.yzw, step(face.distance, 0.0) * face.color.x);

            color_distance ui = player_hud(vertex_input.uv);
            color = lerp(color, ui.color.yzw, step(ui.distance, 0.0) * ui.color.x);

            float n = sample_noise(vertex_input.uv - float2(c_player_time, 0.0), c_player_time);
            //color -= n * 0.2;
            color = lerp(color, color * float3(n, n, n), 0.5);

            float dust = dust_particles(vertex_input.uv, c_player_time);
            color = lerp(color, color * float3(0.5, 0.5, 0.5), dust);

            color = lerp(color, float3(0.0, 0.0, 0.0), c_player_screen_fade);

            //color = lerp(color, float3(1.0,0.0,1.0), step(length(c_player_position-vertex_input.uv)-c_player_radius, 0.0));
            //color = 1.0 - exp( -color );
            return float4(color, 1.0);
        }
    );

    window_ctx* window;
    graphics_d3d11_ctx d3d11_ctx;
    audio_xaudio2_ctx* xaudio2_ctx;
    graphics_buffer vertex_buffer;
    graphics_image noise_image;
    graphics_image credits_image;
    graphics_image main_menu_image;
    graphics_image background_image;
    graphics_image end_image;
    graphics_image spritesheet_image;
    graphics_structured_buffer sdf_level_primitives_buffer;
    graphics_structured_buffer sdf_overlay_primitives_buffer;
    graphics_shader shader;
    graphics_pipeline pipeline;
    graphics_bindings bindings;

    camera_data camera;
    memzero(&camera, sizeof(camera));
    camera.render_size = fvec2{ RENDER_WIDTH, RENDER_HEIGHT };

    window = window_create(RENDER_WIDTH, RENDER_HEIGHT, "Growing Pains");
    d3d11_ctx = graphics_d3d11_init(window);
    xaudio2_ctx = audio_xaudio2_init();

    vertex_buffer = graphics_buffer_create(&d3d11_ctx, {
        vertices_square,
        sizeof(vertices_square)
    });

    noise_image = graphics_image_create(&d3d11_ctx, {
        g_texture_noise,
        TEXTURE_NOISE_WIDTH,
        TEXTURE_NOISE_HEIGHT,
        GRAPHICS_IMAGE_TYPE_R8G8B8A8_UINT,
        GRAPHICS_FILTER_TYPE_LINEAR,
        GRAPHICS_WARP_TYPE_REPEAT,
        GRAPHICS_WARP_TYPE_REPEAT
    });

    credits_image = graphics_image_create(&d3d11_ctx, {
        g_texture_credits,
        TEXTURE_CREDITS_WIDTH,
        TEXTURE_CREDITS_HEIGHT,
        GRAPHICS_IMAGE_TYPE_R8G8B8A8_UINT,
        GRAPHICS_FILTER_TYPE_LINEAR,
        GRAPHICS_WARP_TYPE_REPEAT,
        GRAPHICS_WARP_TYPE_REPEAT
    });

    main_menu_image = graphics_image_create(&d3d11_ctx, {
        g_texture_main_menu,
        TEXTURE_MAIN_MENU_WIDTH,
        TEXTURE_MAIN_MENU_HEIGHT,
        GRAPHICS_IMAGE_TYPE_R8G8B8A8_UINT,
        GRAPHICS_FILTER_TYPE_LINEAR,
        GRAPHICS_WARP_TYPE_REPEAT,
        GRAPHICS_WARP_TYPE_REPEAT
    });

    background_image = graphics_image_create(&d3d11_ctx, {
        g_texture_background,
        TEXTURE_BACKGROUND_WIDTH,
        TEXTURE_BACKGROUND_HEIGHT,
        GRAPHICS_IMAGE_TYPE_R8G8B8A8_UINT,
        GRAPHICS_FILTER_TYPE_LINEAR,
        GRAPHICS_WARP_TYPE_REPEAT,
        GRAPHICS_WARP_TYPE_REPEAT
    });

    end_image = graphics_image_create(&d3d11_ctx, {
        g_texture_end,
        TEXTURE_END_WIDTH,
        TEXTURE_END_HEIGHT,
        GRAPHICS_IMAGE_TYPE_R8G8B8A8_UINT,
        GRAPHICS_FILTER_TYPE_LINEAR,
        GRAPHICS_WARP_TYPE_REPEAT,
        GRAPHICS_WARP_TYPE_REPEAT
    });

    spritesheet_image = graphics_image_create(&d3d11_ctx, {
        g_texture_spritesheet,
        TEXTURE_SPRITESHEET_WIDTH,
        TEXTURE_SPRITESHEET_HEIGHT,
        GRAPHICS_IMAGE_TYPE_R8G8B8A8_UINT,
        GRAPHICS_FILTER_TYPE_LINEAR,
        GRAPHICS_WARP_TYPE_REPEAT,
        GRAPHICS_WARP_TYPE_REPEAT
    });

    memzero(g_level_primitives, sizeof(g_level_primitives));
    sdf_level_primitives_buffer = graphics_structured_buffer_create(&d3d11_ctx, {
            g_level_primitives,
            sizeof(sdf_primitive),
            SDF_PRIMITIVES_COUNT_MAX
    });

    memzero(g_overlay_primitives, sizeof(g_overlay_primitives));
    sdf_overlay_primitives_buffer = graphics_structured_buffer_create(&d3d11_ctx, {
            g_overlay_primitives,
            sizeof(sdf_primitive),
            SDF_PRIMITIVES_COUNT_MAX
    });

    shader = graphics_shader_create(&d3d11_ctx, {
        { sizeof(camera) },
        "vs_5_0",
        "main",
        vertex_shader_src,

        { sizeof(player_data) },
        "ps_5_0",
        "main",
        fragment_shader_src,
    });

    pipeline = graphics_pipeline_create(&d3d11_ctx, {
        shader,
        GRAPHICS_VERTEX_TOPOLOGY_TRIANGLE_STRIP,
        {
            { "POSITION", GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3 },
            { "UV", GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2 },
            { "COLOR", GRAPHICS_VERTEX_ATTRIBUTE_FLOAT4 }
        },
        0
    });

    g_background_type = BACKGROUND_TYPE_LEVEL;
    bindings = graphics_bindings_create(&d3d11_ctx, {
        pipeline,
        { noise_image, background_image, spritesheet_image },
        { sdf_level_primitives_buffer, sdf_overlay_primitives_buffer },
        { vertex_buffer }
    });

    /* -------------------------------------------------- */

    audio_play_sound(xaudio2_ctx, g_sound_music, sizeof(g_sound_music), 1.0f, AUDIO_FLAG_FADE_IN | AUDIO_FLAG_LOOP); 
    audio_play_sound(xaudio2_ctx, g_sound_heartbeat, sizeof(g_sound_heartbeat), 0.75f, AUDIO_FLAG_FADE_IN | AUDIO_FLAG_LOOP); 
    level_load(0);

    u32 level_edit_object = 0;
    u32 level_edit_entity = 0;
    f32 level_edit_move_speed = 10.0f;
    f32 level_edit_size_speed = 10.0f;
    (void) level_edit_move_speed;
    (void) level_edit_size_speed;


    /* -------------------------------------------------- */

    f32 move_speed = 5000.0f;
    f32 max_velocity = 900.0f;
    f32 damping = 0.2f;
    f32 damping_time = 0.1f;
    f32 push_strength = 4.0f;
    f32 growth_speed = 0.3f;
    f32 growth_time = 0.1f;
    sz player_death_particle = ENTITIES_COUNT_MAX;

    fvec2 velocity = { 0.0f, 0.0f };

    while (!window->should_close)
    {
        f32 delta_time = (f32)window->delta_time;
        level_update(window, delta_time);

        /* Player movement physics. */
        fvec2 acceleration = { 0.0f, 0.0f };
        if (g_player.enable_input && (window_key_down(window, KEY_W) || window_key_down(window, KEY_UP)))   
            { acceleration.y = -move_speed; }
        if (g_player.enable_input && (window_key_down(window, KEY_S) || window_key_down(window, KEY_DOWN))) 
            { acceleration.y = move_speed; }
        if (g_player.enable_input && (window_key_down(window, KEY_A) || window_key_down(window, KEY_LEFT))) 
            { acceleration.x = -move_speed; }
        if (g_player.enable_input && (window_key_down(window, KEY_D) || window_key_down(window, KEY_RIGHT))) 
            { acceleration.x = move_speed; }
        if (fvec2_len(acceleration) > 0.0f)
        {
            /* Instantly reset players velocity when receiving input opposite to our current direction to make turning feel snappy. */
            if (acceleration.x > 0.0f && velocity.x < 0.0f) velocity.x = 0.0f;
            if (acceleration.x < 0.0f && velocity.x > 0.0f) velocity.x = 0.0f;
            if (acceleration.y > 0.0f && velocity.y < 0.0f) velocity.y = 0.0f;
            if (acceleration.y < 0.0f && velocity.y > 0.0f) velocity.y = 0.0f;
            acceleration = fvec2_mul_s(fvec2_norm(acceleration), move_speed);
        }

        /* If there is no input, simulate friction to slowly stop the player. */
        if (acceleration.x == 0.0f)
        {
            velocity.x = f32_lerp(velocity.x, 0.0f, f32_lerp_delta_time(delta_time, damping, damping_time));
        }
        if (acceleration.y == 0.0f)
        {
            velocity.y = f32_lerp(velocity.y, 0.0f, f32_lerp_delta_time(delta_time, damping, damping_time));
        }

        /* Apply acceleration and clamp it. 
         * TODO: our move speed is probably high enough we can drop acceleration all together. */
        velocity = fvec2_add(velocity, fvec2_mul_s(acceleration, delta_time));
        if (fvec2_len(velocity) > max_velocity)
        {
            velocity = fvec2_mul_s(fvec2_norm(velocity), max_velocity);
        }

        fvec2 new_position = fvec2_add(g_player.position, fvec2_mul_s(velocity, delta_time));

        /* Collision against distance field. */
        sdf_result collision_result = sdf_get_distance(new_position, g_player.growth_factor, g_player.time);
        g_player.debug = sdf_get_surface_normal(new_position, g_player.growth_factor, g_player.time);
        if (collision_result.distance < 0.1f)
        {
            /* Reflect  velocity along surface normal to allow gliding against objects. */
            fvec2 normal = sdf_get_surface_normal(new_position, g_player.growth_factor, g_player.time);
            f32 along_normal = fvec2_dot(velocity, normal);
            if (along_normal < 0.0f)
            {
                velocity = fvec2_sub(velocity, fvec2_mul_s(normal, along_normal));
            }

            new_position = g_player.position;
            new_position = fvec2_add(new_position, fvec2_mul_s(velocity, delta_time));
        }

        /* Iteratively check if our resolved position is still inside an object in the distance field. 
         * If so, slowly push the player out along the surface normal. */
        for (u32 i = 0; i < 4; ++i) 
        {
            sdf_result new_position_collision = sdf_get_distance(new_position, g_player.growth_factor, g_player.time);
            if (new_position_collision.distance < 0.1f)
            {
                fvec2 normal = sdf_get_surface_normal(new_position, g_player.growth_factor, g_player.time);
                new_position = fvec2_add(new_position, fvec2_mul_s(normal, push_strength *  delta_time * -new_position_collision.distance));
                /* TODO: This is most certainly npt correctly time adjusted... Too bad! */
            }
        }

        /* Collision against level bounds. */
        new_position.x = math_clamp(new_position.x, 0.0f, LEVEL_WIDTH);
        new_position.y = math_clamp(new_position.y, 0.0f, LEVEL_HEIGHT);

        g_player.position = new_position;

        /* Post physics collision processing. */
        if (collision_result.distance < 0.1f && collision_result.closest_object != SDF_RESULT_OBJECT_INVALID)
        {
            sdf_primitive* primitive = &g_level_primitives[collision_result.closest_object];
            entity_data* entity = &g_entities[primitive->entity];
            switch (entity->type)
            {
                case ENTITY_TYPE_T_CELL:
                {
                    if (!g_player.is_dead)
                    {
                        g_player.enable_input = FALSE;
                        g_player.is_dead = TRUE;
                        g_player.deaths += 1;
                        if (g_player.deaths > 99)
                        {
                            g_player.deaths = 99; /* Sorry but you just suck too much... */
                        }

                        /* Death effect. */
                        audio_play_sound(xaudio2_ctx, g_sound_death, sizeof(g_sound_death), 1.0f, AUDIO_FLAG_NONE);
                        g_entities[g_entities_count].type = ENTITY_TYPE_PARTICLES; 
                        g_entities[g_entities_count].position = g_player.position; 
                        g_entities[g_entities_count].growth_sizes1.x = 0.0f; 
                        player_death_particle = g_entities_count;
                        g_entities_count += 1;
                    }
                } break;
            }
        }
        if (collision_result.overlapped_distance < 0.1f && collision_result.overlapped_object != SDF_RESULT_OBJECT_INVALID)
        {
            sdf_primitive* primitive = &g_level_primitives[collision_result.overlapped_object];
            entity_data* entity = &g_entities[primitive->entity];
            switch (entity->type)
            {
                case ENTITY_TYPE_PORTAL:
                {
                    level_next();
                } break;
                case ENTITY_TYPE_MAGGOT:
                {
                    g_player.maggots += 1;
                    entity->type = ENTITY_TYPE_PARTICLES;
                    entity->growth_sizes1.z = 0.0f;
                    audio_play_sound(xaudio2_ctx, g_sound_maggot, sizeof(g_sound_maggot), 1.0f, AUDIO_FLAG_NONE);
                } break;
            }
        }

        /* Player death update. */
        if (g_player.is_dead)
        {
            g_player.scale -= delta_time * 4.0f;
            if (g_player.scale <= 0.0f)
            {
                g_player.scale = 0.0f;
            }
            if (g_entities[player_death_particle].growth_sizes1.z >= 1.0f)
            {
                level_reload();
            }
        }

        /* Player growth mechanic. 
         * This is just incrementing and decrementing growth factor but with wrap around. */
        if (g_player.enable_input && window_key_pressed(window, KEY_E)) 
        { 
            g_player.growth_state = (g_player.growth_state + 2) % 3; 
        }
        if (g_player.enable_input && window_key_pressed(window, KEY_Q)) 
        { 
            g_player.growth_state = (g_player.growth_state + 1) % 3; 
        }

        /* Smoothly interpolate between growth states along a circle.
         * E.g. pretend each growth state is a point on a circle going from 0 to 1. 
         * Now find the shortest path to smoothly interpolate between these points. 
         * This solves the issue interpolating from 1 to 0, where a naive implementation 
         * would have to pass through the centre value. Doing so would cause weird effects 
         * when 0 and 1 represent similar values but 0.5 does not.
         */
        f32 growth_factor_distance = ((f32)g_player.growth_state / 3.0f) - g_player.growth_factor;
        growth_factor_distance = f32_mod((f32_mod(growth_factor_distance, 1.0f) + 1.5f), 1.0f) - 0.5f;
        g_player.growth_factor = f32_lerp(
            g_player.growth_factor, 
            g_player.growth_factor + growth_factor_distance, 
            f32_lerp_delta_time(delta_time, growth_speed, growth_time)
        );

        g_player.growth_factor = f32_mod(g_player.growth_factor, 1.0f);
        if (g_player.growth_factor < 0) 
        {
            g_player.growth_factor = 1.0f + g_player.growth_factor;
        }

        /* Convert entities into primitives we need to render. 
         * This provides a layer of separation between entities and their visuals. 
         * Entities can have 1 or more visual elements or be created and destroyed. 
         * Recreating the list each frame is however not the best way to do this... */
        u32 g_level_primitives_end = 0;
        u32 g_overlay_primitives_end = 0;
        memzero(&g_level_primitives, sizeof(g_level_primitives));
        memzero(&g_overlay_primitives, sizeof(g_overlay_primitives));
        for (u32 i = 0; i < ENTITIES_COUNT_MAX; ++i)
        {
            if (level_edit_entity == i)
            {
                level_edit_object = g_level_primitives_end;
            }

            entity_data* entity = &g_entities[i];
            if (entity->type == ENTITY_TYPE_INVALID)
            {
                break;
            }

            switch (entity->type)
            {
                case ENTITY_TYPE_TUMOR: 
                {
                    /* Create body */
                    g_level_primitives[g_level_primitives_end].type = SDF_PRIMITIVE_CIRCLE;
                    g_level_primitives[g_level_primitives_end].entity = i;
                    g_level_primitives[g_level_primitives_end].position = entity->position;
                    g_level_primitives[g_level_primitives_end].growth_sizes1 = entity->growth_sizes1;
                    ++g_level_primitives_end;

                    /* Calculate face size and position to look at the player without leaving the body sphere. */
                    f32 face_width = lerp_growth_factors(entity->growth_sizes1, g_player.growth_factor) / 2.0f;
                    f32 face_height = face_width * (16.0f / 48.0f);

                    fvec2 face_position = entity->position;
                    fvec2 player_direction = fvec2_sub(g_player.position, face_position);
                    player_direction = fvec2_mul_s(fvec2_norm(player_direction), math_min(fvec2_len(player_direction) / 4.0f, face_width));
                    face_position = fvec2_add(face_position, player_direction);

                    /* Create face */
                    g_overlay_primitives[g_overlay_primitives_end].type = SDF_PRIMITIVE_BOX_TEXTURED;
                    g_overlay_primitives[g_overlay_primitives_end].entity = i;
                    g_overlay_primitives[g_overlay_primitives_end].position = face_position;
                    g_overlay_primitives[g_overlay_primitives_end].growth_sizes1 = { face_width, face_width, face_width };
                    g_overlay_primitives[g_overlay_primitives_end].growth_sizes2 = { face_height, face_height, face_height };
                    g_overlay_primitives[g_overlay_primitives_end].sprite_index[0] = entity->sprite_index[0];
                    g_overlay_primitives[g_overlay_primitives_end].sprite_index[1] = entity->sprite_index[1];
                    ++g_overlay_primitives_end;
                } break;
                case ENTITY_TYPE_MOTHER:
                {
                    f32 mouth_state_time = 0.5f;
                    b8 mouth_state = (entity->timer < mouth_state_time);
                    if (entity->is_talking || mouth_state)
                    {
                        entity->timer += delta_time;
                        if (entity->timer >= mouth_state_time * 2.0f)
                        {
                            entity->timer = 0.0f;
                        }
                    }

                    /* Create body */
                    g_level_primitives[g_level_primitives_end].type = SDF_PRIMITIVE_CIRCLE;
                    g_level_primitives[g_level_primitives_end].entity = i;
                    g_level_primitives[g_level_primitives_end].position = entity->position;
                    g_level_primitives[g_level_primitives_end].growth_sizes1 = entity->growth_sizes1;
                    ++g_level_primitives_end;

                    /* Calculate face size and position to look at the player without leaving the body sphere. */
                    f32 face_width = lerp_growth_factors(entity->growth_sizes1, g_player.growth_factor) / 2.0f;
                    f32 face_height = face_width * (16.0f / 48.0f);

                    fvec2 face_position = fvec2_add(entity->position, {0.0f, 150.0f});
                    fvec2 player_direction = fvec2_sub(g_player.position, face_position);
                    player_direction = fvec2_mul_s(fvec2_norm(player_direction), math_min(fvec2_len(player_direction) / 4.0f, face_width));
                    face_position = fvec2_add(face_position, player_direction);

                    /* Special clamping rules to prevent messy overlap with flower. */
                    face_position.x = math_clamp(
                        face_position.x, 
                        entity->position.x - face_width / 2.0f, 
                        entity->position.x + face_width / 4.0f
                    );
                    face_position.y = math_max(
                        face_position.y, 
                        entity->position.y + face_height / 4.0f
                    );

                    /* Create flower */
                    fvec2 flower_position = fvec2_add(entity->position, {280.0f, 180.0f});
                    f32 flower_width = lerp_growth_factors(entity->growth_sizes1, g_player.growth_factor) / 8.0f;
                    f32 flower_height = flower_width;

                    g_overlay_primitives[g_overlay_primitives_end].type = SDF_PRIMITIVE_BOX_FLOWER;
                    g_overlay_primitives[g_overlay_primitives_end].entity = i;
                    g_overlay_primitives[g_overlay_primitives_end].position = flower_position;
                    g_overlay_primitives[g_overlay_primitives_end].growth_sizes1 = { flower_width, flower_width, flower_width };
                    g_overlay_primitives[g_overlay_primitives_end].growth_sizes2 = { flower_height, flower_height, flower_height };
                    g_overlay_primitives[g_overlay_primitives_end].sprite_index[0] = 8.0f;
                    g_overlay_primitives[g_overlay_primitives_end].sprite_index[1] = 1.0f;
                    ++g_overlay_primitives_end;

                    /* Create face */
                    g_overlay_primitives[g_overlay_primitives_end].type = SDF_PRIMITIVE_BOX_TEXTURED;
                    g_overlay_primitives[g_overlay_primitives_end].entity = i;
                    g_overlay_primitives[g_overlay_primitives_end].position = face_position;
                    g_overlay_primitives[g_overlay_primitives_end].growth_sizes1 = { face_width, face_width, face_width };
                    g_overlay_primitives[g_overlay_primitives_end].growth_sizes2 = { face_height, face_height, face_height };
                    g_overlay_primitives[g_overlay_primitives_end].sprite_index[0] = 2;
                    g_overlay_primitives[g_overlay_primitives_end].sprite_index[1] = 2 + (f32)mouth_state;
                    ++g_overlay_primitives_end;
                } break;
                case ENTITY_TYPE_T_CELL: 
                {
                    /* Create body */
                    g_level_primitives[g_level_primitives_end].type = SDF_PRIMITIVE_SPIKED_CIRCLE;
                    g_level_primitives[g_level_primitives_end].entity = i;
                    g_level_primitives[g_level_primitives_end].position = entity->position;
                    g_level_primitives[g_level_primitives_end].growth_sizes1 = entity->growth_sizes1;
                    ++g_level_primitives_end;
                } break;
                case ENTITY_TYPE_WALL:
                {
                    g_level_primitives[g_level_primitives_end].type = SDF_PRIMITIVE_BOX;
                    g_level_primitives[g_level_primitives_end].entity = i;
                    g_level_primitives[g_level_primitives_end].position = entity->position;
                    g_level_primitives[g_level_primitives_end].growth_sizes1 = entity->growth_sizes1;
                    g_level_primitives[g_level_primitives_end].growth_sizes2 = entity->growth_sizes2;
                    ++g_level_primitives_end;
                } break;
                case ENTITY_TYPE_PORTAL:
                {
                    g_level_primitives[g_level_primitives_end].type = SDF_PRIMITIVE_PORTAL;
                    g_level_primitives[g_level_primitives_end].entity = i;
                    g_level_primitives[g_level_primitives_end].position = entity->position;
                    g_level_primitives[g_level_primitives_end].growth_sizes1 = entity->growth_sizes1;
                    ++g_level_primitives_end;
                } break;
                case ENTITY_TYPE_MAGGOT:
                {
                    g_level_primitives[g_level_primitives_end].type = SDF_PRIMITIVE_MAGGOT;
                    g_level_primitives[g_level_primitives_end].entity = i;
                    g_level_primitives[g_level_primitives_end].position = entity->position;
                    g_level_primitives[g_level_primitives_end].growth_sizes1 = entity->growth_sizes1;
                    ++g_level_primitives_end;
                } break;
                case ENTITY_TYPE_PARTICLES:
                {
                    entity->growth_sizes1.z += delta_time * 2.0f;
                    if (entity->growth_sizes1.z >= 1.0f)
                    {
                        entity->growth_sizes1.z = 1.0f;
                    }

                    g_overlay_primitives[g_overlay_primitives_end].type = SDF_PRIMITIVE_PARTICLES;
                    g_overlay_primitives[g_overlay_primitives_end].entity = i;
                    g_overlay_primitives[g_overlay_primitives_end].position = entity->position;
                    g_overlay_primitives[g_overlay_primitives_end].growth_sizes1 = entity->growth_sizes1;
                    ++g_overlay_primitives_end;
                } break;
                case ENTITY_TYPE_BOX_TEXT:
                {
                    if (entity->should_grow)
                    {
                        entity->growth_sizes1.z += delta_time * 2.0f;
                        if (entity->growth_sizes1.z >= 1.0f)
                        {
                            entity->growth_sizes1.z = 1.0f;
                        }
                    }
                    else 
                    {
                        entity->growth_sizes1.z -= delta_time * 2.0f;
                        if (entity->growth_sizes1.z <= 0.0f)
                        {
                            entity->growth_sizes1.z = 0.0f;
                        }
                    }

                    g_overlay_primitives[g_overlay_primitives_end].type = SDF_PRIMITIVE_BOX_TEXT;
                    g_overlay_primitives[g_overlay_primitives_end].entity = i;
                    g_overlay_primitives[g_overlay_primitives_end].position = entity->position;
                    g_overlay_primitives[g_overlay_primitives_end].growth_sizes1 = entity->growth_sizes1;
                    g_overlay_primitives[g_overlay_primitives_end].growth_sizes2 = entity->growth_sizes2;
                    g_overlay_primitives[g_overlay_primitives_end].sprite_index[0] = entity->sprite_index[0];
                    g_overlay_primitives[g_overlay_primitives_end].sprite_index[1] = entity->sprite_index[1];
                    ++g_overlay_primitives_end;
                } break;
            }

        }

        /* Set background image logic. */
        switch (g_background_type)
        {
            case BACKGROUND_TYPE_CREDITS: bindings.images[1] = credits_image; break;
            case BACKGROUND_TYPE_MAIN_MENU: bindings.images[1] = main_menu_image; break;
            case BACKGROUND_TYPE_LEVEL: bindings.images[1] = background_image; break;
            case BACKGROUND_TYPE_END: bindings.images[1] = end_image; break;
        }


        /* -------------------------------------------------- */

        
        /* Level editing. */
        /*
        if (window_key_down(window, KEY_CONTROL_LEFT))
        {
            if (window_key_pressed(window, KEY_H)) g_entities[level_edit_entity].position.x -= level_edit_move_speed;
            if (window_key_pressed(window, KEY_J)) g_entities[level_edit_entity].position.y += level_edit_move_speed;
            if (window_key_pressed(window, KEY_K)) g_entities[level_edit_entity].position.y -= level_edit_move_speed;
            if (window_key_pressed(window, KEY_L)) g_entities[level_edit_entity].position.x += level_edit_move_speed;

            if (window_key_pressed(window, KEY_1)) g_entities[level_edit_entity].growth_sizes1.x -= level_edit_size_speed;
            if (window_key_pressed(window, KEY_2)) g_entities[level_edit_entity].growth_sizes1.x += level_edit_size_speed;
            if (window_key_pressed(window, KEY_5)) g_entities[level_edit_entity].growth_sizes1.y -= level_edit_size_speed;
            if (window_key_pressed(window, KEY_6)) g_entities[level_edit_entity].growth_sizes1.y += level_edit_size_speed;
            if (window_key_pressed(window, KEY_9)) g_entities[level_edit_entity].growth_sizes1.z -= level_edit_size_speed;
            if (window_key_pressed(window, KEY_0)) g_entities[level_edit_entity].growth_sizes1.z += level_edit_size_speed;

            if (window_key_down(window, KEY_SHIFT_LEFT))
            {
                if (window_key_pressed(window, KEY_1)) g_entities[level_edit_entity].growth_sizes2.x -= level_edit_size_speed;
                if (window_key_pressed(window, KEY_2)) g_entities[level_edit_entity].growth_sizes2.x += level_edit_size_speed;
                if (window_key_pressed(window, KEY_5)) g_entities[level_edit_entity].growth_sizes2.y -= level_edit_size_speed;
                if (window_key_pressed(window, KEY_6)) g_entities[level_edit_entity].growth_sizes2.y += level_edit_size_speed;
                if (window_key_pressed(window, KEY_9)) g_entities[level_edit_entity].growth_sizes2.z -= level_edit_size_speed;
                if (window_key_pressed(window, KEY_0)) g_entities[level_edit_entity].growth_sizes2.z += level_edit_size_speed;
            }
        }
        else
        {
            if (window_key_down(window, KEY_H)) g_entities[level_edit_entity].position.x -= level_edit_move_speed;
            if (window_key_down(window, KEY_J)) g_entities[level_edit_entity].position.y += level_edit_move_speed;
            if (window_key_down(window, KEY_K)) g_entities[level_edit_entity].position.y -= level_edit_move_speed;
            if (window_key_down(window, KEY_L)) g_entities[level_edit_entity].position.x += level_edit_move_speed;

            if (window_key_down(window, KEY_1)) g_entities[level_edit_entity].growth_sizes1.x -= level_edit_size_speed;
            if (window_key_down(window, KEY_2)) g_entities[level_edit_entity].growth_sizes1.x += level_edit_size_speed;
            if (window_key_down(window, KEY_5)) g_entities[level_edit_entity].growth_sizes1.y -= level_edit_size_speed;
            if (window_key_down(window, KEY_6)) g_entities[level_edit_entity].growth_sizes1.y += level_edit_size_speed;
            if (window_key_down(window, KEY_9)) g_entities[level_edit_entity].growth_sizes1.z -= level_edit_size_speed;
            if (window_key_down(window, KEY_0)) g_entities[level_edit_entity].growth_sizes1.z += level_edit_size_speed;

            if (window_key_down(window, KEY_SHIFT_LEFT))
            {
                if (window_key_down(window, KEY_1)) g_entities[level_edit_entity].growth_sizes2.x -= level_edit_size_speed;
                if (window_key_down(window, KEY_2)) g_entities[level_edit_entity].growth_sizes2.x += level_edit_size_speed;
                if (window_key_down(window, KEY_5)) g_entities[level_edit_entity].growth_sizes2.y -= level_edit_size_speed;
                if (window_key_down(window, KEY_6)) g_entities[level_edit_entity].growth_sizes2.y += level_edit_size_speed;
                if (window_key_down(window, KEY_9)) g_entities[level_edit_entity].growth_sizes2.z -= level_edit_size_speed;
                if (window_key_down(window, KEY_0)) g_entities[level_edit_entity].growth_sizes2.z += level_edit_size_speed;
            }
        }

        if (window_key_pressed(window, KEY_T)) 
        {
            u32 type = (u32)g_entities[level_edit_entity].type;
            type = (type + 1) % _ENTITY_TYPE_COUNT;
            g_entities[level_edit_entity].type = (entity_type)type;
        }

        if (window_key_pressed(window, KEY_I) && level_edit_entity < ENTITIES_COUNT_MAX) level_edit_entity +=1;
        if (window_key_pressed(window, KEY_O) && level_edit_entity > 0) level_edit_entity -=1;


        printf("Edit object %u, Edit growth state %u\n", level_edit_object, g_player.growth_state);
        if (window_key_pressed(window, KEY_ENTER))
        {
            printf("SAVING...\n");
            level_print();
            printf("SAVING END\n");
        }

        g_player.level_edit_object = level_edit_object;
        */

        /* -------------------------------------------------- */

        graphics_structured_buffer_update(&sdf_level_primitives_buffer, g_level_primitives, sizeof(g_level_primitives));
        graphics_structured_buffer_update(&sdf_overlay_primitives_buffer, g_overlay_primitives, sizeof(g_overlay_primitives));

        camera.model_view_projection = graphics_calc_image_projection(window, RENDER_WIDTH, RENDER_HEIGHT);
        camera.model_view_projection = fmat44_transpose(camera.model_view_projection);
        g_player.time += (f32)window->delta_time;

        graphics_pass_begin(&d3d11_ctx, window->width, window->height);
        graphics_bindings_set(&bindings);
        graphics_constant_buffer_update(&pipeline, GRAPHICS_SHADER_STAGE_VERTEX, 0, &camera);
        graphics_constant_buffer_update(&pipeline, GRAPHICS_SHADER_STAGE_FRAGMENT, 0, &g_player);
        graphics_pass_end(&d3d11_ctx);

        audio_push_sounds(xaudio2_ctx, (f32_t)window->delta_time);
        window_poll_events(window);
        graphics_print_messages(&d3d11_ctx);
        if (window_key_down(window, KEY_F))
        {
            printf("MS: %f, FPS: %f\n", 1.0 / window->delta_time, window->delta_time);
        }
    }

    graphics_pipeline_destroy(&pipeline);

    graphics_shader_destroy(&shader);
    graphics_structured_buffer_destory(&sdf_level_primitives_buffer);
    graphics_structured_buffer_destory(&sdf_overlay_primitives_buffer);
    graphics_image_destroy(&noise_image);
    graphics_image_destroy(&credits_image);
    graphics_image_destroy(&main_menu_image);
    graphics_image_destroy(&background_image);
    graphics_image_destroy(&end_image);
    graphics_image_destroy(&spritesheet_image);
    graphics_buffer_destroy(&vertex_buffer);

    audio_xaudio2_destory(xaudio2_ctx);
    graphics_d3d11_destroy(&d3d11_ctx);
    window_destroy(window);
    return 0;
}

//--------------------------------------------------

window_ctx* window_create(u32_t i_width, u32_t i_height, char const* i_title)
{
    HMODULE module_handle = NULL;
    WNDCLASS window_class = {};
    RECT window_rect = {};

    window_ctx* window = malloc_type(window_ctx);
    memzero(window, sizeof(*window));

    /* Grab the application handle so the user doesn't have to pass it to us. */
    if (!GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCTSTR)window_create,
        &module_handle
    ))
    {
        assert(0); /* TODO: Handle error. */
        return window;
    }

    window->width = i_width;
    window->height = i_height;
    if (i_title == NULL || i_title[0] == '\0')
    {
        window->title = "Window";
    }
    else
    {
        window->title = i_title;
    }
    window->style = WS_OVERLAPPEDWINDOW;

    memzero(&window_class, sizeof(window_class));
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = module_handle;
    window_class.lpszClassName = window->title;
    window->window_class_atom = RegisterClass(&window_class);

    window_rect.left = 0;
    window_rect.top = 0;
    window_rect.right = (LONG)window->width;
    window_rect.bottom = (LONG)window->height;
    AdjustWindowRect(&window_rect, window->style, FALSE);

    window->window_handle = CreateWindow(
        MAKEINTATOM(window->window_class_atom), /* window class name */
        window->title,                          /* title of the created window */
        window->style,                          /* window style */
        CW_USEDEFAULT,                          /* x-pos */
        CW_USEDEFAULT,                          /* y-pos */
        window_rect.right - window_rect.left,   /* width */
        window_rect.bottom - window_rect.top,   /* height */
        NULL,                                   /* parent window */
        NULL,                                   /* window menu */
        module_handle,                          /* application associated with the window */
        NULL                                    /* additional data */
    );
    if (window->window_handle == NULL)
    {
        assert(0); /* TODO: Handle error. */
        return window;
    }

    SetPropA(window->window_handle, "graphics_window", window);
    ShowWindow(window->window_handle, SW_SHOWDEFAULT);
    window->should_close = FALSE;

    /* Initialise timing. */
    LARGE_INTEGER timer_frequency;
    QueryPerformanceFrequency(&timer_frequency);
    window->timer_frequency_ms = (f64_t)timer_frequency.QuadPart / 1000.0;
    window->target_fps = 60;

    /* Initialise input. */
	window->key_code_to_scan_code[_KEY_INVALID] = 0;

	window->key_code_to_scan_code[KEY_A] = 0x01E;
	window->key_code_to_scan_code[KEY_B] = 0x030;
	window->key_code_to_scan_code[KEY_C] = 0x02E;
	window->key_code_to_scan_code[KEY_D] = 0x020;
	window->key_code_to_scan_code[KEY_E] = 0x012;
	window->key_code_to_scan_code[KEY_F] = 0x021;
	window->key_code_to_scan_code[KEY_G] = 0x022;
	window->key_code_to_scan_code[KEY_H] = 0x023;
	window->key_code_to_scan_code[KEY_I] = 0x017;
	window->key_code_to_scan_code[KEY_J] = 0x024;
	window->key_code_to_scan_code[KEY_K] = 0x025;
	window->key_code_to_scan_code[KEY_L] = 0x026;
	window->key_code_to_scan_code[KEY_M] = 0x032;
	window->key_code_to_scan_code[KEY_N] = 0x031;
	window->key_code_to_scan_code[KEY_O] = 0x018;
	window->key_code_to_scan_code[KEY_P] = 0x019;
	window->key_code_to_scan_code[KEY_Q] = 0x010;
	window->key_code_to_scan_code[KEY_R] = 0x013;
	window->key_code_to_scan_code[KEY_S] = 0x01F;
	window->key_code_to_scan_code[KEY_T] = 0x014;
	window->key_code_to_scan_code[KEY_U] = 0x016;
	window->key_code_to_scan_code[KEY_V] = 0x02F;
	window->key_code_to_scan_code[KEY_W] = 0x011;
	window->key_code_to_scan_code[KEY_X] = 0x02D;
	window->key_code_to_scan_code[KEY_Y] = 0x015;
	window->key_code_to_scan_code[KEY_Z] = 0x02C;

	window->key_code_to_scan_code[KEY_0] = 0x00B;
	window->key_code_to_scan_code[KEY_1] = 0x002;
	window->key_code_to_scan_code[KEY_2] = 0x003;
	window->key_code_to_scan_code[KEY_3] = 0x004;
	window->key_code_to_scan_code[KEY_4] = 0x005;
	window->key_code_to_scan_code[KEY_5] = 0x006;
	window->key_code_to_scan_code[KEY_6] = 0x007;
	window->key_code_to_scan_code[KEY_7] = 0x008;
	window->key_code_to_scan_code[KEY_8] = 0x009;
	window->key_code_to_scan_code[KEY_9] = 0x00A;

	window->key_code_to_scan_code[KEY_SHIFT_LEFT] = 0x02A;
	window->key_code_to_scan_code[KEY_SHIFT_RIGHT] = 0x036;
	window->key_code_to_scan_code[KEY_CONTROL_LEFT] = 0x01D;
	window->key_code_to_scan_code[KEY_CONTROL_RIGHT] = 0x11D;
	window->key_code_to_scan_code[KEY_ALT_LEFT] = 0x038;
	window->key_code_to_scan_code[KEY_ALT_RIGHT] = 0x138;
	window->key_code_to_scan_code[KEY_SUPER_LEFT] = 0x15B;
	window->key_code_to_scan_code[KEY_SUPER_RIGHT] = 0x15C;

	window->key_code_to_scan_code[KEY_DOWN] = 0x150;
	window->key_code_to_scan_code[KEY_LEFT] = 0x14B;
	window->key_code_to_scan_code[KEY_RIGHT] = 0x14D;
	window->key_code_to_scan_code[KEY_UP] = 0x148;
	window->key_code_to_scan_code[KEY_BACKSPACE] = 0x00E;
	window->key_code_to_scan_code[KEY_DELETE] = 0x153;
	window->key_code_to_scan_code[KEY_ENTER] = 0x01C;
	window->key_code_to_scan_code[KEY_ESCAPE] = 0x001;
	window->key_code_to_scan_code[KEY_SPACE] = 0x039;
	window->key_code_to_scan_code[KEY_TAB] = 0x00F;

    for (u8_t key_code = 0; key_code < _KEY_COUNT; ++key_code)
    {
        u16_t scan_code = window->key_code_to_scan_code[key_code];
        window->scan_code_to_key_code[scan_code] = (key_codes)key_code;
    }

    return window;
}

void window_destroy(window_ctx* io_window)
{
    if (io_window != NULL)
    {
        free(io_window);
    }
}

LRESULT CALLBACK window_proc(HWND hWindow, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    window_ctx* window = (window_ctx*)GetPropA(hWindow, "graphics_window");
    switch (uMessage)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        } break;
        case WM_CLOSE:
        {
            DestroyWindow(hWindow);
            return 0;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT paint_struct;
            HDC hdc = BeginPaint(hWindow, &paint_struct);
            FillRect(hdc, &paint_struct.rcPaint, (HBRUSH)(COLOR_WINDOW+2));
            EndPaint(hWindow, &paint_struct);
        } break;

        /* Keyboard input handling. */
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            WORD key_flags = HIWORD(lParam);
            WORD scan_code = LOBYTE(key_flags);
            b8_t is_key_down = (key_flags & KF_UP) != KF_UP;
            scan_code = key_flags & (KF_EXTENDED | 0xff);

            if (scan_code >=512)
            {
                /* TODO: there are keys codes beyond this. Do we care? */
                break;
            }

            printf("Scancode: %u, Keycode: , Down: %s \n", scan_code, is_key_down ? "true" : "false");
            window->key_down[scan_code] = is_key_down;
            window->any_key_down = window->any_key_down || is_key_down;
        } break;
    }
    return DefWindowProc(hWindow, uMessage, wParam, lParam);
}

void window_poll_events(window_ctx* io_window)
{
    /* Update button state. */
    memcpy(io_window->key_down_prev, io_window->key_down, sizeof(io_window->key_down));
    io_window->any_key_down_prev = io_window->any_key_down;
    io_window->any_key_down = FALSE;

    /* Process event messages inside window_proc. */
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
        {
            io_window->should_close = TRUE;
        }
    }

    { /* Calculate frame time and wait if necessary. */
        u64_t timer_now = 0;
        f64_t time_since_last_frame_ms = 0.0;
        f64_t time_until_next_frame_ms = 0.0;

        QueryPerformanceCounter((LARGE_INTEGER*)&timer_now);
        time_since_last_frame_ms = (f64_t)(timer_now - io_window->timer_previous) / io_window->timer_frequency_ms;

        if (io_window->timer_previous != 0)
        {
            if (io_window->target_fps != 0)
            {
                time_until_next_frame_ms = (1000.0 / io_window->target_fps) - time_since_last_frame_ms;

                /* Leave 1 ms for busy waiting as sleep is inaccurate. */
                if (time_until_next_frame_ms > 1.0)
                {
                    Sleep((DWORD)(time_until_next_frame_ms - 1.0));
                }

                while (time_until_next_frame_ms > 0.0)
                {
                    QueryPerformanceCounter((LARGE_INTEGER*)&timer_now);
                    time_since_last_frame_ms = (f64_t)(timer_now - io_window->timer_previous) / io_window->timer_frequency_ms;
                    time_until_next_frame_ms = (1000.0 / io_window->target_fps) - time_since_last_frame_ms;
                }
            }

            io_window->delta_time = time_since_last_frame_ms / 1000.0;
        }
        io_window->timer_previous = timer_now;
    }
}

b8_t window_key_down(window_ctx* i_window, key_codes i_key)
{
    u16_t scan_code;
    assert(i_window);
    if (i_key == KEY_ANY)
    {
        return i_window->any_key_down;
    }
    else
    {
        scan_code = i_window->key_code_to_scan_code[i_key];
        return i_window->key_down[scan_code];
    }
}

b8_t window_key_up(window_ctx* i_window, key_codes i_key)
{
    u16_t scan_code;
    assert(i_window);
    if (i_key == KEY_ANY)
    {
        return !i_window->any_key_down;
    }
    else
    {
        scan_code = i_window->key_code_to_scan_code[i_key];
        return !i_window->key_down[scan_code];
    }
}

b8_t window_key_pressed(window_ctx* i_window, key_codes i_key)
{
    u16_t scan_code;
    assert(i_window);
    if (i_key == KEY_ANY)
    {
        return i_window->any_key_down && !i_window->any_key_down_prev;
    }
    else
    {
        scan_code = i_window->key_code_to_scan_code[i_key];
        return i_window->key_down[scan_code] && !i_window->key_down_prev[scan_code];
    }
}

b8_t window_key_released(window_ctx* i_window, key_codes i_key)
{
    if (i_key == KEY_ANY)
    {
        return !i_window->any_key_down && i_window->any_key_down_prev;
    }
    else
    {
        u16_t scan_code;
        assert(i_window);
        scan_code = i_window->key_code_to_scan_code[i_key];
        return !i_window->key_down[scan_code] && i_window->key_down_prev[scan_code];
    }
}

/* --------------------------------------------------
   DirectX11 graphics backend
   -------------------------------------------------- */

graphics_d3d11_ctx graphics_d3d11_init(window_ctx* i_window_ctx)
{
    HRESULT result;
    graphics_d3d11_ctx d3d11_ctx;
    memzero(&d3d11_ctx, sizeof(d3d11_ctx));

    /* Setup the swap chain for the window render target. */
    /* https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/bb173064(v=vs.85) */
    {
        UINT device_flags;
        DXGI_SWAP_CHAIN_DESC* swap_chain_desc = &d3d11_ctx.swap_chain_desc;
        swap_chain_desc->BufferDesc.Width = (UINT)i_window_ctx->width;
        swap_chain_desc->BufferDesc.Height = (UINT)i_window_ctx->height;
        swap_chain_desc->BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; /* 32 bit color */
        swap_chain_desc->SampleDesc.Count = 1;                           /* Multi sample anti-aliasing count */
        swap_chain_desc->SampleDesc.Quality = 0;                         /* Multi sample anti-aliasing quality */
        swap_chain_desc->BufferCount = 1;                                /* Back buffer count */
        swap_chain_desc->BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc->Windowed = TRUE;
        swap_chain_desc->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; /* Allow switching to fullscreen */
        swap_chain_desc->OutputWindow = i_window_ctx->window_handle;

        device_flags = 0;
        device_flags |= D3D11_CREATE_DEVICE_SINGLETHREADED;    /* Access only from 1 thread, disable thread safety */
        device_flags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;      /* Enable support for Direct2D */
        #ifdef DEBUG
            device_flags |= D3D11_CREATE_DEVICE_DEBUG;         /* Enable debugging */
        #endif

        result = D3D11CreateDeviceAndSwapChain(
            NULL,                           /* Default adaptor */
            D3D_DRIVER_TYPE_HARDWARE,       /* Driver */
            NULL,                           /* Software */
            device_flags,                   /* Flags */
            NULL,                           /* Feature levels pointer */
            0,                              /* Feature levels count */
            D3D11_SDK_VERSION,
            &d3d11_ctx.swap_chain_desc,
            &d3d11_ctx.swap_chain,
            &d3d11_ctx.device,
            NULL,                           /* Dont care about feature level used */
            &d3d11_ctx.device_context
        );
        if (!SUCCEEDED(result))
        {
            /* TODO: handle error */
            assert(0);
            return d3d11_ctx;
        }
    }

    /* Setup debug messaging. */
    #if DEBUG
    {
        result = d3d11_ctx.device->QueryInterface( __uuidof(ID3D11InfoQueue), (void**)&d3d11_ctx.debug_info_queue);
        if (!SUCCEEDED(result))
        {
            /* TODO: handle error */
            assert(0);
            return d3d11_ctx;
        }

        /* Do not filter any messages. */
        result = d3d11_ctx.debug_info_queue->PushEmptyStorageFilter();
        if (!SUCCEEDED(result))
        {
            /* TODO: handle error */
            assert(0);
            return d3d11_ctx;
        }

        /* Break on errors. */
        result = d3d11_ctx.debug_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        if (!SUCCEEDED(result))
        {
            /* TODO: handle error */
            assert(0);
            return d3d11_ctx;
        }
        result = d3d11_ctx.debug_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);
        if (!SUCCEEDED(result))
        {
            /* TODO: handle error */
            assert(0);
            return d3d11_ctx;
        }
    }
    #endif

    /* Create back buffer */
    /* https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-getbuffer */
    /* https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createrendertargetview */
    {
        ID3D11Device* device = d3d11_ctx.device;
        IDXGISwapChain* swap_chain = d3d11_ctx.swap_chain;

        result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&d3d11_ctx.render_target);
        if (!SUCCEEDED(result))
        {
            /* TODO: handle error */
            assert(0);
            return d3d11_ctx;
        }

        result = device->CreateRenderTargetView(d3d11_ctx.render_target, NULL, &d3d11_ctx.render_target_view);
        if (!SUCCEEDED(result))
        {
            /* TODO: handle error */
            assert(0);
            return d3d11_ctx;
        }
    }

    return d3d11_ctx;
}

void graphics_d3d11_destroy(graphics_d3d11_ctx* io_d3d11_ctx)
{
    if (io_d3d11_ctx->swap_chain != NULL)
    {
        /* D3D cannot shutdown in fullscreen mode. */
        io_d3d11_ctx->swap_chain->SetFullscreenState(FALSE, NULL);
        io_d3d11_ctx->swap_chain->Release();
    }
    if (io_d3d11_ctx->device != NULL)
    {
        io_d3d11_ctx->device->Release();
    }
    if (io_d3d11_ctx->device_context != NULL)
    {
        io_d3d11_ctx->device_context->Release();
    }
    if (io_d3d11_ctx->render_target != NULL)
    {
        io_d3d11_ctx->render_target->Release();
    }
    if (io_d3d11_ctx->render_target_view != NULL)
    {
        io_d3d11_ctx->render_target_view->Release();
    }
    memzero(io_d3d11_ctx, sizeof(*io_d3d11_ctx));
}

void graphics_print_messages(graphics_d3d11_ctx* i_d3d11_ctx)
{
    UINT64 message_count = i_d3d11_ctx->debug_info_queue->GetNumStoredMessages();
    for( UINT64 i = 0; i < message_count; i++)
    {
        SIZE_T message_size = 0;
        D3D11_MESSAGE* message = NULL;

        i_d3d11_ctx->debug_info_queue->GetMessage(i, nullptr, &message_size);
        message = (D3D11_MESSAGE*)malloc(message_size);

        HRESULT result;
        result = i_d3d11_ctx->debug_info_queue->GetMessage(i, message, &message_size);
        if (SUCCEEDED(result))
        {
            char buffer[512];
            sprintf(buffer, "Directx11: %.*s\n", (int)message->DescriptionByteLength, message->pDescription);
            OutputDebugStringA(buffer);
        }
        else
        {
            OutputDebugStringA("Directx11: FAILED TO GET MESSAGE\n");
        }

        free(message);
    }
    i_d3d11_ctx->debug_info_queue->ClearStoredMessages();
}

graphics_buffer graphics_buffer_create(graphics_d3d11_ctx* i_d3d11_ctx, graphics_buffer_desc i_desc)
{
    HRESULT result;
    D3D11_BUFFER_DESC buffer_desc;
    D3D11_SUBRESOURCE_DATA buffer_data;
    D3D11_SUBRESOURCE_DATA* buffer_data_ptr = NULL;

    graphics_buffer buffer;
    memzero(&buffer, sizeof(buffer));

    /* https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_buffer_desc */
    memzero(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage = D3D11_USAGE_DYNAMIC;                /* Write access from CPU, read access GPU */
    buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;       /* Use as vertex buffer */
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;    /* CPU write access */
    buffer_desc.ByteWidth = (UINT)i_desc.size;              /* Total buffer size */

    /* Can reserve a buffer without setting it's data. */
    if (i_desc.data != NULL)
    {
        buffer_data.pSysMem = i_desc.data;
        buffer_data_ptr = &buffer_data;
    }

    result = i_d3d11_ctx->device->CreateBuffer(&buffer_desc, buffer_data_ptr, &buffer.buffer);
    if (!SUCCEEDED(result))
    {
        /* TODO: handle error */
        assert(0);
        return buffer;
    }

    buffer.context = *i_d3d11_ctx;
    buffer.size = i_desc.size;
    return buffer;
}

void graphics_buffer_destroy(graphics_buffer* io_buffer)
{
    if (io_buffer->buffer != NULL)
    {
        io_buffer->buffer->Release();
    }
    memzero(io_buffer, sizeof(*io_buffer));
}

void graphics_buffer_update(graphics_buffer* i_buffer, void* i_data, size_t i_size)
{
    HRESULT result;
    D3D11_MAPPED_SUBRESOURCE resource;
    ID3D11DeviceContext* device_context = i_buffer->context.device_context;
    assert(i_buffer->size >= i_size);

    result = device_context->Map(i_buffer->buffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &resource);
    if (!SUCCEEDED(result))
    {
        /* TODO: handle error */
        assert(0);
        return;
    }

    memcpy(resource.pData, i_data, i_size);
    device_context->Unmap(i_buffer->buffer, NULL);
}

graphics_structured_buffer graphics_structured_buffer_create(graphics_d3d11_ctx* i_d3d11_ctx, graphics_structured_buffer_desc i_desc)
{

    HRESULT result;
    D3D11_BUFFER_DESC buffer_desc;
    D3D11_SUBRESOURCE_DATA buffer_data;
    D3D11_SUBRESOURCE_DATA* buffer_data_ptr = NULL;
    D3D11_SHADER_RESOURCE_VIEW_DESC resource_view_desc;

    graphics_structured_buffer buffer;
    memzero(&buffer, sizeof(buffer));

    /* https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_buffer_desc */
    memzero(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;                        
    buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;             /* Use as structured buffer in shaders */
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;            /* CPU write access */
    buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    buffer_desc.StructureByteStride = (UINT)i_desc.size;            /* Individual elements size */
    buffer_desc.ByteWidth = (UINT)(i_desc.size * i_desc.count);     /* Total buffer size */

    /* Can reserve a buffer without setting it's data. */
    if (i_desc.data != NULL)
    {
        buffer_data.pSysMem = i_desc.data;
        buffer_data_ptr = &buffer_data;
    }

    result = i_d3d11_ctx->device->CreateBuffer(&buffer_desc, buffer_data_ptr, &buffer.buffer);
    if (!SUCCEEDED(result))
    {
        /* TODO: handle error */
        assert(0);
        return buffer;
    }


    /* Create shader resource view. */
    memzero(&resource_view_desc, sizeof(resource_view_desc));
    resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
    resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    resource_view_desc.Buffer.ElementOffset = 0;
    resource_view_desc.Buffer.ElementWidth = (UINT)i_desc.count;

    result = i_d3d11_ctx->device->CreateShaderResourceView(buffer.buffer, &resource_view_desc, &buffer.shader_resource_view);
    if (!SUCCEEDED(result))
    {
        /* TODO: handle error */
        assert(0);
        return buffer;
    }

    buffer.context = *i_d3d11_ctx;
    buffer.size = i_desc.size;
    return buffer;
}

void graphics_structured_buffer_destory(graphics_structured_buffer* io_buffer)
{
    if (io_buffer->buffer != NULL)
    {
        io_buffer->buffer->Release();
    }
    if (io_buffer->shader_resource_view != NULL)
    {
        io_buffer->shader_resource_view->Release();
    }
    memzero(io_buffer, sizeof(*io_buffer));
}

void graphics_structured_buffer_update(graphics_structured_buffer* i_buffer, void* i_data, size_t i_size)
{
    assert(i_buffer->size <= i_size);
    i_buffer->context.device_context->UpdateSubresource(
        i_buffer->buffer,
        0,
        NULL,
        i_data,
        0,
        0
    );
}

graphics_image graphics_image_create(graphics_d3d11_ctx* i_d3d11_ctx, graphics_image_desc i_desc)
{
    HRESULT result;
    unsigned int format_byte_size;
    D3D11_TEXTURE2D_DESC texture_desc;
    D3D11_SUBRESOURCE_DATA init_data;
    D3D11_SHADER_RESOURCE_VIEW_DESC resource_view_desc;
    D3D11_SAMPLER_DESC sampler_desc;

    graphics_image image;
    memzero(&image, sizeof(image));

    /* Determine format. */
    switch (i_desc.type)
    {
        case GRAPHICS_IMAGE_TYPE_R8G8B8A8_UINT: image.format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case GRAPHICS_IMAGE_TYPE_R8G8B8_UINT: image.format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case GRAPHICS_IMAGE_TYPE_R8G8_UINT: image.format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case GRAPHICS_IMAGE_TYPE_R8_UINT: image.format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        default:;
    }

    format_byte_size = 0;
    switch (i_desc.type)
    {
        case GRAPHICS_IMAGE_TYPE_R8G8B8A8_UINT: format_byte_size = 4; break;
        case GRAPHICS_IMAGE_TYPE_R8G8B8_UINT: format_byte_size = 3; break;
        case GRAPHICS_IMAGE_TYPE_R8G8_UINT: format_byte_size = 2; break;
        case GRAPHICS_IMAGE_TYPE_R8_UINT: format_byte_size = 1; break;
        default:;
    }

    /* Create Texture2D. */
    memzero(&texture_desc, sizeof(texture_desc));
    texture_desc.Width = i_desc.width;
    texture_desc.Height = i_desc.height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = image.format;
    texture_desc.Usage = D3D11_USAGE_IMMUTABLE;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0; /* Immutable. No read or write access. */
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;

    memzero(&init_data, sizeof(init_data));
    init_data.pSysMem = i_desc.bytes;
    init_data.SysMemPitch = i_desc.width * format_byte_size;

    result = i_d3d11_ctx->device->CreateTexture2D(&texture_desc, &init_data, &image.texture_2d);
    if (!SUCCEEDED(result))
    {
        /* TODO: handle error */
        assert(0);
        return image;
    }

    /* Create shader resource view. */
    memzero(&resource_view_desc, sizeof(resource_view_desc));
    resource_view_desc.Format = image.format;
    resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    resource_view_desc.Texture2D.MipLevels = 1;

    result = i_d3d11_ctx->device->CreateShaderResourceView(image.texture_2d, &resource_view_desc, &image.shader_resource_view);
    if (!SUCCEEDED(result))
    {
        /* TODO: handle error */
        assert(0);
        return image;
    }

    /* Create sampler. */
    memzero(&sampler_desc, sizeof(sampler_desc));
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD = 0;
    sampler_desc.MinLOD = D3D11_FLOAT32_MAX;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    switch (i_desc.filter)
    {
        case GRAPHICS_FILTER_TYPE_POINT: sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; break;
        case GRAPHICS_FILTER_TYPE_LINEAR: sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; break;
        default:;
    }

    switch (i_desc.wrap_u)
    {
        case GRAPHICS_WARP_TYPE_REPEAT: sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; break;
        case GRAPHICS_WRAP_TYPE_CLAMP: sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP; break;
        case GRAPHICS_WRAP_TYPE_REPEAT_MIRROR: sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR; break;
        default:;
    }
    switch (i_desc.wrap_v)
    {
        case GRAPHICS_WARP_TYPE_REPEAT: sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP; break;
        case GRAPHICS_WRAP_TYPE_CLAMP: sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP; break;
        case GRAPHICS_WRAP_TYPE_REPEAT_MIRROR: sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR; break;
        default:;
    }

    result = i_d3d11_ctx->device->CreateSamplerState(&sampler_desc, &image.sampler_state);
    if (!SUCCEEDED(result))
    {
        /* TODO: handle error */
        assert(0);
        return image;
    }

    return image;
}

void graphics_image_destroy(graphics_image* io_image)
{
    if (io_image->shader_resource_view != NULL)
    {
        io_image->shader_resource_view->Release();
    }
    if (io_image->texture_2d != NULL)
    {
        io_image->texture_2d->Release();
    }
    if (io_image->sampler_state != NULL)
    {
        io_image->sampler_state->Release();
    }
    memzero(io_image, sizeof(*io_image));
}

graphics_shader graphics_shader_create(graphics_d3d11_ctx* i_d3d11_ctx, graphics_shader_desc i_desc)
{
    HRESULT result;
    graphics_shader shader;
    memzero(&shader, sizeof(shader));

    /* Compile shaders. */
    shader.vertex_shader_blob = graphics_shader_compile(
        i_desc.vertex_source,
        i_desc.vertex_shader_model,
        i_desc.vertex_entry_point
    );
    if (shader.vertex_shader_blob == NULL)
    {
        /* TODO: handle error */
        assert(0);
        return shader;
    }

    shader.fragment_shader_blob = graphics_shader_compile(
        i_desc.fragment_source,
        i_desc.fragment_shader_model,
        i_desc.fragment_entry_point
    );
    if (shader.fragment_shader_blob == NULL)
    {
        /* TODO: handle error */
        assert(0);
        return shader;
    }

    /* Create shader resources */
    result = i_d3d11_ctx->device->CreateVertexShader(
        shader.vertex_shader_blob->GetBufferPointer(),
        shader.vertex_shader_blob->GetBufferSize(),
        NULL,
        &shader.vertex_shader
    );
    if (!SUCCEEDED(result))
    {
        /* TODO: handle error */
        assert(0);
        return shader;
    }

    result = i_d3d11_ctx->device->CreatePixelShader(
        shader.fragment_shader_blob->GetBufferPointer(),
        shader.fragment_shader_blob->GetBufferSize(),
        NULL,
        &shader.fragment_shader
    );
    if (!SUCCEEDED(result))
    {
        /* TODO: handle error */
        assert(0);
        return shader;
    }

    /* Create constant buffers */
    for (sz_t i = 0; i < GRAPHICS_CONSTANT_BUFFERS_MAX; ++i)
    {
        u32_t size = i_desc.vertex_constant_buffer_sizes[i];
        if (size == 0)
        {
            break;
        }

        D3D11_BUFFER_DESC buffer_desc;
        memzero(&buffer_desc, sizeof(buffer_desc));
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;                    /* Write access from CPU, read access GPU */
        buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;         /* Use as constant buffer */
        buffer_desc.CPUAccessFlags = 0;
        buffer_desc.ByteWidth = (UINT)math_round_up(size, 16); /* Total buffer size */

        result = i_d3d11_ctx->device->CreateBuffer(&buffer_desc, NULL, &shader.vertex_constant_buffers[i]);
        if (!SUCCEEDED(result))
        {
            /* TODO: handle error */
            assert(0);
            return shader;
        }
    }

    for (sz_t i = 0; i < GRAPHICS_CONSTANT_BUFFERS_MAX; ++i)
    {
        u32_t size = i_desc.fragment_constant_buffer_sizes[i];
        if (size == 0)
        {
            break;
        }

        D3D11_BUFFER_DESC buffer_desc;
        memzero(&buffer_desc, sizeof(buffer_desc));
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;                    /* Write access from CPU, read access GPU */
        buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;         /* Use as constant buffer */
        buffer_desc.CPUAccessFlags = 0;
        buffer_desc.ByteWidth = (UINT)math_round_up(size, 16); /* Total buffer size */

        result = i_d3d11_ctx->device->CreateBuffer(&buffer_desc, NULL, &shader.fragment_constant_buffers[i]);
        if (!SUCCEEDED(result))
        {
            /* TODO: handle error */
            assert(0);
            return shader;
        }
    }

    return shader;
}

void graphics_shader_destroy(graphics_shader* io_shader)
{
    if (io_shader->vertex_shader != NULL)
    {
        io_shader->vertex_shader->Release();
    }
    if (io_shader->vertex_shader_blob != NULL)
    {
        io_shader->vertex_shader_blob->Release();
    }
    for(sz_t i = 0; i < GRAPHICS_CONSTANT_BUFFERS_MAX; ++i)
    {
        if (io_shader->vertex_constant_buffers[i] != NULL)
        {
            io_shader->vertex_constant_buffers[i]->Release();
        }
    }

    if (io_shader->fragment_shader != NULL)
    {
        io_shader->fragment_shader->Release();
    }
    if (io_shader->fragment_shader_blob != NULL)
    {
        io_shader->fragment_shader_blob->Release();
    }
    for(sz_t i = 0; i < GRAPHICS_CONSTANT_BUFFERS_MAX; ++i)
    {
        if (io_shader->fragment_constant_buffers[i] != NULL)
        {
            io_shader->fragment_constant_buffers[i]->Release();
        }
    }

    memzero(io_shader, sizeof(*io_shader));
}

ID3DBlob* graphics_shader_compile(char const* i_source, char const* i_shader_model, char const* i_entry_point)
{
    HRESULT result;
    UINT flags = 0;
    ID3DBlob* shader = NULL;
    ID3DBlob* errors = NULL;

    char const* entry_point = "main";
    if (i_entry_point != NULL)
    {
        entry_point = i_entry_point;
    }

    flags |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
    #ifdef DEBUG
        flags |= D3DCOMPILE_DEBUG;
        flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
    #else
        flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
    #endif

    /* https://learn.microsoft.com/en-us/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dcompile */
    result = D3DCompile(
        i_source,           /* Source code */
        strlen(i_source),   /* Source code size */
        NULL,               /* Source name for errors */
        NULL,               /* Defines */
        NULL,               /* Includes */
        entry_point,        /* Entry point function name */
        i_shader_model,     /* Shader model target */
        flags,              /* Flags */
        NULL,               /* Flags2 */
        &shader,
        &errors             /* Compilation issues */
    );

    if (errors != NULL)
    {
        /* TODO: handle error */
        OutputDebugStringA((char*)errors->GetBufferPointer());
        errors->Release();
        if (shader != NULL)
        {
            shader->Release();
        }
        assert(0);
    }

    if (!SUCCEEDED(result))
    {
        /* TODO: handle error */
        assert(0);
        return NULL;
    }

    return shader;
}

graphics_pipeline graphics_pipeline_create(graphics_d3d11_ctx* i_d3d11_ctx, graphics_pipeline_desc i_desc)
{
    HRESULT result;
    u32_t total_offset = 0;
    u8_t input_descriptions_count = 0;
    D3D11_INPUT_ELEMENT_DESC input_descriptions[GRAPHICS_VERTEX_ATTRIBUTES_MAX];

    graphics_pipeline pipeline;
    memzero(&pipeline, sizeof(pipeline));

    /* Compute attribute defaults. */
    for (; input_descriptions_count < GRAPHICS_VERTEX_ATTRIBUTES_MAX; ++input_descriptions_count)
    {
        graphics_vertex_attribute_desc* attribute = &i_desc.attributes[input_descriptions_count];
        if (attribute->format == GRAPHICS_VERTEX_ATTRIBUTE_INVALID)
        {
            break;
        }

        if (attribute->offset == 0)
        {
            attribute->offset = total_offset;
        }

        switch (attribute->format)
        {
            case GRAPHICS_VERTEX_ATTRIBUTE_FLOAT: total_offset += 4; break;
            case GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2: total_offset += 8; break;
            case GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3: total_offset += 12; break;
            case GRAPHICS_VERTEX_ATTRIBUTE_FLOAT4: total_offset += 16; break;
            default:;
        }
    }

    /* Compute vertex stride. */
    if (i_desc.stride == 0)
    {
        pipeline.stride = total_offset;
    }

    /* Set topology. */
    switch (i_desc.topology)
    {
        case GRAPHICS_VERTEX_TOPOLOGY_TRIANGLES: pipeline.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
        case GRAPHICS_VERTEX_TOPOLOGY_TRIANGLE_STRIP: pipeline.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
        case GRAPHICS_VERTEX_TOPOLOGY_LINES: pipeline.topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST; break;
        case GRAPHICS_VERTEX_TOPOLOGY_LINE_STRIP: pipeline.topology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
        case GRAPHICS_VERTEX_TOPOLOGY_POINTS: pipeline.topology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST; break;
        default:;
    }

    /* Create input layout. */
    memzero(input_descriptions, sizeof(input_descriptions));
    for (sz_t i = 0; i < GRAPHICS_VERTEX_ATTRIBUTES_MAX; ++i)
    {
        graphics_vertex_attribute_desc* attribute = &i_desc.attributes[i];
        if (attribute->format == GRAPHICS_VERTEX_ATTRIBUTE_INVALID)
        {
            break;
        }

        D3D11_INPUT_ELEMENT_DESC* input_desc = &input_descriptions[i];
        input_desc->SemanticName = attribute->semantic_name;
        input_desc->SemanticIndex = attribute->semantic_index;
        input_desc->InputSlot = attribute->input_slot;
        input_desc->AlignedByteOffset = attribute->offset;

        switch (attribute->format)
        {
            case GRAPHICS_VERTEX_ATTRIBUTE_FLOAT: input_desc->Format = DXGI_FORMAT_R32_FLOAT; break;
            case GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2: input_desc->Format = DXGI_FORMAT_R32G32_FLOAT; break;
            case GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3: input_desc->Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
            case GRAPHICS_VERTEX_ATTRIBUTE_FLOAT4: input_desc->Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
            default: break;
        }

        switch (attribute->step_func)
        {
            case GRAPHICS_VERTEX_STEP_FUNC_PER_VERTEX: input_desc->InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA; break;
            case GRAPHICS_VERTEX_STEP_FUNC_PER_INSTANCE: input_desc->InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA; break;
            default: break;
        }

        if (attribute->step_func == GRAPHICS_VERTEX_STEP_FUNC_PER_INSTANCE)
        {
            input_desc->InstanceDataStepRate = attribute->step_rate;
        }
    }

    result = i_d3d11_ctx->device->CreateInputLayout(
        input_descriptions,
        input_descriptions_count,
        i_desc.shader.vertex_shader_blob->GetBufferPointer(),
        i_desc.shader.vertex_shader_blob->GetBufferSize(),
        &pipeline.input_layout
    );
    if (!SUCCEEDED(result))
    {
        /* TODO: handle error */
        assert(0);
        return pipeline;
    }

    pipeline.shader = i_desc.shader;
    pipeline.context = *i_d3d11_ctx;
    return pipeline;
}

void graphics_pipeline_destroy(graphics_pipeline* io_pipline)
{
    if (io_pipline->input_layout != NULL)
    {
        io_pipline->input_layout->Release();
    }
    memzero(io_pipline, sizeof(*io_pipline));
}

graphics_bindings graphics_bindings_create(graphics_d3d11_ctx* i_d3d11_ctx, graphics_bindings i_desc)
{
    /* TODO: is there really a point in doing this? Stupid C++... */
    (void)i_d3d11_ctx;
    return i_desc;
}

void graphics_bindings_set(graphics_bindings* i_bindings)
{
    ID3D11DeviceContext* device_context = i_bindings->pipeline.context.device_context;
    u32_t shader_resources_count = 0;

    ID3D11Buffer* vertex_buffers[GRAPHICS_VERTEX_BUFFERS_MAX];
    UINT strides[GRAPHICS_VERTEX_BUFFERS_MAX];
    UINT offsets[GRAPHICS_VERTEX_BUFFERS_MAX];
    ID3D11ShaderResourceView* shader_resources[GRAPHICS_IMAGES_MAX + GRAPHICS_STRUCTURED_BUFFERS_MAX];
    ID3D11SamplerState* samplers[GRAPHICS_IMAGES_MAX];

    /* Apply pipeline. */
    graphics_pipeline* pipeline = &i_bindings->pipeline;
    device_context->IASetPrimitiveTopology(pipeline->topology);
    device_context->IASetInputLayout(pipeline->input_layout);
    device_context->VSSetShader(pipeline->shader.vertex_shader, NULL, 0);
    device_context->VSSetConstantBuffers(0, GRAPHICS_CONSTANT_BUFFERS_MAX, pipeline->shader.vertex_constant_buffers);
    device_context->PSSetShader(pipeline->shader.fragment_shader, NULL, 0);
    device_context->PSSetConstantBuffers(0, GRAPHICS_CONSTANT_BUFFERS_MAX, pipeline->shader.fragment_constant_buffers);

    /* Apply bindings. */
    for (sz_t i = 0; i < GRAPHICS_VERTEX_BUFFERS_MAX; ++i)
    {
        vertex_buffers[i] = i_bindings->vertex_buffers[i].buffer;
        strides[i] = i_bindings->pipeline.stride;
        offsets[i] = 0;
    }

    memzero(shader_resources, sizeof(shader_resources));
    memzero(samplers, sizeof(samplers));
    for (sz_t i = 0; i < GRAPHICS_IMAGES_MAX; ++i)
    {
        if (i_bindings->images[i].shader_resource_view == NULL)
        {
            break;
        }
        shader_resources[i] = i_bindings->images[i].shader_resource_view;
        samplers[i] = i_bindings->images[i].sampler_state;
        shader_resources_count += 1;
    }
    for (sz_t i = 0; i < GRAPHICS_STRUCTURED_BUFFERS_MAX; ++i)
    {
        if (i_bindings->structured_buffers[i].shader_resource_view == NULL)
        {
            break;
        }
        shader_resources[shader_resources_count + i] = i_bindings->structured_buffers[i].shader_resource_view;
    }

    device_context->IASetVertexBuffers(
        0,
        GRAPHICS_VERTEX_BUFFERS_MAX,
        vertex_buffers,
        strides,
        offsets
    );
    device_context->PSSetShaderResources(
        0,
        GRAPHICS_IMAGES_MAX + GRAPHICS_STRUCTURED_BUFFERS_MAX,
        shader_resources
    );
    device_context->PSSetSamplers(
        0,
        GRAPHICS_IMAGES_MAX,
        samplers
    );
}

void graphics_pass_begin(graphics_d3d11_ctx* i_d3d11_ctx, u32_t i_width, u32_t i_height)
{
    D3D11_VIEWPORT viewport;

    /* Apply back buffer render target. */
    i_d3d11_ctx->device_context->OMSetRenderTargets(1, &i_d3d11_ctx->render_target_view, NULL);

    /* Apply viewport to cover the whole screen. */
    memzero(&viewport, sizeof(viewport));
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)i_width;
    viewport.Height = (FLOAT)i_height;
    viewport.MaxDepth = 1.0f;
    i_d3d11_ctx->device_context->RSSetViewports(1, &viewport);

    FLOAT clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    i_d3d11_ctx->device_context->ClearRenderTargetView(i_d3d11_ctx->render_target_view, clear_color);
}

void graphics_pass_end(graphics_d3d11_ctx* i_d3d11_ctx)
{
    i_d3d11_ctx->device_context->Draw(4, 0);
    i_d3d11_ctx->swap_chain->Present(0, 0);
}

void graphics_constant_buffer_update(graphics_pipeline* i_pipeline, graphics_shader_stage i_stage, sz_t i_index, void* i_data)
{
    ID3D11Buffer* constant_buffer = NULL;
    switch (i_stage)
    {
        case GRAPHICS_SHADER_STAGE_VERTEX:
        {
            constant_buffer = i_pipeline->shader.vertex_constant_buffers[i_index];
        } break;
        case GRAPHICS_SHADER_STAGE_FRAGMENT:
        {
            constant_buffer = i_pipeline->shader.fragment_constant_buffers[i_index];
        } break;
        default:;
    }
    i_pipeline->context.device_context->UpdateSubresource(
        constant_buffer,
        0,
        NULL,
        i_data,
        0,
        0
    );
}

fmat44_t graphics_calc_image_projection(window_ctx* i_window, f32_t i_image_width, f32_t i_image_height)
{
    f32_t width, height;
    f32_t left, right, top, bottom;
    fmat44_t view, projection;
    f32_t target_aspect_ratio = i_image_width / i_image_height;

    /* First try scalling the image to the window width and see if the hight gets cut off. */
    width = (f32_t)i_window->width;
    height = width / target_aspect_ratio;
    if (height > (f32_t)i_window->height)
    {
        /* Switch to scaling to the window height. */
        height = (f32_t)i_window->height;
        width = height * target_aspect_ratio;    
    }

    /* Scale the screen coordinates so the image renders at the correct position without stretching to the window edge. */
    width = i_image_width * ((f32_t)i_window->width / width);
    height = i_image_height * ((f32_t)i_window->height / height);

    /* Move the image half the extra space so it's centered. */
    left = (i_image_width - width) / 2.0f;
    right = width + left;
    top = (i_image_height - height) / 2.0f;
    bottom = height + top;

    view = math_look_at({0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    projection = math_orthorgraphic_projection(left, right, bottom, top, 0.01f, 10.0f);
    return fmat44_mul(projection, view);
}

/* --------------------------------------------------
   XAudio2 backend 
   -------------------------------------------------- */

audio_xaudio2_ctx* audio_xaudio2_init()
{
    WAVEFORMATEX wave_format;
    HRESULT result;

    audio_xaudio2_ctx* xaudio2_ctx = new audio_xaudio2_ctx();
    //audio_xaudio2_ctx* xaudio2_ctx = malloc_type(audio_xaudio2_ctx);
    //memzero(xaudio2_ctx, sizeof(*xaudio2_ctx));
    xaudio2_ctx->volume = AUDIO_VOLUME_DEFAULT;

    result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (!SUCCEEDED(result))
    {
        /* TODO: handle error */
        assert(0);
        return xaudio2_ctx;
    }
    
    result = XAudio2Create(&xaudio2_ctx->xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (!SUCCEEDED(result))
    {
        /* TODO: handle error */
        assert(0);
        return xaudio2_ctx;
    }

    result = xaudio2_ctx->xaudio2->CreateMasteringVoice(&xaudio2_ctx->master_voice);
    if (!SUCCEEDED(result))
    {
        /* TODO: handle error */
        assert(0);
        return xaudio2_ctx;
    }

    memzero(&wave_format, sizeof(wave_format));
    wave_format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.nChannels = 2;
    wave_format.nSamplesPerSec = 44100;
    wave_format.wBitsPerSample = 16;
    wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
    wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;

    for (sz_t i = 0; i < AUDIO_CONCURRENT_SOUNDS_MAX; ++i)
    {
        //audio_xaudio2_voice* voice = &xaudio2_ctx->voices[i];
        //voice = new(&xaudio2_ctx->voices[i]) audio_xaudio2_voice();
        audio_xaudio2_voice* voice = new audio_xaudio2_voice();
        xaudio2_ctx->voices[i] = voice;
        
        result = xaudio2_ctx->xaudio2->CreateSourceVoice(
            &voice->voice, 
            &wave_format, 
            0, 
            XAUDIO2_DEFAULT_FREQ_RATIO, 
            voice, 
            NULL, 
            NULL
        );
        voice->voice->SetVolume(xaudio2_ctx->volume);
        if (!SUCCEEDED(result))
        {
            /* TODO: handle error */
            assert(0);
            return xaudio2_ctx;
        }
    }
    return xaudio2_ctx;
}

void audio_xaudio2_destory(audio_xaudio2_ctx* io_xaudio2_ctx)
{
    if (io_xaudio2_ctx->xaudio2 != NULL)
    {
        io_xaudio2_ctx->xaudio2->Release();
    }
    memzero(io_xaudio2_ctx, sizeof(*io_xaudio2_ctx));
}

void audio_push_sounds(audio_xaudio2_ctx* i_xaudio2_ctx, f32_t i_delta_time)
{
    for (sz_t i = 0; i < i_xaudio2_ctx->sounds_count; ++i)
    {
        audio_sound* sound = &i_xaudio2_ctx->sounds[i];

        /* Start any new sounds. */
        if (sound->flags & AUDIO_FLAG_PLAY || sound->flags & AUDIO_FLAG_FADE_IN)
        {
            HRESULT result;
            XAUDIO2_BUFFER buffer;
            b8 looping = sound->flags & AUDIO_FLAG_LOOP; 

            /* Get a voice to play this sound on. */
            audio_xaudio2_voice* voice = NULL;
            for (sz_t voice_idx = 0; voice_idx < AUDIO_CONCURRENT_SOUNDS_MAX; ++voice_idx)
            {
                if (!i_xaudio2_ctx->voices[voice_idx]->is_playing)
                {
                    voice = i_xaudio2_ctx->voices[voice_idx];
                    printf("Play voice: %zu\n", voice_idx);
                    break;
                }
            }

            /* If no voice available, simply don't play the sound 
             * TODO: add virtual sounds? */
            if (voice == NULL)
            {
                continue;
            }

            memzero(&buffer, sizeof(buffer));
            buffer.Flags = XAUDIO2_END_OF_STREAM;
            buffer.AudioBytes = sound->size;
            buffer.pAudioData = sound->data;
            buffer.LoopCount = looping ? XAUDIO2_MAX_LOOP_COUNT : 0;

            result = voice->voice->SubmitSourceBuffer(&buffer);
            if (!SUCCEEDED(result))
            {
                /* TODO: handle error */
                assert(0);
                continue;
            }

            voice->volume = sound->volume;
            voice->fade_in_duration = sound->fade_in_duration;
            voice->fade_out_duration = sound->fade_out_duration;
            voice->fade_timer = 0.0f;
            voice->sound_id = sound->sound_id;
            voice->flags = sound->flags;
            voice->voice->Start();
            InterlockedExchange((LONG*)&voice->is_playing, TRUE);
        }

        /* Stop any playing sound. */
        if (sound->flags & AUDIO_FLAG_STOP || sound->flags & AUDIO_FLAG_FADE_OUT)
        {
            /* Find the voice currently playing this sound. */
            audio_xaudio2_voice* voice = NULL;
            for (sz_t voice_idx = 0; voice_idx < AUDIO_CONCURRENT_SOUNDS_MAX; ++voice_idx)
            {
                if (i_xaudio2_ctx->voices[voice_idx]->sound_id == sound->sound_id && 
                    i_xaudio2_ctx->voices[voice_idx]->is_playing)
                {
                    voice = i_xaudio2_ctx->voices[voice_idx];
                    break;
                }
            }

            if (voice != NULL)
            {
                voice->flags |= AUDIO_FLAG_FADE_OUT;
                if (sound->flags & AUDIO_FLAG_FADE_OUT)
                {
                    voice->fade_out_duration = sound->fade_out_duration;
                }
            }
        }
    }

    /* Clear sound events. */
    memzero(i_xaudio2_ctx->sounds, sizeof(i_xaudio2_ctx->sounds));
    i_xaudio2_ctx->sounds_count = 0;

    /* Update voice fade in and out. */
    for (sz_t voice_idx = 0; voice_idx < AUDIO_CONCURRENT_SOUNDS_MAX; ++voice_idx)
    {
        audio_xaudio2_voice* voice = i_xaudio2_ctx->voices[voice_idx];

        if (voice->flags & AUDIO_FLAG_FADE_IN)
        {
            f32_t fade_factor;
            voice->fade_timer += i_delta_time;
            if (voice->fade_timer >= voice->fade_in_duration)
            {
                voice->fade_timer = voice->fade_in_duration;
                voice->flags ^= AUDIO_FLAG_FADE_IN;
            }

            fade_factor = voice->fade_timer / voice->fade_in_duration;
            voice->voice->SetVolume(i_xaudio2_ctx->volume * voice->volume * fade_factor);

            continue; /* Finish fade in before processing fade out. */
        }

        if (voice->flags & AUDIO_FLAG_FADE_OUT)
        {
            f32_t fade_factor;
            voice->fade_timer += i_delta_time;
            if (voice->fade_timer >= voice->fade_out_duration)
            {
                voice->fade_timer = voice->fade_out_duration;
                voice->flags ^= AUDIO_FLAG_FADE_OUT;
            }

            fade_factor = 1.0f - (voice->fade_timer / voice->fade_out_duration);
            voice->voice->SetVolume(i_xaudio2_ctx->volume * voice->volume * fade_factor);

            if (fade_factor <= 0.0f)
            {
                voice->voice->Stop(); /* Voice callback will set is_playing to false. */
                voice->voice->FlushSourceBuffers();
                voice->voice->SetVolume(i_xaudio2_ctx->volume);

                voice->fade_in_duration = 0.0f;
                voice->fade_out_duration = 0.0f;
                voice->fade_timer = 0.0f;
                voice->sound_id = 0;
                voice->flags = 0;
            }
        }
    }
}

audio_sound_id audio_play_sound(audio_xaudio2_ctx* i_xaudio2_ctx, void* i_data, u32_t i_size, f32_t i_volume, u8_t i_flags)
{
    audio_sound* sound = &i_xaudio2_ctx->sounds[i_xaudio2_ctx->sounds_count];
    sound->sound_id = i_xaudio2_ctx->sound_id_next;
    sound->volume = i_volume;
    sound->fade_in_duration = 1.0f;
    sound->fade_out_duration = 1.0f;
    sound->flags = i_flags | AUDIO_FLAG_PLAY;
    sound->data = (u8_t*)i_data;
    sound->size = i_size;
    i_xaudio2_ctx->sounds_count += 1;
    i_xaudio2_ctx->sound_id_next += 1;
    return sound->sound_id;
}
