/*
 * Copyright � 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Xiang Haihao <haihao.xiang@intel.com>
 *    Zou Nan hai <nanhai.zou@intel.com>
 *
 */

#ifndef _I965_DRV_VIDEO_H_
#define _I965_DRV_VIDEO_H_

#include <va/va.h>
#include <va/va_backend.h>

#include "i965_mutext.h"
#include "object_heap.h"
#include "intel_driver.h"

#define I965_MAX_PROFILES                       11
#define I965_MAX_ENTRYPOINTS                    5
#define I965_MAX_CONFIG_ATTRIBUTES              10
#define I965_MAX_IMAGE_FORMATS                  3
#define I965_MAX_SUBPIC_FORMATS                 4
#define I965_MAX_DISPLAY_ATTRIBUTES             4

#define INTEL_STR_DRIVER_VENDOR                 "Intel"
#define INTEL_STR_DRIVER_NAME                   "i965"

#define I965_SURFACE_IMAGE      0
#define I965_SURFACE_SURFACE    1

struct i965_surface
{
    VAGenericID id;
    int flag;
};

struct i965_kernel 
{
    char *name;
    int interface;
    const uint32_t (*bin)[4];
    int size;
    dri_bo *bo;
};

struct buffer_store
{
    unsigned char *buffer;
    dri_bo *bo;
    int ref_count;
    int num_elements;
};
    
struct object_config 
{
    struct object_base base;
    VAProfile profile;
    VAEntrypoint entrypoint;
    VAConfigAttrib attrib_list[I965_MAX_CONFIG_ATTRIBUTES];
    int num_attribs;
};

#define NUM_SLICES     10

struct decode_state
{
    struct buffer_store *pic_param;
    struct buffer_store **slice_params;
    struct buffer_store *iq_matrix;
    struct buffer_store *bit_plane;
    struct buffer_store **slice_datas;
    VASurfaceID current_render_target;
    int max_slice_params;
    int max_slice_datas;
    int num_slice_params;
    int num_slice_datas;
};

struct encode_state
{
    struct buffer_store *seq_param;
    struct buffer_store *pic_param;
    struct buffer_store *pic_control;
    struct buffer_store *iq_matrix;
    struct buffer_store *q_matrix;
    struct buffer_store **slice_params;
    int max_slice_params;
    int num_slice_params;

    /* for ext */
    struct buffer_store *seq_param_ext;
    struct buffer_store *pic_param_ext;
    struct buffer_store *dec_ref_pic_marking;
    struct buffer_store *packed_sps;
    struct buffer_store *packed_pps;
    struct buffer_store *packed_slice_header;
    struct buffer_store **slice_params_ext;
    int max_slice_params_ext;
    int num_slice_params_ext;

    VASurfaceID current_render_target;
};

struct proc_state
{
    struct buffer_store *pipeline_param;
    struct buffer_store *input_param;
    struct buffer_store *filter_param[VA_PROC_PIPELINE_MAX_NUM_FILTERS];

    VASurfaceID current_render_target;
};

#define CODEC_DEC       0
#define CODEC_ENC       1
#define CODEC_PROC      2

union codec_state
{
    struct decode_state decode;
    struct encode_state encode;
    struct proc_state proc;
};

struct hw_context
{
    void (*run)(VADriverContextP ctx, 
                VAProfile profile, 
                union codec_state *codec_state,
                struct hw_context *hw_context);
    void (*destroy)(void *);
    struct intel_batchbuffer *batch;
};

struct object_context 
{
    struct object_base base;
    VAContextID context_id;
    VAConfigID config_id;
    VASurfaceID *render_targets;		//input->encode, output->decode
    int num_render_targets;
    int picture_width;
    int picture_height;
    int flags;
    int codec_type;
    union codec_state codec_state;
    struct hw_context *hw_context;
};

#define SURFACE_REFERENCED      (1 << 0)
#define SURFACE_DISPLAYED       (1 << 1)
#define SURFACE_DERIVED         (1 << 2)
#define SURFACE_REF_DIS_MASK    ((SURFACE_REFERENCED) | \
                                 (SURFACE_DISPLAYED))
#define SURFACE_ALL_MASK        ((SURFACE_REFERENCED) | \
                                 (SURFACE_DISPLAYED) |  \
                                 (SURFACE_DERIVED))

struct object_surface 
{
    struct object_base base;
    VASurfaceStatus status;
    VASubpictureID subpic;
    int width;
    int height;
    int size;
    int orig_width;
    int orig_height;
    int flags;
    unsigned int fourcc;    
    dri_bo *bo;
    VAImageID locked_image_id;
    void (*free_private_data)(void **data);
    void *private_data;
};

struct object_buffer 
{
    struct object_base base;
    struct buffer_store *buffer_store;
    int max_num_elements;
    int num_elements;
    int size_element;
    VABufferType type;
};

struct object_image 
{
    struct object_base base;
    VAImage image;
    dri_bo *bo;
    unsigned int *palette;
    VASurfaceID derived_surface;
};

struct object_subpic 
{
    struct object_base base;
    VAImageID image;
    VARectangle src_rect;
    VARectangle dst_rect;
    unsigned int format;
    int width;
    int height;
    int pitch;
    dri_bo *bo;
    unsigned int flags;
};

struct hw_codec_info
{
    struct hw_context *(*dec_hw_context_init)(VADriverContextP, VAProfile);
    struct hw_context *(*enc_hw_context_init)(VADriverContextP, VAProfile);
    struct hw_context *(*proc_hw_context_init)(VADriverContextP, VAProfile);
};


#include "i965_render.h"

struct i965_driver_data 
{
    struct intel_driver_data intel;
    struct object_heap config_heap;
    struct object_heap context_heap;
    struct object_heap surface_heap;
    struct object_heap buffer_heap;
    struct object_heap image_heap;
    struct object_heap subpic_heap;
    struct hw_codec_info *codec_info;

    _I965Mutex render_mutex;
    struct intel_batchbuffer *batch;
    struct i965_render_state render_state;
    void *pp_context;
    char va_vendor[256];
};

#define NEW_CONFIG_ID() object_heap_allocate(&i965->config_heap);
#define NEW_CONTEXT_ID() object_heap_allocate(&i965->context_heap);
#define NEW_SURFACE_ID() object_heap_allocate(&i965->surface_heap);
#define NEW_BUFFER_ID() object_heap_allocate(&i965->buffer_heap);
#define NEW_IMAGE_ID() object_heap_allocate(&i965->image_heap);
#define NEW_SUBPIC_ID() object_heap_allocate(&i965->subpic_heap);

#define CONFIG(id) ((struct object_config *)object_heap_lookup(&i965->config_heap, id))
#define CONTEXT(id) ((struct object_context *)object_heap_lookup(&i965->context_heap, id))
#define SURFACE(id) ((struct object_surface *)object_heap_lookup(&i965->surface_heap, id))
#define BUFFER(id) ((struct object_buffer *)object_heap_lookup(&i965->buffer_heap, id))
#define IMAGE(id) ((struct object_image *)object_heap_lookup(&i965->image_heap, id))
#define SUBPIC(id) ((struct object_subpic *)object_heap_lookup(&i965->subpic_heap, id))

#define FOURCC_IA44 0x34344149
#define FOURCC_AI44 0x34344941

#define STRIDE(w)               (((w) + 0xf) & ~0xf)
#define SIZE_YUV420(w, h)       (h * (STRIDE(w) + STRIDE(w >> 1)))

static INLINE struct i965_driver_data *
i965_driver_data(VADriverContextP ctx)
{
    return (struct i965_driver_data *)(ctx->pDriverData);
}

void 
i965_check_alloc_surface_bo(VADriverContextP ctx,
                            struct object_surface *obj_surface,
                            int tiled,
                            unsigned int fourcc);

#endif /* _I965_DRV_VIDEO_H_ */