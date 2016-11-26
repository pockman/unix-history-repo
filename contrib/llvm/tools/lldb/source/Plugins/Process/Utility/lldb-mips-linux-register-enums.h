//===-- lldb-mips-linux-register-enums.h -------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef lldb_mips_linux_register_enums_h
#define lldb_mips_linux_register_enums_h

namespace lldb_private
{
    // LLDB register codes (e.g. RegisterKind == eRegisterKindLLDB)

    //---------------------------------------------------------------------------
    // Internal codes for all mips registers.
    //---------------------------------------------------------------------------
    enum
    {
        k_first_gpr_mips,
        gpr_zero_mips = k_first_gpr_mips,
        gpr_r1_mips,
        gpr_r2_mips,
        gpr_r3_mips,
        gpr_r4_mips,
        gpr_r5_mips,
        gpr_r6_mips,
        gpr_r7_mips,
        gpr_r8_mips,
        gpr_r9_mips,
        gpr_r10_mips,
        gpr_r11_mips,
        gpr_r12_mips,
        gpr_r13_mips,
        gpr_r14_mips,
        gpr_r15_mips,
        gpr_r16_mips,
        gpr_r17_mips,
        gpr_r18_mips,
        gpr_r19_mips,
        gpr_r20_mips,
        gpr_r21_mips,
        gpr_r22_mips,
        gpr_r23_mips,
        gpr_r24_mips,
        gpr_r25_mips,
        gpr_r26_mips,
        gpr_r27_mips,
        gpr_gp_mips,
        gpr_sp_mips,
        gpr_r30_mips,
        gpr_ra_mips,
        gpr_sr_mips,
        gpr_mullo_mips,
        gpr_mulhi_mips,
        gpr_badvaddr_mips,
        gpr_cause_mips,
        gpr_pc_mips,
        gpr_config5_mips,

        k_last_gpr_mips = gpr_config5_mips,

        k_first_fpr_mips,
        fpr_f0_mips = k_first_fpr_mips,
        fpr_f1_mips,
        fpr_f2_mips,
        fpr_f3_mips,
        fpr_f4_mips,
        fpr_f5_mips,
        fpr_f6_mips,
        fpr_f7_mips,
        fpr_f8_mips,
        fpr_f9_mips,
        fpr_f10_mips,
        fpr_f11_mips,
        fpr_f12_mips,
        fpr_f13_mips,
        fpr_f14_mips,
        fpr_f15_mips,
        fpr_f16_mips,
        fpr_f17_mips,
        fpr_f18_mips,
        fpr_f19_mips,
        fpr_f20_mips,
        fpr_f21_mips,
        fpr_f22_mips,
        fpr_f23_mips,
        fpr_f24_mips,
        fpr_f25_mips,
        fpr_f26_mips,
        fpr_f27_mips,
        fpr_f28_mips,
        fpr_f29_mips,
        fpr_f30_mips,
        fpr_f31_mips,
        fpr_fcsr_mips,
        fpr_fir_mips,
        fpr_config5_mips,
        k_last_fpr_mips = fpr_config5_mips,

        k_first_msa_mips,
        msa_w0_mips = k_first_msa_mips,
        msa_w1_mips,
        msa_w2_mips,
        msa_w3_mips,
        msa_w4_mips,
        msa_w5_mips,
        msa_w6_mips,
        msa_w7_mips,
        msa_w8_mips,
        msa_w9_mips,
        msa_w10_mips,
        msa_w11_mips,
        msa_w12_mips,
        msa_w13_mips,
        msa_w14_mips,
        msa_w15_mips,
        msa_w16_mips,
        msa_w17_mips,
        msa_w18_mips,
        msa_w19_mips,
        msa_w20_mips,
        msa_w21_mips,
        msa_w22_mips,
        msa_w23_mips,
        msa_w24_mips,
        msa_w25_mips,
        msa_w26_mips,
        msa_w27_mips,
        msa_w28_mips,
        msa_w29_mips,
        msa_w30_mips,
        msa_w31_mips,
        msa_fcsr_mips,
        msa_fir_mips,
        msa_mcsr_mips,
        msa_mir_mips,
        msa_config5_mips,
        k_last_msa_mips = msa_config5_mips,

        k_num_registers_mips,

        k_num_gpr_registers_mips = k_last_gpr_mips - k_first_gpr_mips + 1,
        k_num_fpr_registers_mips = k_last_fpr_mips - k_first_fpr_mips + 1,
        k_num_msa_registers_mips = k_last_msa_mips - k_first_msa_mips + 1,
        k_num_user_registers_mips = k_num_gpr_registers_mips + k_num_fpr_registers_mips + k_num_msa_registers_mips
    };

    //---------------------------------------------------------------------------
    // Internal codes for all mips64 registers.
    //---------------------------------------------------------------------------
    enum
    {
        k_first_gpr_mips64,
        gpr_zero_mips64 = k_first_gpr_mips64,
        gpr_r1_mips64,
        gpr_r2_mips64,
        gpr_r3_mips64,
        gpr_r4_mips64,
        gpr_r5_mips64,
        gpr_r6_mips64,
        gpr_r7_mips64,
        gpr_r8_mips64,
        gpr_r9_mips64,
        gpr_r10_mips64,
        gpr_r11_mips64,
        gpr_r12_mips64,
        gpr_r13_mips64,
        gpr_r14_mips64,
        gpr_r15_mips64,
        gpr_r16_mips64,
        gpr_r17_mips64,
        gpr_r18_mips64,
        gpr_r19_mips64,
        gpr_r20_mips64,
        gpr_r21_mips64,
        gpr_r22_mips64,
        gpr_r23_mips64,
        gpr_r24_mips64,
        gpr_r25_mips64,
        gpr_r26_mips64,
        gpr_r27_mips64,
        gpr_gp_mips64,
        gpr_sp_mips64,
        gpr_r30_mips64,
        gpr_ra_mips64,
        gpr_sr_mips64,
        gpr_mullo_mips64,
        gpr_mulhi_mips64,
        gpr_badvaddr_mips64,
        gpr_cause_mips64,
        gpr_pc_mips64,
        gpr_config5_mips64,
        k_last_gpr_mips64 = gpr_config5_mips64,

        k_first_fpr_mips64,
        fpr_f0_mips64 = k_first_fpr_mips64,
        fpr_f1_mips64,
        fpr_f2_mips64,
        fpr_f3_mips64,
        fpr_f4_mips64,
        fpr_f5_mips64,
        fpr_f6_mips64,
        fpr_f7_mips64,
        fpr_f8_mips64,
        fpr_f9_mips64,
        fpr_f10_mips64,
        fpr_f11_mips64,
        fpr_f12_mips64,
        fpr_f13_mips64,
        fpr_f14_mips64,
        fpr_f15_mips64,
        fpr_f16_mips64,
        fpr_f17_mips64,
        fpr_f18_mips64,
        fpr_f19_mips64,
        fpr_f20_mips64,
        fpr_f21_mips64,
        fpr_f22_mips64,
        fpr_f23_mips64,
        fpr_f24_mips64,
        fpr_f25_mips64,
        fpr_f26_mips64,
        fpr_f27_mips64,
        fpr_f28_mips64,
        fpr_f29_mips64,
        fpr_f30_mips64,
        fpr_f31_mips64,
        fpr_fcsr_mips64,
        fpr_fir_mips64,
        fpr_config5_mips64,
        k_last_fpr_mips64 = fpr_config5_mips64,

        k_first_msa_mips64,
        msa_w0_mips64 = k_first_msa_mips64,
        msa_w1_mips64,
        msa_w2_mips64,
        msa_w3_mips64,
        msa_w4_mips64,
        msa_w5_mips64,
        msa_w6_mips64,
        msa_w7_mips64,
        msa_w8_mips64,
        msa_w9_mips64,
        msa_w10_mips64,
        msa_w11_mips64,
        msa_w12_mips64,
        msa_w13_mips64,
        msa_w14_mips64,
        msa_w15_mips64,
        msa_w16_mips64,
        msa_w17_mips64,
        msa_w18_mips64,
        msa_w19_mips64,
        msa_w20_mips64,
        msa_w21_mips64,
        msa_w22_mips64,
        msa_w23_mips64,
        msa_w24_mips64,
        msa_w25_mips64,
        msa_w26_mips64,
        msa_w27_mips64,
        msa_w28_mips64,
        msa_w29_mips64,
        msa_w30_mips64,
        msa_w31_mips64,
        msa_fcsr_mips64,
        msa_fir_mips64,
        msa_mcsr_mips64,
        msa_mir_mips64,
        msa_config5_mips64,
        k_last_msa_mips64 = msa_config5_mips64,

        k_num_registers_mips64,

        k_num_gpr_registers_mips64 = k_last_gpr_mips64 - k_first_gpr_mips64 + 1,
        k_num_fpr_registers_mips64 = k_last_fpr_mips64 - k_first_fpr_mips64 + 1,
        k_num_msa_registers_mips64 = k_last_msa_mips64 - k_first_msa_mips64 + 1,
        k_num_user_registers_mips64 = k_num_gpr_registers_mips64 + k_num_fpr_registers_mips64 + k_num_msa_registers_mips64
    };

// Register no. for RegisterKind = eRegisterKindProcessPlugin
// The ptrace request PTRACE_PEEKUSER/PTRACE_POKEUSER used this number
enum {
  ptrace_zero_mips,
  ptrace_r1_mips,
  ptrace_r2_mips,
  ptrace_r3_mips,
  ptrace_r4_mips,
  ptrace_r5_mips,
  ptrace_r6_mips,
  ptrace_r7_mips,
  ptrace_r8_mips,
  ptrace_r9_mips,
  ptrace_r10_mips,
  ptrace_r11_mips,
  ptrace_r12_mips,
  ptrace_r13_mips,
  ptrace_r14_mips,
  ptrace_r15_mips,
  ptrace_r16_mips,
  ptrace_r17_mips,
  ptrace_r18_mips,
  ptrace_r19_mips,
  ptrace_r20_mips,
  ptrace_r21_mips,
  ptrace_r22_mips,
  ptrace_r23_mips,
  ptrace_r24_mips,
  ptrace_r25_mips,
  ptrace_r26_mips,
  ptrace_r27_mips,
  ptrace_gp_mips,
  ptrace_sp_mips,
  ptrace_r30_mips,
  ptrace_ra_mips,
  ptrace_f0_mips,
  ptrace_f1_mips,
  ptrace_f2_mips,
  ptrace_f3_mips,
  ptrace_f4_mips,
  ptrace_f5_mips,
  ptrace_f6_mips,
  ptrace_f7_mips,
  ptrace_f8_mips,
  ptrace_f9_mips,
  ptrace_f10_mips,
  ptrace_f11_mips,
  ptrace_f12_mips,
  ptrace_f13_mips,
  ptrace_f14_mips,
  ptrace_f15_mips,
  ptrace_f16_mips,
  ptrace_f17_mips,
  ptrace_f18_mips,
  ptrace_f19_mips,
  ptrace_f20_mips,
  ptrace_f21_mips,
  ptrace_f22_mips,
  ptrace_f23_mips,
  ptrace_f24_mips,
  ptrace_f25_mips,
  ptrace_f26_mips,
  ptrace_f27_mips,
  ptrace_f28_mips,
  ptrace_f29_mips,
  ptrace_f30_mips,
  ptrace_f31_mips,
  ptrace_pc_mips,
  ptrace_cause_mips,
  ptrace_badvaddr_mips,
  ptrace_mulhi_mips,
  ptrace_mullo_mips,
  ptrace_fcsr_mips,
  ptrace_fir_mips,
  ptrace_sr_mips,
  ptrace_config5_mips
};
}

#endif // #ifndef lldb_mips_linux_register_enums_h
