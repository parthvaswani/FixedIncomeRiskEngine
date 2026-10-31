add_executable(FixedIncomeRiskEngine
    FixedIncomeRiskEngine.cpp
    IBKRSocketClient.cpp
    main.cpp
)

target_include_directories(FixedIncomeRiskEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)