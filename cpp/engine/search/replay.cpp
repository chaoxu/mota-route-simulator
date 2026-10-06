// Continuous original-token replay for independent native-engine comparison.
// Only the initial state and action list enter this process. Oracle checkpoints
// are deliberately absent from its input and are compared by a separate script.
#include "engine/runtime.hpp"
#include <iostream>
#include <set>

using namespace engine;using mota::read_json;using mota::checked_int;

int main(int argc, char **argv) {
  try {
    if (argc != 3) {
      std::cerr << "Usage: mota-native-replay DATA_DIRECTORY INPUT_JSON\n";
      return 2;
    }
    const auto input = read_json(argv[2]);
    if (input.value("schema", std::string()) != "mota.cpp.native-replay-input.v1")
      throw std::runtime_error("Unsupported native-replay input schema");
    const auto &initial = input.at("initial");
    if (initial.value("model_mode", std::string()) != "native-corrected")
      throw std::runtime_error("Continuous parity requires native-corrected mode");
    engine::io::Compiler compiler(mota::World::load(argv[1]));auto world=compiler.world;
    const auto reference_initial=mota::Game::from_reference(compiler.reference_world,initial);
    const auto &nativeFloors = input.at("initial").at("floors");
    for (int floor = world->first_floor; floor < floors_count; ++floor) {
      const auto id = floor_name(floor);
      if (!nativeFloors.contains(id) || !nativeFloors.at(id).contains("retained_blocks")
          || !nativeFloors.at(id).contains("retained_block_order"))
        throw std::runtime_error("Native replay/search requires complete retained block state; regenerate input from the original initial capture");
    }

    Game game = make_native_game(compiler, initial);
    const auto &actions = input.at("actions");
    if (!actions.is_array())
      throw std::runtime_error("actions must be an array of original route tokens");
    std::set<std::size_t> boundaries{0, actions.size()};
    for (const auto &n : input.at("checkpoints")) {
      auto index = checked_int(n);
      if (index < 0 || std::size_t(index) > actions.size())
        throw std::runtime_error("Checkpoint index is outside action list");
      boundaries.insert(std::size_t(index));
    }
    auto emit = [&](std::size_t applied) {
      std::cout << Json({{"kind", "checkpoint"}, {"appliedActions", applied},
                        {"state", compiler.project(game,reference_initial).export_json()}}).dump() << '\n';
      std::cout.flush();
    };
    emit(0);
    for (std::size_t i = 0; i < actions.size(); ++i) {
      const auto token = actions.at(i).get<std::string>();
      try {
        game.apply(decode_token(*game.world,token),false);
      } catch (const Unsupported &error) {
        std::cout << Json({{"kind", "failure"}, {"classification", "unsupported"},
                          {"actionIndex", i}, {"appliedActions", i}, {"token", token},
                          {"error", error.what()}, {"partialState", compiler.project(game,reference_initial).export_json()}}).dump() << '\n';
        return 3;
      } catch (const Error &error) {
        std::cout << Json({{"kind", "failure"}, {"classification", "rejected"},
                          {"actionIndex", i}, {"appliedActions", i}, {"token", token},
                          {"error", error.what()}, {"partialState", compiler.project(game,reference_initial).export_json()}}).dump() << '\n';
        return 4;
      }
      if (boundaries.contains(i + 1))
        emit(i + 1);
    }
    std::cout << Json({{"kind", "complete"}, {"appliedActions", actions.size()},
                      {"continuous", true}, {"initializations", 1},
                      {"simulator",native_runtime_info(game)}}).dump() << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 2;
  }
}
