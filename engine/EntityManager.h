#pragma once

#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <type_traits>
#include <vector>
#include "Component.h"
#include "Entity.h"
#include "pack.h"
#include <functional>

namespace ECSEngine {

template <typename... Components>
class EntityManager {
public:
    static constexpr size_t kNullComponent = std::numeric_limits<size_t>::max();

    [[nodiscard]] EntityID CreateEntity(const std::string& name) {
        
        const auto& entity_idx = mEntitiesIdx.store({});
        mEntitiesIdx[entity_idx].mID = entity_idx; // set entity ID
        mEntitiesIdx[entity_idx].mType = name; // set type

        std::function<void(std::vector<size_t>&)> modifyVector;
        if (entity_idx < mEntityToComponent.at(0).size()) modifyVector = [&entity_idx](std::vector<size_t>& vec) -> void {vec[entity_idx] = kNullComponent;};
        else modifyVector = [&entity_idx](std::vector<size_t>& vec) -> void {vec.push_back(kNullComponent);};

        for (auto& vec : mEntityToComponent) modifyVector(vec);
        return entity_idx;
        
    }

    void RemoveEntity(EntityID entity) {

    }

    template <typename ComponentType> 
    void addComponent(EntityID id, ComponentType& component) {
        assert(mEntitiesIdx.valid(id));
        static_assert(contains<ComponentType>);
        constexpr auto index = idx<ComponentType>;

        const auto ticket = std::get<index>(mRegistries).store(component);
        SetComponentIdx<ComponentType>(id, ticket);    
        return;
    }

    template <typename T>
    void RemoveComponent(EntityID entity) {
        assert(mEntitiesIdx.valid(entity)); // assert valid entity
        static_assert(contains<T>); 
        constexpr auto index = idx<T>;

        assert(HasComponent<T>(entity));

        const auto ticket = std::get<index>(mRegistries).remove(GetComponentIdx<T>(entity));
        GetComponentIdx<T>(entity) = kNullComponent;

    }

    template <typename T>
    bool HasComponent(EntityID entity) const {
        return mEntityToComponent[idx<T>][entity] != kNullComponent;
    }

    template <typename T>
    T& GetComponent(EntityID entity) {
        assert(HasComponent<T>(entity));
        
        return std::get<idx<T>>(mRegistries)[GetComponentIdx<T>(entity)];
    }

    template <typename T>
    ComponentStorage<T>& GetComponentStorage() {
        // Re-write code
        static ComponentStorage<T> dummy;
        return dummy;
    }

    using tEntity = std::vector<Entity>;


    public:

	using const_iterator = tEntity::const_iterator;
	const_iterator cbegin() const { return mEntitiesIdx.cbegin(); }
	const_iterator cend() const { return mEntitiesIdx.cend(); }
	const_iterator begin() { return mEntitiesIdx.begin(); }
	const_iterator end() { return mEntitiesIdx.end(); }

    private:
    std::tuple<ComponentStorage<Components>...> mRegistries;
    std::array<std::vector<size_t>, Pack<Components...>::size> mEntityToComponent;
    ComponentStorage<Entity> mEntitiesIdx; 
    struct Pack<Components...> mPack;

    template <typename Component>
    static constexpr bool contains = Pack<Components...>::template contains<Component>;

    template <typename Component>
    static constexpr size_t idx = Pack<Components...>::template index<Component>;

    template<size_t t>
    using type = Pack<Components...>::template type<t>;

    template<typename T>
    inline void SetComponentIdx(const EntityID& e, const size_t& idx) {
        GetComponentIdx<T>(e) = idx;
    }

    template<typename T>
    inline size_t& GetComponentIdx(const EntityID& e) {
        static_assert(contains<T>);
        auto& index = mEntityToComponent[idx<T>][e];
        return index;

    }


};

}  // namespace ECSEngine
