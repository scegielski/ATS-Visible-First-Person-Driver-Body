#pragma once
#include <cstdint>
constexpr uintptr_t kUpdateRva = 0x545990;
constexpr char kUpdatePattern[] = "40 56 48 83 EC 70 48 8B F1 0F 29 74 24 60 48 8B 89 18 14 00 00 0F 28 F1 48 85 C9 74 12 83 B9 F8 00 00 00 02 75 09 E8 ?? ?? ?? ?? B1 01";
constexpr uintptr_t kSetRotationRva = 0x164a70;
constexpr char kSetRotationPattern[] = "48 89 5C 24 08 57 48 83 EC 70 F3 0F 10 0A 48 8B DA 0F 29 74 24 60 48 8B F9 F3 0F 10 35 ?? ?? ?? ?? F3 0F 59 CE 0F 29 7C 24 50 44 0F 29 44 24 40 0F 28 C1";
constexpr uintptr_t kSelectRva=0x545c08;
constexpr char kSelectPattern[]="48 89 AC 24 88 00 00 00 84 C9 74 ?? 33 ED EB ??";
constexpr uintptr_t kSlotRva=0x545c45;
constexpr char kSlotPattern[]="48 8B 86 18 01 00 00 4C 89 B4 24 98 00 00 00";
constexpr uintptr_t kLeftRva=0x545cc5;
constexpr char kLeftPattern[]="84 C9 74 ?? 0F 28 C6 B2 ?? F3 0F 59 05 ?? ?? ?? ??";
constexpr uintptr_t kRightRva=0x545d0b;
constexpr char kRightPattern[]="84 C9 74 ?? F3 0F 59 35 ?? ?? ?? ?? B2 ??";
constexpr uintptr_t kUpRva=0x545d43;
constexpr char kUpPattern[]="84 C9 74 ?? F3 0F 58 E5 B2 ?? F3 0F 11 64 24 44";
constexpr uintptr_t kDownRva=0x545d73;
constexpr char kDownPattern[]="84 C9 74 ?? F3 0F 5C E5 B2 ?? F3 0F 11 64 24 44";
constexpr uintptr_t kResetRva=0x545da3;
constexpr char kResetPattern[]="84 C9 74 ?? 48 C7 44 24 40 ?? ?? ?? ?? EB ??";
