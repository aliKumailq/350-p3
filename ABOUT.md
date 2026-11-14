## Ali
### Managers
- Implemented Entity, Sprite, and Window Managers.
- Window manager uses code from lab assignments, same with Entity Manager
- 1-1 Scaling between world space and screen space for simplicity. 
#### Entity Manager
- Each component has its own registry with its own ticketing.
- Entities also have their own registry disjoint from components
- Entity manager keeps an `std::array<std::vector<size_t>, K>`, where `K` is the # of components. In each vector, the `i`'th element stores the ticket for the entity `i` for a particular component.
- Stored this way to quickly iterate through all instances of a specific component across entities by exhibiting spacial locality.
- Component registries are stored in an `std::tuple` as a sequence of distinct`ComponentStorage`'s, (i.e. registries). 
- Template structs and counters like `Contains<T>` and `Idx<T>` inside the entity manager help deduce at compile time which index of the tuple need be read when reading/writing a component.

#### Sprite Manger
- Uses an unordered map to store mappings between texturePath and the sf::Texture to prevent creating multiple instances of the same texture.




### Systems
#### Sprite System
- Iterates over all entities with a `SpriteComponent`, while asserting that a `LocationComponent` is also present.
- Location component is needed to offset the SpriteComponents bounding box.
- The box is then transferred from World Space to Screen Space.
- Each SpriteComponent mutates its corresponding `sf::Sprite`'s size and location. So its vital to make sure all entities with a `SpriteComponent` have an initialized bounding box.

### Map Loading (First half)
- Implemented the parsing of map data by looping through each line with `GetLine` for both the .map files and within the extracted lines by treating each extracted like as an `std::stringstream`.



## <AI Usage\>