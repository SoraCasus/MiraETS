# Mira ETS (Entity Trait System)

A lightweight, modern C++ library for Entity management, offering multiple paradigms for different use cases. Mira ETS provides high-performance ECS, flexible type erasure, and static polymorphism through mixins.

## Features

- **High-Performance ECS**: A Sparse Set-based Entity Component System with generic views and automatic component registration. Optimized query iteration ($O(N_{smallest\_set})$), optimized entity destruction ($O(N_{components\_per\_entity})$), and entity recycling with generation tracking.
- **Observer System**: Reactive callbacks for component lifecycle events (Added, Removed, Modified), enabling automated responses to state changes.
- **Static Polymorphism**: Mixin-based entity design using C++20 concepts and traits.
- **Type Erasure**: `AnyMovable` container for heterogeneous object updates with Small Buffer Optimization (SBO) up to 64 bytes and transparent heap fallback.
- **Efficient System Scheduler**: High-performance execution of systems using a work-stealing thread pool and optimized batch-based task graph execution.
- **Modern C++**: Built with C++23 features like concepts, views, and execution policies.

## Components

### 1. Sparse Set ECS (`World`)

The `World` class is the central registry for entities and components. It uses Sparse Sets for cache-friendly, contiguous component storage.

```cpp
#include <Mira/ETS/World.hpp>

using namespace Mira::ETS;

World world;

// Create entities
auto entity = world.CreateEntity();

// Bulk create entities for better performance
auto entities = world.CreateEntitiesBulk(100);

// Add arbitrary components
world.AddComponent(entity, Position{0, 0});
world.AddComponent(entity, Velocity{10, 5});

// Query entities using Views
world.GetView<Position, Velocity>().Each([](Position& pos, Velocity& vel) {
    pos.x += vel.x;
    pos.y += vel.y;
});

// Automatic type deduction in systems
world.SystemUpdate([](Position& pos, Velocity& vel) {
    // Process only entities having both Position and Velocity
});
```

#### Tag Components (Memory Optimized)

Tag components are components with no data (empty structs). Mira ETS automatically detects these and optimizes their storage to use only sparse and dense entity arrays, avoiding any memory allocation for the component data itself.

```cpp
struct DeadTag {}; // Empty struct = Tag

world.AddComponent(entity, DeadTag{});

// Views work exactly the same, but with zero data memory overhead for DeadTag
world.GetView<Position, DeadTag>().Each([](Position& pos, DeadTag&) {
    // Process only dead entities
});
```

### 2. Static Polymorphism (Traits/Mixins)

Use traits to compose entity behavior at compile-time without virtual overhead.

#### Using Existing Traits
```cpp
#include <Mira/ETS/Traits.hpp>

struct MyEntity {
    float X, Y, Vx, Vy;
    MIRA_TRAIT(MovableTrait, MoveLogic);
    
    void Update(float dt) {
        MoveLogic.Update(*this, dt);
    }
};
```

#### Creating New Traits
Creating a new trait involves two steps: defining a concept for the required data, and defining the trait logic.

```cpp
#include <Mira/ETS/Concepts.hpp>

// 1. Define a concept for the required members
template<typename T>
concept HasHealth = requires(T t) {
    { t.Health } -> std::convertible_to<float>;
};

// 2. Define the trait with a template method constrained by the concept
struct DamageableTrait {
    template<typename Self>
    requires HasHealth<std::remove_cvref_t<Self>>
    void TakeDamage(Self& self, float amount) {
        self.Health -= amount;
    }
};
```

These traits can be used both in standalone structs for static polymorphism and as part of ECS components for shared logic.

### 3. Type Erasure (`AnyMovable`)

For cases where you need a heterogeneous collection of objects that share an interface but not a base class.

```cpp
#include <Mira/ETS/Movable.hpp>

std::vector<AnyMovable> objects;
objects.push_back(Hero{...});
objects.push_back(Monster{...});

for (auto& obj : objects) {
    obj.Update(1.0f);
}
```

### 4. System Scheduler

Execute systems in parallel, sequentially, or via a task-based dependency graph.

#### Simple Parallel Execution
```cpp
#include <Mira/ETS/SystemScheduler.hpp>

SystemScheduler scheduler;
scheduler.Frame(
    [&]() { /* AI Logic */ },
    [&]() { /* Physics Logic */ }
);
```

#### Dependency Graph
For complex systems, define a dependency graph to ensure correct execution order while maximizing parallel utilization.

```cpp
SystemScheduler scheduler;

// Add systems with dependencies
scheduler.AddSystem("Input", [&]() { /* Read Input */ });
scheduler.AddSystem("AI",    [&]() { /* AI Logic */ }, {"Input"});
scheduler.AddSystem("Phys",  [&]() { /* Physics */ },  {"Input"});
scheduler.AddSystem("Render",[&]() { /* Render */ },   {"AI", "Phys"});

// Run systems based on graph
scheduler.RunGraph(); 
// 'Input' runs first, then 'AI' and 'Phys' in parallel, finally 'Render'
```

### 5. Observer System

The Observer System allows you to react to component lifecycle events. This is useful for synchronizing external systems (like physics or UI) with the ECS state.

```cpp
#include <Mira/ETS/World.hpp>

// Register a callback for when a component is added
world.OnEvent<Position>(ComponentEvent::Added, [](EntityID id, Position& pos) {
    std::cout << "Position added to entity " << id << std::endl;
});

// Register a callback for when a component is modified
world.OnEvent<Position>(ComponentEvent::Modified, [](EntityID id, Position& pos) {
    std::cout << "Position modified for entity " << id << std::endl;
});

// Use PatchComponent to trigger Modified events
world.PatchComponent<Position>(entity, [](Position& pos) {
    pos.x += 10.0f;
});

// Removed events are triggered on world.RemoveComponent<T>(id) 
// or automatically when world.DestroyEntity(id) is called.
world.OnEvent<Position>(ComponentEvent::Removed, [](EntityID id, Position& pos) {
    std::cout << "Position removed from entity " << id << std::endl;
});
```

### 6. Serialization

Mira ETS integrates `simdjson` for high-performance JSON deserialization and provides a streamlined binary serialization path for performance-critical save/load paths.

#### JSON Serialization
You can serialize and deserialize the entire world state to JSON by registering component-specific serialization logic.

```cpp
#include <Mira/ETS/Serialization.hpp>

SerializationContext ctx;

// Register components with their serialization logic
ctx.Register<Position>("Position",
    [](const Position& p, std::ostream& os) {
        os << "{\"x\":" << p.x << ",\"y\":" << p.y << "}";
    },
    [](const simdjson::dom::element& el) {
        return Position{ (float)el["x"].get_double(), (float)el["y"].get_double() };
    }
);

// Save world state to JSON
std::ofstream ofs("save.json");
ctx.Serialize(world, ofs);

// Load world state from JSON string
std::string json = /* load from file */;
Result res = ctx.Deserialize(world, json); 
```

#### Binary Serialization (Streamlined)
For performance-critical paths, use the binary serialization methods which are up to 10x faster than JSON.

```cpp
// Register binary serializers
ctx.RegisterBinary<Position>("Position",
    [](const Position& p, std::ostream& os) {
        os.write(reinterpret_cast<const char*>(&p), sizeof(p));
    },
    [](std::istream& is) {
        Position p;
        is.read(reinterpret_cast<char*>(&p), sizeof(p));
        return p;
    }
);

// Save world state to binary
std::ofstream bofs("save.bin", std::ios::binary);
ctx.SerializeBinary(world, bofs);

// Load world state from binary
std::ifstream bifs("save.bin", std::ios::binary);
ctx.DeserializeBinary(world, bifs);
```

#### Error Handling

The serialization and prefab systems use a non-exception based error reporting mechanism. Methods return a `Result` object containing an `ErrorCode` and an optional message. You can also provide a custom `ErrorReporter` to the `SerializationContext` or `PrefabManager` to handle errors globally.

```cpp
class MyErrorReporter : public ErrorReporter {
public:
    void Report(const Result& result) override {
        // Log to your engine's logger
    }
};

MyErrorReporter reporter;
ctx.SetErrorReporter(&reporter);
```

### 7. Prefab System

The Prefab System allows you to define entity templates in JSON and instantiate them multiple times. It uses the `SerializationContext` to handle component data.

```cpp
#include <Mira/ETS/Prefab.hpp>

PrefabManager pm(context);

// Load templates from JSON
Result res = pm.LoadPrefabs(R"(
{
    "Warrior": {
        "Position": {"x": 10, "y": 10},
        "Health": {"current": 100, "max": 100}
    }
}
)");

if (!res) {
    std::cerr << "Failed to load prefabs: " << res.message << std::endl;
}

// Instantiate a prefab
EntityID entity = pm.Instantiate("Warrior", world);
```

## Integration

The library is a static library. You should link against the `Mira::ETS` target in your CMake project.

```cmake
target_link_libraries(your_target PRIVATE Mira::ETS)
```

## Build Options

You can customize the build using the following CMake options:

- `ETS_BUILD_TESTS`: Build unit tests (Default: ON).
- `ETS_BUILD_EXAMPLES`: Build example projects (Default: ON).
- `ETS_ENABLE_COVERAGE`: Enable code coverage instrumentation (Default: OFF, requires GCC/Clang).

Example:
```bash
cmake -B build -DETS_BUILD_EXAMPLES=OFF -DETS_BUILD_TESTS=ON
```

## Examples

The `examples/` directory contains several sub-projects demonstrating the library's features:

1.  **01_ECS_Basics**: Core ECS functionality (World, Entities, Components, Views).
2.  **02_Static_Polymorphism**: Compile-time behavior composition using Traits and Mixins.
3.  **03_Type_Erasure**: Heterogeneous collections with `AnyMovable` and SBO.
4.  **04_Parallel_Systems**: Parallel system execution using `SystemScheduler`.
5.  **05_Custom_Traits**: Guide on defining and using new traits for static polymorphism.
6.  **06_Observer_System**: Reactive programming using component lifecycle events.
7.  **07_Serialization**: World state save/load using `simdjson`.
8.  **08_Prefabs**: Entity templates and instantiation from JSON.
9.  **09_Tags**: Tag components for memory-efficient sparse sets.

Each example is fully commented and serves as a reference for production usage.

## Requirements

- C++20 or higher (uses concepts, std::execution, etc.)
- A modern compiler (GCC 11+, Clang 13+, MSVC 19.30+)

## License

Copyright (c) 2025 Mirror Domain Studios. All rights reserved.
