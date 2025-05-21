#include "events.hpp"
#include "actions.hpp"
#include "guards.hpp"
#include "dependencies.hpp"
#include "boost/sml.hpp"

namespace sml = boost::sml;

struct tcp_release
{
    auto operator()() const
    {
        using namespace sml;
        using namespace actions;
        /**
         * Initial state: *initial_state
         * Transition DSL: src_state + event [ guard ] / action = dst_state
         */
        // return make_transition_table(
        //     *"established"_s + event<release> / send_fin = "fin wait 1"_s,
        //     "fin wait 1"_s + event<ack>[is_valid] = "fin wait 2"_s,
        //     "fin wait 2"_s + event<fin>[is_valid] / send_ack = "timed wait"_s,
        //     "timed wait"_s + event<timeout> = X);

        const auto test_guard = [](const fin &f, sender &s)
        {
            ESP_LOGI("TEST", "test_guard");
            return true;
        };

        return make_transition_table(
            *"established"_s + event<release> / send_fin = "fin wait 1"_s,
            "fin wait 1"_s = X,
            "fin wait 2"_s + event<fin>[test_guard] / send_ack<fin> = "timed wait"_s,
            "timed wait"_s + event<timeout> = X);
    }
};