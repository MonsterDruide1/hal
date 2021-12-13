#include "netlist_test_utils.h"
#include "gtest/gtest.h"
#include "hal_core/netlist/boolean_function.h"
#include "hal_core/netlist/boolean_function/solver.h"
#include "hal_core/netlist/boolean_function/types.h"

#include <iostream>
#include <type_traits>

namespace hal {
    TEST(BooleanFunction, EnumConstruction) {
        EXPECT_EQ(static_cast<BooleanFunction::Value>(0), BooleanFunction::Value::ZERO);
        EXPECT_EQ(static_cast<BooleanFunction::Value>(1), BooleanFunction::Value::ONE);
    }

    TEST(BooleanFunction, IsEmpty) {
        EXPECT_TRUE(BooleanFunction().is_empty());
        EXPECT_FALSE(BooleanFunction::Var("A").is_empty());
        EXPECT_FALSE(BooleanFunction::Const(0, 1).is_empty());
    }

    TEST(BooleanFunction, GetVariableNames) {
        auto a = BooleanFunction::Var("A"),
             b = BooleanFunction::Var("B"),
             c = BooleanFunction::Var("C"),
            _0 = BooleanFunction::Const(0, 1),
            _1 = BooleanFunction::Const(1, 1);

        EXPECT_EQ((a.clone() & b.clone()).get_variable_names(), std::set<std::string>({"A", "B"}));
        EXPECT_EQ(((a.clone() & b.clone()) | a.clone()).get_variable_names(), std::set<std::string>({"A", "B"}));
        EXPECT_EQ(((a.clone() & b.clone()) & c.clone()).get_variable_names(), std::set<std::string>({"A", "B", "C"}));
        EXPECT_EQ((_0.clone() & b.clone()).get_variable_names(), std::set<std::string>({"B"}));
        EXPECT_EQ((_0.clone() & _1.clone()).get_variable_names(), std::set<std::string>({}));
    }

    TEST(BooleanFunction, CopyMoveSemantics) {
        EXPECT_EQ( std::is_copy_constructible<BooleanFunction>::value, true);
        EXPECT_EQ( std::is_copy_assignable<BooleanFunction>::value, true);
        EXPECT_EQ( std::is_move_constructible<BooleanFunction>::value, true);
        EXPECT_EQ( std::is_move_assignable<BooleanFunction>::value, true);
    }

    TEST(BooleanFunction, Operator) {
        auto  a = BooleanFunction::Var("A"),
              b = BooleanFunction::Var("B"),
             _0 = BooleanFunction::Const(0, 1),
             _1 = BooleanFunction::Const(1, 1);

        EXPECT_TRUE(a == a);
        EXPECT_TRUE(a != b);
        
        EXPECT_TRUE(_0 == _0);
        EXPECT_TRUE(_0 != _1);

        EXPECT_TRUE(a != _0);
    }

    TEST(BooleanFunction, ToString) {
        auto a = BooleanFunction::Var("A"),
             b = BooleanFunction::Var("B"),
             c = BooleanFunction::Var("C"),
            _0 = BooleanFunction::Const(0, 1),
            _1 = BooleanFunction::Const(1, 1);

        auto data = std::vector<std::tuple<std::string, BooleanFunction>>{
            {"<empty>", BooleanFunction()},
            {"(A & B)", a.clone() & b.clone()},
            {"(A & (B | C))", (a.clone() & (b.clone() | c.clone()))},
            {"((A & B) ^ (B & C))", ((a.clone() & b.clone()) ^ (b.clone() & c.clone()))},
            {"(A ^ 0b1)", a.clone() ^ _1.clone()}, 
            {"(A ^ 0b0)", a.clone() ^ _0.clone()},
            {"(! A)", ~a.clone()},
        };

        for (auto&& [expected, function]: data) {
            EXPECT_EQ(expected, function.to_string());
        }
    }

    TEST(BooleanFunction, Parser) {
        std::vector<std::tuple<std::string, BooleanFunction>> data = {
            ////////////////////////////////////////////////////////////////////
            // GENERIC PARSER
            ////////////////////////////////////////////////////////////////////
            {"0", 
                BooleanFunction::Const(0, 1)
            },
            {"1", 
                BooleanFunction::Const(1, 1)
            },
            {"A & B", 
                BooleanFunction::Var("A") & BooleanFunction::Var("B")
            },
            {"(a & bb) | (ccc & dddd)", 
                (BooleanFunction::Var("a") & BooleanFunction::Var("bb")) | (BooleanFunction::Var("ccc") & BooleanFunction::Var("dddd"))
            },
            {"A(1) ^ B(1)", 
                BooleanFunction::Var("A(1)") ^ BooleanFunction::Var("B(1)")
            },
            {"!(a ^ a) ^ !(!(b ^ b))", 
                ~(BooleanFunction::Var("a") ^ BooleanFunction::Var("a")) ^ (~(~(BooleanFunction::Var("b") ^ BooleanFunction::Var("b"))))
            },
            {"(!I0 & I1 & I2) | (I0 & I1 & I2)", 
                (~BooleanFunction::Var("I0") & (BooleanFunction::Var("I1") & BooleanFunction::Var("I2"))) | (BooleanFunction::Var("I0") & (BooleanFunction::Var("I1") & BooleanFunction::Var("I2")))
            },
            ////////////////////////////////////////////////////////////////////
            // LIBERTY PARSER
            ////////////////////////////////////////////////////////////////////
            {"A B C D(1)",
                BooleanFunction::Var("A") & (BooleanFunction::Var("B") & (BooleanFunction::Var("C") & BooleanFunction::Var("D(1)")))
            },
            {"A'", 
                ~BooleanFunction::Var("A")
            },
        };

        for (const auto& [s, expected] : data) {
            auto function = BooleanFunction::from(s);

            if (std::get_if<std::string>(&function) != nullptr) {
                log_error("netlist", "{}", std::get<std::string>(function));   
            }

            ASSERT_NE(std::get_if<BooleanFunction>(&function), nullptr);
            ASSERT_EQ(std::get<BooleanFunction>(function), expected);
        }
    }

    TEST(BooleanFunction, Parameters) {
        auto a = BooleanFunction::Var("A"),
             b = BooleanFunction::Var("B"),
             c = BooleanFunction::Var("C");

        EXPECT_EQ((a.clone() & b.clone()).get_parameters(), std::vector<BooleanFunction>({a.clone(), b.clone()}));
        EXPECT_EQ(((a.clone() & b.clone()) | c.clone()).get_parameters(), std::vector<BooleanFunction>({(a.clone() & b.clone()), c.clone()}));
    }

    TEST(BooleanFunction, ConstantSimplification) {
        auto _0 = BooleanFunction::Const(0, 1),
             _1 = BooleanFunction::Const(1, 1),
              a = BooleanFunction::Var("A");

        EXPECT_TRUE(_0.is_constant(0));
        EXPECT_TRUE(_1.is_constant(1));
        EXPECT_FALSE(_0.is_constant(1));
        EXPECT_FALSE(_1.is_constant(0));

        EXPECT_FALSE(a.is_constant());

        EXPECT_TRUE((~_1.clone()).simplify().is_constant(0));
        EXPECT_TRUE((~_0.clone()).simplify().is_constant(1));
        EXPECT_TRUE((_0.clone() | _0.clone()).simplify().is_constant(0));
        EXPECT_TRUE((_0.clone() | _1.clone()).simplify().is_constant(1));
        EXPECT_TRUE((_1.clone() | _1.clone()).simplify().is_constant(1));
        EXPECT_TRUE((_0.clone() & _0.clone()).simplify().is_constant(0));
        EXPECT_TRUE((_0.clone() & _1.clone()).simplify().is_constant(0));
        EXPECT_TRUE((_1.clone() & _1.clone()).simplify().is_constant(1));    
        EXPECT_TRUE((_0.clone() ^ _0.clone()).simplify().is_constant(0));
        EXPECT_TRUE((_0.clone() ^ _1.clone()).simplify().is_constant(1));
        EXPECT_TRUE((_1.clone() ^ _1.clone()).simplify().is_constant(0));

        EXPECT_TRUE((a.clone() | _1.clone()).simplify().is_constant(1));
        EXPECT_TRUE((a.clone() ^ a.clone()).simplify().is_constant(0));
        EXPECT_TRUE((a.clone() & _0.clone()).simplify().is_constant(0));
    }

    TEST(BooleanFunction, Simplification) {
        auto a = BooleanFunction::Var("A"),
             b = BooleanFunction::Var("B"),
             c = BooleanFunction::Var("C"),
            _0 = BooleanFunction::Const(0, 1),
            _1 = BooleanFunction::Const(1, 1);

        ////////////////////////////////////////////////////////////////////////
        // AND RULES
        ////////////////////////////////////////////////////////////////////////

        // (a & 0)   =>    0
        EXPECT_EQ((a.clone() & _0.clone()).simplify(), _0.clone());
        // (a & 1)   =>    a
        EXPECT_EQ((a.clone() & _1.clone()).simplify(), a.clone());
        // (a & a)   =>    a
        EXPECT_EQ((a.clone() & a.clone()).simplify(), a.clone());
        // (a & ~a)  =>    0
        EXPECT_EQ((a.clone() & ~a.clone()).simplify(), _0.clone());

        // (a & b) & a   =>   a & b
        EXPECT_EQ(((a.clone() & b.clone()) & a.clone()).simplify(), a.clone() & b.clone());
        // (a & b) & b   =>   a & b
        EXPECT_EQ(((a.clone() & b.clone()) & b.clone()).simplify(), a.clone() & b.clone());
        // a & (b & a)   =>   a & b
        EXPECT_EQ((a.clone() & (b.clone() & a.clone())).simplify(), a.clone() & b.clone());
        // b & (b & a)   =>   a & b
        EXPECT_EQ((b.clone() & (b.clone() & a.clone())).simplify(), a.clone() & b.clone());

        // a & (a | b)   =>    a
        EXPECT_EQ((a.clone() & (a.clone() | b.clone())).simplify(), a.clone());
        // b & (a | b)   =>    b
        EXPECT_EQ((b.clone() & (a.clone() | b.clone())).simplify(), b.clone());
        // (a | b) & a   =>    a
        EXPECT_EQ(((a.clone() | b.clone()) & a.clone()).simplify(), a.clone());
        // (a | b) & b   =>    b
        EXPECT_EQ(((a.clone() | b.clone()) & b.clone()).simplify(), b.clone());

        // (~a & b) & a   =>   0
        EXPECT_EQ(((~a.clone() & b.clone()) & a.clone()).simplify(), _0.clone());
        // (a & ~b) & b   =>   0
        EXPECT_EQ(((a.clone() & ~b.clone()) & b.clone()).simplify(), _0.clone());
        // a & (b & ~a)   =>   0
        EXPECT_EQ((a.clone() & (b.clone() & ~a.clone())).simplify(), _0.clone());
        // b & (~b & a)   =>   0
        EXPECT_EQ((b.clone() & (~b.clone() & a.clone())).simplify(), _0.clone());

        // a & (~a | b)   =>    a & b
        EXPECT_EQ((a.clone() & (~a.clone() | b.clone())).simplify(), a.clone() & b.clone());
        // b & (a | ~b)   =>    a & b
        EXPECT_EQ((b.clone() & (a.clone() | ~b.clone())).simplify(), a.clone() & b.clone());
        // (~a | b) & a   =>    a & b
        EXPECT_EQ(((~a.clone() | b.clone()) & a.clone()).simplify(), a.clone() & b.clone());
        // (a | ~b) & b   =>    a & b
        EXPECT_EQ(((a.clone() | ~b.clone()) & b.clone()).simplify(), a.clone() & b.clone());

        ////////////////////////////////////////////////////////////////////////
        // OR RULES
        ////////////////////////////////////////////////////////////////////////

        // (a | 0)   =>    a
        EXPECT_EQ((a.clone() | _0.clone()).simplify(), a.clone());
        // (a | 1)   =>    1
        EXPECT_EQ((a.clone() | _1.clone()).simplify(), _1.clone());
        // (a | a)   =>    a
        EXPECT_EQ((a.clone() | a.clone()).simplify(), a.clone());
        // (a | ~a)  =>    1
        EXPECT_EQ((a.clone() | ~a.clone()).simplify(), _1.clone());

        // a | (a | b)   =>    a | b
        EXPECT_EQ((a.clone() | (a.clone() | b.clone())).simplify(), a.clone() | b.clone());
        // b | (a | b)   =>    a | b
        EXPECT_EQ((b.clone() | (a.clone() | b.clone())).simplify(), a.clone() | b.clone());
        // (a | b) | a   =>    a | b
        EXPECT_EQ(((a.clone() | b.clone()) | a.clone()).simplify(), a.clone() | b.clone());
        // (a | b) | b   =>    a | b
        EXPECT_EQ(((a.clone() | b.clone()) | b.clone()).simplify(), a.clone() | b.clone());

        // (a & b) | a   =>   a
        EXPECT_EQ(((a.clone() & b.clone()) | a.clone()).simplify(), a.clone());
        // (a & b) | b   =>   b
        EXPECT_EQ(((a.clone() & b.clone()) | b.clone()).simplify(), b.clone());
        // a | (b & a)   =>   a
        EXPECT_EQ((a.clone() | (b.clone() & a.clone())).simplify(), a.clone());
        // b | (b & a)   =>   b
        EXPECT_EQ((b.clone() | (b.clone() & a.clone())).simplify(), b.clone());

        // a | (~a | b)   =>   1
        EXPECT_EQ((a.clone() | (~a.clone() | b.clone())).simplify(), _1.clone());
        // b | (a | ~b)   =>   1
        EXPECT_EQ((b.clone() | (a.clone() | ~b.clone())).simplify(), _1.clone());
        // (~a | b) | a   =>   1
        EXPECT_EQ(((~a.clone() | b.clone()) | a.clone()).simplify(), _1.clone());
        // (a | ~b) | b   =>   1
        EXPECT_EQ(((a.clone() | ~b.clone()) | b.clone()).simplify(), _1.clone());

        // (~a & b) | a   =>   a | b
        EXPECT_EQ(((~a.clone() & b.clone()) | a.clone()).simplify(), a.clone() | b.clone());
        // (a & ~b) | b   =>   a | b
        EXPECT_EQ(((a.clone() & ~b.clone()) | b.clone()).simplify(), a.clone() | b.clone());
        // a | (b & ~a)   =>   a | b
        EXPECT_EQ((a.clone() | (b.clone() & ~a.clone())).simplify(), a.clone() | b.clone());
        // b | (~b & a)   =>   a | b
        EXPECT_EQ((b.clone() | (~b.clone() & a.clone())).simplify(), a.clone() | b.clone());

        ////////////////////////////////////////////////////////////////////////
        // NOT RULES
        ////////////////////////////////////////////////////////////////////////

        // ~~a   =>   a
        EXPECT_EQ((~(~a.clone())).simplify(), a.clone());
        // ~(~a | ~b)   =>   a & b
        EXPECT_EQ((~(~a.clone() | ~b.clone())).simplify(), a.clone() & b.clone());
        // ~(~a & ~b)   =>   a | b
        EXPECT_EQ((~(~a.clone() & ~b.clone())).simplify(), a.clone() | b.clone());

        ////////////////////////////////////////////////////////////////////////
        // XOR RULES
        ////////////////////////////////////////////////////////////////////////

        // (a ^ 0)   =>    a
        EXPECT_EQ((a.clone() ^ _0.clone()).simplify(), a.clone());
        // (a ^ 1)   =>    ~a
        EXPECT_EQ((a.clone() ^ _1.clone()).simplify(), ~a.clone());
        // (a ^ a)   =>    0
        EXPECT_EQ((a.clone() ^ a.clone()).simplify(), _0.clone());
        // (a ^ ~a)  =>    1
        EXPECT_EQ((a.clone() ^ ~a.clone()).simplify(), _1.clone());

        ////////////////////////////////////////////////////////////////////////
        // GENERAL SIMPLIFICATION RULES
        ////////////////////////////////////////////////////////////////////////

        // TODO: fixme (uncomment simplification rules and check for endless loop)

        // (a & ~a) | (b & ~b)  =>   0
        // EXPECT_EQ(((a.clone() & ~a.clone()) | (b.clone() & ~b.clone())).simplify(), _0.clone());
        // (a & b) | (~a & b)   =>   b
        // EXPECT_EQ(((a.clone() & b.clone()) | (~a.clone() & b.clone())).simplify(), b.clone());
        // (a & ~b) | (~a & ~b)  =>  ~b
        // EXPECT_EQ(((a.clone() & ~b.clone()) | (~a.clone() & ~b.clone())).simplify(), ~b.clone());
        // (a & b) | (~a & b) | (a & ~b) | (~a & ~b)   =>   1
        // EXPECT_EQ(((a.clone() & b.clone()) | (~a.clone() & b.clone()) | (a.clone() & ~b.clone()) | (~a.clone() & ~b.clone())).simplify(), _1.clone());
        // (a | b) | (b & c)   => a | b
        // EXPECT_EQ(((a.clone() | b.clone()) | (b.clone() & c.clone())).simplify(), a.clone() | b.clone());    
        // (a & c) | (b & ~c) | (a & b)   =>   (a & c) | (b & ~c)
        // EXPECT_EQ(((a.clone() & c.clone()) | (b.clone() & ~c.clone()) | (a.clone() & b.clone())).simplify(), (a.clone() & c.clone()) | (b.clone() & ~c.clone()));
    }

    TEST(BooleanFunction, Substitution) {
        const auto  a = BooleanFunction::Var("A"),
                    b = BooleanFunction::Var("B"),
                    c = BooleanFunction::Var("C"),
                    d = BooleanFunction::Var("D"),
                   _0 = BooleanFunction::Const(0, 1);

        EXPECT_EQ((a & b & c).substitute("C", "D"), a & b & d);

        EXPECT_EQ(std::get<BooleanFunction>((a & b).substitute("B", _0)), a & _0);
        EXPECT_EQ(std::get<BooleanFunction>((a & b).substitute("B", ~c)), a & ~c);
        EXPECT_EQ(std::get<BooleanFunction>((a & b).substitute("B", ~c)), a & ~c);
        EXPECT_EQ(std::get<BooleanFunction>((a & b).substitute("B", b | c | d)),  a & (b | c | d));
    }

    TEST(BooleanFunction, EvaluateSingleBit) {
        const auto a = BooleanFunction::Var("A"),
                   b = BooleanFunction::Var("B"),
                  _0 = BooleanFunction::Const(0, 1),
                  _1 = BooleanFunction::Const(1, 1);

        using Value = BooleanFunction::Value;

        std::vector<std::tuple<BooleanFunction, std::unordered_map<std::string, Value>, Value>> data = {
            {a, {{"A", Value::ZERO}}, Value::ZERO},
            {a, {{"A", Value::ONE}}, Value::ONE},

            {~a, {{"A", Value::ZERO}}, Value::ONE},
            {~a, {{"A", Value::ONE}}, Value::ZERO},
            
            {a & b, {{"A", Value::ZERO}, {"B", Value::ZERO}}, Value::ZERO},
            {a & b, {{"A", Value::ONE}, {"B", Value::ZERO}}, Value::ZERO},
            {a & b, {{"A", Value::ONE}, {"B", Value::ONE}}, Value::ONE},

            {a | b, {{"A", Value::ZERO}, {"B", Value::ZERO}}, Value::ZERO},
            {a | b, {{"A", Value::ONE}, {"B", Value::ZERO}}, Value::ONE},
            {a | b, {{"A", Value::ONE}, {"B", Value::ONE}}, Value::ONE},

            {a ^ b, {{"A", Value::ZERO}, {"B", Value::ZERO}}, Value::ZERO},
            {a ^ b, {{"A", Value::ONE}, {"B", Value::ZERO}}, Value::ONE},
            {a ^ b, {{"A", Value::ONE}, {"B", Value::ONE}}, Value::ZERO},
        };
        
        for (const auto& [function, input, expected]: data) {
            auto value = function.evaluate(input);
            if (std::get_if<std::string>(&value) != nullptr) {
                log_error("netlist", "{}", std::get<std::string>(value));
            }
            EXPECT_EQ(expected, std::get<Value>(value));
        }
    }

    TEST(BooleanFunction, EvaluateMultiBit) {
        const auto a = BooleanFunction::Var("A", 2),
                   b = BooleanFunction::Var("B", 2),
                  _0 = BooleanFunction::Const(0, 2),
                  _1 = BooleanFunction::Const(1, 2);

        using Value = BooleanFunction::Value;

        std::vector<std::tuple<BooleanFunction, std::unordered_map<std::string, std::vector<Value>>, std::vector<Value>>> data = {
            {a, {{"A", {Value::ZERO, Value::ZERO}}}, {Value::ZERO, Value::ZERO}},
            {a, {{"A", {Value::ONE, Value::ZERO}}}, {Value::ONE, Value::ZERO}},
            {a, {{"A", {Value::ONE, Value::ONE}}}, {Value::ONE, Value::ONE}},

            {~a, {{"A", {Value::ZERO, Value::ZERO}}}, {Value::ONE, Value::ONE}},
            {~a, {{"A", {Value::ONE, Value::ZERO}}}, {Value::ZERO, Value::ONE}},
            {~a, {{"A", {Value::ONE, Value::ONE}}}, {Value::ZERO, Value::ZERO}},
         
            {a & b, {{"A", {Value::ZERO, Value::ZERO}}, {"B", {Value::ONE, Value::ONE}}}, {Value::ZERO, Value::ZERO}},
            {a & b, {{"A", {Value::ONE, Value::ZERO}}, {"B", {Value::ONE, Value::ONE}}}, {Value::ONE, Value::ZERO}},
            {a & b, {{"A", {Value::ZERO, Value::ONE}}, {"B", {Value::ONE, Value::ONE}}}, {Value::ZERO, Value::ONE}},
            {a & b, {{"A", {Value::ONE, Value::ONE}}, {"B", {Value::ONE, Value::ONE}}}, {Value::ONE, Value::ONE}},

            {a | b, {{"A", {Value::ZERO, Value::ZERO}}, {"B", {Value::ZERO, Value::ZERO}}}, {Value::ZERO, Value::ZERO}},
            {a | b, {{"A", {Value::ONE, Value::ZERO}}, {"B", {Value::ONE, Value::ZERO}}}, {Value::ONE, Value::ZERO}},
            {a | b, {{"A", {Value::ZERO, Value::ONE}}, {"B", {Value::ZERO, Value::ONE}}}, {Value::ZERO, Value::ONE}},
            {a | b, {{"A", {Value::ZERO, Value::ONE}}, {"B", {Value::ONE, Value::ZERO}}}, {Value::ONE, Value::ONE}},

            {a ^ b, {{"A", {Value::ZERO, Value::ZERO}}, {"B", {Value::ONE, Value::ONE}}}, {Value::ONE, Value::ONE}},
            {a ^ b, {{"A", {Value::ONE, Value::ZERO}}, {"B", {Value::ONE, Value::ONE}}}, {Value::ZERO, Value::ONE}},
            {a ^ b, {{"A", {Value::ZERO, Value::ONE}}, {"B", {Value::ONE, Value::ONE}}}, {Value::ONE, Value::ZERO}},
            {a ^ b, {{"A", {Value::ONE, Value::ONE}}, {"B", {Value::ONE, Value::ONE}}}, {Value::ZERO, Value::ZERO}},
        };
        
        for (const auto& [function, input, expected]: data) {
            auto value = function.evaluate(input);
            if (std::get_if<std::string>(&value) != nullptr) {
                log_error("netlist", "{}", std::get<std::string>(value));
            }
            EXPECT_EQ(expected, std::get<std::vector<Value>>(value));
        }
    }

    TEST(BooleanFunction, TruthTable) {
        auto a = BooleanFunction::Var("A"),
             b = BooleanFunction::Var("B"),
             c = BooleanFunction::Var("C");

        using Value = BooleanFunction::Value;
        std::vector<std::tuple<BooleanFunction, std::vector<std::vector<Value>>, std::vector<std::string>>> data = {
            {a.clone() & b.clone(), std::vector<std::vector<Value>>({
                {Value::ZERO, Value::ZERO, Value::ZERO, Value::ONE}
            }), {}},
            {a.clone() | b.clone(), std::vector<std::vector<Value>>({
                {Value::ZERO, Value::ONE, Value::ONE, Value::ONE}
            }), {}},
            {a.clone() ^ b.clone(), std::vector<std::vector<Value>>({
                {Value::ZERO, Value::ONE, Value::ONE, Value::ZERO}
            }), {}},
            {~((a & b) | c), std::vector<std::vector<Value>>({
                {Value::ONE, Value::ONE, Value::ONE, Value::ZERO, Value::ZERO, Value::ZERO, Value::ZERO, Value::ZERO}
            }), {}},
            {~((a & b) | c), std::vector<std::vector<Value>>({
                {Value::ONE, Value::ZERO, Value::ONE, Value::ZERO, Value::ONE, Value::ZERO, Value::ZERO, Value::ZERO}
            }), {"C", "B", "A"}},
        };

        for (const auto& [function, expected, variable_order] : data) {
            auto truth_table = function.compute_truth_table(variable_order);
            ASSERT_EQ(expected, std::get<std::vector<std::vector<Value>>>(truth_table));
        }
    }

    TEST(BooleanFunction, SimplificationVsTruthTable) {
        const auto  a = BooleanFunction::Var("A"),
                    b = BooleanFunction::Var("B"),
                    c = BooleanFunction::Var("C"),
                   _1 = BooleanFunction::Const(1, 1);
        
        std::vector<BooleanFunction> data = {
            (~(a ^ b & c) | (b | c & _1)) ^ ((a & b) | (a | b | c)),
            (a | b | c),
        };

        for (const auto& function: data) {
            ASSERT_EQ(function.compute_truth_table(), function.simplify().compute_truth_table());
        }
    }

    TEST(BooleanFunction, QueryConfig) {
        {
            const auto config = SMT::QueryConfig()
                .with_solver(SMT::SolverType::Z3)
                .with_local_solver()
                .with_model_generation()
                .with_timeout(42);

            EXPECT_EQ(config.solver, SMT::SolverType::Z3);
            EXPECT_EQ(config.local, true);
            EXPECT_EQ(config.generate_model, true);
            EXPECT_EQ(config.timeout_in_seconds, 42);
        }
        {
        const auto config = SMT::QueryConfig()
            .with_solver(SMT::SolverType::Boolector)
            .with_remote_solver()
            .without_model_generation();

        EXPECT_EQ(config.solver, SMT::SolverType::Boolector);
        EXPECT_EQ(config.local, false);
        EXPECT_EQ(config.generate_model, false);
        }
    }

    TEST(BooleanFunction, SatisfiableConstraint) {
        const auto  a = BooleanFunction::Var("A"),
                    b = BooleanFunction::Var("B"),
                   _0 = BooleanFunction::Const(0, 1),
                   _1 = BooleanFunction::Const(1, 1);

        auto formulas = std::vector<std::vector<SMT::Constraint>>({
            {
                SMT::Constraint(a.clone() & b.clone(), _1.clone())
            },
            {
                SMT::Constraint(a.clone() | b.clone(), _1.clone()),
                SMT::Constraint(a.clone(), _1.clone()),
                SMT::Constraint(b.clone(), _0.clone()),
            },
            {
                SMT::Constraint(a.clone() & b.clone(), _1.clone()),
                SMT::Constraint(a.clone(), _1.clone()),
                SMT::Constraint(b.clone(), _1.clone()),
            },
            {
                SMT::Constraint((a.clone() & ~b.clone()) | (~a.clone() & b.clone()), _1.clone()),
                SMT::Constraint(a.clone(), _1.clone()),
            }
        });

        for (auto&& constraints : formulas) {
            const auto solver = SMT::Solver(std::move(constraints));

            for (auto&& solver_type : {SMT::SolverType::Z3}) {
                if (!SMT::Solver::has_local_solver_for(solver_type)) {
                    continue;
                }

                auto [status, result] = solver.query(
                    SMT::QueryConfig()
                        .with_solver(solver_type)
                        .with_local_solver()
                        .with_model_generation()
                        .with_timeout(1000)
                );

                EXPECT_TRUE(status);
                EXPECT_EQ(result.type, SMT::ResultType::Sat);
                EXPECT_TRUE(result.model.has_value());
            }
        }
    }

    TEST(BooleanFunction, UnSatisfiableConstraint) {
        const auto  a = BooleanFunction::Var("A"),
                    b = BooleanFunction::Var("B"),
                   _0 = BooleanFunction::Const(0, 1),
                   _1 = BooleanFunction::Const(1, 1);

        auto formulas = std::vector<std::vector<SMT::Constraint>>({
            {
                SMT::Constraint(a.clone(), b.clone()),
                SMT::Constraint(a.clone(), _1.clone()),
                SMT::Constraint(b.clone(), _0.clone()),
            },
            {
                SMT::Constraint(a.clone() | b.clone(), _1.clone()),
                SMT::Constraint(a.clone(), _0.clone()),
                SMT::Constraint(b.clone(), _0.clone()),
            },
            {
                SMT::Constraint(a.clone() & b.clone(), _1.clone()),
                SMT::Constraint(a.clone(), _0.clone()),
                SMT::Constraint(b.clone(), _1.clone()),
            },
            {
                SMT::Constraint(a & b, _1.clone()),
                SMT::Constraint(a.clone(), _1.clone()),
                SMT::Constraint(b.clone(), _0.clone()),
            },
            {
                SMT::Constraint((a.clone() & ~b.clone()) | (~a.clone() & b.clone()), _1.clone()),
                SMT::Constraint(a.clone(), _1.clone()),
                SMT::Constraint(b.clone(), _1.clone()),
            }
        });

        for (auto&& constraints : formulas) {
            const auto solver = SMT::Solver(std::move(constraints));
            for (auto&& solver_type : {SMT::SolverType::Z3}) {
                if (!SMT::Solver::has_local_solver_for(solver_type)) {
                    continue;
                }

                auto [status, result] = solver.query(
                    SMT::QueryConfig()
                        .with_solver(solver_type)
                        .with_local_solver()
                        .with_model_generation()
                        .with_timeout(1000)
                );

                EXPECT_TRUE(status);
                EXPECT_EQ(result.type, SMT::ResultType::UnSat);
                EXPECT_FALSE(result.model.has_value());
            }
        }
    }

    TEST(BooleanFunction, Model) {
        const auto  a = BooleanFunction::Var("A"),
                    b = BooleanFunction::Var("B"),
                   _0 = BooleanFunction::Const(0, 1),
                   _1 = BooleanFunction::Const(1, 1);

        auto formulas = std::vector<std::tuple<std::vector<SMT::Constraint>, SMT::Model>>({
            {
                {
                    SMT::Constraint(a.clone() & b.clone(), _1.clone())
                },
                SMT::Model({{"A", {1, 1}}, {"B", {1, 1}}})
            },
            {
                {
                    SMT::Constraint(a.clone() | b.clone(), _1.clone()),
                    SMT::Constraint(b.clone(), _0.clone()),
                },
                SMT::Model({{"A", {1, 1}}, {"B", {0, 1}}})
            },
            {
                {
                    SMT::Constraint(a.clone() & b.clone(), _1.clone()),
                    SMT::Constraint(a.clone(), _1.clone()),
                },
                SMT::Model({{"A", {1, 1}}, {"B", {1, 1}}})
            },
            {
                {
                    SMT::Constraint((~a.clone() & b.clone()) | (a.clone() & ~b.clone()), _1.clone()),
                    SMT::Constraint(a.clone(), _1.clone()),
                },
                SMT::Model({{"A", {1, 1}}, {"B", {0, 1}}})
            }
        });

        for (auto&& [constraints, model] : formulas) {
            const auto solver = SMT::Solver(std::move(constraints));

            for (auto&& solver_type : {SMT::SolverType::Z3}) {
                if (!SMT::Solver::has_local_solver_for(solver_type)) {
                    continue;
                }

                auto [status, result] = solver.query(
                    SMT::QueryConfig()
                        .with_solver(solver_type)
                        .with_local_solver()
                        .with_model_generation()
                        .with_timeout(1000)
                );

                EXPECT_EQ(status, true);
                EXPECT_EQ(result.type, SMT::ResultType::Sat);
                EXPECT_EQ(*result.model, model);
            }
        }
    }
} //namespace hal
