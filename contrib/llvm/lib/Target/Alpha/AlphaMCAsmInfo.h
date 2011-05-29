//=====-- AlphaMCAsmInfo.h - Alpha asm properties -------------*- C++ -*--====//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the AlphaMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef ALPHATARGETASMINFO_H
#define ALPHATARGETASMINFO_H

#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCAsmInfo.h"

namespace llvm {
  class Target;

  struct AlphaMCAsmInfo : public MCAsmInfo {
    explicit AlphaMCAsmInfo(const Target &T, StringRef TT);
  };

} // namespace llvm

#endif
