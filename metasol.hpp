/**
 * @file metasol.hpp
 * @author Tom Harley
 *
 * A header file exposing the API information for `metasol`, a system for using
 * solitaire solvers that can see all cards to solve games without knowing where
 * the face-down cards are.
 *
 * Using `metasol` typically requires creating a few callback functions, putting
 * together one @ref ms_game_state , one @ref ms_rules , and one
 * @ref ms_settings , and passing them to @ref ms_run .
 */

#ifndef METASOL_H
#define METASOL_H

#include <list>
#include <vector>
#include <random>

#include <stdint.h>
#include <string.h>

#include "ctpl/ctpl_stl.h"

/**
 * Rule describing how the stock is used.
 */
enum stock_deal_t {

    /**
     * Cards are taken from the stock and placed into the waste.
     */
    STOCK_TO_WASTE,

    /**
     * A card is taken from the stock and placed on the top of each tableau
     * pile.
     */
    STOCK_TO_TABLEAU,

    /**
     * Cards are taken from the stock and placed into the hole.
     */
    STOCK_TO_HOLE
};

/**
 * Converts a string to a stock deal method.
 *
 * @param str The string to convert.
 * @returns A stock deal method represented by the given string.
 */
stock_deal_t str_stock_deal(char *str);

/**
 * Converts a build policy to its string representation.
 *
 * @param sd The build policy to convert.
 * @returns The string representation of the given build policy.
 */
const char *stock_deal_str(stock_deal_t sd);

/**
 * Rule to describe which cards are face-up at the beginning of the game.
 */
enum face_up_policy_t {

    /**
     * All cards on the tableau are face-up.
     */
    ALL_CARDS_FACE_UP,

    /**
     * Only the top cards of tableau piles are face-up.
     */
    TOP_CARDS_FACE_UP
};

/**
 * Converts a string to a face-up policy.
 *
 * @param str The string to convert.
 * @returns A face-up policy represented by the given string.
 */
face_up_policy_t str_face_up_policy(char *str);

/**
 * Converts a face-up policy to its string representation.
 *
 * @param fup The face-up policy to convert.
 * @returns The string representation of the given face-up policy.
 */
const char *face_up_policy_str(face_up_policy_t fup);

/**
 * Rule describing how many foundations piles start with a initial card.
 */
enum foundations_init_t {

    /**
     * No foundations start with cards.
     */
    NO_FOUNDATION_INIT,

    /**
     * Only one foundation starts with a card.
     */
    ONE_FOUNDATION_INIT,

    /**
     * All foundations start with one card each.
     */
    ALL_FOUNDATIONS_INIT
};

/**
 * Converts a string to a foundation initialisation policy.
 *
 * @param str The string to convert.
 * @returns A foundation initialisation policy represented by the given string.
 */
foundations_init_t str_foundations_init(char *str);

/**
 * Converts a foundation initialisation policy to its string
 * representation.
 *
 * @param fip The foundation initialisation policy to convert.
 * @returns The string representation of the given foundation initialisation
 * policy.
 */
const char *foundations_init_str(foundations_init_t fip);

/**
 * The different card suits.
 */
enum card_suit {
    HEARTS,
    SPADES,
    CLUBS,
    DIAMONDS
};

/**
 * An array containing the different card suits.
 */
extern card_suit SUITS[4];

/**
 * The different card ranks.
 */
typedef uint8_t card_rank;

/**
 * Convert the character representation of a card's rank to its data
 * representation.
 *
 * @param r The card's rank as a character.
 * @returns The card's rank as type `card_rank`.
 */
card_rank char_rank(char r);

/**
 * A collection of rules that together describe a solitaire game.
 */
typedef struct {

    /**
     * The number of piles in the tableau (`0` if no tableau).
     */
    unsigned tableau_size;

    /**
     * The number of decks used for dealing.
     */
    unsigned deck_count;

    /**
     * The highest rank of card included in the deck(s).
     *
     * A standard set of French playing cards includes up to rank 13 (kings).
     */
    unsigned max_rank;

    /**
     * Whether or not a "hole" pile is used in the game.
     */
    bool hole_present;

    /**
     * Whether or not foundations are used in the game.
     *
     * The number of foundations is always set to `4 * deck_count`.
     */
    bool foundations_present;

    /**
     * How to initialise the foundations.
     */
    foundations_init_t foundations_init_cards;

    /**
     * Whether or not foundations have a set base rank that is always used.
     */
    bool specific_foundations_base;

    /**
     * The base rank for foundations.
     *
     * Only used if `specific_foundations_base` is `true`.
     */
    card_rank foundations_base;

    /**
     * Whether or not the tableau should be dealt as a digonal deal.
     *
     * If set to `false`, all piles in the tableau are dealt to equal sizes.
     * Diagonal deals are the default in games such as Klondike.
     */
    bool diagonal_deal;

    /**
     * The number of cell piles in this game.
     */
    unsigned cells;

    /**
     * The number of cells that start containing a card.
     */
    unsigned cells_pre_filled;

    /**
     * The number of cards in the stock at the start of the game.
     */
    unsigned stock_size;

    /**
     * The stock deal method.
     */
    stock_deal_t stock_deal_method;

    /**
     * The number of cards dealt from the stock at a time.
     *
     * Only takes effect when using an appropriate stock deal method.
     */
    unsigned stock_deal_count;

    /**
     * Whether or not the waste can be emptied into the stock once the stock is
     * empty.
     *
     * Also controls whether or not the stock contains hidden cards: if the
     * stock can be redealt, its cards can be searched through at the start of
     * the game, and so its contents are not hidden.
     */
    bool stock_redeal;

    /**
     * The number of cards in the reserve at the start of the game.
     */
    unsigned reserve_size;

    /**
     * Whether or not the reserve is stacked.
     */
    bool reserve_stacked;

    /**
     * The face-up policy for revealing cards at the start of the game.
     */
    face_up_policy_t face_up_policy;

    /**
     * The number of sequences.
     */
    unsigned sequence_count;

    /**
     * The number of cards in the accordion at the beginning of the game.
     */
    unsigned accordion_size;

} ms_rules;

/**
 * Representation of a card.
 */
typedef struct {

    /**
     * The card's suit.
     */
    card_suit suit;

    /**
     * The card's rank.
     */
    card_rank rank;

    /**
     * Whether the card is face-up (revealed) or face-down (hidden).
     */
    bool hidden;

} ms_card;

/**
 * Representation of a pile or list of cards.
 */
typedef std::vector<ms_card> ms_card_pile;

/**
 * Convert a string to a card.
 *
 * @param str The string to convert.
 * @param fd Whether or not the card is face-down.
 * @returns The card represented by the given string.
 */
ms_card ms_str_card(char *str, bool fd);

/**
 * Representation of a move.
 *
 * Stock moves are treated differently from regular moves.
 */
typedef struct {

    /**
     * Whether or not the move is a stock move.
     */
    bool stock;

    /**
     * The index of the pile that this move places cards in.
     */
    unsigned to;

    /**
     * The index of the pile that this move takes cards from.
     */
    unsigned from;

    /**
     * The number of cards to move.
     *
     * A value above `1` indicates a built group move.
     */
    unsigned size;

} ms_move;

/**
 * Representation of a deal-in-progress.
 *
 * Basically just the exact status of each pile.
 */
typedef struct {
    std::vector<ms_card_pile> foundations;
    ms_card_pile stock;
    ms_card_pile waste;
    std::vector<ms_card_pile> tableau;
    ms_card_pile hole;
    std::vector<ms_card_pile> cells;
    ms_card_pile reserve;
    std::list<ms_card> accordion;

    /**
     * The seed used for random generation of the game-state.
     */
    long seed;

    /**
     * The moves made in order to get to this game-state.
     */
    std::vector<ms_move> moves_made;

} ms_game_state;

/**
 * The reported status of a voter after a run.
 */
enum finished_state {

    /**
     * The voter found a solution to the game-state it was given.
     */
    SOLUTION_FOUND,

    /**
     * The voter exhausted all possible moves and could not find a solution to
     * the game-state it was given.
     */
    NO_SOLUTION,

    /**
     * The voter was unable to find a solution _or_ exhaust all options before
     * stopping for some other reason.
     */
    CANCELLED

};

/**
 * A collection of information pertaining to a single vote by a voter.
 */
typedef struct {

    /**
     * How the vote concluded.
     */
    finished_state result;

    /**
     * The moves the voter took to reach its solution.
     */
    std::vector<ms_move> moves;

} vote;

/**
 * The type for the _thoughtful run_ callback.
 *
 * This type of callback is used to check if a solution can be found for the
 * deal in its current game-state - if there's no solution when the face-down
 * cards are known, then there is no point to continue running voters.
 *
 * @param game_state The current game-state of the game.
 * @param rules The rules used in this game.
 * @param user_data A pointer to data given to `metasol` by the user.
 * @returns How the thoughtful run concluded.
 */
typedef finished_state thoughtful_run_func_t(ms_game_state *game_state,
        ms_rules *rules, void *user_data);

/**
 * The type for the _voter run_ callback.
 *
 * This type of callback is used to run a voter once.
 *
 * @param game_state The current game-state of the game.
 * @param rules The rules used in this game.
 * @param move_buffer Where to put the moves the voter used to reach a solution.
 * @param requested_moves The number of moves to put in the buffer. This is only
 * a guideline - the buffer should accept any number of moves. If set to `0`. no
 * guideline is given: as many moves as possible should be placed in the buffer.
 * @param user_data A pointer to the data given to `metasol` by the user.
 * @returns How the voter run concluded.
 */
typedef finished_state run_func_t(ms_game_state *game_state, ms_rules *rules,
        std::vector<ms_move> *move_buffer, unsigned requested_moves,
        void *user_data);

/**
 * The type for the _solved_ callback.
 *
 * This type of callback is used to check if the game-state is a solution to the
 * deal.
 *
 * @param game_state The current game-state of the game.
 * @param rules The rules used in this game.
 * @param user_data A pointer to the data given to `metasol` by the user.
 * @returns `true` if the given game-state is a solution to the game described
 * by the given rules; `false` otherwise.
 */
typedef bool solved_func_t(ms_game_state *, ms_rules *, void *);

/**
 * A collection of settings used by `metasol`.
 */
typedef struct {

    /**
     * The _voter run_ callback function.
     */
    run_func_t *run_func;

    /**
     * The _thoughtful run_ callback function.
     */
    thoughtful_run_func_t *thoughtful_run_func;

    /**
     * The _solved_ callback function.
     */
    solved_func_t *solved_func;

    /**
     * The number of moves to request for the move buffer.
     */
    unsigned reserved_move_count;

    /**
     * User-specified data that is passed as a final argument to all callback
     * functions.
     */
    void *user_data;

    /**
     * The maximum number of threads to run concurrently.
     *
     * This works best when set to the number of cores on the system.
     */
    unsigned max_concurrent_threads;

    /**
     * How many games to run in parallel.
     */
    unsigned max_concurrent_games;

    /**
     * How many voters are initially used to determine the next move.
     */
    unsigned initial_vote_count;

    /**
     * The number of voters to add to the voting pool each time voters are
     * added.
     */
    unsigned vote_increase_step;

    /**
     * The magnitude with which to increase the number of voters being used each
     * time voters are added.
     *
     * Applied after @ref vote_increase_step.
     */
    unsigned vote_increase_magnitude;

    /**
     * The maximum number of voters to use when determining the next move.
     */
    unsigned max_vote_count;

    /**
     * The ratio of votes required for a satisfying vote. This is honoured
     * unless the max voter count has already been reached.
     */
    unsigned agree_ratio;

    /**
     * The seed used when shuffling face-down cards around.
     */
    unsigned long seed;

    /**
     * The random generator used when shuffling face-down cards around.
     */
    std::mt19937 rng;

    /**
     * `true` when running infinitely.
     */
    bool forever;

} ms_settings;

/**
 * Get the number of foundations in the game.
 *
 * @param rules The rules describing the game.
 * @returns The number of foundation piles used in the game described by the
 * given rules.
 */
unsigned foundation_count(ms_rules *rules);

/**
 * Get the total number of piles in the game.
 *
 * @param rules The rules describing the game.
 * @returns The total number of piles used in the game described by the given
 * rules.
 */
unsigned total_pile_count(ms_rules *rules);

/**
 * Get a pile from its index.
 *
 * @param game_state The game-state to get the pile from.
 * @param index The index of the pile.
 * @returns The pile referred to by the given index in the given game-state.
 */
ms_card_pile *get_pile_by_index(ms_game_state *game_state, uint8_t index);

/**
 * Get some default rules to build on to make a game.
 *
 * Sensible entries here reduces the number of rules that have to be listed
 * explicitly in a rules file.
 *
 * @returns Some default rules.
 */
ms_rules fetch_default_rules();

/**
 * Get some default settings.
 *
 * @returns Some default settings.
 */
ms_settings fetch_default_settings();

/**
 * Generates an initial game-state based on a generator seed.
 *
 * @param seed The seed to use when generating the deal.
 * @param rules The rules to follow when generating the deal.
 * @returns A game-state from the game described in the given rules.
 */
ms_game_state random_game_state(long seed, ms_rules *rules);

/**
 * Runs `metasol`.
 *
 * @param game_state The game-state to solve from.
 * @param rules The rules describing the game.
 * @param settings The settings to use when running.
 * @param thpool The thread pool to use.
 */
int ms_run(ms_game_state *game_state, ms_rules *rules, ms_settings *settings,
        ctpl::thread_pool *thpool);

/**
 * Constructs and attempts to solve a single game-state.
 *
 * @param rules The rules of the game-state to create.
 * @param settings The settings to run the solver with.
 * @param seed The seed for generating the game-state.
 */
int ms_run_single(ms_rules *rules, ms_settings *settings, unsigned long seed);

/**
 * Constructs and plays games until terminated.
 *
 * @param rules The rules of the game-state to create.
 * @param settings The settings to run the solvers with.
 * @returns 0.
 */
int ms_run_many(ms_rules *rules, ms_settings *settings);

int ms_run_from_file(ms_rules *rules, ms_settings *settings,
        const char *filename, ctpl::thread_pool *tp);

#endif /* METASOL_H */
