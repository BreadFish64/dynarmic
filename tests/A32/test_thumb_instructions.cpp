/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <catch.hpp>

#include <dynarmic/A32/a32.h>

#include "common/common_types.h"
#include "A32/skyeye_interpreter/dyncom/arm_dyncom_interpreter.h"
#include "A32/skyeye_interpreter/skyeye_common/armstate.h"
#include "testenv.h"

static Dynarmic::A32::UserConfig GetUserConfig(ThumbTestEnv* testenv) {
    Dynarmic::A32::UserConfig user_config;
    user_config.callbacks = testenv;
    return user_config;
}

TEST_CASE( "thumb: lsls r0, r1, #2", "[thumb]" ) {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem.fill({});
    test_env.code_mem[0] = 0x0088; // lsls r0, r1, #2
    test_env.code_mem[1] = 0xE7FE; // b +#0

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 2;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE( jit.Regs()[0] == 8 );
    REQUIRE( jit.Regs()[1] == 2 );
    REQUIRE( jit.Regs()[15] == 2 );
    REQUIRE( jit.Cpsr() == 0x00000030 );
}

TEST_CASE( "thumb: lsls r0, r1, #31", "[thumb]" ) {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem.fill({});
    test_env.code_mem[0] = 0x07C8; // lsls r0, r1, #31
    test_env.code_mem[1] = 0xE7FE; // b +#0

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 0xFFFFFFFF;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE( jit.Regs()[0] == 0x80000000 );
    REQUIRE( jit.Regs()[1] == 0xffffffff );
    REQUIRE( jit.Regs()[15] == 2 );
    REQUIRE( jit.Cpsr() == 0xA0000030 ); // N, C flags, Thumb, User-mode
}

TEST_CASE( "thumb: revsh r4, r3", "[thumb]" ) {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem.fill({});
    test_env.code_mem[0] = 0xBADC; // revsh r4, r3
    test_env.code_mem[1] = 0xE7FE; // b +#0

    jit.Regs()[3] = 0x12345678;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE( jit.Regs()[3] == 0x12345678 );
    REQUIRE( jit.Regs()[4] == 0x00007856 );
    REQUIRE( jit.Regs()[15] == 2 );
    REQUIRE( jit.Cpsr() == 0x00000030 ); // Thumb, User-mode
}

TEST_CASE( "thumb: ldr r3, [r3, #28]", "[thumb]" ) {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem.fill({});
    test_env.code_mem[0] = 0x69DB; // ldr r3, [r3, #28]
    test_env.code_mem[1] = 0xE7FE; // b +#0

    jit.Regs()[3] = 0x12345678;
    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE( jit.Regs()[3] == 0x97969594 ); // Memory location 0x12345694
    REQUIRE( jit.Regs()[15] == 2 );
    REQUIRE( jit.Cpsr() == 0x00000030 ); // Thumb, User-mode
}

TEST_CASE( "thumb: blx +#67712", "[thumb]" ) {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem.fill({});
    test_env.code_mem[0] = 0xF010; test_env.code_mem[1] = 0xEC3E; // blx +#67712
    test_env.code_mem[2] = 0xE7FE; // b +#0

    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE( jit.Regs()[14] == (0x4 | 1) );
    REQUIRE( jit.Regs()[15] == 0x10880 );
    REQUIRE( jit.Cpsr() == 0x00000010 ); // User-mode
}

TEST_CASE( "thumb: bl +#234584", "[thumb]" ) {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem.fill({});
    test_env.code_mem[0] = 0xF039; test_env.code_mem[1] = 0xFA2A; // bl +#234584
    test_env.code_mem[2] = 0xE7FE; // b +#0

    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE( jit.Regs()[14] == (0x4 | 1) );
    REQUIRE( jit.Regs()[15] == 0x39458 );
    REQUIRE( jit.Cpsr() == 0x00000030 ); // Thumb, User-mode
}

TEST_CASE( "thumb: bl -#42", "[thumb]" ) {
    ThumbTestEnv test_env;
    Dynarmic::A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem.fill({});
    test_env.code_mem[0] = 0xF7FF; test_env.code_mem[1] = 0xFFE9; // bl -#42
    test_env.code_mem[2] = 0xE7FE; // b +#0

    jit.Regs()[15] = 0; // PC = 0
    jit.SetCpsr(0x00000030); // Thumb, User-mode

    test_env.ticks_left = 1;
    jit.Run();

    REQUIRE( jit.Regs()[14] == (0x4 | 1) );
    REQUIRE( jit.Regs()[15] == 0xFFFFFFD6 );
    REQUIRE( jit.Cpsr() == 0x00000030 ); // Thumb, User-mode
}