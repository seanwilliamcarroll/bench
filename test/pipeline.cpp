#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <optional>
#include <random>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// BoundedQueue — blocking producer-consumer queue
// ---------------------------------------------------------------------------

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(size_t capacity) : capacity_(capacity) {}

    // Blocks if full.  Returns false if the queue was shut down.
    bool push(T item) {
        std::unique_lock lock(mu_);
        cv_not_full_.wait(lock, [&] { return buf_.size() < capacity_ || done_; });
        if (done_) {
            return false;
        }
        buf_.push_back(std::move(item));
        cv_not_empty_.notify_one();
        return true;
    }

    // Blocks if empty.  Returns nullopt if the queue was shut down and drained.
    std::optional<T> pop() {
        std::unique_lock lock(mu_);
        cv_not_empty_.wait(lock, [&] { return !buf_.empty() || done_; });
        if (buf_.empty()) {
            return std::nullopt;
        }
        T item = std::move(buf_.front());
        buf_.erase(buf_.begin());
        cv_not_full_.notify_one();
        return item;
    }

    void shutdown() {
        std::lock_guard lock(mu_);
        done_ = true;
        cv_not_full_.notify_all();
        cv_not_empty_.notify_all();
    }

private:
    size_t capacity_;
    std::vector<T> buf_;
    std::mutex mu_;
    std::condition_variable cv_not_full_;
    std::condition_variable cv_not_empty_;
    bool done_ = false;
};

// ---------------------------------------------------------------------------
// Data types flowing through the pipeline
// ---------------------------------------------------------------------------

constexpr int MATRIX_SIZE = 64;

struct WorkItem {
    double a[MATRIX_SIZE * MATRIX_SIZE];
    double b[MATRIX_SIZE * MATRIX_SIZE];
    uint64_t seq;
};

struct Result {
    double checksum;
    uint64_t seq;
};

// ---------------------------------------------------------------------------
// Heavy compute — matrix multiply + checksum
// ---------------------------------------------------------------------------

static void matrix_multiply(const double *a, const double *b, double *out,
                            int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double sum = 0.0;
            for (int k = 0; k < n; k++) {
                sum += a[i * n + k] * b[k * n + j];
            }
            out[i * n + j] = sum;
        }
    }
}

static double checksum(const double *m, int n) {
    double acc = 0.0;
    for (int i = 0; i < n * n; i++) {
        acc += std::sin(m[i] * 0.001);
    }
    return acc;
}

// ---------------------------------------------------------------------------
// Pipeline stages
// ---------------------------------------------------------------------------

static std::atomic<bool> running{true};

// Producer: generate random matrices and push work items
static void produce(BoundedQueue<WorkItem> &queue) {
    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    uint64_t seq = 0;

    while (running.load(std::memory_order_relaxed)) {
        WorkItem item;
        item.seq = seq++;
        for (int i = 0; i < MATRIX_SIZE * MATRIX_SIZE; i++) {
            item.a[i] = dist(rng);
            item.b[i] = dist(rng);
        }
        if (!queue.push(std::move(item))) {
            break;
        }
    }
}

// Compute worker: pull items, multiply, checksum, push results
static void compute(BoundedQueue<WorkItem> &in, BoundedQueue<Result> &out) {
    std::vector<double> product(MATRIX_SIZE * MATRIX_SIZE);

    while (true) {
        auto item = in.pop();
        if (!item) {
            break;
        }
        matrix_multiply(item->a, item->b, product.data(), MATRIX_SIZE);
        double cs = checksum(product.data(), MATRIX_SIZE);

        Result r{cs, item->seq};
        if (!out.push(std::move(r))) {
            break;
        }
    }
}

// IO writer: consume results, format and write to a file
static void write_results(BoundedQueue<Result> &queue) {
    FILE *f = std::fopen("/dev/null", "w");
    if (!f) {
        return;
    }

    char buf[256];
    while (true) {
        auto result = queue.pop();
        if (!result) {
            break;
        }
        int len = std::snprintf(buf, sizeof(buf),
                                "seq=%lu checksum=%.12f\n",
                                static_cast<unsigned long>(result->seq),
                                result->checksum);
        std::fwrite(buf, 1, len, f);
        std::fflush(f);
    }

    std::fclose(f);
}

// ---------------------------------------------------------------------------
// Main — launch pipeline, run for a fixed duration
// ---------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    int duration_sec = 10;
    if (argc > 1) {
        duration_sec = std::atoi(argv[1]);
        if (duration_sec <= 0) {
            duration_sec = 10;
        }
    }

    std::printf("pipeline: %d threads, %dx%d matrices, running %ds\n",
                5, MATRIX_SIZE, MATRIX_SIZE, duration_sec);

    BoundedQueue<WorkItem> work_queue(4);
    BoundedQueue<Result> result_queue(8);

    // Launch pipeline stages
    std::thread producer(produce, std::ref(work_queue));
    std::thread worker1(compute, std::ref(work_queue), std::ref(result_queue));
    std::thread worker2(compute, std::ref(work_queue), std::ref(result_queue));
    std::thread writer(write_results, std::ref(result_queue));

    // Let it run
    std::this_thread::sleep_for(std::chrono::seconds(duration_sec));

    // Shut down: signal producer to stop, then drain queues
    running.store(false, std::memory_order_relaxed);
    work_queue.shutdown();
    result_queue.shutdown();

    producer.join();
    worker1.join();
    worker2.join();
    writer.join();

    std::printf("pipeline: done\n");
    return 0;
}
