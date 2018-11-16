#include <cstdint>
#include <iostream>
#include <algorithm>
#include <random>
#include <string>
#include <cstdio>

#include "src/main/api.h"

typedef std::vector<std::string> card_pile;

card_pile& make_deck(card_pile& space, int deck_count);
card_pile& make_deck(card_pile& space, int deck_count = 1) {
    space.clear();
    for (int i = 0; i < deck_count; ++i) {
        for (std::string suit : { "S", "C", "D", "H" }) {
            for (std::string rank : { "A", "2", "3", "4", "5", "6", "7", "8",
                    "9", "10", "J", "Q", "K" }) {
                space.push_back(rank + suit);
            }
        }
    }
    return space;
}

std::random_device rd;
auto rng = std::default_random_engine{rd()};
card_pile& shuffle(card_pile& cards);
card_pile& shuffle(card_pile& cards) {
    std::shuffle(std::begin(cards), std::end(cards), rng);
    return cards;
}

std::vector<card_pile>& split(card_pile& in, std::vector<card_pile>& out,
        std::vector<std::size_t> numbers);
std::vector<card_pile>& split(card_pile& in, std::vector<card_pile>& out,
        std::vector<std::size_t> numbers) {
    out.clear();
    out.resize(numbers.size());

    std::size_t acc = 0;
    auto i = 0;
    for (auto& n : numbers) {
        card_pile cp(in.begin() + acc, in.begin() + acc + n);
        out[i] = cp;
        acc += n;
        ++i;
    }

    assert(acc == in.size());
    return out;
}

std::vector<card_pile>& split_even(card_pile& in, std::vector<card_pile>& out,
        std::size_t n_piles);
std::vector<card_pile>& split_even(card_pile& in, std::vector<card_pile>& out,
        std::size_t n_piles) {
    std::vector<std::size_t> nums;

    auto t = in.size();
    for (int i = 0; i < (int)in.size(); ++i) {
        auto w = t / (n_piles - i);
        nums.push_back(w);
        t -= w;
    }

    return split(in, out, nums);
}

typedef struct scard {
    enum class suit {
        HEARTS, SPADES, CLUBS, DIAMONDS
    } suit;
    std::uint8_t rank;
    bool hidden;
} scard;

int main() {
    auto rules = rules_parser::from_preset("canfield");

    card_pile cp;
    std::vector<card_pile> t;

    make_deck(cp);
    shuffle(cp);

    std::cerr << cp.size();

    split(cp, t, {13, 1, 1, 1, 1, 1, 34});

    movelist ml;

    piles ps = piles{
            t[1],{},{},{}, // Foundations
            t[6],{}, // Stock-waste
            t[0], // Reserve
            t[2],t[3],t[4],t[5] // Tableau
    };

    game_state gs(rules, ps);

    std::cerr << get_moves(ml, gs, 1024u, 102400u) << std::endl;
    std::cerr << ml.size() << std::endl;

    bool first = true;
    for (auto& m : ml) {
        printf("%d->%d\n", m.from, m.to);

        if (!first) {
            gs.make_move(m);
        }
        first = false;
    }

    std::cerr << gs << std::endl;

    std::cerr << "woa" << std::endl;

    return 0;
}
