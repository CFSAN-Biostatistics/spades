#pragma once

#include "indices/perfect_hash_map.hpp"
#include "omni/coverage.hpp"
#include "verify.hpp"
#include <vector>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <fstream>

namespace debruijn_graph {

template<class Graph>
class NewFlankingCoverage : public GraphActionHandler<Graph>,
        public omnigraph::AbstractFlankingCoverage<Graph> {
    typedef GraphActionHandler<Graph> base;
    typedef typename Graph::EdgeId EdgeId;
    typedef typename Graph::VertexId VertexId;
    typedef pair<EdgeId, unsigned> Pos;

    Graph& g_;
    const size_t averaging_range_;

    void SetRawCoverage(EdgeId e, unsigned cov) {
        g_.data(e).set_flanking_coverage(cov);
    }

    unsigned RawCoverage(EdgeId e) const {
        return g_.data(e).flanking_coverage();
    }

    size_t EdgeAveragingRange(EdgeId e) const {
        return std::min(this->g().length(e), averaging_range_);
    }

    double AverageFlankingCoverage(EdgeId e) const {
        return double(RawCoverage(e)) / double(EdgeAveragingRange(e));
    }

    unsigned InterpolateCoverage(EdgeId e, size_t l) const {
        VERIFY(l <= averaging_range_);
        VERIFY(l < g_.length(e));
        return unsigned(math::round(AverageFlankingCoverage(e) * double(l)));
    }

public:

    //todo think about interactions with gap closer
    NewFlankingCoverage(Graph& g, size_t averaging_range)
            : base(g, "NewFlankingCoverage"), g_(g),
              averaging_range_(averaging_range) {
    }

    size_t averaging_range() const {
        return averaging_range_;
    }

    //todo currently left for saves compatibility! remove later!
    template<class CoverageIndex>
    void Fill(const CoverageIndex& count_index) {
        TRACE("Filling flanking coverage from index");

        for (auto I = count_index.value_cbegin(), E = count_index.value_cend();
                I != E; ++I) {
            const auto& edge_info = *I;
            EdgeId e = edge_info.edge_id;
            unsigned offset = edge_info.offset;
            unsigned count = edge_info.count;
            VERIFY(offset != -1u);
            VERIFY(e.get() != NULL);
            if (offset < averaging_range_) {
                IncRawCoverage(e, count);
            }
        }
    }

    void IncRawCoverage(EdgeId e, unsigned count) {
        g_.data(e).inc_flanking_coverage(count);
    }

    double CoverageOfStart(EdgeId e) const {
        return AverageFlankingCoverage(e);
    }

    double CoverageOfEnd(EdgeId e) const {
        return CoverageOfStart(this->g().conjugate(e));
    }

    virtual void HandleAdd(EdgeId /*e*/) {
    }

    virtual void HandleMerge(const vector<EdgeId>& old_edges, EdgeId new_edge) {
//        SetRawCoverage(new_edge, RawCoverage(old_edges.front()));
        size_t kpomers_left = averaging_range_;
        unsigned acc = 0;
        FOREACH(EdgeId e, old_edges) {
            if (kpomers_left >= g_.length(e)) {
                acc += RawCoverage(e);
                kpomers_left -= g_.length(e);
            } else {
                if (kpomers_left != 0)
                    acc += InterpolateCoverage(e, kpomers_left);
                break;
            }
        }
        SetRawCoverage(new_edge, acc);
    }

    virtual void HandleGlue(EdgeId new_edge, EdgeId edge1, EdgeId edge2) {
        SetRawCoverage(new_edge, RawCoverage(edge1) + RawCoverage(edge2));
    }

    virtual void HandleSplit(EdgeId old_edge, EdgeId new_edge_1,
                             EdgeId /*new_edge_2*/) {
        //hard to think of any other solution
        SetRawCoverage(new_edge_1, RawCoverage(old_edge));
    }

    virtual void HandleDelete(EdgeId e) {
        SetRawCoverage(e, 0);
    }

    double LocalCoverage(EdgeId e, VertexId v) const {
        if (this->g().EdgeStart(e) == v) {
            return GetInCov(e);
        } else if (this->g().EdgeEnd(e) == v) {
            return GetOutCov(e);
        } else {
            VERIFY(false);
            return 0.0;
        }
    }

    //left for compatibility
    //todo rename
    double GetInCov(EdgeId e) const {
        return CoverageOfStart(e);
    }

    //todo rename
    double GetOutCov(EdgeId e) const {
        return CoverageOfEnd(e);
    }

    //////////////////////////

    void Save(EdgeId e, ostream& out) const {
        out << RawCoverage(e);
    }

    void Load(EdgeId e, istream& in) {
        unsigned cov;
        in >> cov;
        SetRawCoverage(e, cov);
    }

private:
    DECL_LOGGER("NewFlankingCoverage")
    ;
};


template<class StoringType>
struct SimultaneousCoverageCollector {
};

template<>
struct SimultaneousCoverageCollector<SimpleStoring> {
    template<class SimultaneousCoverageFiller, class Info>
    static void CollectCoverage(SimultaneousCoverageFiller& filler, const Info &edge_info) {
        filler.inc_coverage(edge_info);
    }
};

template<>
struct SimultaneousCoverageCollector<InvertableStoring> {
    template<class SimultaneousCoverageFiller, class Info>
    static void CollectCoverage(SimultaneousCoverageFiller& filler, const Info &edge_info) {
        filler.inc_coverage(edge_info);
        filler.inc_coverage(edge_info.conjugate(filler.k()));
    }
};

template<class Graph, class CountIndex>
class SimultaneousCoverageFiller {
    const Graph& g_;
    const CountIndex& count_index_;
    NewFlankingCoverage<Graph>& flanking_coverage_;
    CoverageIndex<Graph>& coverage_index_;
    typedef typename CountIndex::Value Value;
public:
    SimultaneousCoverageFiller(const Graph& g, const CountIndex& count_index,
                               NewFlankingCoverage<Graph>& flanking_coverage,
                               CoverageIndex<Graph>& coverage_index) :
                                   g_(g),
                                   count_index_(count_index),
                                   flanking_coverage_(flanking_coverage),
                                   coverage_index_(coverage_index) {
    }

    size_t k() const {
        return count_index_.k();
    }

    void inc_coverage(const Value &edge_info) {
        coverage_index_.IncRawCoverage(edge_info.edge_id, edge_info.count);
        if (edge_info.offset < flanking_coverage_.averaging_range()) {
            flanking_coverage_.IncRawCoverage(edge_info.edge_id, edge_info.count);
        }
    }

    void Fill() {
        for (auto I = count_index_.value_cbegin(), E = count_index_.value_cend();
                I != E; ++I) {
            const auto& edge_info = *I;
            VERIFY(edge_info.valid() != -1u);
            VERIFY(edge_info.edge_id.get() != NULL);
            SimultaneousCoverageCollector<typename CountIndex::storing_type>::CollectCoverage(*this, edge_info);
        }
    }
};

template<class Graph, class CountIndex>
void FillCoverageAndFlanking(const CountIndex& count_index, Graph& g,
                             NewFlankingCoverage<Graph>& flanking_coverage) {
    SimultaneousCoverageFiller<Graph, CountIndex> filler(g, count_index, flanking_coverage, g.coverage_index());
    filler.Fill();
}

}
