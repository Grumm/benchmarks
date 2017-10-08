
template<size_t First>
class GenericTemplateIteratorImpl{
public:
    template<template<size_t> class Runner, class ...TArgs>
    static void f(TArgs&& ...args){
        Runner<First>::run(std::forward<TArgs>(args)...);
    }
};

template<size_t ...Rest>
class GenericTemplateIterator;

template<size_t First>
class GenericTemplateIterator<First>{
public:
    template<template<size_t> class Runner, class ...TArgs>
    static void f(TArgs&& ...args){
        GenericTemplateIteratorImpl<First>::template f<Runner>(std::forward<TArgs>(args)...);
    }
};

template<size_t First, size_t ...Rest>
class GenericTemplateIterator<First, Rest...>{
public:
    template<template<size_t> class Runner, class ...TArgs>
    static void f(TArgs&& ...args){
        GenericTemplateIteratorImpl<First>::template f<Runner>(std::forward<TArgs>(args)...);
        GenericTemplateIterator<Rest...>::template f<Runner>(std::forward<TArgs>(args)...);
    }
};