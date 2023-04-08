#include <node.h>
#include <nan.h>
#include <thread>
#include <mutex>
#include <string.h>

extern "C" {
long compute_nonce(const char* prefix_str, const char* suffix_str, int target_difficulty, int start, int step, int* interrupt);
}

static long compute_nonce_threaded(const char* prefix_str, const char* suffix_str, int target_difficulty) {
    int num_threads = std::thread::hardware_concurrency();

    std::thread threads[num_threads];
    std::mutex mut;

    long nonce;
    int interrupt = 0;

    for (int i = 0; i < num_threads; ++i) {
        threads[i] = std::thread([&, thread_idx(i)]() {
            long local_nonce = compute_nonce(prefix_str, suffix_str, target_difficulty, thread_idx, num_threads, &interrupt);

            if (!interrupt) {
                mut.lock();
                interrupt = 1;
                nonce = local_nonce;
                mut.unlock();
            }
        });
    }

    for (int i = 0; i < num_threads; ++i) {
        threads[i].join();
    }

    return nonce;
}

class NostrPowWorker : public Nan::AsyncWorker {
public:
    NostrPowWorker(Nan::Callback *callback, std::string prefix_str, std::string suffix_str, int target_difficulty)
        : AsyncWorker(callback),
          prefix_str(prefix_str),
          suffix_str(suffix_str),
          target_difficulty(target_difficulty) {}

    ~NostrPowWorker() {}

    void Execute() {
        nonce = compute_nonce_threaded(prefix_str.c_str(), suffix_str.c_str(), target_difficulty);
    }

    void HandleOKCallback() {
        Nan::HandleScope scope;

        v8::Local<v8::Value> argv[] = {
            Nan::Null(), Nan::New<v8::Number>((double)nonce)
        };

        callback->Call(2, argv, async_resource);
    }

private:
    std::string prefix_str;
    std::string suffix_str;
    int target_difficulty;
    long nonce;
};

NAN_METHOD(Compute) {

    if (info.Length() < 3) {
        return Nan::ThrowError("NostrPowCtx.compute() requires 3 arguments: prefix, suffix, target_difficulty.");
    }

    v8::Local<v8::Object> prefix_buf = info[0].As<v8::Object>();
    if (!node::Buffer::HasInstance(prefix_buf)) {
        return Nan::ThrowTypeError("prefix must be of type Buffer.");
    }

    v8::Local<v8::Object> suffix_buf = info[1].As<v8::Object>();
    if (!node::Buffer::HasInstance(suffix_buf)) {
        return Nan::ThrowTypeError("suffix must be of type Buffer.");
    }

    if (!info[2]->IsNumber()) {
        return Nan::ThrowTypeError("target_difficulty must be of type number.");
    }

    std::string prefix_str(node::Buffer::Data(prefix_buf), node::Buffer::Length(prefix_buf));
    std::string suffix_str(node::Buffer::Data(suffix_buf), node::Buffer::Length(suffix_buf));
    int target_difficulty = Nan::To<int>(info[2]).FromJust();

    // Compute synchronous
    if (info.Length() < 4) {
        long nonce = compute_nonce_threaded(prefix_str.c_str(), suffix_str.c_str(), target_difficulty);
        return info.GetReturnValue().Set((double)nonce);
    }

    // Compute asynchronous
    Nan::Callback* callback = new Nan::Callback(Nan::To<v8::Function>(info[3]).ToLocalChecked());
    Nan::AsyncQueueWorker(new NostrPowWorker(callback, prefix_str, suffix_str, target_difficulty));

}

NAN_MODULE_INIT(Init) {
  Nan::Set(target, Nan::New<v8::String>("compute").ToLocalChecked(),
    Nan::GetFunction(Nan::New<v8::FunctionTemplate>(Compute)).ToLocalChecked());
}

NODE_MODULE(nostrpow, Init)
