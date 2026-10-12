#pragma once
#include <string>
#include "match_recording.pb.h"
#include "sim.hpp"

// Appends one tick's inputs and the checksum computed right after the tick
// was applied. The recording is the only thing persisted; replay re-derives
// everything else from it.
class MatchRecorder {
public:
    explicit MatchRecorder(uint64_t seed) {
        rec_.set_seed(seed);
        rec_.set_player_count(kMaxPlayers);
    }

    void append(uint64_t tick, const TickInputs& inputs, uint64_t checksum) {
        matchrec::TickRecord* tr = rec_.add_ticks();
        tr->set_tick(tick);
        tr->set_checksum(checksum);
        for (const auto& in : inputs) {
            matchrec::InputFrame* f = tr->add_inputs();
            f->set_move_x(in.move_x);
            f->set_move_y(in.move_y);
            f->set_ability(in.ability);
            f->set_target(in.target);
        }
    }

    const matchrec::MatchRecording& recording() const { return rec_; }

    bool save(const std::string& path) const {
        std::string bytes;
        if (!rec_.SerializeToString(&bytes)) return false;
        FILE* f = fopen(path.c_str(), "wb");
        if (!f) return false;
        fwrite(bytes.data(), 1, bytes.size(), f);
        fclose(f);
        return true;
    }

private:
    matchrec::MatchRecording rec_;
};

inline bool load_recording(const std::string& path, matchrec::MatchRecording& out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    std::string bytes;
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) bytes.append(buf, n);
    fclose(f);
    return out.ParseFromString(bytes);
}

inline TickInputs to_tick_inputs(const matchrec::TickRecord& tr) {
    TickInputs inputs{};
    for (int i = 0; i < kMaxPlayers && i < tr.inputs_size(); ++i) {
        const matchrec::InputFrame& f = tr.inputs(i);
        inputs[i].move_x = (int8_t)f.move_x();
        inputs[i].move_y = (int8_t)f.move_y();
        inputs[i].ability = (uint8_t)f.ability();
        inputs[i].target = f.target();
    }
    return inputs;
}
