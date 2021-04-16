#ifndef NODEGIT_CONTEXT
#define NODEGIT_CONTEXT

#include <map>
#include <memory>
#include <nan.h>
#include <string>
#include <uv.h>
#include <v8.h>

/*
 * Determine if node module is compiled under a supported node release.
 * Currently 12 - 15 (ignoring pre-releases). Will need to be updated
 * for new major versions.
 * 
 * See: https://github.com/nodejs/node/issues/36349
 * and: https://github.com/nodejs/node/blob/master/doc/abi_version_registry.json
 */
#define IS_CONTEXT_AWARE_NODE_MODULE_VERSION \
  (NODE_MODULE_VERSION == 72 \
  || NODE_MODULE_VERSION == 79 \
  || NODE_MODULE_VERSION == 83 \
  || NODE_MODULE_VERSION == 88)

#include "async_worker.h"
#include "thread_pool.h"

namespace nodegit {
  class AsyncContextCleanupHandle;
  class Context {
  public:
    Context(v8::Isolate *isolate);
    Context(const Context &) = delete;
    Context(Context &&) = delete;
    Context &operator=(const Context &) = delete;
    Context &operator=(Context &&) = delete;

    ~Context();

    static Context *GetCurrentContext();

    v8::Local<v8::Value> GetFromPersistent(std::string key);

    void QueueWorker(nodegit::AsyncWorker *worker);

    void SaveToPersistent(std::string key, const v8::Local<v8::Value> &value);

    void ShutdownThreadPool(std::unique_ptr<AsyncContextCleanupHandle> cleanupHandle);

  private:
    v8::Isolate *isolate;

    ThreadPool threadPool;

    // This map contains persistent handles that need to be cleaned up
    // after the context has been torn down.
    // Often this is used as a context-aware storage cell for `*::InitializeComponent`
    // to store function templates on them.
    Nan::Persistent<v8::Object> persistentStorage;

    static std::map<v8::Isolate *, Context *> contexts;
  };

  class AsyncContextCleanupHandle {
    public:
      AsyncContextCleanupHandle(const AsyncContextCleanupHandle &) = delete;
      AsyncContextCleanupHandle(AsyncContextCleanupHandle &&) = delete;
      AsyncContextCleanupHandle &operator=(const AsyncContextCleanupHandle &) = delete;
      AsyncContextCleanupHandle &operator=(AsyncContextCleanupHandle &&) = delete;
      ~AsyncContextCleanupHandle();

    private:
      static void AsyncCleanupContext(void *data, void (*uvCallback)(void *), void *uvCallbackData);

      friend class Context;
      AsyncContextCleanupHandle(v8::Isolate *isolate, Context *context);
      Context *context;
      node::AsyncCleanupHookHandle handle;
      void (*doneCallback)(void *);
      void *doneData;
  };
}

#endif
