#pragma once

#include <cstdint>
#include <string>

using namespace std;

enum UeType { URLLC, eMBB, mMTC, NUM_OF_UE_TYPES };

struct UE {
  UeType type;
  uint32_t data;
};

inline UeType getUeType(UE ue) { return ue.type; }

inline string printUeType(UE ue) {
  if (ue.type == UeType::URLLC) {
    return "URLLC";
  } else if (ue.type == UeType::eMBB) {
    return "eMBB";
  } else if (ue.type == UeType::mMTC) {
    return "mMTC";
  } else {
    return "Wrong Type!";
  }
}

inline string printUeType(UeType type) {
  if (type == UeType::URLLC) {
    return "URLLC";
  } else if (type == UeType::eMBB) {
    return "eMBB";
  } else if (type == UeType::mMTC) {
    return "mMTC";
  } else {
    return "Wrong Type!";
  }
}
