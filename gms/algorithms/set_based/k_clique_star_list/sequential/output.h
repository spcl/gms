#pragma once
#include <algorithm>
#include <type_traits>
#include <vector>

#include <gms/common/types.h>

namespace GMS::KCliqueStar::Seq {
    enum struct OutputMode {
        Count,
        List,
        PrintSet
    };

    template <class TValueType, OutputMode TOutputMode, int TNumOutputs = 1>
    class ListOutput {
    public:
        ListOutput() : count(0)
        {}

        ListOutput(ListOutput&&) = default;
        ListOutput &operator=(ListOutput&&) = default;

        using ValueType = TValueType;
        static constexpr OutputMode mode() { return TOutputMode; }
        static constexpr int num_outputs() { return TNumOutputs; }

        auto begin() const {
            return outputs.begin();
        }

        auto end() const {
            return outputs.end();
        }

        auto begin() {
            return outputs.begin();
        }

        auto end() {
            return outputs.end();
        }

        void extend(ListOutput &output) {
            count += output.count;
            outputs.insert(outputs.end(), output.outputs.begin(), output.outputs.end());
        }

        void extend(ListOutput &&output) {
            count += output.count;
            outputs.insert(outputs.end(),
                           std::make_move_iterator(output.outputs.begin()),
                           std::make_move_iterator(output.outputs.end()));
        }

        int64_t size() const {
            if (TOutputMode == OutputMode::List) {
                return outputs.size();
            } else {
                return count;
            }
        }

        void push(const std::array<TValueType, TNumOutputs> &value) {
            if (TOutputMode == OutputMode::List) {
                outputs.push_back(value);
            } else if (TOutputMode == OutputMode::Count) {
                ++count;
            } else if (TOutputMode == OutputMode::PrintSet) {
                print_set_helper(value);
            }
        }

        void push(std::array<TValueType, TNumOutputs> &&value) {
            if (TOutputMode == OutputMode::List) {
                outputs.push_back(std::move(value));
            } else if (TOutputMode == OutputMode::Count) {
                ++count;
            } else if (TOutputMode == OutputMode::PrintSet) {
                print_set_helper(value);
            }
        }
    private:
        int64_t count;
        std::vector<std::array<TValueType, TNumOutputs>> outputs;

        void print_set_helper(const std::array<TValueType, TNumOutputs> &val) {
            if (TNumOutputs == 1) {
                printSet(val[0]);
            } else {
                for (int i = 0; i < TNumOutputs; ++i) {
                    printSet(std::to_string(i), val[i]);
                }
            }
        }
    };
}