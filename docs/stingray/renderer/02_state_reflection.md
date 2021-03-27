# State Reflection

## Overview

The Stingray engine has two controller threads -- the main thread and the render thread. These two threads build up work for our job system, which is distributed on the remaining threads. The main thread and the render thread are pipelined, so that while the main thread runs the simulation/update for frame *N*, the render thread is processing the rendering work for the previous frame *(N-1)*. This post will dive into the details how state is propagated from the main thread to the render thread.

I will use code snippets to explain how the state reflection works. It's mostly actual code from the engine but it has been cleaned up to a certain extent. Some stuff has been renamed and/or removed to make it easier to understand what's going on.

## The main loop

Here is a slimmed down version of the update loop which is part of the main thread:

```C++
while (!quit())
{
    // Calls out to the mandatory user supplied `update` Lua function, Lua is used 
    // as a scripting language to manipulate objects. From Lua worlds, objects etc
    // can be created, manipulated, destroyed, etc. All these changes are recorded
    // on a `StateStream` that is a part of each world.
    _game->update();

    // Flush state changes recorded on the `StateStream` for each world to
    // the rendering world representation.
    unsigned n_worlds = _worlds.size();
    for (uint32_t i = 0; i < n_worlds; ++i) {
        auto &world = *_worlds[i];
        _render_interface->update_world(world);
    }

    // Begin a new render frame.
    _render_interface->begin_frame();

    // Calls out to the user supplied `render` Lua function. It's up to the script
    // to call render on worlds(). The script controls what camera and viewport
    // are used when rendering the world.
    _game->render();

    // Present the frame.
    _render_interface->present_frame();

    // End frame.
    _render_interface->end_frame(_delta_time);

    // Never let the main thread run more than 1 frame a head of the render thread.
    _render_interface->wait_for_fence(_frame_fence);

    // Create a new fence for the next frame.
    _frame_fence = _render_interface->create_fence();
}
```

First thing to point out is the **_render_interface**. This is not a class full of virtual functions that some other class can inherit from and override as the name might suggest. The word "interface" is used in the sense that it's used to communicate from one thread to another. So in this context the **_render_interface** is used to post messages from the main thread to the render thread.

As said in the first comment in the code snippet above, Lua is used as our scripting language and from Lua things such as worlds, objects, etc can be created, destroyed, manipulated, etc.

The state between the main thread and the render thread is very rarely shared, instead each thread has its own representation and when state is changed on the main thread that state is reflected over to the render thread. E.g., the **MeshObject**, which is the representation of a mesh with vertex buffers, materials, textures, shaders, skinning, data etc to be rendered, is the main thread representation and **RenderMeshObject** is the corresponding render thread representation. All objects that have a representation on both the main and render thread are setup to work the same way:

```C++
class MeshObject : public RenderStateObject
{
};

class RenderMeshObject : public RenderObject
{
};
```

The corresponding render thread class is prefixed with **Render**. We use this naming convention for all objects that have both a main and a render thread representation.

The main thread objects inherit from **RenderStateObject** and the render thread objects inherit from **RenderObject**. These structs are defined as:

```C++
struct RenderStateObject
{
    uint32_t render_handle;
    StateReflection *state_reflection;
};

struct RenderObject
{
    uint32_t type;
};
```

The **render_handle** is an ID that identifies the corresponding object on the render thread. **state_reflection** is a stream of data that is used to propagate state changes from the main thread to the render thread. **type** is an enum used to identify the type of render objects.

## Object creation

In Stingray a *world* is a container of renderable objects, physical objects, sounds, etc. On the main thread, it is represented by the **World** class, and on the render thread by a **RenderWorld**.

When a **MeshObject** is created in a world on the main thread, there's an explicit call to **WorldRenderInterface::create()** to create the corresponding render thread representation:

```C++
MeshObject *mesh_object = MAKE_NEW(_allocator, MeshObject);
_world_render_interface.create(mesh_object);
```

The purpose of the call to **WorldRenderInterface::create** is to explicitly create the render thread representation, acquire a **render_handle** and to post that to the render thread:

```C++
void WorldRenderInterface::create(MeshObject *mesh_object)
{
    // Get a unique render handle.
    mesh_object->render_handle = new_render_handle();

    // Set the state_reflection pointer, more about this later.
    mesh_object->state_reflection = &_state_reflection;

    // Create the render thread representation.
    RenderMeshObject *render_mesh_object = MAKE_NEW(_allocator, RenderMeshObject);

    // Pass the data to the render thread
    create_object(mesh_object->render_handle, RenderMeshObject::TYPE, render_mesh_object);
}
```

The **new_render_handle** function speaks for itself.

```C++
uint32_t WorldRenderInterface::new_render_handle()
{
    if (_free_render_handles.any()) {
        uint32_t handle = _free_render_handles.back();
        _free_render_handles.pop_back();
        return handle;
    } else
        return _render_handle++;
}
```

There is a recycling mechanism for the render handles and a similar pattern reoccurs at several places in the engine. The **release_render_handle** function together with the **new_render_handle** function should give the complete picture of how it works.

```C++
void WorlRenderInterface::release_render_handle(uint32_t handle)
{
    _free_render_handles.push_back(handle);
}
```

There is one **WorldRenderInterface** per world which contains the **_state_reflection** that is used by the world and all of its objects to communicate with the render thread. The StateReflection in its simplest form is defined as:

```C++
struct StateReflection
{
    StateStream *state_stream;
};
```

The **create_object** function needs a bit more explanation though:

```C++
void WorldRenderInterface::create_object(uint32_t render_handle, RenderObject::Type type, void *user_data)
{
    // Allocate a message on the `state_stream`.
    ObjectManagementPackage *omp;
    alloc_message(_state_reflection.state_stream, WorldRenderInterface::CREATE, &omp);

    omp->object_type = RenderWorld::TYPE;
    omp->render_handle = render_handle;
    omp->type = type;
    omp->user_data = user_data;
}
```

What happens here is that **alloc_message** will allocate enough bytes to make room for a **MessageHeader** together with the size of **ObjectManagementPackage** in a buffer owned by the **StateStream**. The **StateStream** is defined as:

```C++
struct StateStream
{
    void *buffer;
    uint32_t capacity;
    uint32_t size;
};
```

**capacity** is the size of the memory pointed to by **buffer**, **size** is the current amount of bytes allocated from **buffer**.

The **MessageHeader** is defined as:

```C++
struct MessageHeader
{
    uint32_t type;
    uint32_t size;
    uint32_t data_offset;
};
```

The **alloc_message** function will first place the **MessageHeader** and then comes the **data**, some ASCII to the rescue:

```
+-------------------------------------------------------------------+
| MessageHeader | data                                              |
+-------------------------------------------------------------------+
<- data_offset ->
<-                          size                                   ->
```

The **size** and **data_offset** mentioned in the ASCII are two of the members of MessageHeader, these are assigned during the **alloc_message** call:

```C++
template<Class T>
void alloc_message(StateStream *state_stream, uint32_t type, T **data)
{
    uint32_t data_size = sizeof(T);

    uint32_t message_size = sizeof(MessageHeader) + data_size;

    // Allocate message and fill in the header.
    void *buffer = allocate(state_stream, message_size, alignof(MessageHeader));
    auto header = (MessageHeader*)buffer;

    header->type = type;
    header->size = message_size;
    header->data_offset = sizeof(MessageHeader);

    *data = memory_utilities::pointer_add(buffer, header->data_offset);
}
```

The **buffer** member of the **StateStream** will contain several consecutive chunks of message headers and data blocks.

```
+-----------------------------------------------------------------------+
| Header | data | Header | data | Header | data | Header | data | etc   |
+-----------------------------------------------------------------------+
```

This is the necessary code on the main thread to create an object and populate the **StateStream** which will later on be consumed by the render thread. A very similar pattern is used when changing the state of an object on the main thread, e.g:

```C++
void MeshObject::set_flags(renderable::Flags flags)
{
    _flags = flags;

    // Allocate a message on the `state_stream`.
    SetVisibilityPackage *svp;
    alloc_message(state_reflection->state_stream, MeshObject::SET_VISIBILITY, &svp);

    // Fill in message information.
    svp->object_type = RenderMeshObject::TYPE;

    // The render handle that got assigned in `WorldRenderInterface::create`
    // to be able to associate the main thread object with its render thread 
    // representation.
    svp->handle = render_handle;

    // The new flags value.
    svp->flags = _flags;
}
```

## Getting the recorded state to the render thread

Let's take a step back and explain what happens in the main update loop during the following code excerpt:

```C++
// Flush state changes recorded on the `StateStream` for each world to
// the rendering world representation.
unsigned n_worlds = _worlds.size();
for (uint32_t i = 0; i < n_worlds; ++i) {
    auto &world = *_worlds[i];
    _render_interface->update_world(world);
}
```

When Lua has been creating, destroying, manipulating, etc objects during **update()** and is done, each world's **StateStream** which contains all the recorded changes is ready to be sent over to the render thread for consumption. The call to **RenderInterface::update_world()** will do just that, it roughly looks like:

```C++
void RenderInterface::update_world(World &world)
{
    UpdateWorldMsg uw;

    // Get the render thread representation of the `world`.
    uw.render_world = render_world_representation(world);

    // The world's current `state_stream` that contains all changes made 
    // on the main thread.
    uw.state_stream = world->_world_reflection_interface.state_stream;

    // Create and assign a new `state_stream` to the world's `_world_reflection_interface`
    // that will be used for the next frame.
    world->_world_reflection_interface->state_stream = new_state_stream();

    // Post a message to the render thread to update the world.
    post_message(UPDATE_WORLD, &uw);
}
```

This function will create a new message and post it to the render thread. The world being flushed and its **StateStream** are stored in the message and a new **StateStream** is created that will be used for the next frame. This new **StateStream** is set on the **WorldRenderInterface** of the World, and since all objects being created got a pointer to the same **WorldRenderInterface** they will use the newly created **StateStream** when storing state changes for the next frame.

## Render thread

The render thread is spinning in a message loop:

```C++
void RenderInterface::render_thread_entry()
{
    while (!_quit) {
        // If there's no message -- put the thread to sleep until there's
        // a new message to consume.
        RenderMessage *message = get_message();

        void *data = data(message);
        switch (message->type) {
            case UPDATE_WORLD:
                internal_update_world((UpdateWorldMsg*)(data));
                break;

            // ... And a lot more case statements to handle different messages. There
            // are other threads than the main thread that also communicate with the
            // render thread. E.g., the resource loading happens on its own thread
            // and will post messages to the render thread.
        }
    }
}
```

The **internal_update_world()** function is defined as:

```C++
void RenderInterface::internal_update_world(UpdateWorldMsg *uw)
{
    // Call update on the `render_world` with the `state_stream` as argument.
    uw->render_world->update(uw->state_stream);

    // Release and recycle the `state_stream`.
    release_state_stream(uw->state_stream);
}
```

It calls **update()** on the **RenderWorld** with the **StateStream** and when that is done the **StateStream** is released to a pool.

```C++
void RenderWorld::update(StateStream *state_stream)
{
    MessageHeader *message_header;
    StatePackageHeader *package_header;

    // Consume a message and get the `message_header` and `package_header`.
    while (get_message(state_stream, &message_header, (void**)&package_header)) {
        switch (package_header->object_type) {
            case RenderWorld::TYPE:
            {
                auto omp = (WorldRenderInterface::ObjectManagementPackage*)package_header;
                // The call to `WorldRenderInterface::create` created this message.
                if (message_header->type == WorldRenderInterface::CREATE)
                    create_object(omp);
            }
            case (RenderMeshObject::TYPE)
            {
                if (message_header->type == MeshObject::SET_VISIBILITY) {
                    auto svp = (MeshObject::SetVisibilityPackage*>)package_header;

                    // The `render_handle` is used to do a lookup in `_objects_lut` to
                    // to get the `object_index`.
                    uint32_t object_index = _object_lut[package_header->render_handle];

                    // Get the `render_object`.
                    void *render_object = _objects[object_index];

                    // Cast it since the type is already given from the `object_type`
                    // in the `package_header`.
                    auto rmo = (RenderMeshObject*)render_object;

                    // Call update on the `RenderMeshObject`.
                    rmo->update(message_header->type, package_header);
                }
            }
            // ... And a lot more case statements to handle different kind of messages.
        }
    }
}
```

The above is mostly infrastructure to extract messages from the **StateStream**. It can be a bit involved since a lot of stuff is written out explicitly but the basic idea is hopefully simple and easy to understand.

On to the **create_object** call done when **(message_header->type == WorldRenderInterface::CREATE)** is satisfied:

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

So the take away from the code above lies in the general usage of the **render_handle** and the **object_index**. The **render_handle** of objects are used to do a look up in **_object_lut** to get the **object_index** and **type**. Let's look at an example, the same **RenderWorld::update** code presented earlier but this time the focus is when the message is **MeshObject::SET_VISIBILITY**:

```C++
void RenderWorld::update(StateStream *state_stream)
{
    StateStream::MessageHeader *message_header;
    StatePackageHeader *package_header;

    while (get_message(state_stream, &message_header, (void**)&package_header)) {
        switch (package_header->object_type) {
            case (RenderMeshObject::TYPE)
            {
                if (message_header->type == MeshObject::SET_VISIBILITY) {
                    auto svp = (MeshObject::SetVisibilityPackage*>)package_header;

                    // The `render_handle` is used to do a lookup in `_objects_lut` to
                    // to get the `object_index`.
                    uint32_t object_index = _object_lut[package_header->render_handle];

                    // Get the `render_object` from the `object_index`.
                    void *render_object = _objects[object_index];

                    // Cast it since the type is already given from the `object_type`
                    // in the `package_header`.
                    auto rmo = (RenderMeshObject*)render_object;

                    // Call update on the `RenderMeshObject`.
                    rmo->update(message_header->type, svp);
                }
            }
        }
    }
}
```

The state reflection pattern shown in this post is a fundamental part of the engine. Similar patterns appear in other places as well and having a good understanding of this pattern makes it much easier to understand the internals of the engine.
