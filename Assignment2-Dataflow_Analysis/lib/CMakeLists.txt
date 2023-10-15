add_library(DFA SHARED DFA.cpp
                       1-AvailExprs.cpp
                       2-Liveness.cpp
                       3-SCCP.cpp
                       DFA/Domain/Expression.cpp
                       DFA/Domain/Variable.cpp)
