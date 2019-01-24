#include <iostream>

#include "metasol.h"
#include "src/main/api.h"

sol_rules::build_policy convert_build_policy(build_policy_t bp) {
    switch (bp) {
        case NO_BUILD:          return sol_rules::build_policy::NO_BUILD;
        case BUILD_SAME_SUIT:   return sol_rules::build_policy::SAME_SUIT;
        case BUILD_ALTERNATING: return sol_rules::build_policy::RED_BLACK;
        case BUILD_ANY:         return sol_rules::build_policy::ANY_SUIT;
        default: assert(false);
    }
}

sol_rules::spaces_policy convert_spaces_policy(spaces_policy_t sp) {
    switch (sp) {
        case NO_SPACE_FILL:
            return sol_rules::spaces_policy::NO_BUILD;
        case KINGS_FILL_SPACE:
            return sol_rules::spaces_policy::KINGS;
        case ANY_FILL_SPACE:
            return sol_rules::spaces_policy::ANY;
        case AUTO_RESERVE_THEN_WASTE:
            return sol_rules::spaces_policy::AUTO_RESERVE_THEN_WASTE;
        case AUTO_WASTE_THEN_STOCK:
            return sol_rules::spaces_policy::AUTO_WASTE_THEN_STOCK;
        default: assert(false);
    }
}

sol_rules::accordion_policy convert_accordion_policy(accordion_policy_t ap) {
    switch (ap) {
        case ACCORDION_SAME_RANK:
            return sol_rules::accordion_policy::SAME_RANK;
        case ACCORDION_SAME_SUIT:
            return sol_rules::accordion_policy::SAME_SUIT;
        case ACCORDION_ALTERNATE_COLOUR:
            return sol_rules::accordion_policy::RED_BLACK;
        case ACCORDION_ANY_SUIT:
            return sol_rules::accordion_policy::ANY_SUIT;
        default: assert(false);
    }
}

sol_rules::stock_deal_type convert_stock_deal(stock_deal_t sd) {
    switch (sd) {
        case STOCK_TO_WASTE:   return sol_rules::stock_deal_type::WASTE;
        case STOCK_TO_TABLEAU: return sol_rules::stock_deal_type::TABLEAU_PILES;
        default: assert(false);
    }
}

sol_rules::face_up_policy convert_face_up_policy(face_up_policy_t fup) {
    switch (fup) {
        case ALL_CARDS_FACE_UP: return sol_rules::face_up_policy::ALL;
        case TOP_CARDS_FACE_UP: return sol_rules::face_up_policy::TOP_CARDS;
        default: assert(false);
    }
}

sol_rules::direction convert_direction(direction_t d) {
    switch (d) {
        case LEFT:  return sol_rules::direction::LEFT;
        case RIGHT: return sol_rules::direction::RIGHT;
        case BOTH:  return sol_rules::direction::BOTH;
        default: assert(false);
    }
}

sol_rules::built_group_type convert_built_group(built_group_t bg) {
    switch (bg) {
        case CAN_MOVE_BUILT_GROUP:
            return sol_rules::built_group_type::YES;
        case CANNOT_MOVE_BUILT_GROUP:
            return sol_rules::built_group_type::NO;
        case CAN_MOVE_WHOLE_PILE:
            return sol_rules::built_group_type::WHOLE_PILE;
        case CAN_MOVE_MAXIMAL_GROUP:
            return sol_rules::built_group_type::MAXIMAL_GROUP;
        default: assert(false);
    }
}

sol_rules::foundations_init_type convert_foundations_init(
        foundations_init_t fi) {
    switch (fi) {
        case NO_FOUNDATION_INIT:
            return sol_rules::foundations_init_type::NONE;
        case ONE_FOUNDATION_INIT:
            return sol_rules::foundations_init_type::ONE;
        case ALL_FOUNDATIONS_INIT:
            return sol_rules::foundations_init_type::ALL;
        default: assert(false);
    }
}

sol_rules convert_rules(ms_rules *sr) {
    sol_rules r;

    r.tableau_pile_count = sr->tableau_size;
    r.build_pol = convert_build_policy(sr->build_policy);
    r.spaces_pol = convert_spaces_policy(sr->spaces_policy);
    r.move_built_group = convert_built_group(sr->move_built_group);
    r.built_group_pol = convert_build_policy(sr->built_group_policy);
    r.max_rank = sr->max_rank;
    r.hole = sr->hole_present;
    r.foundations_present = sr->foundations_present;
    r.foundations_init_cards =
        convert_foundations_init(sr->foundations_init_cards);
    r.foundations_removable = sr->foundations_removable;
    r.foundations_only_comp_piles = sr->foundations_only_comp_piles;
    r.diagonal_deal = sr->diagonal_deal;
    r.cells = sr->cells;
    r.cells_pre_filled = sr->cells_pre_filled;
    r.stock_size = sr->stock_size;
    r.stock_deal_t = convert_stock_deal(sr->stock_deal_method);
    r.stock_deal_count = sr->stock_deal_count;
    r.stock_redeal = sr->stock_redeal;
    r.reserve_size = sr->reserve_size;
    r.reserve_stacked = sr->reserve_stacked;
    r.face_up = convert_face_up_policy(sr->face_up_policy);
    r.sequence_count = sr->sequence_count;
    r.sequence_direction = convert_direction(sr->sequence_direction);
    r.sequence_build_pol = convert_build_policy(sr->sequence_build_policy);
    r.sequence_fixed_suit = sr->sequence_fixed_suit;
    r.accordion_size = sr->accordion_size;

    assert(sr->deck_count <= 2);
    r.two_decks = sr->deck_count == 2;

    if (sr->specific_foundations_base)
        r.foundations_base = sr->foundations_base;
    else
        r.foundations_base = boost::optional<card::rank_t>{};

    for (auto &p : sr->accordion_moves)
        r.accordion_moves.push_back(std::pair<sol_rules::direction, uint8_t>
                (convert_direction(p.first), p.second));

    for (auto &x : sr->accordion_policy)
        r.accordion_pol.push_back(convert_accordion_policy(x));

    return r;
}

int split(ms_card_pile &in, std::vector<ms_card_pile> &out,
        std::vector<unsigned> numbers) {
    out.clear();

    unsigned acc = 0;
    for (auto &n : numbers) {
        ms_card_pile cp(in.begin() + acc, in.begin() + acc + n);
        out.push_back(cp);
        acc += n;
    }

    assert(acc == in.size());
    return 0;
}

int split_even(ms_card_pile &in, std::vector<ms_card_pile> &out,
        unsigned n_piles) {
    std::vector<unsigned> nums;

    auto t = in.size();
    for (int i = 0; i < (int)in.size(); ++i) {
        auto w = t / (n_piles - i);
        nums.push_back(w);
        t -= w;
    }

    return split(in, out, nums);
}

char const *mtype_str(move::mtype t) {
    switch (t) {
        case move::mtype::regular:              return "regular";
        case move::mtype::dominance:            return "dominance";
        case move::mtype::built_group:          return "built group";
        case move::mtype::stock_k_plus:         return "stock k+";
        case move::mtype::stock_to_all_tableau: return "stock-tableau";
        case move::mtype::sequence:             return "sequence";
        case move::mtype::accordion:            return "accordion";
        case move::mtype::null:                 return "null(!)";
        default: assert(false);
    }
}

card::suit_t convert_suit(card_suit s) {
    switch (s) {
        case HEARTS:   return card::suit::Hearts;
        case SPADES:   return card::suit::Spades;
        case CLUBS:    return card::suit::Clubs;
        case DIAMONDS: return card::suit::Diamonds;
        default: assert(false);
    }
}

card::rank_t convert_rank(card_rank r) {
    return r;
}

void convert_game_state_pile(ms_card_pile *p,
        std::vector<std::vector<card::suit_t>> *suits,
        std::vector<std::vector<card::rank_t>> *ranks,
        std::vector<std::vector<bool>> *face_down,
        bool force_face_up = false) {
    suits->push_back({});
    ranks->push_back({});
    face_down->push_back({});

    for (auto &c : *p) {
        suits->back().push_back(convert_suit(c.suit));
        ranks->back().push_back(convert_rank(c.rank));
        face_down->back().push_back(force_face_up ? false : c.hidden);
    }
}

void convert_game_state_pile_set(std::vector<ms_card_pile> *set,
        std::vector<std::vector<card::suit_t>> *suits,
        std::vector<std::vector<card::rank_t>> *ranks,
        std::vector<std::vector<bool>> *face_down,
        bool force_face_up = false) {
    for (auto &p : *set) {
        convert_game_state_pile(&p, suits, ranks, face_down, force_face_up);
    }
}

/*
 * game_state pile creation order:
 * 1. hole
 * 2. 4 foundations (8 for two decks)
 * 3. cell piles
 * 4. stock
 * 5. waste
 * 6. reserve (maybe multiple?)
 * 7. accordion piles
 * 8. tableau piles
 * 9. sequence piles
 */

game_state convert_game_state(ms_game_state *sgs, ms_rules *r) {
    std::vector<std::vector<card::suit_t>> suits;
    std::vector<std::vector<card::rank_t>> ranks;
    std::vector<std::vector<bool>> face_down;

    if (r->hole_present) {
        convert_game_state_pile(&sgs->hole, &suits, &ranks, &face_down);
    }

    convert_game_state_pile_set(&sgs->foundations, &suits, &ranks, &face_down);
    convert_game_state_pile_set(&sgs->cells, &suits, &ranks, &face_down);
    if (r->stock_size) {
        // solvitaire requires that the stock be entirely face-up
        convert_game_state_pile(&sgs->stock, &suits, &ranks, &face_down, true);
        if (r->stock_deal_method == STOCK_TO_WASTE) {
            convert_game_state_pile(&sgs->waste, &suits, &ranks, &face_down);
        }
    }
    // TODO: how does the reserve work? leaving empty for now
    // TODO: how does the accordion work? leaving empty for now
    convert_game_state_pile_set(&sgs->tableau, &suits, &ranks, &face_down);
    // TODO: how does the sequence work? leaving empty for now

    game_state gs(convert_rules(r), suits, ranks, face_down);
    return gs;
}

typedef struct user_data {
    unsigned run_cache_size;
    unsigned run_timeout;
    unsigned thoughtful_run_timeout;
} user_data;

void print_sgs(ms_game_state *gs, ms_rules *r) {
    game_state solv_gs = convert_game_state(gs, r);
    std::cerr << solv_gs << std::endl;
}

bool is_dud_move(move *m) {
    if (m->to == 255u && m->from == 255u) return true;
    return false;
}

bool goes_through_stock(move *a) {
    return a->type == move::mtype::stock_k_plus
        && a->count != 0;
}

int convert_move(ms_game_state *gs, move *m, ms_move *sm) {
    sm->stock = goes_through_stock(m);

    if (m->type == move::mtype::stock_k_plus) {
        sm->from = &gs->waste;
    } else {
        sm->from = get_pile_by_index(gs, m->from);
    }

    sm->to = get_pile_by_index(gs, m->to);

    return 0;
}

/**
 * The function that is called by every voter, to get its vote.
 */
finished_state run_single(ms_game_state *gs, ms_rules *r,
        ms_move *first_move, unsigned move_count, void *data) {
    // TODO: this function can't currently handle producing multiple moves at
    // once
    assert(move_count == 1u);

    user_data *d = (user_data *) data;

    game_state solv_gs = convert_game_state(gs, r);

    movelist ml;
    finished_state result = get_moves(ml, solv_gs, d->run_cache_size,
            d->run_timeout);

    if (result == SOLUTION_FOUND) {
        if (!is_dud_move(&ml[0])) {
            convert_move(gs, &ml[0], first_move);
        } else {
            convert_move(gs, &ml[1], first_move);
        }
    }

    return result;
}

finished_state thoughtful_run(ms_game_state *gs, ms_rules *r,
        void *data) {
    user_data *d = (user_data *) data;

    print_sgs(gs, r);

    game_state solv_gs = convert_game_state(gs, r);
    movelist ml;
    return get_moves(ml, solv_gs, d->run_cache_size, d->thoughtful_run_timeout);
}

ms_rules fortunes_favor() {
    // default rules
    ms_rules r = fetch_default_rules();

    // rules specific to fortunes favor
    r.tableau_size = 12u;
    r.build_policy = BUILD_SAME_SUIT;
    r.spaces_policy = AUTO_WASTE_THEN_STOCK;
    r.foundations_init_cards = ALL_FOUNDATIONS_INIT;
    r.stock_size = 36u;

    return r;
}

bool solved(ms_game_state *gs, ms_rules *r, void *d) {
    return convert_game_state(gs, r).is_solved();
}

ms_settings get_settings(user_data *d) {
    ms_settings s = fetch_default_settings();

    s.run_func = &run_single;
    s.thoughtful_run_func = &thoughtful_run;
    s.solved_func = &solved;

    s.reserved_move_count = 1u;

    s.user_data = d;

    return s;
}

user_data get_user_data() {
    user_data d;

    d.run_cache_size = 1024u;
    d.run_timeout = 3000u;
    d.thoughtful_run_timeout = 10000u;

    return d;
}

int main() {
    ms_rules r = fortunes_favor();
    user_data d = get_user_data();
    ms_settings s = get_settings(&d);
    ms_game_state gs = random_game_state(&r, &s);

    ms_run(&gs, &r, &s);
}
