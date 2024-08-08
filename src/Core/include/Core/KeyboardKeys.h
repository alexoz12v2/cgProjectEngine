#pragma once

#include "Core/Type.h"

/// Copied from GLFW

namespace cge
{

namespace button
{
    inline I32_t constexpr LMB = 0;
    inline I32_t constexpr RMB = 1;
    inline I32_t constexpr MMB = 2;
}; // namespace button

namespace key
{
    /* The unknown key */;
    inline I32_t constexpr UNKNOWN{ -1 };
    ;
    /* Printable keys */;
    inline I32_t constexpr SPACE{ 32 };
    inline I32_t constexpr APOSTROPHE{ 39 }; /* ' */
    inline I32_t constexpr COMMA{ 44 };      /* , */
    inline I32_t constexpr MINUS{ 45 };      /* - */
    inline I32_t constexpr PERIOD{ 46 };     /* . */
    inline I32_t constexpr SLASH{ 47 };      /* / */
    inline I32_t constexpr _0{ 48 };
    inline I32_t constexpr _1{ 49 };
    inline I32_t constexpr _2{ 50 };
    inline I32_t constexpr _3{ 51 };
    inline I32_t constexpr _4{ 52 };
    inline I32_t constexpr _5{ 53 };
    inline I32_t constexpr _6{ 54 };
    inline I32_t constexpr _7{ 55 };
    inline I32_t constexpr _8{ 56 };
    inline I32_t constexpr _9{ 57 };
    inline I32_t constexpr SEMICOLON{ 59 }; /* ; */
    inline I32_t constexpr EQUAL{ 61 };     /* = */
    inline I32_t constexpr A{ 65 };
    inline I32_t constexpr B{ 66 };
    inline I32_t constexpr C{ 67 };
    inline I32_t constexpr D{ 68 };
    inline I32_t constexpr E{ 69 };
    inline I32_t constexpr F{ 70 };
    inline I32_t constexpr G{ 71 };
    inline I32_t constexpr H{ 72 };
    inline I32_t constexpr I{ 73 };
    inline I32_t constexpr J{ 74 };
    inline I32_t constexpr K{ 75 };
    inline I32_t constexpr L{ 76 };
    inline I32_t constexpr M{ 77 };
    inline I32_t constexpr N{ 78 };
    inline I32_t constexpr O{ 79 };
    inline I32_t constexpr P{ 80 };
    inline I32_t constexpr Q{ 81 };
    inline I32_t constexpr R{ 82 };
    inline I32_t constexpr S{ 83 };
    inline I32_t constexpr T{ 84 };
    inline I32_t constexpr U{ 85 };
    inline I32_t constexpr V{ 86 };
    inline I32_t constexpr W{ 87 };
    inline I32_t constexpr X{ 88 };
    inline I32_t constexpr Y{ 89 };
    inline I32_t constexpr Z{ 90 };
    inline I32_t constexpr LEFT_BRACKET{ 91 };  /* [ */
    inline I32_t constexpr BACKSLASH{ 92 };     /* \ */
    inline I32_t constexpr RIGHT_BRACKET{ 93 }; /* ] */
    inline I32_t constexpr GRAVE_ACCENT{ 96 };  /* ` */
    inline I32_t constexpr WORLD_1{ 161 };      /* non-US #1 */
    inline I32_t constexpr WORLD_2{ 162 };      /* non-US #2 */

    /* Function keys */
    inline I32_t constexpr ESCAPE{ 256 };
    inline I32_t constexpr ENTER{ 257 };
    inline I32_t constexpr TAB{ 258 };
    inline I32_t constexpr BACKSPACE{ 259 };
    inline I32_t constexpr INSERT{ 260 };
    inline I32_t constexpr DELETE{ 261 };
    inline I32_t constexpr RIGHT{ 262 };
    inline I32_t constexpr LEFT{ 263 };
    inline I32_t constexpr DOWN{ 264 };
    inline I32_t constexpr UP{ 265 };
    inline I32_t constexpr PAGE_UP{ 266 };
    inline I32_t constexpr PAGE_DOWN{ 267 };
    inline I32_t constexpr HOME{ 268 };
    inline I32_t constexpr END{ 269 };
    inline I32_t constexpr CAPS_LOCK{ 280 };
    inline I32_t constexpr SCROLL_LOCK{ 281 };
    inline I32_t constexpr NUM_LOCK{ 282 };
    inline I32_t constexpr PRINT_SCREEN{ 283 };
    inline I32_t constexpr PAUSE{ 284 };
    inline I32_t constexpr F1{ 290 };
    inline I32_t constexpr F2{ 291 };
    inline I32_t constexpr F3{ 292 };
    inline I32_t constexpr F4{ 293 };
    inline I32_t constexpr F5{ 294 };
    inline I32_t constexpr F6{ 295 };
    inline I32_t constexpr F7{ 296 };
    inline I32_t constexpr F8{ 297 };
    inline I32_t constexpr F9{ 298 };
    inline I32_t constexpr F10{ 299 };
    inline I32_t constexpr F11{ 300 };
    inline I32_t constexpr F12{ 301 };
    inline I32_t constexpr F13{ 302 };
    inline I32_t constexpr F14{ 303 };
    inline I32_t constexpr F15{ 304 };
    inline I32_t constexpr F16{ 305 };
    inline I32_t constexpr F17{ 306 };
    inline I32_t constexpr F18{ 307 };
    inline I32_t constexpr F19{ 308 };
    inline I32_t constexpr F20{ 309 };
    inline I32_t constexpr F21{ 310 };
    inline I32_t constexpr F22{ 311 };
    inline I32_t constexpr F23{ 312 };
    inline I32_t constexpr F24{ 313 };
    inline I32_t constexpr F25{ 314 };
    inline I32_t constexpr KP_0{ 320 };
    inline I32_t constexpr KP_1{ 321 };
    inline I32_t constexpr KP_2{ 322 };
    inline I32_t constexpr KP_3{ 323 };
    inline I32_t constexpr KP_4{ 324 };
    inline I32_t constexpr KP_5{ 325 };
    inline I32_t constexpr KP_6{ 326 };
    inline I32_t constexpr KP_7{ 327 };
    inline I32_t constexpr KP_8{ 328 };
    inline I32_t constexpr KP_9{ 329 };
    inline I32_t constexpr KP_DECIMAL{ 330 };
    inline I32_t constexpr KP_DIVIDE{ 331 };
    inline I32_t constexpr KP_MULTIPLY{ 332 };
    inline I32_t constexpr KP_SUBTRACT{ 333 };
    inline I32_t constexpr KP_ADD{ 334 };
    inline I32_t constexpr KP_ENTER{ 335 };
    inline I32_t constexpr KP_EQUAL{ 336 };
    inline I32_t constexpr LEFT_SHIFT{ 340 };
    inline I32_t constexpr LEFT_CONTROL{ 341 };
    inline I32_t constexpr LEFT_ALT{ 342 };
    inline I32_t constexpr LEFT_SUPER{ 343 };
    inline I32_t constexpr RIGHT_SHIFT{ 344 };
    inline I32_t constexpr RIGHT_CONTROL{ 345 };
    inline I32_t constexpr RIGHT_ALT{ 346 };
    inline I32_t constexpr RIGHT_SUPER{ 347 };
    inline I32_t constexpr MENU{ 348 };

    inline I32_t constexpr LAST{ MENU };
} // namespace key

namespace action
{
    inline I32_t constexpr RELEASE{ 0 };
    inline I32_t constexpr PRESS{ 1 };
    inline I32_t constexpr REPEAT{ 2 };
} // namespace action
} // namespace cge
