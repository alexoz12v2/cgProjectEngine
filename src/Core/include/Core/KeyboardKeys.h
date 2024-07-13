#pragma once

#include "Core/Type.h"

/// Copied from GLFW

namespace cge
{
namespace key
{
    /* The unknown key */;
    inline I32_t constexpr CGE_KEY_UNKNOWN{ -1 };
    ;
    /* Printable keys */;
    inline I32_t constexpr CGE_KEY_SPACE{ 32 };
    inline I32_t constexpr CGE_KEY_APOSTROPHE{ 39 }; /* ' */
    inline I32_t constexpr CGE_KEY_COMMA{ 44 };      /* , */
    inline I32_t constexpr CGE_KEY_MINUS{ 45 };      /* - */
    inline I32_t constexpr CGE_KEY_PERIOD{ 46 };     /* . */
    inline I32_t constexpr CGE_KEY_SLASH{ 47 };      /* / */
    inline I32_t constexpr CGE_KEY_0{ 48 };
    inline I32_t constexpr CGE_KEY_1{ 49 };
    inline I32_t constexpr CGE_KEY_2{ 50 };
    inline I32_t constexpr CGE_KEY_3{ 51 };
    inline I32_t constexpr CGE_KEY_4{ 52 };
    inline I32_t constexpr CGE_KEY_5{ 53 };
    inline I32_t constexpr CGE_KEY_6{ 54 };
    inline I32_t constexpr CGE_KEY_7{ 55 };
    inline I32_t constexpr CGE_KEY_8{ 56 };
    inline I32_t constexpr CGE_KEY_9{ 57 };
    inline I32_t constexpr CGE_KEY_SEMICOLON{ 59 }; /* ; */
    inline I32_t constexpr CGE_KEY_EQUAL{ 61 };     /* = */
    inline I32_t constexpr CGE_KEY_A{ 65 };
    inline I32_t constexpr CGE_KEY_B{ 66 };
    inline I32_t constexpr CGE_KEY_C{ 67 };
    inline I32_t constexpr CGE_KEY_D{ 68 };
    inline I32_t constexpr CGE_KEY_E{ 69 };
    inline I32_t constexpr CGE_KEY_F{ 70 };
    inline I32_t constexpr CGE_KEY_G{ 71 };
    inline I32_t constexpr CGE_KEY_H{ 72 };
    inline I32_t constexpr CGE_KEY_I{ 73 };
    inline I32_t constexpr CGE_KEY_J{ 74 };
    inline I32_t constexpr CGE_KEY_K{ 75 };
    inline I32_t constexpr CGE_KEY_L{ 76 };
    inline I32_t constexpr CGE_KEY_M{ 77 };
    inline I32_t constexpr CGE_KEY_N{ 78 };
    inline I32_t constexpr CGE_KEY_O{ 79 };
    inline I32_t constexpr CGE_KEY_P{ 80 };
    inline I32_t constexpr CGE_KEY_Q{ 81 };
    inline I32_t constexpr CGE_KEY_R{ 82 };
    inline I32_t constexpr CGE_KEY_S{ 83 };
    inline I32_t constexpr CGE_KEY_T{ 84 };
    inline I32_t constexpr CGE_KEY_U{ 85 };
    inline I32_t constexpr CGE_KEY_V{ 86 };
    inline I32_t constexpr CGE_KEY_W{ 87 };
    inline I32_t constexpr CGE_KEY_X{ 88 };
    inline I32_t constexpr CGE_KEY_Y{ 89 };
    inline I32_t constexpr CGE_KEY_Z{ 90 };
    inline I32_t constexpr CGE_KEY_LEFT_BRACKET{ 91 };  /* [ */
    inline I32_t constexpr CGE_KEY_BACKSLASH{ 92 };     /* \ */
    inline I32_t constexpr CGE_KEY_RIGHT_BRACKET{ 93 }; /* ] */
    inline I32_t constexpr CGE_KEY_GRAVE_ACCENT{ 96 };  /* ` */
    inline I32_t constexpr CGE_KEY_WORLD_1{ 161 };      /* non-US #1 */
    inline I32_t constexpr CGE_KEY_WORLD_2{ 162 };      /* non-US #2 */

    /* Function keys */
    inline I32_t constexpr CGE_KEY_ESCAPE{ 256 };
    inline I32_t constexpr CGE_KEY_ENTER{ 257 };
    inline I32_t constexpr CGE_KEY_TAB{ 258 };
    inline I32_t constexpr CGE_KEY_BACKSPACE{ 259 };
    inline I32_t constexpr CGE_KEY_INSERT{ 260 };
    inline I32_t constexpr CGE_KEY_DELETE{ 261 };
    inline I32_t constexpr CGE_KEY_RIGHT{ 262 };
    inline I32_t constexpr CGE_KEY_LEFT{ 263 };
    inline I32_t constexpr CGE_KEY_DOWN{ 264 };
    inline I32_t constexpr CGE_KEY_UP{ 265 };
    inline I32_t constexpr CGE_KEY_PAGE_UP{ 266 };
    inline I32_t constexpr CGE_KEY_PAGE_DOWN{ 267 };
    inline I32_t constexpr CGE_KEY_HOME{ 268 };
    inline I32_t constexpr CGE_KEY_END{ 269 };
    inline I32_t constexpr CGE_KEY_CAPS_LOCK{ 280 };
    inline I32_t constexpr CGE_KEY_SCROLL_LOCK{ 281 };
    inline I32_t constexpr CGE_KEY_NUM_LOCK{ 282 };
    inline I32_t constexpr CGE_KEY_PRINT_SCREEN{ 283 };
    inline I32_t constexpr CGE_KEY_PAUSE{ 284 };
    inline I32_t constexpr CGE_KEY_F1{ 290 };
    inline I32_t constexpr CGE_KEY_F2{ 291 };
    inline I32_t constexpr CGE_KEY_F3{ 292 };
    inline I32_t constexpr CGE_KEY_F4{ 293 };
    inline I32_t constexpr CGE_KEY_F5{ 294 };
    inline I32_t constexpr CGE_KEY_F6{ 295 };
    inline I32_t constexpr CGE_KEY_F7{ 296 };
    inline I32_t constexpr CGE_KEY_F8{ 297 };
    inline I32_t constexpr CGE_KEY_F9{ 298 };
    inline I32_t constexpr CGE_KEY_F10{ 299 };
    inline I32_t constexpr CGE_KEY_F11{ 300 };
    inline I32_t constexpr CGE_KEY_F12{ 301 };
    inline I32_t constexpr CGE_KEY_F13{ 302 };
    inline I32_t constexpr CGE_KEY_F14{ 303 };
    inline I32_t constexpr CGE_KEY_F15{ 304 };
    inline I32_t constexpr CGE_KEY_F16{ 305 };
    inline I32_t constexpr CGE_KEY_F17{ 306 };
    inline I32_t constexpr CGE_KEY_F18{ 307 };
    inline I32_t constexpr CGE_KEY_F19{ 308 };
    inline I32_t constexpr CGE_KEY_F20{ 309 };
    inline I32_t constexpr CGE_KEY_F21{ 310 };
    inline I32_t constexpr CGE_KEY_F22{ 311 };
    inline I32_t constexpr CGE_KEY_F23{ 312 };
    inline I32_t constexpr CGE_KEY_F24{ 313 };
    inline I32_t constexpr CGE_KEY_F25{ 314 };
    inline I32_t constexpr CGE_KEY_KP_0{ 320 };
    inline I32_t constexpr CGE_KEY_KP_1{ 321 };
    inline I32_t constexpr CGE_KEY_KP_2{ 322 };
    inline I32_t constexpr CGE_KEY_KP_3{ 323 };
    inline I32_t constexpr CGE_KEY_KP_4{ 324 };
    inline I32_t constexpr CGE_KEY_KP_5{ 325 };
    inline I32_t constexpr CGE_KEY_KP_6{ 326 };
    inline I32_t constexpr CGE_KEY_KP_7{ 327 };
    inline I32_t constexpr CGE_KEY_KP_8{ 328 };
    inline I32_t constexpr CGE_KEY_KP_9{ 329 };
    inline I32_t constexpr CGE_KEY_KP_DECIMAL{ 330 };
    inline I32_t constexpr CGE_KEY_KP_DIVIDE{ 331 };
    inline I32_t constexpr CGE_KEY_KP_MULTIPLY{ 332 };
    inline I32_t constexpr CGE_KEY_KP_SUBTRACT{ 333 };
    inline I32_t constexpr CGE_KEY_KP_ADD{ 334 };
    inline I32_t constexpr CGE_KEY_KP_ENTER{ 335 };
    inline I32_t constexpr CGE_KEY_KP_EQUAL{ 336 };
    inline I32_t constexpr CGE_KEY_LEFT_SHIFT{ 340 };
    inline I32_t constexpr CGE_KEY_LEFT_CONTROL{ 341 };
    inline I32_t constexpr CGE_KEY_LEFT_ALT{ 342 };
    inline I32_t constexpr CGE_KEY_LEFT_SUPER{ 343 };
    inline I32_t constexpr CGE_KEY_RIGHT_SHIFT{ 344 };
    inline I32_t constexpr CGE_KEY_RIGHT_CONTROL{ 345 };
    inline I32_t constexpr CGE_KEY_RIGHT_ALT{ 346 };
    inline I32_t constexpr CGE_KEY_RIGHT_SUPER{ 347 };
    inline I32_t constexpr CGE_KEY_MENU{ 348 };

    inline I32_t constexpr CGE_KEY_LAST{ CGE_KEY_MENU };
} // namespace key

namespace action
{
    inline I32_t constexpr CGE_RELEASE{ 0 };
    inline I32_t constexpr CGE_PRESS{ 1 };
    inline I32_t constexpr CGE_REPEAT{ 2 };
} // namespace action
} // namespace cge
