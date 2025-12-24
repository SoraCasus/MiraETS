add_executable( Example_ECS_Basics 01_ECS_Basics/main.cpp )
target_link_libraries( Example_ECS_Basics PRIVATE Mira::ETS )

add_executable( Example_Static_Polymorphism 02_Static_Polymorphism/main.cpp )
target_link_libraries( Example_Static_Polymorphism PRIVATE Mira::ETS )

add_executable( Example_Type_Erasure 03_Type_Erasure/main.cpp )
target_link_libraries( Example_Type_Erasure PRIVATE Mira::ETS )

add_executable( Example_Parallel_Systems 04_Parallel_Systems/main.cpp )
target_link_libraries( Example_Parallel_Systems PRIVATE Mira::ETS Threads::Threads )

add_executable( Example_Custom_Traits 05_Custom_Traits/main.cpp )
target_link_libraries( Example_Custom_Traits PRIVATE Mira::ETS )

add_executable( Example_Observer_System 06_Observer_System/main.cpp )
target_link_libraries( Example_Observer_System PRIVATE Mira::ETS )

add_executable( Example_Serialization 07_Serialization/main.cpp )
target_link_libraries( Example_Serialization PRIVATE Mira::ETS )

add_executable( Example_Prefabs 08_Prefabs/main.cpp )
target_link_libraries( Example_Prefabs PRIVATE Mira::ETS )

add_executable( Example_Tags 09_Tags/main.cpp )
target_link_libraries( Example_Tags PRIVATE Mira::ETS )
