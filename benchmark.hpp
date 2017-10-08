#include <algorithm>
#include <random>
#include <iostream>
#include <iomanip>
#include <map>
#include <unordered_map>
#include <limits>
#include <chrono>
#include <string>
#include <type_traits>
#include <functional>
#include <cstdlib>
#include <utility>
#include <cstring>

#include <cstddef>

typedef uint8_t u8;
typedef uint64_t u64;
typedef int32_t i32;
typedef std::size_t size_t;
#define likely
#define unlikely

template<size_t KEY_SIZE, size_t VAL_SIZE = 8>
class KeyValueGenerator{
public:
    class key_t{
    public:
        u8 key[KEY_SIZE];
        bool operator<(const key_t &k2) const{
            return memcmp(this, &k2, sizeof(key_t)) < 0;
        }
        bool operator==(const key_t &k2) const{
            return memcmp(this, &k2, sizeof(key_t)) == 0;
        }
    };
    class val_t{
    public:
        u8 val[VAL_SIZE];
        bool operator<(const val_t &v2) const{
            return memcmp(this, &v2, sizeof(val_t)) < 0;
        }
        bool operator==(const val_t &v2) const{
            return memcmp(this, &v2, sizeof(val_t)) == 0;
        }
    };
    using PAIR_T = std::pair<key_t, val_t>;

    std::random_device rd;
    std::mt19937 mersenne_engine;
    std::uniform_int_distribution<u8> dist;
    std::uniform_int_distribution<i32> dist32;
    std::function<u8()> gen;
    std::function<size_t(size_t i)> gen_shuffle;

    KeyValueGenerator(): mersenne_engine(rd()), dist(0, 255),
                    dist32(0, std::numeric_limits<i32>::max()){
        gen = [&] () { return dist(mersenne_engine); };
        gen_shuffle = [&] (size_t i) { return dist32(mersenne_engine) % i; };
    }
    static void dump(key_t &key){
        std::cout << std::hex;
        for(size_t i = 0; i < KEY_SIZE; i++)
            std::cout << std::setfill('0') << std::setw(2) << (int)key.key[i];
        std::cout << std::dec;
        std::cout << std::endl;
    }
    static void dump(const void *key){
        auto key_ = (key_t *)key;
        dump(*key_);
    }

    val_t generate_val(val_t oldv){
        val_t newv;
        do{
            std::generate((char *)&newv, (char *)&newv + VAL_SIZE, gen);
        } while(oldv == newv);
        return newv;
    }

    size_t generate_i32(size_t max){
        return dist32(mersenne_engine) % max;
    }

    PAIR_T generate_pair(){
        PAIR_T p;

        std::generate((char *)&p.first, (char *)&p.first + KEY_SIZE, gen);
        std::generate((char *)&p.second, (char *)&p.second + VAL_SIZE, gen);
        //dump(p.first);
        return p;
    }
    template<class T>
    std::pair<double, double> get_avg_dev(std::vector<T> v){
        double sum = std::accumulate(v.begin(), v.end(), 0.0);
        double mean = sum / v.size();

        std::vector<double> diff(v.size());
        std::transform(v.begin(), v.end(), diff.begin(),
                        [mean](double x) { return x - mean; });

        double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
        double stdev = std::sqrt(sq_sum / v.size());
        return {mean, stdev};
    }
};

template<template<size_t, size_t> class Container, size_t KEY_SIZE, size_t VAL_SIZE = 8>
class GenericContainerBenchmark{
public:
    using TIME_T = std::remove_reference<
                            decltype(std::chrono::high_resolution_clock::now())>::type;

    static TIME_T get_time(){
        return std::chrono::high_resolution_clock::now();
    }
    using TIME_DURATION_T = decltype(get_time() - get_time());

    enum  test_type_t{
        SHUFFLED,
        SHUFFLED_WITH_MISSES,
        SHUFFLED_NON_RECURRENT,
        ALLUNIQUE_NONSHUFFLED,
    };
    enum test_target_t{
        INSERT,
        ERASE,
        LOOKUP,
        CUSTOM_FUNCTION,
    };

    struct request_t{
        test_target_t target;
        test_type_t type;
        float misses_percent;
        size_t confidence_interval;
        size_t elements;
        u64 iterations;
    };

    struct result_t{
        request_t request;
        double duration_avg; /* millisec */
        double duration_dev;
        size_t memory;

        void dump(){
            std::cout << std::fixed << std::right <<
                    Container<KEY_SIZE, VAL_SIZE>::name() << "<" <<
                    std::setw(5) << std::left <<
                    KEY_SIZE << ", " << VAL_SIZE <<"> iter:" << 
                    request.iterations << std::setw(6) << std::left << " elem:" << 
                    std::left << std::setprecision(2) << std::setw(7) <<
                    request.elements << " " << std::setw(8) <<
                    std::left << duration_avg/1000000 << "msec dev=" << 
                    std::setprecision(4) << 
                    std::left << duration_dev/1000000 << std::endl;
        }
    };

private:

    KeyValueGenerator<KEY_SIZE, VAL_SIZE> kvg;
    Container<KEY_SIZE, VAL_SIZE> container;
    std::vector<typename KeyValueGenerator<KEY_SIZE, VAL_SIZE>::key_t> data_v;

    struct iteration_state_t{
        TIME_T start, finish;
        size_t memory;
    };

    void prepare_lookup(request_t &request){
        request.elements = std::min((double)request.elements, std::pow(2, VAL_SIZE*8));
        request.elements = std::min((double)request.elements, std::pow(2, KEY_SIZE*8));
        /* TODO implement non-recurrent */
        size_t actual_elements = request.elements;
        size_t empty_elements = 0;
        if(request.type == SHUFFLED_WITH_MISSES){
            empty_elements = actual_elements * request.misses_percent;
            actual_elements -= empty_elements;
        }

        for(size_t i = 0; i < actual_elements; i++){
            decltype(kvg.generate_pair()) p;
            do{
                p = kvg.generate_pair();
            }while(find(data_v.begin(), data_v.end(), p.first) != data_v.end());
            data_v.push_back(p.first);
            container.insert(p.first, p.second);
        }

        for(size_t i = 0; i < empty_elements; i++){
            decltype(kvg.generate_pair()) p;
            do{
                p = kvg.generate_pair();
            }while(find(data_v.begin(), data_v.end(), p.first) != data_v.end());
            data_v.push_back(p.first);
        }
        if(request.type == SHUFFLED || request.type == SHUFFLED_WITH_MISSES
                    || request.type == SHUFFLED_NON_RECURRENT)
            std::random_shuffle(data_v.begin(), data_v.end(), kvg.gen_shuffle);
    }
    void prepare(request_t &request){
        switch(request.target){
            case LOOKUP:
                prepare_lookup(request);
                break;
            case ERASE:
                break;
            case INSERT:
                break;
            case CUSTOM_FUNCTION:
                break;
        }
    }

    void escape(void *p) {
      asm volatile("" : : "g"(p) : "memory");
    }

    void clobber() {
      asm volatile("" : : : "memory");
    }


    void do_run_lookup(request_t &request){
        u64 unique_iterations = request.iterations / request.elements;
        for(u64 i = 0; i < unique_iterations; i++){
            /* XXX make sure cycles are not reordered */
            for(auto it: data_v){
                clobber();
                auto ret = container.search(it);
                escape(&ret);
            }
        }
        size_t leftover = request.iterations - unique_iterations * data_v.size();
        size_t j = 0;
        for(auto it: data_v){
            clobber();
            auto ret = container.search(it);
            escape(&ret);
            if(unlikely(++j >= leftover))
                break;
        }
    }
    void do_run(request_t &request){
        switch(request.target){
            case LOOKUP:
                do_run_lookup(request);
                break;
            case ERASE:
                break;
            case INSERT:
                break;
            case CUSTOM_FUNCTION:
                break;
        }
    }

    result_t report(request_t &req, std::vector<iteration_state_t> &states){
        result_t res;
        res.request = req;
        /* XXX fucking useless shit chrono/decltype/C++11 go fuck yourself? */
        std::vector<double> tdiff;
        for(auto it: states){
            tdiff.push_back(std::chrono::duration_cast<
                    std::chrono::nanoseconds>(it.finish - it.start).count());
        }
        auto p = kvg.get_avg_dev(tdiff);

        res.duration_avg = p.first;
        res.duration_dev = p.second;
        res.memory = 0; /* XXX max states[].memory */
        return res;
    }

public:
    result_t run(request_t req){
        std::vector<iteration_state_t> states;
        prepare(req);
        for(size_t i = 0; i < req.confidence_interval; i++){
            iteration_state_t newstate;
            newstate.start = get_time();
            do_run(req);
            newstate.finish = get_time();
            states.push_back(newstate);
        }
        return report(req, states);
    }

    static void benchmark_lookup(std::vector<size_t> elements, size_t iterations, size_t ci){
        for(auto elem: elements){
            using T = GenericContainerBenchmark<Container, KEY_SIZE, VAL_SIZE>;
            T b;
            typename T::request_t req = {
                    T::LOOKUP,
                    T::SHUFFLED_WITH_MISSES,
                    0.05, ci, elem, iterations};
            b.run(req).dump();
        }
}
};

template<size_t KEY_SIZE, size_t VAL_SIZE>
class BenchmarkableContainerInterface{
public:
    using KEY_T = typename KeyValueGenerator<KEY_SIZE, VAL_SIZE>::key_t;
    using VAL_T = typename KeyValueGenerator<KEY_SIZE, VAL_SIZE>::val_t;

    virtual VAL_T &search(KEY_T &key) = 0;
    virtual void insert(KEY_T &key, VAL_T &val) = 0;
    //virtual static std::string name() = 0;
};