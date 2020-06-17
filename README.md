TLDR; Ruby is awesome, Sidekiq is great. Awesome Farm is an attempt to adopt (clone) sidekiq, written in C, and used to serve C programs.

# Awesome Farm

I'm a C newbie from the Ruby land. When working with Ruby, I always feel that the language is trying to make developers happy, in exchange for a lot of abstractions, performance penalties, etc. There aren't much similarities between C and Ruby. One common pattern used in Ruby is to leverage background job system to offload heavy tasks out of latency-sensitive tasks. There are some major players in Ruby land supporting this pattern is [Sidekiq](https://sidekiq.org/). It is a multi-threaded background processing system that provides a lot of magic to simplify the interface and boilerplate. Just simple like this:

```ruby
class HardWorker
  include Sidekiq::Worker

  def perform(name, count)
    puts "Hello #{name}. You have #{count} apples."
  end
end
```

```
HardWorker.perform_async('bob', 5)
```

The data type encapsulation, message queuing, function call dispatching is faded from the developers so that they just need to focus in the processing logic. Besides, Sidekiq provides many advanced features, such as automatically retrying, advanced time scheduling, logging, etc.

All of the above makes me wonder, what if I adopt (clone) sidekiq features and move it to C? That's why I create Awesome Farm, which is a multi-threaded, automatic data-encapsulating background job system for C.

# Requirements
To build from source, you'll need those dependencies:
- [Hiredis](https://github.com/redis/hiredis)
- [Libffi](https://github.com/libffi/libffi)
- [Msgpack](https://github.com/msgpack/msgpack)

At runtime, you'll need to setup a redis server, and change the configuration of processing server.

# Getting started

A typical work flow involves 3 objects:
- Worker definition, shared between client and server.
- Client, whatever web server, real time chat servers, etc. that needs to offload its heavy tasks.
- Server, the main process to handle background jobs.

First, you'll need to declare the worker signature in a header file, and its definition in a source file.

```c
// In test_worker.h
AF_DECLARE_WORKER(worker_1, char *, int)
AF_DECLARE_WORKER(worker_2, int, int)

// In test_worker.c
AF_WORKER(worker_1, char * a, int b) {
    printf("worker_1: a = %s, b = %i\n", a, b);
}

AF_WORKER(worker_2, int a, int b) {
    printf("worker_2: a = %i, b = %i\n", a, b);
}
```

Next, anywhere in the client, you can schedule a background job:

```c
AF_SCHEDULE(worker_1, result, str, i);
AF_SCHEDULE(worker_2, result, i, i * 3);
```

That's it. The interface of Awesome Farm is elegant, and simple enough. Although there are still some boilerplates and longer than Ruby's original system.

The full example source code and configuration is located at [https://github.com/nguyenquangminh0711/awesome-farm/tree/master/test](https://github.com/nguyenquangminh0711/awesome-farm/tree/master/test) folder. You can bring it up with:

```
make test_client
make test_server
```

# How it works
- The jobs are queued into Redis server. The job server pulls the data from Redis server. Redis acts as a lock to prevent race condition when polling jobs.
- All the data encoding/decoding is handled by Msgpack automatically. Sadly, only integer and string are supported that this alpha version.
- Awesome Farm implement `AF_DECLARE_WORKER` and `AF_WORKER` macros to define jobs and its hidden helpers.
- When you call `AF_SCHEDULE`, type validation is triggered, the program packs the arguments into transferable binary (using msgpack), and push to Redis.
- A job server thread pulls, decode, and dispatch a job dynamically thanks to FFI.

# Reliability
- Job server has some mechanisms to prevent job processing error (like segment faults, devided by zero, etc.) and auto-recovery afterward. Failed jobs will be pushed back to Redis to debug/re-run later.
- By default, job server employs a timeout for each job execution, and a graceful timeout when the process shutdowns. Failed jobs will be pushed back to Redis to debug/re-run later.
- [Comming soon] `AF_CHECKPOINT` supports atomic, multi-state, and idempotent processing with ease.
- [Comming soon] If the job server runs under cluster mode, each process in the cluster is replaced periodically when the server is idle. This is to prevent memory leaks.
- [Comming soon] If the job server is killed unexpected (SIGKILL for example), the job is guranteed to be restored and handled later.

# Personal gains and thoughts
- I know that the whole implementation is so naive, as I am just a newbie in C, and gonna learn a lot of things to make it better.
- I learned a lot about multi-thread processing, virtual address space, debugging with GDB, trace memory leaks, etc. It's awesome.
- Applying high-level programming language pragmatics and design patterns into low-level programming such as C is extremely hard, and stratined. The system is not at the level of conveniency as I expected.
- Making this sytem reliable regardless of user's program exution is hard. I am adoting some native mechanisms to increase the level of reliability, but they are all easy to break. Maybe I'll need higher level of isolation or even containers in future.
