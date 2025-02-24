#include <benchmark/benchmark.h>
#include <random>
#include <vector>
#include <cmath>
#include <string>

extern "C" {
#include "cache.h"
Cache* create_cache_lru(int capacity);
Cache* create_cache_clock(int capacity);
Cache* create_cache_2q(int capacity);
Cache* create_cache_arc(int capacity);
}

enum CacheAlgo { LRU = 1, CLOCK, TWO_Q, ARC };

static void BM_CacheAlgorithm(benchmark::State& state,
                              CacheAlgo algo,
                              int datasetSize,
                              int cacheCapacity,
                              int numOps) {
    std::mt19937 gen(42);
    std::vector<double> zipfWeights;
    
        zipfWeights.resize(datasetSize);
        for (int i = 0; i < datasetSize; ++i) {
            zipfWeights[i] = 1.0 / std::pow(static_cast<double>(i + 1), 0.8);
        }
    
    int cyclicCounter = 0;
    bool useZipf = true;
    std::discrete_distribution<int> zipfDist(zipfWeights.begin(), zipfWeights.end());

    for (auto _ : state) {
        Cache* cache = nullptr;
        switch(algo) {
            case LRU:   cache = create_cache_lru(cacheCapacity); break;
            case CLOCK: cache = create_cache_clock(cacheCapacity); break;
            case TWO_Q: cache = create_cache_2q(cacheCapacity); break;
            case ARC:   cache = create_cache_arc(cacheCapacity); break;
        }
        for (int i = 0; i < numOps; i++) {
            int key = 0;
            cache->accessess++;
        
            key = zipfDist(gen) + 1;

            int value;
            if (!cache->ops->get(cache, key, &value)) {
                cache->ops->put(cache, key, key * 2);
            }
            else{
                cache->hits++;
            }
        }
        double hitRatio =  (double)cache->hits/cache->accessess;
       // double hitRatio = cache->ops->hit_ratio(cache);
        state.SetLabel(std::to_string(hitRatio * 100) + "% hit ratio");
        cache->ops->destroy(cache);
    }
}

static void BM_Small_LRU(benchmark::State& state) {
    BM_CacheAlgorithm(state, LRU, 10000, state.range(0),  10000);
}

static void BM_Small_CLOCK(benchmark::State& state) {
    BM_CacheAlgorithm(state, CLOCK, 10000, state.range(0), 10000);
}

static void BM_Small_2Q(benchmark::State& state) {
    BM_CacheAlgorithm(state, TWO_Q, 10000, state.range(0),  10000);
}

static void BM_Small_ARC(benchmark::State& state) {
    BM_CacheAlgorithm(state, ARC, 10000, state.range(0), 10000);
}

BENCHMARK(BM_Small_LRU)->Arg(100);
BENCHMARK(BM_Small_CLOCK)->Arg(100);
BENCHMARK(BM_Small_2Q)->Arg(100);
BENCHMARK(BM_Small_ARC)->Arg(100);

static void BM_Medium_LRU(benchmark::State& state) {
    BM_CacheAlgorithm(state, LRU, 100000, state.range(0),  100000);
}

static void BM_Medium_CLOCK(benchmark::State& state) {
    BM_CacheAlgorithm(state, CLOCK, 100000, state.range(0), 100000);
}

static void BM_Medium_2Q(benchmark::State& state) {
    BM_CacheAlgorithm(state, TWO_Q, 100000, state.range(0),  100000);
}

static void BM_Medium_ARC(benchmark::State& state) {
    BM_CacheAlgorithm(state, ARC, 100000, state.range(0),  100000);
}

BENCHMARK(BM_Medium_LRU)->Arg(1000);
BENCHMARK(BM_Medium_CLOCK)->Arg(1000);
BENCHMARK(BM_Medium_2Q)->Arg(1000);
BENCHMARK(BM_Medium_ARC)->Arg(1000);

static void BM_High_LRU(benchmark::State& state) {
    BM_CacheAlgorithm(state, LRU, 1000000, state.range(0),  100000);
}

static void BM_High_CLOCK(benchmark::State& state) {
    BM_CacheAlgorithm(state, CLOCK, 1000000, state.range(0),  100000);
}

static void BM_High_2Q(benchmark::State& state) {
    BM_CacheAlgorithm(state, TWO_Q, 1000000, state.range(0), 100000);
}

static void BM_High_ARC(benchmark::State& state) {
    BM_CacheAlgorithm(state, ARC, 1000000, state.range(0), 100000);
}

BENCHMARK(BM_High_LRU)->Arg(1000);
BENCHMARK(BM_High_CLOCK)->Arg(1000);
BENCHMARK(BM_High_2Q)->Arg(1000);
BENCHMARK(BM_High_ARC)->Arg(1000);



BENCHMARK_MAIN();
