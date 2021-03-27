# Part 10: Threading and timing


## Threading

* Goal of threading
  * Maximize throughput
  * Minimize latency
* Cores as busy as possible
  * Doing actual work


## Thread model

* Two pipelined + worker threads:

```
---- main #23 -----\ ---- main #24 -------
---- render #22 --- \---- render #23 -----

--- worker -------------------------------
--- worker -------------------------------
--- worker -------------------------------
--- worker -------------------------------

--- async task ---------------------------
--- async task ---------------------------
```

* Both main and render use worker threads to process jobs
* Latency 2 x frame time (+ display lag)
* Both rendering and the main update have single threaded parts
  * Pipelining allows these "holes" to be filled


## Main & render thread synchronization

* Render thread has copy of the mutable state (worlds, objects, etc)
  * Only the stuff needed by the renderer
* When main object is created / modified / destroyed mirror commands are posted
  * CREATE(id, type, info)
  * MODIFY(id, changes)
  * DESTROY(id)
* Renderer uses this to update its state
* Since states are separate, deleting the main state is ok
* Immutable state (geometry buffers, etc) is not copied
  * Care is taken to synchronize creation/deletion


## Worker threads

* We spawn as many of these as we need to get one thread/core
* Main and render threads post jobs to a queue
* Worker threads pull jobs from queue
* Main and render can wait for a particular job to finish
* While they are waiting, they work on jobs from the queue
* Typically we branch and join quickly
* `animation_player.cpp`
* AnimationPlayer::update()
  * Gather all animations that need to updated
  * Post N animation jobs
  * Wait for jobs to finish
* Waiting later would let us utilize the threads better
  * But it is harder to reason about

