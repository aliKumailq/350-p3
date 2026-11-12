#pragma once
#include <vector>
namespace ECSEngine {

template <typename T>
class ComponentStorage {
   // std::vector<T> mStorage;
    //std::vector<bool> mValid;       // Quickly track if a component is valid
    //std::vector<size_t> mFreeList;  // Avoid scanning for free slots

    public:
    size_t store(T&& t) {

        if (mFreeList.empty()) { // t is gonna sit at the back of the vector
            mStorage.push_back(std::move(t)); 

            auto idx = mStorage.size() - 1;
            mValid.push_back(1);
            return idx;

        } else {
            auto newTag = mFreeList.back();
            assert(!valid(newTag));
            mFreeList.pop_back();

            mValid[newTag] = 1;

            (*this)[newTag] = (t);
            return newTag;

        }

    }

        size_t store(const T& t) {

        if (mFreeList.empty()) { // t is gonna sit at the back of the vector
            mStorage.push_back(t); 

            auto idx = mStorage.size() - 1;
            mValid.push_back(1);
            return idx;

        } else {
            auto newTag = mFreeList.back();
            assert(!valid(newTag));
            mFreeList.pop_back();

            mValid[newTag] = 1;

            (*this)[newTag] = (t);
            return newTag;

        }

    }


    inline void remove(size_t t) {
        /*
        "Noooo you cant just inline everything! Your binary size will explode!!!"

        "Haha inline go brrrrr"
        */

        (*this)[t] = T{}; // calls destructor for OG value. assertion implicitly called.
        mValid[t] = false;
        mFreeList.push_back(t);






    }

    inline decltype(auto) operator[](size_t t) {
        // decl type to also account for rvalues that cannot be bound to lvalues like for a vector of booleans.
        assert(valid(t));
        return mStorage[t];
    }

    inline decltype(auto) operator[](size_t t) const {
        return assert(valid(t));
        return mStorage[t];
    }

    inline bool valid(size_t t) const {
        return t < size() ? mValid[t] : false;
    }

    inline size_t size() const {
        return mValid.size();
    }
    
    using const_iterator = std::vector<T>::const_iterator;
	const_iterator cbegin() const { return mStorage.cbegin(); } const 
	const_iterator cend() const { return mStorage.cend(); } const 
	const_iterator begin() { return mStorage.begin(); }
	const_iterator end() { return mStorage.end(); }

    

    std::vector<T> mStorage;
    std::vector<bool> mValid ; // keeps track of all valid tags
    std::vector<size_t> mFreeList;
};

}  // namespace ECSEngine
