#pragma once

#include "bombe/Bombe.hpp"

namespace bombe {

enigma::MachineSettings parseSettings(std::string_view starts,
                                      std::string_view rings,
                                      std::string_view leads);

SearchRequest parseSearch(std::string ciphertext, std::string crib,
                          std::string_view rings, std::string_view position,
                          PlugboardMode mode, std::string_view leads);

/** Encodes the public inputs for a Bombe challenge. Throws on invalid hints. */
std::string encodeChallenge(const SearchRequest &request);
/** Reads a versioned challenge from clipboard text. Throws on invalid data. */
SearchRequest decodeChallenge(std::string_view payload);

std::string formatStarts(const enigma::MachineSettings &settings);
std::string formatRings(const enigma::MachineSettings &settings);
std::string formatRings(std::span<const int> rings);
std::string formatProfile(const enigma::MachineSettings &settings);
std::string formatLeads(const std::vector<enigma::PlugPair> &pairs);
std::string_view statusText(SearchStatus status);

} // namespace bombe
