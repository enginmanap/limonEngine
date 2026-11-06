//
// Created by Engin Manap on 06/11/2026.
//

#ifndef LIMONENGINE_FORCEINLINE_H
#define LIMONENGINE_FORCEINLINE_H

//GCC and Clang fail the build when they can't inline, MSVC only warns (C4714), anything else gets a plain hint
#if defined(__GNUC__) || defined(__clang__)
#define LIMON_FORCE_INLINE [[gnu::always_inline]] inline
#elif defined(_MSC_VER)
#define LIMON_FORCE_INLINE __forceinline
#else
#define LIMON_FORCE_INLINE inline
#endif

#endif //LIMONENGINE_FORCEINLINE_H
