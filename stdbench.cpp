#include <benchmark.hpp>
#include <template_unwrapper.hpp>
#include <map>
#include <unordered_map>

/***************************************/

template<size_t KEY_SIZE, size_t VAL_SIZE>
class BenchmarkableStdmap:
        public BenchmarkableContainerInterface<KEY_SIZE, VAL_SIZE>{
public:
    using KEY_T = typename BenchmarkableContainerInterface<KEY_SIZE, VAL_SIZE>::KEY_T;
    using VAL_T = typename BenchmarkableContainerInterface<KEY_SIZE, VAL_SIZE>::VAL_T;
private:
    std::map<KEY_T, VAL_T> sm;
public:
    VAL_T &search(KEY_T &key){
        return sm.find(key)->second;
    }

    void insert(KEY_T &key, VAL_T &val){
        sm[key] = val;
    }

    static std::string name(){
        return "std::map";
    };
};

template<size_t KEY_SIZE>
class GenericStdMapBenchmarkRunner{
public:
    static void run(std::vector<size_t> elements, size_t iterations, size_t ci){
        GenericContainerBenchmark<BenchmarkableStdmap, KEY_SIZE>::
                            benchmark_lookup(elements, iterations, ci);
    }
};

/***************************************/

template<size_t KEY_SIZE, size_t VAL_SIZE>
class BenchmarkableUnorderedStdmap:
        public BenchmarkableContainerInterface<KEY_SIZE, VAL_SIZE>{
public:
    using KEY_T = typename BenchmarkableContainerInterface<KEY_SIZE, VAL_SIZE>::KEY_T;
    using VAL_T = typename BenchmarkableContainerInterface<KEY_SIZE, VAL_SIZE>::VAL_T;
private:

    class hasher_t
    {
        /* http://www.partow.net/programming/hashfunctions/ */
        unsigned int DJBHash(const char* str, size_t length) const
        {
           unsigned int hash = 5381;
           size_t i    = 0;

           for (i = 0; i < length; ++str, ++i)
           {
              hash = ((hash << 5) + hash) + (*str);
           }

           return hash;
        }
    public:
        size_t operator() (KEY_T const& key) const
        {
            return DJBHash((const char*)&key, KEY_SIZE);
        }
    };
    std::unordered_map<KEY_T, VAL_T, hasher_t> sm;
    VAL_T tmp_val;
public:
    BenchmarkableUnorderedStdmap(): sm(8, hasher_t()), tmp_val({0}){}
    VAL_T &search(KEY_T &key){
        auto x = sm.find(key);
        if(unlikely(x == sm.end()))
            return tmp_val;
        return x->second;
    }

    void insert(KEY_T &key, VAL_T &val){
        sm[key] = val;
    }

    static std::string name(){
        return "std::unordered_map";
    };
};

template<size_t KEY_SIZE>
class GenericUnorderedStdMapBenchmarkRunner{
public:
    static void run(std::vector<size_t> elements, size_t iterations, size_t ci){
        GenericContainerBenchmark<BenchmarkableUnorderedStdmap, KEY_SIZE>::
                            benchmark_lookup(elements, iterations, ci);
    }
};

std::vector<size_t> elements_variations =
            {1, 2, 3, 4, 8, 10, 12, 16, 24, 32, 48, 60, 256, 512, 1024, 2048, 4096, 10000};
int main(){
    GenericTemplateIterator<
                        1, 2, 3, 4, 5, 6, 7, 8, 16, 32, 64, 128>::
                            template f<GenericUnorderedStdMapBenchmarkRunner>(
                                        elements_variations, 1000000, 10);
    GenericTemplateIterator<
                        1, 2, 3, 4, 5, 6, 7, 8, 16, 32, 64, 128>::
                            template f<GenericStdMapBenchmarkRunner>(
                                        elements_variations, 1000000, 10);
}