#pragma once

#include <gms/common/types.h>
#include <omp.h>
#include "../sequential/output.h"
#include <gms/third_party/robin_hood.h>
#include <shared_mutex>

namespace GMS::KCliqueStar::Par {
    using Seq::ListOutput;
    using OutputMode = Seq::OutputMode;

    template <class TValueType, OutputMode TOutputMode, int TNumOutputs>
    class ListOutputPar {
    public:
        using OutputSeq = typename Seq::ListOutput<TValueType, TOutputMode, TNumOutputs>;

        class Writer {
            friend ListOutputPar;
            using Array = std::array<TValueType, TNumOutputs>;
        public:
            Writer(const Writer &other) : target(other.target), local(nullptr)
            {
                auto writer = target.writer();
                local = std::move(writer.local);
            }

            void push(const Array &value) {
                local->push(value);
            }

            void push(Array &&value) {
                local->push(std::move(value));
            }
        protected:
            // Target parallel output.
            ListOutputPar &target;
            // The thread-local output.
            std::shared_ptr<OutputSeq> local;

            Writer(ListOutputPar &target, std::shared_ptr<OutputSeq> &&local) :
                target(target), local(std::move(local))
            {}
        };

        ListOutputPar() = default;

        Writer writer() {
            auto output = std::make_shared<OutputSeq>();
#pragma omp critical (outputvector)
            outputs.push_back(output);
            return Writer(*this, std::move(output));
        }

        OutputSeq collect() {
            OutputSeq result;
#pragma omp critical (outputvector)
            {
                for (const auto &o : outputs) {
                    result.extend(std::move(*o));
                }
                outputs.clear();
            }
            return result;
        }
    private:
        std::vector<std::shared_ptr<OutputSeq>> outputs;
    };
}