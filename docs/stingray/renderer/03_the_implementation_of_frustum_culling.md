# The Implementation of Frustom Culling

## Overview

Frustum culling can be an expensive operation. Stingray accelerates it by making heavy use of SIMD and distributing the workload over several threads. The basic workflow is:

 * Kick jobs to do frustum vs sphere culling
   * For each frustum plane, test plane vs sphere
 * Wait for sphere culling to finish
 * For objects that pass sphere test, kick jobs to do frustum vs object-oriented bounding box (OOBB) culling
   * For each frustum plane, test plane vs OOBB
 * Wait for OOBB culling to finish

Frustum vs sphere tests are significantly faster than frustum vs OOBB. By rejecting objects that fail sphere culling first, we have fewer objects to process in the more expensive OOBB pass.

Why go over all objects brute force instead of using some sort of spatial partition data structure? We like to keep things simple and with the current setup we have yet to encounter a case where we've been bound by the culling. Brute force sphere culling followed by OOBB culling is fast enough for all cases we've encountered so far. That might of course change in the future, but we'll take care of that when it's an actual problem.

The brute force culling is pretty fast, because:

 1. The sphere and the OOBB culling use SIMD and only load the minimum amount of needed data.
 2. The workload is distributed over several threads.

In this post, we will first look at the single threaded SIMD code and then how the culling is distributed over multiple threads.

I'll use a lot of code to show how it's all done. It's mostly actual code from the engine, but it has been cleaned up to a certain extent. Some stuff has been renamed and/or removed to make it easier to understand what's going on.

## Data structures used

If you go back to my previous post about [state reflection](02_state_reflection.md), you can read that each object on the main thread is associated with a render thread representation via a **render_handle**. The **render_handle** is used to get the **object_index** which is the index of an object in the **_objects** array.

Take a look at the following code for a refresher:

```C++
void RenderWorld::create_object(WorldRenderInterface::ObjectManagementPackage *omp)
{
    // Acquire an `object_index`.
    uint32_t object_index = _objects.size();

    // Same recycling mechanism as seen for render handles.
    if (_free_object_indices.any()) {
        object_index = _free_object_indices.back();
        _free_object_indices.pop_back();
    } else {
        _objects.resize(object_index + 1);
        _object_types.resize(object_index + 1);
    }

    void *render_object = omp->user_data;
    if (omp->type == RenderMeshObject::TYPE) {
        // Cast the `render_object` to a `MeshObject`.
        RenderMeshObject *rmo = (RenderMeshObject*)render_object;

        // If needed, do more stuff with `rmo`.
    }

    // Store the `render_object` and `type`.
    _objects[object_index] = render_object;
    _object_types[object_index] = omp->type;

    if (omp->render_handle >= _object_lut.size())
        _object_lut.resize(omp->handle + 1);
    // The `render_handle` is used
    _object_lut[omp->render_handle] = object_index;
}
```

The **_objects** array stores objects of all kinds of different types. It is defined as:

```C++
Array<void*> _objects;
```

The types of the objects are stored in a corresponding **_object_types** array, defined as:

```C++
Array<uint32_t> _object_types;
```

From **_object_types**, we know the actual type of the objects and we can use that to cast the void * into the proper type (mesh, terrain, gui, particle_system, etc).

The culling happens in the **// If needed, do more stuff with rmo** section above. It looks like this:

```C++
void *render_object = omp->user_data;
if (omp->type == RenderMeshObject::TYPE) {
    // Cast the `render_object` to a `MeshObject`.
    RenderMeshObject *rmo = (RenderMeshObject*)render_object;

    // If needed, do more stuff with `rmo`.
    if (!(rmo->flags() & renderable::CULLING_DISABLED)) {
        culling::Object o;
        // Extract necessary information to do culling.

        // The index of the object.
        o.id = object_index;

        // The type of the object.
        o.type = rmo->type;

        // Get the mininum and maximum corner positions of a boudning box in object space.
        o.min = float4(rmo->bounding_volume().min, 1.f);
        o.max = float4(rmo->bounding_volume().max, 1.f);

        // World transform matrix.
        o.m = float4x4(rmo->world());

        // Depending on the value of `flags` add the culling representation to different culling sets.
        if (rmo->flags() & renderable::VIEWPORT_VISIBLE)
            _cullable_objects.add(o, rmo->node());
        if (rmo->flags() & renderable::SHADOW_CASTER)
            _cullable_shadow_casters.add(o, rmo->node());
        if (rmo->flags() & renderable::OCCLUDER)
            _occluders.add(o, rmo->node());
    }
}
```

For culling **MeshObject**s and other cullable types are represented by **culling::Object**s that are used to populate the culling data structures. As can be seen in the code they are **_cullable_objects**, **_cullable_shadow_casters** and **_occluders** and they are all represented by an **ObjectSet**:

```C++
struct ObjectSet
{
    // Minimum bounding box corner position.
    Array<float> min_x;
    Array<float> min_y;
    Array<float> min_z;

    // Maximum bounding box corner position.
    Array<float> max_x;
    Array<float> max_y;
    Array<float> max_z;

    // Object->world matrix.
    Array<float> world_xx;
    Array<float> world_xy;
    Array<float> world_xz;
    Array<float> world_xw;
    Array<float> world_yx;
    Array<float> world_yy;
    Array<float> world_yz;
    Array<float> world_yw;
    Array<float> world_zx;
    Array<float> world_zy;
    Array<float> world_zz;
    Array<float> world_zw;
    Array<float> world_tx;
    Array<float> world_ty;
    Array<float> world_tz;
    Array<float> world_tw;

    // World space center position of bounding sphere.
    Array<float> ws_pos_x;
    Array<float> ws_pos_y;
    Array<float> ws_pos_z;

    // Radius of bounding sphere.
    Array<float> radius;

    // Flag to indicate if an object is culled or not.
    Array<uint32_t> visibility_flag;

    // The type and id of an object.
    Array<uint32_t> type;
    Array<uint32_t> id;

    uint32_t n_objects;
};
```

When an object is added to, e.g. **_cullable_objects** the **culling::Object** data is added to the **ObjectSet**. The **ObjectSet** flattens the data into a structure-of-arrays representation. The arrays are padded to the SIMD lane count to make sure there's valid data to read.

## Frustum-sphere culling

The world space positions and sphere radii of objects are represented by the following members of the **ObjectSet**:

```C++
Array<float> ws_pos_x;
Array<float> ws_pos_y;
Array<float> ws_pos_z;
Array<float> radius;
```

This is all we need to do frustum-sphere culling.

The frustum-sphere culling needs the planes of the frustum defined in world space. Information on how to find that can be found in: http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf.

The frustum-sphere intersection code tests one plane against several spheres using SIMD instructions. The **ObjectSet** data is already laid out in a SIMD friendly way. To test one plane against several spheres, the plane's data is splatted out in the following way:

```C++
// `float4` is our cross platform abstraction of SSE, NEON etc.
struct SIMDPlane
{
    float4 normal_x; // the normal's x value replicted 4 times.
    float4 normal_y; // the normal's y value replicted 4 times.
    float4 normal_z; // etc.
    float4 d;
};
```

The single threaded code needed to do frustum-sphere culling is:

```C++
void simd_sphere_culling(const SIMDPlane planes[6], culling::ObjectSet &object_set)
{
    const auto all_true = bool4_all_true();
    const uint32_t n_objects = object_set.n_objects;

    uint32_t *visibility_flag = object_set.visibility_flag.begin();

    // Test each plane of the frustum against each sphere.
    for (uint32_t i = 0; i < n_objects; i += 4)
    {
        const auto ws_pos_x = float4_load_aligned(&object_set->ws_pos_x[i]);
        const auto ws_pos_y = float4_load_aligned(&object_set->ws_pos_y[i]);
        const auto ws_pos_z = float4_load_aligned(&object_set->ws_pos_z[i]);
        const auto radius = float4_load_aligned(&object_set->radius[i]);

        auto inside = all_true;
        for (unsigned p = 0; p < 6; ++p) {
            auto &n_x = planes[p].normal_x;
            auto &n_y = planes[p].normal_y;
            auto &n_z = planes[p].normal_z;
            auto n_dot_pos = dot_product(ws_pos_x, ws_pos_y, ws_pos_z, n_x, n_y, n_z);
            auto plane_test_point = n_dot_pos + radius;
            auto plane_test = plane_test_point >= planes[p].d;
            inside = vector_and(plane_test, inside);
        }

        // Store 0 for spheres that didn't intersect or ended up on the positive side of the
        // frustum planes. Store 0xffffffff for spheres that are visible.
        store_aligned(inside, &visibility_flag[i]);
    }
}
```

After the **simd_sphere_culling** call, the **visibility_flag** array contains 0 for all objects that failed the test and **0xffffffff** for all objects that passed. We chain this together with the OOBB culling by doing a compactness pass over the **visibility_flag** array and populating an **indirection** array:

```C++
{
    // Splat out the planes to be able to do plane-sphere test with SIMD.
    const auto &frustum = camera.frustum();

    const SIMDPlane planes[6] = {
        float4_splat(frustum.planes[0].n.x),
        float4_splat(frustum.planes[0].n.y),
        float4_splat(frustum.planes[0].n.z),
        float4_splat(frustum.planes[0].d),

        float4_splat(frustum.planes[1].n.x),
        float4_splat(frustum.planes[1].n.y),
        float4_splat(frustum.planes[1].n.z),
        float4_splat(frustum.planes[1].d),

        float4_splat(frustum.planes[2].n.x),
        float4_splat(frustum.planes[2].n.y),
        float4_splat(frustum.planes[2].n.z),
        float4_splat(frustum.planes[2].d),

        float4_splat(frustum.planes[3].n.x),
        float4_splat(frustum.planes[3].n.y),
        float4_splat(frustum.planes[3].n.z),
        float4_splat(frustum.planes[3].d),

        float4_splat(frustum.planes[4].n.x),
        float4_splat(frustum.planes[4].n.y),
        float4_splat(frustum.planes[4].n.z),
        float4_splat(frustum.planes[4].d),

        float4_splat(frustum.planes[5].n.x),
        float4_splat(frustum.planes[5].n.y),
        float4_splat(frustum.planes[5].n.z),
        float4_splat(frustum.planes[5].d),
    };


    // Do frustum-sphere culling.
    simd_sphere_culling(planes, object_set);

    // Make sure to align the size to the simd lane count.
    const uint32_t n_aligned_objects = align_to_simd_lane_count(object_set.n_objects);

    // Store the indices of the objects that passed the frustum-sphere culling in the `indirection` array.
    Array<uint32_t> indirection(n_aligned_objects);

    const uint32_t n_visible = remove_not_visible(object_set, object_set.n_objects, indirection.begin());
}
```

Where **remove_not_visible** is:

```C++
uint32_t remove_not_visible(const ObjectSet &object_set, uint32_t count, uint32_t *output_indirection)
{
    const uint32_t *visibility_flag = object_set.visibility_flag.begin();
    uint32_t n_visible = 0U;
    for (uint32_t i = 0; i < count; ++i) {
        if (visibility_flag[i]) {
            output_indirection[n_visible] = i;
            ++n_visible;
        }
    }

    const uint32_t n_aligned_visible = align_to_simd_lane_count(n_visible);
    const uint32_t last_visible = n_visible? output_indirection[n_visible- 1] : 0;

    // Pad out to the simd alignment.
    for (unsigned i = n_visible; i < n_aligned_visible; ++i)
        output_indirection[i] = last_visible;

    return n_visible;
}
```

**n_visible** together with **indirection** provides the input for doing the frustum-OOBB culling on the objects that survived the frustum-sphere culling.

## Frustum-OOBB culling

The frustum-OOBB culling takes ideas from Fabian Giesen's https://fgiesen.wordpress.com/2010/10/17/view-frustum-culling/ and Arseny Kapoulkine's http://zeuxcg.org/2009/01/31/view-frustum-culling-optimization-introduction/.

More specifically we use the **Method 2: Transform box vertices to clip space, test against clip-space planes** that both Fabian and Arseny write about. But we also go with **Method 2b: Saving arithmetic ops** that Fabian mentions. I won't dwelve into how the culling actually works, to understand that please read their posts.

The code is SIMDified to process several OOBBs at the same time. The same corner of four multiple OOBBs is tested against one frustum plane as a single SIMD operation.

To be able to write the SIMD code in a more intuitive form a few data structures and functions are used:

```C++
struct SIMDVector
{
    float4 x; // stores x0, x1, x2, x3
    float4 y; // stores y0, y1, y2, y3
    float4 z; // etc.
    float4 w;
};
```

A **SIMDVector** stores x, y, z & w for four objects. To store a matrix for four objects a **SIMDMatrix** is used:

```C++
struct SIMDMatrix
{
    SIMDVector x;
    SIMDVector y;
    SIMDVector z;
    SIMDVector w;
};
```

A **SIMDMatrix**-**SIMDVector** multiplication can then be written as a regular matrix-vector multiplication:

```C++
SIMDVector simd_multiply(const SIMDVector &v, const SIMDMatrix &m)
{
    float4 x = v.x * m.x.x;     x = v.y * m.y.x + x;    x = v.z * m.z.x + x;    x = v.w * m.w.x + x;
    float4 y = v.x * m.x.y;     y = v.y * m.y.y + y;    y = v.z * m.z.y + y;    y = v.w * m.w.y + y;
    float4 z = v.x * m.x.z;     z = v.y * m.y.z + z;    z = v.z * m.z.z + z;    z = v.w * m.w.z + z;
    float4 w = v.x * m.x.w;     w = v.y * m.y.w + w;    w = v.z * m.z.w + w;    w = v.w * m.w.w + w;
    SIMDVector res = { x, y, z, w };
    return res;
}
```

A **SIMDMatrix**-**SIMDMatrix** multiplication is:

```C++
SIMDMatrix simd_multiply(const SIMDMatrix &lhs, const SIMDMatrix &rhs)
{
    SIMDVector x = simd_multiply(lhs.x, rhs);
    SIMDVector y = simd_multiply(lhs.y, rhs);
    SIMDVector z = simd_multiply(lhs.z, rhs);
    SIMDVector w = simd_multiply(lhs.w, rhs);
    SIMDMatrix res = { x, y, z, w };
    return res;
}
```

The code needed to do the actual frustum-OOBB culling is:

```C++
void simd_oobb_culling(const SIMDMatrix &view_proj, const culling::ObjectSet &object_set, uint32_t n_objects, const uint32_t *indirection)
{
    // Get pointers to the necessary members of the object set.
    const float *min_x = object_set->min_x.begin();
    const float *min_y = object_set->min_y.begin();
    const float *min_z = object_set->min_z.begin();

    const float *max_x = object_set->max_x.begin();
    const float *max_y = object_set->max_y.begin();
    const float *max_z = object_set->max_z.begin();

    const float *world_xx = object_set->world_xx.begin();
    const float *world_xy = object_set->world_xy.begin();
    const float *world_xz = object_set->world_xz.begin();
    const float *world_xw = object_set->world_xw.begin();
    const float *world_yx = object_set->world_yx.begin();
    const float *world_yy = object_set->world_yy.begin();
    const float *world_yz = object_set->world_yz.begin();
    const float *world_yw = object_set->world_yw.begin();
    const float *world_zx = object_set->world_zx.begin();
    const float *world_zy = object_set->world_zy.begin();
    const float *world_zz = object_set->world_zz.begin();
    const float *world_zw = object_set->world_zw.begin();
    const float *world_tx = object_set->world_tx.begin();
    const float *world_ty = object_set->world_ty.begin();
    const float *world_tz = object_set->world_tz.begin();
    const float *world_tw = object_set->world_tw.begin();

    uint32_t *visibility_flag = object_set.visibility_flag.begin();

    for (uint32_t i = 0; i < n_objects; i += 4) {
        SIMDMatrix world;

        // Load the world transform matrix for four objects via the indirection table.

        const uint32_t i0 = indirection[i];
        const uint32_t i1 = indirection[i + 1];
        const uint32_t i2 = indirection[i + 2];
        const uint32_t i3 = indirection[i + 3];

        world.x.x = float4(world_xx[i0], world_xx[i1], world_xx[i2], world_xx[i3]);
        world.x.y = float4(world_xy[i0], world_xy[i1], world_xy[i2], world_xy[i3]);
        world.x.z = float4(world_xz[i0], world_xz[i1], world_xz[i2], world_xz[i3]);
        world.x.w = float4(world_xw[i0], world_xw[i1], world_xw[i2], world_xw[i3]);

        world.y.x = float4(world_yx[i0], world_yx[i1], world_yx[i2], world_yx[i3]);
        world.y.y = float4(world_yy[i0], world_yy[i1], world_yy[i2], world_yy[i3]);
        world.y.z = float4(world_yz[i0], world_yz[i1], world_yz[i2], world_yz[i3]);
        world.y.w = float4(world_yw[i0], world_yw[i1], world_yw[i2], world_yw[i3]);

        world.z.x = float4(world_zx[i0], world_zx[i1], world_zx[i2], world_zx[i3]);
        world.z.y = float4(world_zy[i0], world_zy[i1], world_zy[i2], world_zy[i3]);
        world.z.z = float4(world_zz[i0], world_zz[i1], world_zz[i2], world_zz[i3]);
        world.z.w = float4(world_zw[i0], world_zw[i1], world_zw[i2], world_zw[i3]);

        world.w.x = float4(world_tx[i0], world_tx[i1], world_tx[i2], world_tx[i3]);
        world.w.y = float4(world_ty[i0], world_ty[i1], world_ty[i2], world_ty[i3]);
        world.w.z = float4(world_tz[i0], world_tz[i1], world_tz[i2], world_tz[i3]);
        world.w.w = float4(world_tw[i0], world_tw[i1], world_tw[i2], world_tw[i3]);

        // Create the matrix to go from object->world->view->clip space.
        const auto clip = simd_multiply(world, view_proj);

        SIMDVector min_pos;
        SIMDVector max_pos;

        // Load the mininum and maximum corner positions of the bounding box in object space.
        min_pos.x = float4(min_x[i0], min_x[i1], min_x[i2], min_x[i3]);
        min_pos.y = float4(min_y[i0], min_y[i1], min_y[i2], min_y[i3]);
        min_pos.z = float4(min_z[i0], min_z[i1], min_z[i2], min_z[i3]);
        min_pos.w = float4_splat(1.0f);

        max_pos.x = float4(max_x[i0], max_x[i1], max_x[i2], max_x[i3]);
        max_pos.y = float4(max_y[i0], max_y[i1], max_y[i2], max_y[i3]);
        max_pos.z = float4(max_z[i0], max_z[i1], max_z[i2], max_z[i3]);
        max_pos.w = float4_splat(1.0f);

        SIMDVector clip_pos[8];

        // Transform each bounding box corner from object to clip space by sharing calculations.
        simd_min_max_transform(clip, min_pos, max_pos, clip_pos);

        const auto zero = float4_zero();
        const auto all_true = bool4_all_true();

        // Initialize test conditions.
        auto all_x_less = all_true;
        auto all_x_greater = all_true;
        auto all_y_less = all_true;
        auto all_y_greater = all_true;
        auto all_z_less = all_true;
        auto any_z_less = bool4_all_false();
        auto all_z_greater = all_true;

        // Test each corner of the oobb and if any corner intersects the frustum that object
        // is visible.
        for (unsigned cs = 0; cs < 8; ++cs) {
            const auto neg_cs_w = negate(clip_pos[cs].w);

            auto x_le = clip_pos[cs].x <= neg_cs_w;
            auto x_ge = clip_pos[cs].x >= clip_pos[cs].w;
            all_x_less = vector_and(x_le, all_x_less);
            all_x_greater = vector_and(x_ge, all_x_greater);

            auto y_le = clip_pos[cs].y <= neg_cs_w;
            auto y_ge = clip_pos[cs].y >= clip_pos[cs].w;
            all_y_less = vector_and(y_le, all_y_less);
            all_y_greater = vector_and(y_ge, all_y_greater);

            auto z_le = clip_pos[cs].z <= zero;
            auto z_ge = clip_pos[cs].z >= clip_pos[cs].w;
            all_z_less = vector_and(z_le, all_z_less);
            all_z_greater = vector_and(z_ge, all_z_greater);
            any_z_less = vector_or(z_le, any_z_less);
        }

        const auto any_x_outside = vector_or(all_x_less, all_x_greater);
        const auto any_y_outside = vector_or(all_y_less, all_y_greater);
        const auto any_z_outside = vector_or(all_z_less, all_z_greater);
        auto outside = vector_or(any_x_outside, any_y_outside);
        outside = vector_or(outside, any_z_outside);

        auto inside = vector_xor(outside, all_true);

        // Store the result in the `visibility_flag` array in a compacted way.
        store_aligned(inside, &visibility_flag[i]);
    }
}
```

The function **simd_min_max_transforms** used above is the function to transform each OOBB corner from object space to clip space by sharing some of the calculations, for completeness the function is:

```C++
void simd_min_max_transform(const SIMDMatrix &m, const SIMDVector &min, const SIMDVector &max, SIMDVector result[])
{
    auto m_xx_x = m.x.x * min.x;    m_xx_x = m_xx_x + m.w.x;
    auto m_xy_x = m.x.y * min.x;    m_xy_x = m_xy_x + m.w.y;
    auto m_xz_x = m.x.z * min.x;    m_xz_x = m_xz_x + m.w.z;
    auto m_xw_x = m.x.w * min.x;    m_xw_x = m_xw_x + m.w.w;

    auto m_xx_X = m.x.x * max.x;    m_xx_X = m_xx_X + m.w.x;
    auto m_xy_X = m.x.y * max.x;    m_xy_X = m_xy_X + m.w.y;
    auto m_xz_X = m.x.z * max.x;    m_xz_X = m_xz_X + m.w.z;
    auto m_xw_X = m.x.w * max.x;    m_xw_X = m_xw_X + m.w.w;

    auto m_yx_y = m.y.x * min.y;
    auto m_yy_y = m.y.y * min.y;
    auto m_yz_y = m.y.z * min.y;
    auto m_yw_y = m.y.w * min.y;

    auto m_yx_Y = m.y.x * max.y;
    auto m_yy_Y = m.y.y * max.y;
    auto m_yz_Y = m.y.z * max.y;
    auto m_yw_Y = m.y.w * max.y;

    auto m_zx_z = m.z.x * min.z;
    auto m_zy_z = m.z.y * min.z;
    auto m_zz_z = m.z.z * min.z;
    auto m_zw_z = m.z.w * min.z;

    auto m_zx_Z = m.z.x * max.z;
    auto m_zy_Z = m.z.y * max.z;
    auto m_zz_Z = m.z.z * max.z;
    auto m_zw_Z = m.z.w * max.z;

    {
        auto xyz_x = m_xx_x + m_yx_y;   xyz_x = xyz_x + m_zx_z;
        auto xyz_y = m_xy_x + m_yy_y;   xyz_y = xyz_y + m_zy_z;
        auto xyz_z = m_xz_x + m_yz_y;   xyz_z = xyz_z + m_zz_z;
        auto xyz_w = m_xw_x + m_yw_y;   xyz_w = xyz_w + m_zw_z;
        result[0].x = xyz_x;
        result[0].y = xyz_y;
        result[0].z = xyz_z;
        result[0].w = xyz_w;
    }

    {
        auto Xyz_x = m_xx_X + m_yx_y;   Xyz_x = Xyz_x + m_zx_z;
        auto Xyz_y = m_xy_X + m_yy_y;   Xyz_y = Xyz_y + m_zy_z;
        auto Xyz_z = m_xz_X + m_yz_y;   Xyz_z = Xyz_z + m_zz_z;
        auto Xyz_w = m_xw_X + m_yw_y;   Xyz_w = Xyz_w + m_zw_z;
        result[1].x = Xyz_x;
        result[1].y = Xyz_y;
        result[1].z = Xyz_z;
        result[1].w = Xyz_w;
    }

    {
        auto xYz_x = m_xx_x + m_yx_Y;   xYz_x = xYz_x + m_zx_z;
        auto xYz_y = m_xy_x + m_yy_Y;   xYz_y = xYz_y + m_zy_z;
        auto xYz_z = m_xz_x + m_yz_Y;   xYz_z = xYz_z + m_zz_z;
        auto xYz_w = m_xw_x + m_yw_Y;   xYz_w = xYz_w + m_zw_z;
        result[2].x = xYz_x;
        result[2].y = xYz_y;
        result[2].z = xYz_z;
        result[2].w = xYz_w;
    }

    {
        auto XYz_x = m_xx_X + m_yx_Y;   XYz_x = XYz_x + m_zx_z;
        auto XYz_y = m_xy_X + m_yy_Y;   XYz_y = XYz_y + m_zy_z;
        auto XYz_z = m_xz_X + m_yz_Y;   XYz_z = XYz_z + m_zz_z;
        auto XYz_w = m_xw_X + m_yw_Y;   XYz_w = XYz_w + m_zw_z;
        result[3].x = XYz_x;
        result[3].y = XYz_y;
        result[3].z = XYz_z;
        result[3].w = XYz_w;
    }

    {
        auto xyZ_x = m_xx_x + m_yx_y;   xyZ_x = xyZ_x + m_zx_Z;
        auto xyZ_y = m_xy_x + m_yy_y;   xyZ_y = xyZ_y + m_zy_Z;
        auto xyZ_z = m_xz_x + m_yz_y;   xyZ_z = xyZ_z + m_zz_Z;
        auto xyZ_w = m_xw_x + m_yw_y;   xyZ_w = xyZ_w + m_zw_Z;
        result[4].x = xyZ_x;
        result[4].y = xyZ_y;
        result[4].z = xyZ_z;
        result[4].w = xyZ_w;
    }

    {
        auto XyZ_x = m_xx_X + m_yx_y;   XyZ_x = XyZ_x + m_zx_Z;
        auto XyZ_y = m_xy_X + m_yy_y;   XyZ_y = XyZ_y + m_zy_Z;
        auto XyZ_z = m_xz_X + m_yz_y;   XyZ_z = XyZ_z + m_zz_Z;
        auto XyZ_w = m_xw_X + m_yw_y;   XyZ_w = XyZ_w + m_zw_Z;
        result[5].x = XyZ_x;
        result[5].y = XyZ_y;
        result[5].z = XyZ_z;
        result[5].w = XyZ_w;
    }

    {
        auto xYZ_x = m_xx_x + m_yx_Y;   xYZ_x = xYZ_x + m_zx_Z;
        auto xYZ_y = m_xy_x + m_yy_Y;   xYZ_y = xYZ_y + m_zy_Z;
        auto xYZ_z = m_xz_x + m_yz_Y;   xYZ_z = xYZ_z + m_zz_Z;
        auto xYZ_w = m_xw_x + m_yw_Y;   xYZ_w = xYZ_w + m_zw_Z;
        result[6].x = xYZ_x;
        result[6].y = xYZ_y;
        result[6].z = xYZ_z;
        result[6].w = xYZ_w;
    }

    {
        auto XYZ_x = m_xx_X + m_yx_Y;   XYZ_x = XYZ_x + m_zx_Z;
        auto XYZ_y = m_xy_X + m_yy_Y;   XYZ_y = XYZ_y + m_zy_Z;
        auto XYZ_z = m_xz_X + m_yz_Y;   XYZ_z = XYZ_z + m_zz_Z;
        auto XYZ_w = m_xw_X + m_yw_Y;   XYZ_w = XYZ_w + m_zw_Z;
        result[7].x = XYZ_x;
        result[7].y = XYZ_y;
        result[7].z = XYZ_z;
        result[7].w = XYZ_w;
    }
}
```

To get a compact indirection array of all the objects that passed the frustum-OOBB culling, the **remove_not_visible** function needs to be slightly modified:

```C++
uint32_t remove_not_visible(const ObjectSet &object_set, uint32_t count, uint32_t *output_indirection, const uint32_t *input_indirection/*new argument*/)
{
    const uint32_t *visibility_flag = object_set.visibility_flag.begin();
    uint32_t n_visible = 0U;
    for (uint32_t i = 0; i < count; ++i) {

        // Each element of `input_indirection` represents an object that has either been culled
        // or not culled. If it's not null then do a lookup to get the actual object index else
        // use `i` directly.
        const uint32_t index = input_indirection? input_indirection[i] : i;

        // `visibility_flag` is already compacted, so use `i` directly.
        if (visibility_flag[i]) {
            output_indirection[n_visible] = i;
            ++n_visible;
        }
    }

    const uint32_t n_aligned_visible = align_to_simd_lane_count(n_visible);
    const uint32_t last_visible = n_visible? output_indirection[n_visible- 1] : 0;

    // Pad out to the simd alignment.
    for (unsigned i = n_visible; i < n_aligned_visible; ++i)
        output_indirection[i] = last_visible;

    return n_visible;
}
```

Bringing the frustum-sphere and frustum-OOBB code together we get:

```C++
{
    // Splat out the planes to be able to do plane-sphere test with SIMD.
    const auto &frustum = camera.frustum();

    const SIMDPlane planes[6] = {
        float4_splat(frustum.planes[0].n.x),
        float4_splat(frustum.planes[0].n.y),
        float4_splat(frustum.planes[0].n.z),
        float4_splat(frustum.planes[0].d),

        float4_splat(frustum.planes[1].n.x),
        float4_splat(frustum.planes[1].n.y),
        float4_splat(frustum.planes[1].n.z),
        float4_splat(frustum.planes[1].d),

        float4_splat(frustum.planes[2].n.x),
        float4_splat(frustum.planes[2].n.y),
        float4_splat(frustum.planes[2].n.z),
        float4_splat(frustum.planes[2].d),

        float4_splat(frustum.planes[3].n.x),
        float4_splat(frustum.planes[3].n.y),
        float4_splat(frustum.planes[3].n.z),
        float4_splat(frustum.planes[3].d),

        float4_splat(frustum.planes[4].n.x),
        float4_splat(frustum.planes[4].n.y),
        float4_splat(frustum.planes[4].n.z),
        float4_splat(frustum.planes[4].d),

        float4_splat(frustum.planes[5].n.x),
        float4_splat(frustum.planes[5].n.y),
        float4_splat(frustum.planes[5].n.z),
        float4_splat(frustum.planes[5].d),
    };

    // Do frustum-sphere culling.
    simd_sphere_culling(planes, object_set);

    // Make sure to align the size to the simd lane count.
    const uint32_t n_aligned_objects = align_to_simd_lane_count(object_set.n_objects);

    // Store the indices of the objects that passed the frustum-sphere culling in the `indirection` array.
    Array<uint32_t> indirection(n_aligned_objects);

    const uint32_t n_visible = remove_not_visible(object_set, object_set.n_objects, indirection.begin(), nullptr);

    const auto &view_proj = camera.view() * camera.proj();

    // Construct the SIMDMatrix `simd_view_proj`.
    const SIMDMatrix simd_view_proj = {
        float4_splat(view_proj.v[xx]),
        float4_splat(view_proj.v[xy]),
        float4_splat(view_proj.v[xz]),
        float4_splat(view_proj.v[xw]),

        float4_splat(view_proj.v[yx]),
        float4_splat(view_proj.v[yy]),
        float4_splat(view_proj.v[yz]),
        float4_splat(view_proj.v[yw]),

        float4_splat(view_proj.v[zx]),
        float4_splat(view_proj.v[zy]),
        float4_splat(view_proj.v[zz]),
        float4_splat(view_proj.v[zw]),

        float4_splat(view_proj.v[tx]),
        float4_splat(view_proj.v[ty]),
        float4_splat(view_proj.v[tz]),
        float4_splat(view_proj.v[tw]),
    };

    // Cull objects via frustum-oobb tests.
    simd_oobb_culling(simd_view_proj, object_set, n_visible, indirection.begin());

    // Build up the indirection array that represents the objects that survived the frustum-oobb culling.
    const uint32_t n_oobb_visible = remove_not_visible(object_set, n_visible, indirection.begin(), indirection.begin());
}
```

The final call to **remove_not_visible** populates the **indirection** array with the objects that passed both the frustum-sphere and the frustum-OOBB culling. **indirection** together with **n_oobb_visible** is all that is needed to know what objects should be rendered.

## Distributing the work over several threads

In Stingray, work is distributed by submitting jobs to a pool of worker threads -- conveniently called the **ThreadPool**. Submitted jobs are put in a thread safe work queue from which the worker threads pop jobs to work on. A task is defined as:

```C++
typedef void (*TaskCallback)(void *user_data);

struct TaskDefinition
{
    TaskCallback callback;
    void *user_data;
};
```

For the purpose of this article, the interesting methods of the **ThreadPool** are:

```C++
class ThreadPool
{
    // Adds `count` tasks to the work queue.
    void add_tasks(const TaskDefinition *tasks, uint32_t count);

    // Tries to pop one task from the queue and do that work. Returns true if any work was done.
    bool do_work();

    // Will call `do_work` while `signal` == value.
    void wait_atomic(std::atomic<uint32_t> *signal, uint32_t value);
};
```

The **ThreadPool** doesn't dictate how to synchronize when a job is fully processed, but usually a **std::atomic<uint32_t> signal** is used for that purpose. The value is 0 while the job is being processed and set to 1 when it's done. **wait_atomic()** is a convenience method that can be used to wait for such values:

```C++
void ThreadPool::wait_atomic(std::atomic<uint32_t> *signal, uint32_t value)
{
    while (signal->load(std::memory_order_acquire) == value) {
        if (!do_work())
            YieldProcessor();
    }
}
```

do_work:

```C++
bool ThreadPool::do_work()
{
    TaskDefinition task;
    if (pop_task(task)) {
        task.callback(task.user_data);
        return true;
    }
    return false;
}
```

Multi-threading the culling only requires a few changes to the code. For the **simd_sphere_culling()** method we need to add **offset** and **count** parameters to specify the range of objects we are processing:

```C++
void simd_sphere_culling(const SIMDPlane planes[6], culling::ObjectSet &object_set, uint32_t offset, uint32_t count)
{
    const auto all_true = bool4_all_true();
    const uint32_t n_objects = offset + count;

    uint32_t *visibility_flag = object_set.visibility_flag.begin();

    // Test each plane of the frustum against each sphere.
    for (uint32_t i = offset; i < n_objects; i += 4)
    {
        const auto ws_pos_x = float4_load_aligned(&object_set->ws_pos_x[i]);
        const auto ws_pos_y = float4_load_aligned(&object_set->ws_pos_y[i]);
        const auto ws_pos_z = float4_load_aligned(&object_set->ws_pos_z[i]);
        const auto radius = float4_load_aligned(&object_set->radius[i]);

        auto inside = all_true;
        for (unsigned p = 0; p < 6; ++p) {
            auto &n_x = planes[p].normal_x;
            auto &n_y = planes[p].normal_y;
            auto &n_z = planes[p].normal_z;
            auto n_dot_pos = dot_product(ws_pos_x, ws_pos_y, ws_pos_z, n_x, n_y, n_z);
            auto plane_test_point = n_dot_pos + radius;
            auto plane_test = plane_test_point >= planes[p].d;
            inside = vector_and(plane_test, inside);
        }

        // Store 0 for spheres that didn't intersect or ended up on the positive side of the
        // frustum planes. Store 0xffffffff for spheres that are visible.
        store_aligned(inside, &visibility_flag[i]);
    }
}
```

Bringing the previous code snippet together with multi-threaded culling:

```C++
{
    // Calculate the number of work items based on that each work will process `work_size` elements.
    const uint32_t work_size = 512;

    // `div_ceil(a, b)` calculates `(a + b - 1) / b`.
    const uint32_t n_work_items = math::div_ceil(n_objects, work_size);

    Array<CullingWorkItem> culling_work_items(n_work_items);
    Array<TaskDefinition> tasks(n_work_items);

    // Splat out the planes to be able to do plane-sphere test with SIMD.
    const auto &frustum = camera.frustum();

    const SIMDPlane planes[6] = {
        same code as previously shown...
    };

    // Make sure to align the size to the simd lane count.
    const uint32_t n_aligned_objects = align_to_simd_lane_count(object_set.n_objects);

    for (unsigned i = 0; i < n_work_items; ++i) {

        // The `offset` and `count` for the work item.
        const uint32_t offset = math::min(work_size * i, n_objects);
        const uint32_t count = math::min(work_size, n_objects - offset);

        auto &culling_item = culling_work_items[i];
        memcpy(culling_data.planes, planes, sizeof(planes));
        culling_item.object_set = &object_set;
        culling_item.offset = offset;
        culling_item.count = count;
        culling_item.signal = 0;

        auto &task = tasks[i];
        task.callback = simd_sphere_culling_task;
        task.user_data = &culling_item;
    }

    // Add the tasks to the `ThreadPool`.
    thread_pool.add_tasks(n_work_items, tasks.begin());
    // Wait for each `item` and if it's not done, help out with the culling work.
    for (auto &item : culling_work_items)
        thread_pool.wait_atomic(&item.signal, 0);
}
```

**CullingWorkItem** and **simd_sphere_culling_task** are defined as:

```C++
struct CullingWorkItem
{
    SIMDPlane planes[6];
    const culling::ObjectSet *object_set;
    uint32_t offset;
    uint32_t count;
    std::atomic<uint32_t> signal;
};

void simd_sphere_culling_task(void *user_data)
{
    auto culling_item = (CullingWorkItem*)(user_data);

    // Call the frustum-sphere culling function.
    simd_sphere_culling(culling_item->planes, *culling_item->object_set, culling_item->offset, culling_item->count);

    // Signal that the work is done.
    culling_item->store(1, std::memory_order_release);
}
```

The same pattern is used to multi-thread the frustum-OOBB culling. That is "left as an exercise for the reader" ;)

## Conclusion

This type of culling is done for all of the objects that can be rendered, i.e. meshes, particle systems, terrain, etc. We also use it to cull light sources. It is used both when rendering the main scene and for rendering shadows.

I've left out a few details of our solution. One thing we also do is something called contribution culling. In the frustum-OOBB culling step, the extents of the OOBB corners are projected to the near plane and from that the screen space extents are derived. If the object is smaller than a certain threshold in any axis the object is considered as culled. Special care needs to be considered if any of the corners intersect or is behind the near plane so we don't have to deal with "external line segments" caused by the projection. If you don't know what that is see: http://www.gamasutra.com/view/news/168577/Indepth_Software_rasterizer_and_triangle_clipping.php. In our case the contribution culling is disabled by expanding the extents to span the entire screen when any corner intersects or is behind the near plane.

For our cascaded shadow maps, the extents are also used to detect if an object is fully enclosed by a cascade. If that is the case, then that object is culled from the later cascades. Let me illustrate with some ASCII:

```
+-----------+-----------+
|           |           |
|     /\    |           |
|    /--\   |           |
+-----------+-----------+
|           |           |
|           |           |
|           |           |
+-----------+-----------+
```

The squares are the different cascades. The top left square is the first cascades, the top right is the second cascade, bottom left the third and the bottom right is the fourth cascade. In this case the weird triangle shaped object is fully enclosed by the first cascade. What that means is that the object doesn't need to be rendered to any of the later cascades, since the shadow contribution from that object will be fully taken care of from the first cascade.

