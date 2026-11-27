#pragma once

#include "bombe/Bombe.hpp"

namespace bombe {

enigma::MachineSettings parseSettings(std::string_view starts,
                                      std::string_view rings,
                                      std::string_view leads);

SearchRequest parseSearch(std::string ciphertext, std::string crib,
                          std::string_view rings, std::string_view position,
                          PlugboardMode mode, std::string_view leads);

std::string formatStarts(const enigma::MachineSettings &settings);
std::string formatRings(const enigma::MachineSettings &settings);
std::string formatProfile(const enigma::MachineSettings &settings);
std::string formatLeads(const std::vector<enigma::PlugPair> &pairs);
std::string_view statusText(SearchStatus status);

} // namespace bombe
